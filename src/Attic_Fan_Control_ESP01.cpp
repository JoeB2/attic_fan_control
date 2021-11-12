/*
      Master: Web Server, ESPNOW, WebSock, Relay, BMP280 
            - compare remote/self temp turn on/off relay
            - serve web page showing temps & humidity & Realy staus (on/off)
            - serve web page allow update of relay on/off temperature differential
            - store last updated differential values : SPIFFS

      Remote: ESPNOW, Dallas 180B20 - Send outside temp & humidity to master
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WEB
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <index.h>
#include <time.h>
#include <espnow.h>

// OTA
#include <AsyncElegantOTA.h>

const byte DNS_PORT  = 53;
bool AP_MODE=false;

#define READINGS_DELAY 60000  // 1 minutes: this is only used if no ack received from attic ESP
#define ONE_WIRE_BUS 2
#define RELAY_PIN    1

const byte DNS_PORT  = 53;
char AP_NAME[] = "Attic Fan WiFi Setup";
bool AP_MODE=false;

WiFiUDP ntpUDP;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8*60*60, 10*60*1000); //poool, GMT offset seconds, refresh ms

typedef struct WifiCreds_t{
      String    SSID;
      String    PWD;
      bool      isDHCP;
      IPAddress IP;
      IPAddress GW;
      IPAddress MASK;

      std::string toStr(){
        char s[150];
        sprintf(s, "{\"SSID\":%s,\"PWD\":%s,\"isDHCP\":%s,\"IP\":%s,\"GW\":%s,\"MASK\":%s", SSID.c_str(), PWD.c_str(), isDHCP?"true":"false", IP.toString().c_str(), GW.toString().c_str(), MASK.toString().c_str());
        return(s);
      }
} WifiCreds_t;
WifiCreds_t creds;


// objects
DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket webSock("/");

const char AP_NAME[]="Attic Fan Control : 10.0.1.23";
IPAddress  // soft AP IP info
          ip_AP(10,0,1,23)
        , ip_GW(10,0,0,125)
        , ip_AP_GW(10,0,1,23)
        , ip_subNet(255,255,255,128);

typedef struct wifi_t{
  std::string ssid;  // A, I, O : Attic, Inside, Outside
  std::string pwd;        // 
  bool isDHCP;
  IPAddress ip;
  IPAddress GW;
  IPAddress mask;

  std::string toStr(){
    char c[100];
    sprintf(c, "{\"ssid\":\"%s\",\"pwd\":%s,\"isDHCP\":%i,\"ip\":\"%s\",\"GW\":\"%s\",\"mask\":\"%s\"}"
                                , ssid.c_str(), pwd.c_str(), isDHCP, ip.toString().c_str(), GW.toString().c_str(), mask.toString().c_str());
    return c;
  }
} wifi_t;
wifi_t creds;

typedef struct conditions_t{
  float temp;
  unsigned long epochTime;

  uint64_t sleepTIMEus=5*60*1000000; // 5 minutes
 
  String toStr(){
    char c[100];
    sprintf(c, "{\"temp\":%f,\"epochTime\":%u}", temp, epochTime);
    return c;
  }
}conditions_t;

typedef struct atticFanControl_t{
  float t1;  // attic
  float t2;  // inside
  float t3;  // outside - remote sensor
  unsigned long atticEpochTime;
  unsigned long remoteEpochTime;
  bool fanOn; // fan on/off indicator
  int onMins; // timer for fan on minutes
  float attic_on;   // Attic temp to force Fan ON: 0 to disable (Master Override)
  float attic_off;  // Attic temp to turn fan OFF: 0 to dissable
  float AID_on;     // Attic - Inside temp delta for fan ON: 0 to dissable   (precedence 3)
  float AID_off;    // Attic - Inside temp delta for fan OFF: 0 to dissable
  float AOD_on;     // Attic - Outside temp delta for fan ON: 0 to dissable  (precedence 2)
  float AOD_off;    // Attic - Outside temp delta for fan OFF: 0 to dissable
  float IOD_on;     // Inside - Outside temp delta for fan ON: 0 to dissable (precedence 1)
  float IOD_off;    // Inside - Outside temp delta for fan OFF: 0 to dissable
  bool overide_OFF; // Fan is OFF when set to true;
  unsigned long delay; // how often are temps checked in millisecopnds

  std::string toStr(){ // make JSON string
    char c[275];
    sprintf(c, "{\"t1\":\"%.2f\",\"t2\":%.2f,\"t3\":%.2f,\"atticEpochTime\":%u,\"remoteEpochTime\":%u,\"fanOn\":%i,\"onMins\":%i},{\"attic_on\":%.2f,\"attic_off\":%.2f,\"AID_on\":%.2f,\"AID_off\":%.2f,\"AOD_on\":%.2f,\"AOD_off\":%.2f,\"IOD_on\":%.2f,\"IOD_off\":%.2f,\"overide_OFF\":%i,\"delay\":%lu}]",
    t1, t2, t3, atticEpochTime, remoteEpochTime, fanOn, onMins, attic_on, attic_off, AID_on, AID_off, AOD_on, AOD_off, IOD_on, IOD_off, overide_OFF, delay);
    return c;
  }
} atticFanControl_t;

atticFanControl_t atticFan;

/******  PROTOTYPES  *******/
  void notFound(AsyncWebServerRequest *request);
  std::string valFromJson(const std::string &json, const std::string &element);
  bool wifiConnect(WiFiMode m);
  void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len);
  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len);
  void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus);
  void initAtticFanStruct();
  bool initCreds();
  void setCreds(const std::string& s);
  bool saveCreds();
  bool FanOnOff();

  // ESP_NOW
  void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus);
  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len);

  // ESP_NOW broadcast
  uint8_t peer[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // MAC: MultiCast"

  void setup() {
    Serial.begin(115200);
    SPIFFS.begin();

    AP_MODE=!initCreds();
    AP_MODE=AP_MODE || !wifiConnect(WIFI_STA);

    if(AP_MODE){
                wifiConnect(WIFI_AP);

                DNSServer dnsServer;
                AsyncWebServer server(80);
                AsyncWebSocket webSock("/");

                // init Websock
                webSock.onEvent(onWsEvent);
                server.addHandler(&webSock);

                /* Setup the DNS server redirecting all the domains to the apIP */
                dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
                dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
                server.onNotFound(notFound);
                // Web Pages
                server.on("/"        , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", SSIDPWD_HTML);});
                AsyncElegantOTA.begin(&server);    // Start ElegantOTA
                server.begin();
        // END: Get Credentials "AP_MODE"
      }else{// NOT get WiFi Credentials
                initAtticFanStruct();
                timeClient.begin();
                yield(); // I break for unicorns
                pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

                esp_now_init();
                esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
                esp_now_register_send_cb(OnDataSent);
                esp_now_register_recv_cb(OnDataRecv);
                esp_now_add_peer(peer, ESP_NOW_ROLE_COMBO, 3, NULL, 0);

                server.onNotFound(notFound);
                // Web Pages
                server.on("/"        , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", INDEX_HTML);});
                server.on("/settings"        , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", SETTINGS_HTML);});
                AsyncElegantOTA.begin(&server);    // Start ElegantOTA
                server.begin();

        }
  }

  unsigned long startMillis=millis();
  void loop() {
        if(millis()-startMillis > atticFan.delay){
                sensors.requestTemperatures();
                atticFan.t1 = sensors.getTempFByIndex(0);
                atticFan.t2 = sensors.getTempFByIndex(1);
                atticFan.atticEpochTime=timeClient.getEpochTime();

                FanOnOff();

                startMillis=millis();

                webSock.textAll(atticFan.toStr().c_str());
                delay(1); // arbitrary
                webSock.cleanupClients();
                startMillis=millis();
        }

  }
  bool FanOnOff(){

  }

  // WebSock Event Handler
  void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

  if(type == WS_EVT_CONNECT){ Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());}
  else if(type == WS_EVT_DISCONNECT){Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id()); } 
  else if(type == WS_EVT_ERROR){Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);  }
  else if(type == WS_EVT_PONG){Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");  } 
  else if(type == WS_EVT_DATA){
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
                if(info->final && !info->index && info->len == len){
                        if(info->opcode == WS_TEXT){
                                data[len]=0; 
                                std::string const s=(char *)data;
                                if(AP_MODE){
                                        setCreds(s);
                                        AP_MODE=!saveCreds();
                                        return;
                                }
                                update_atticFanStructFromString(s);
                                saveStruct();
                        }
                }
        }
}
  // ESP NOW  sent call back(cb)
  void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (sendStatus == 0){
    Serial.println("\nNOW: Delivery success");
  }
  else{
    Serial.println("\nNOW: Delivery fail");
  }
}
  //ESP NOW  call back(cb) OnDataReceive
  // unless I am the Attic disregard all inbound espnow
  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {// updates loacal scaleDiv.  updates the correct s#_Msg
        conditions_t c;  // needed to land inbound binary data;
        memcpy(&c, incomingData, sizeof(c));  // maybe need len or sizeof incomming data?? (TBD)
#ifdef dbg
        Serial.printf("\nNOW-in: 203: now_Msg: %s", now_Msg.toStr().c_str());Serial.flush();
#endif
        atticFan.t3=c.temp;
        atticFan.remoteEpochTime=c.epochTime;
        c.epochTime=0;
        esp_now_send(peer, (uint8_t *)&c, sizeof(c)); delay(1);
}
  // load SPIFFS copy of JSON into the local scale's systemValues_t
  void initAtticFanStruct(){
        File f = SPIFFS.open("/atticFanStruct.json", "r");
        if(f){
                const std::string s = f.readString().c_str();
                f.close();
                update_atticFanStructFromString(s);
        }
#ifdef dbg
Serial.printf("initLocalStruct : sv:%s", atticFan.toStr().c_str());
#endif       
  }
  bool update_atticFanStructFromString(const std::string &s){
        atticFan.attic_on       =::atof(valFromJson(s, "attic_on").c_str());
        atticFan.attic_off      =::atof(valFromJson(s, "attic_off").c_str());
        atticFan.AID_on         =::atof(valFromJson(s, "AID_on").c_str());
        atticFan.AID_off        =::atof(valFromJson(s, "AID_off").c_str());
        atticFan.AOD_on         =::atof(valFromJson(s, "AOD_on").c_str());
        atticFan.AOD_off        =::atof(valFromJson(s, "AOD_off").c_str());
        atticFan.IOD_on         =::atof(valFromJson(s, "IOD_on").c_str());
        atticFan.IOD_off        =::atof(valFromJson(s, "IOD_off").c_str());
        atticFan.onMins         =::atoi(valFromJson(s, "onMins").c_str());
        atticFan.overide_OFF    =::atoi(valFromJson(s, "overide_OFF").c_str());
        atticFan.delay          =::atoi(valFromJson(s, "delay").c_str());
  }
  // save the local systemValues_t data as JSON string to SPIFFS
  bool saveStruct(){
        File f = SPIFFS.open(F("/atticFanStruct.json"), "w");
        if(f){
                f.print(atticFan.toStr().c_str());
                f.close();
                return true;
        }
        else{return false;}
  }
  // save the local systemValues_t data as JSON string to SPIFFS
  bool saveSSIDPWD(const std::string &s){
        File f = SPIFFS.open(F("/SSIDPWD"), "w");
        if(f){
                f.print(s.c_str());
                f.close();
                return true;
        }
        else{return false;}
  }
  bool initSSIDPWD(){
        File f = SPIFFS.open("/SSIDPWD", "r");
        if(f){
               const std::string s = f.readString().c_str();
                f.close();
                initSSIDPWD(s);
                return(true);
        }
#ifdef dbg
Serial.printf("initSSIDPWD : sv:%s", creds.toStr().c_str());
#endif       
        return(false);
  }
  void initSSIDPWD(const std::string &s){
                 creds.SSID=valFromJson(s, "SSID").c_str();
                 creds.PWD=valFromJson(s, "PWD").c_str();
                 creds.isDHCP=valFromJson(s, "isDHCP").c_str();
                 creds.IP.fromString(valFromJson(s, "IP").c_str());
                 creds.GW.fromString(valFromJson(s, "GW").c_str());
                 creds.MASK.fromString(valFromJson(s, "MASK").c_str());
  }
  // process inbound websock json
  // NOTE: Stack Dumps when trying to use <ArduinoJson.h>
  std::string valFromJson(const std::string &json, const std::string &element){
    size_t start, end;
    start = json.find(element);
    start = json.find(":", start)+1;
    if(json.substr(start,1) =="\"")start++;
    end  = json.find_first_of(",]}\"", start);
    return(json.substr(start, end-start));
  }
  bool wifiConnect(WiFiMode m){ 
        WiFi.disconnect();
        WiFi.softAPdisconnect();

        WiFi.mode(m);// WIFI_AP_STA,WIFI_AP; WIFI_STA;
        
        switch(m){
                case WIFI_STA:
                                WiFi.begin(creds.SSID.c_str(), creds.PWD.c_str());
                                WiFi.channel(2);
                                WiFi.config(creds.IP, creds.GW, creds.MASK);
                                break;

                case WIFI_AP:
                                WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
                                WiFi.softAP(AP_NAME, "");
                                WiFi.begin();
                                break;

                case WIFI_AP_STA: break;
                case WIFI_OFF: return(true);

        }

        unsigned int startup = millis();
        while(WiFi.status() != WL_CONNECTED){
              delay(250);
              Serial.print(".");
              if(millis() - startup >= 5000) break;
        }
#ifdef dbg
Serial.printf("wifiConnect : IP:%s, GW: %s, Mask: %s\n", WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.subnetMask().toString().c_str());
#endif
        return(WiFi.status() == WL_CONNECTED);
  }