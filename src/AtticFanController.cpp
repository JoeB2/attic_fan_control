#include <Arduino.h>
/*
  Attic Fan Controller

  Purpose:  Regulate an attic fan's on and off.  The fan will move air from the hallway into the attic
            and the attic air will consequently move out of the attic's roof and eves vents.

  What this does: Measurement of attic temperature, hallway temperatue, and outside temperature are calculated
            fed into a function to determine whether to turn the fan on or turn the fan off.

  The Fan ON/OFF function: uses the three temperatures (Attic, Inside, Outside) and compares them and
            their differences to values provided by you to automate the attic fan during the hot summer days.

  Function: There are four hosted web pages: 1. Main page http://ip showing status and providing you with a fan overide
            function.  2. A settings page http://ip/settings where you can enter temperature difference values that will trigger
            Fan ON/OFF activity.  3. A WiFi settings page http://ip/wifi where local WiFi information can be entered.
            4. A over the air OTA page http://ip/update where firmware or static storage information can be uploaded to
            the ESP01 attic MCU.
            Stored in the MCU's static memory are two JSON strings named: atticFan and creds.  They host the fan's
            operational temperature deltas and the WiFi credential information, respectively.

            A value of zero "0" for a given Fan operational parameter disables that parameter.
  NOTE:     The parameters entered on the http://ip/settings page denote the temperature difference in degess Fahrenheit.

            example: AOD on value: "Attic Outside Delta on" a value of 10 indicates you want the fan on
            if Attic Temp minus Outside temp is greter than or equal to 10 degrees f.  Absolute temperature settings are
            specifically labeled as such.
  int(4), uint(4), uint32(4), long(4), uLong(4), uLongLong(8), short(2), ushort(2), float(4), double(8)

*/

//#define dbg
#ifdef dbg
  u32 ms=millis();
  #define trace Serial.printf("%i %lums, ", __LINE__, millis()-ms);ms=millis();Serial.flush();
#else
  #define trace ;
#endif
//#define dbg2

#include <LittleFS.h>
#include <ESP8266WiFi.h>
//#include <WiFiClientSecure.h>
#include <ESP8266mDNS.h>
#include <espnow.h>
#include <string>

// OTA
#include <AsyncElegantOTA.h>

// Web Server
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <index.h>

// DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

// bit macros
#define SET_BIT(x, pos) (x |= (1U << pos))

#define CLEAR_BIT(x, pos) (x &= (~(1U<< pos)))
#define CHECK_BIT(x, pos) (x & (1UL << pos) )

// Standard settings for compile
#define SSR_PIN 5       // ESP01: 3
#define ALTITUDE 394    // MSL altitude at JJ's house
#define WiFiChannel 7   // ESP NOW requires all peers are on the same channel
#define ONE_WIRE_BUS 4  //  ESP01: 0

const char AP_SSID[] = "AtticFan AP: 10.0.1.23"; // max length 32

const DeviceAddress // DS18B20 Addresses
          T1={0x28, 0x9C, 0x50, 0xF4, 0x3E, 0x19, 0x01, 0x12}  // white mark : Attic ...0x12
        , T2={0x28, 0x31, 0xCA, 0xC7, 0x3E, 0x19, 0x01, 0xCE}; // Corded     : Inside ...0xCE

// ESPNOW
//u8 peer[]       = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};   // MAC: MultiCast"
u8 remote_Mac[]       = {0xDC,0x4F,0x22,0x49,0x59,0x15};   // MAC: MultiCast"

float tSync[] = {-1.2,0.55}; // attic, inside : sync DS18B20 temps to BME280

u32 startMillis=millis(); // loop sensor read delay
u32 last_Remote_Contact_millis=millis(); //

AsyncWebServer webServer(80);
AsyncWebSocket webSock("/");

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

IPAddress  // soft AP IP info
          ip_AP(10,0,1,23)
        , ip_GW(10,0,0,125)
        , ip_AP_GW(10,0,1,23)
        , ip_subNet(255,255,255,128);

  struct {
      std::string ssid = "myWiFi";     // max 342 characters
      std::string pwd  = "secretPWD";  // max 64 characters
      bool isDHCP      = false;
      std::string ip   = "198.162.123.117";
      std::string gw   = "198.162.123.117";
      std::string mask = "255.255.255.128";
      bool updated     = 0;

      std::string toStr(){
        char s[146];
        size_t n;
        n = sprintf(s, "{\"src\":\"mcuNetwork\",\"ssid\":\"%s\",\"pwd\":\"%s\",\"isDHCP\":%hhu,\"ip\":\"%s\",\"gw\":\"%s\",\"mask\":\"%s\",\"updated\":%hhu}"
        , ssid.c_str(), pwd.c_str(), isDHCP?1:0, ip.c_str(), gw.c_str(), mask.c_str(), updated);
        return(std::string(s, n));
      };
  } network;

typedef struct remote_Data{
  int temp;
  u32 press;
  u32 humid;
  u64 sleep_us = 4000000;
  u16 vcc;

  String toStr(){
    char c[120];
    int n = sprintf(c, "{\"temp\":%i,\"press\":%u,\"humid\",:%u,\"vcc:%hu,\"sleep_us\":%llu}"
      , temp, press, humid, vcc, sleep_us);
    c[n]=0;
    return String(c);
  }
}remote_data_t;
remote_data_t now_Msg;

 struct{
  float t1;		                  // attic
  float t2;	                    // inside
  float t3;	                    // outside - remote sensor temp
  u32 p3;                       // outside air pressure inHg
  u32 h3;	                      // humidity outside
  u32 vcc;	                    // battery voltage Vcc at remote sensor
  bool fanOn;	                  // fan on/off indicator
  int fanOnMs=0;         
  byte attic_on;                // Attic temp to force Fan ON: 0 to disable (Master Override)
  byte attic_off;               // Attic temp to turn fan OFF: 0 to dissable
  byte inside_off=70;           // Fan is off if t2 (inside) is below 70 (default) degrees;
  byte AID_on;                  // Attic - Inside temp delta for fan ON: 0 to dissable   (precedence 3)
  byte AID_off;                 // Attic - Inside temp delta for fan OFF: 0 to dissable
  byte AOD_on;                  // Attic - Outside temp delta for fan ON: 0 to dissable  (precedence 2)
  byte AOD_off;                 // Attic - Outside temp delta for fan OFF: 0 to dissable
  byte IOD_on;                  // Inside - Outside temp delta for fan ON: 0 to dissable (precedence 1)
  byte IOD_off;                 // Inside - Outside temp delta for fan OFF: 0 to dissable
  bool systemEnabled;           // true: ok to turn on fan; // logic simplifies some c++ later on
  unsigned long delayMs = 15000;// how often are attic temps checked in milliseconds
  u64 sleep_us = 2*60*1e6+2;    // default: 2mins 2us: remote sensor deepSleep time
  bool updated = false;

    std::string mcuMain(){      // make JSON string: #1 Readings, #2 Settings
      char c[275];
      size_t n;
      n = sprintf(c, "{\"src\":\"mcuMain\",\"t1\":%# 6.1f,\"t2\":%# 6.1f,\"t3\":%# 6.1f,\"p3\":%u,\"h3\":%u,\"vcc\":%hu,\"fanOn\":%hhu,\"fanOnMs\":%i,\"systemEnabled\":%hhd,\"sleep_us\":%llu}"
      , t1, t2, t3, p3, h3, vcc, fanOn?1:0, fanOnMs, systemEnabled?1:0, sleep_us);

      return(std::string(c, n));
    }
    std::string mcuSettings(){ // make JSON string: #1 Readings, #2 Settings
      char c[285];
      size_t n;
      // we send this json using webSock to the settings web page
      n = sprintf(c, "{\"src\":\"mcuSettings\",\"fanOn\":%hhu,\"attic_on\":%hhu,\"attic_off\":%hhu,\"inside_off\":%hhu,\"AID_on\":%hhu,\"AID_off\":%hhu,\"AOD_on\":%hhu,\"AOD_off\":%hhu,\"IOD_on\":%hhu,\"IOD_off\":%hhu,\"systemEnabled\":%hhu,\"delayMs\":%lu,\"sleep_us\":%llu,\"updated\":%hhu}"
      , fanOn?1:0, attic_on, attic_off, inside_off, AID_on, AID_off, AOD_on, AOD_off, IOD_on, IOD_off, systemEnabled?1:0, delayMs, sleep_us, updated);

      return(std::string(c, n));
    }
    // atticFan struct: true==turn on fan
    bool setFanOn(){
        // 1)systemEnabled, 2)timmer, 3)inside_off -> order of precedence
        if(systemEnabled==0){fanOn=0;return 0;}
        if(fanOnMs){fanOn=1;return 1;}
        if(inside_off && t2 <= inside_off){fanOn=0;return 0;}

        // fan OFF tests: is there an off setting that is active?
        if(IOD_off && t2-t3<IOD_off)fanOn=0;
        if(AOD_off && t1-t3<AOD_off)fanOn=0;
        if(AID_off && t1-t2<AID_off)fanOn=0;
        if(attic_off && t1<attic_off)fanOn=0;

        // turn off fan, if there is not an "Off Setting" that has been set else leave as is
        fanOn &= (AID_off | AOD_off | IOD_off | attic_off | inside_off);

        // NOTE: On settings overide conflicting (AID, AOD, IOD) temp off settings
        // fanOn ON checks: Should the fan be turned on because of a setting?  else leave as is
        fanOn |= ( (AOD_on&(t1-t3 >= AOD_on)) | (AID_on&(t1-t2 >= AID_on)) | (IOD_on&(t2-t3 >= IOD_on)) | (attic_on&(t1>=attic_on)));
        return fanOn;
    }
  }atticFan;

  /******  PROTOTYPES  *******/
  // web
  void notFound(AsyncWebServerRequest *request);
  void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

	// ESP_NOW
	void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus);
	void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len);

  void initAtticFanStruct(); // load struct atticFan from LittleFS
  void setAtticFanStruct(const std::string &s); // set struct atticFan's differential switching values from WebSock string
  bool saveAtticFanStruct(); // save struct atticFan's JSON string in LittleFS
  bool initWiFi(); // load struct creds from LittleFS
  bool saveWiFi(); // save struct creds JSON string in LittleFS 
  void initWiFi(const std::string &s); // load struct creds from WebSock string
  wl_status_t wifiConnect(WiFiMode_t m);
  IPAddress str2IP(const std::string &s);
  String mac2string(const uint8_t *mac);
  std::string valFromJson(const std::string &json, const std::string &element);// Parse JSON string


  void setup() {
#ifdef dbg
    Serial.begin(74880); // initial boot message is at 74880
    delay(500);
    Serial.printf("\n%i\n", __LINE__);Serial.flush();
#endif
    sensors.begin();
    LittleFS.begin();     // store WiFi Creds
trace
    initWiFi();           // get WiFi credentials from LittleFS
    initAtticFanStruct(); // get temp settings
trace

    wifi_set_phy_mode(PHY_MODE_11B); // high power wifi
    if(wifiConnect(WIFI_STA) != WL_CONNECTED)wifiConnect(WIFI_AP);
		pinMode(SSR_PIN, OUTPUT);
    digitalWrite(SSR_PIN, 0);
trace

		esp_now_init();
		esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
		esp_now_register_send_cb(OnDataSent);
		esp_now_register_recv_cb(OnDataRecv);
		esp_now_add_peer(remote_Mac, ESP_NOW_ROLE_COMBO, WiFiChannel, NULL, 0);
trace

		// init Websock
		webSock.onEvent(onWsEvent);
		webServer.addHandler(&webSock);
trace

		// Web Pages
		webServer.onNotFound(notFound);
    if(WiFi.softAPIP().toString() == "(IP unset)"){
      webServer.on("/"         , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", MAIN);});
      webServer.on("/settings" , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", SETTINGS);});
      webServer.on("/wifi"     , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", NETWORK);});
    }
    else {webServer.on("/"     , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", NETWORK);});}

    ESP.wdtFeed();
		AsyncElegantOTA.begin(&webServer);    // Start ElegantOTA
trace
		webServer.begin();
trace
		startMillis=millis()-atticFan.delayMs; // get temps immediately after startup
  }
  void loop() {
    if(millis()-startMillis > atticFan.delayMs){
trace

        sensors.requestTemperatures();
        atticFan.t1 = sensors.getTempF(T1)+tSync[0];
        atticFan.t2 = sensors.getTempF(T2)+tSync[1];
trace

        ESP.wdtFeed();

        /** Timer Section **/
        if(atticFan.fanOnMs)atticFan.fanOnMs-=(millis()-startMillis);
        if(atticFan.fanOnMs>120*60000)atticFan.fanOnMs=120*60000;// 2hr max timer
        if(atticFan.fanOnMs<(int)atticFan.delayMs)atticFan.fanOnMs=0;
        atticFan.setFanOn();
        /** END Timer Section **/

        digitalWrite(SSR_PIN, atticFan.fanOn);
        webSock.textAll(atticFan.mcuMain().c_str());
        ESP.wdtFeed();
trace

        /** LastRemoteContact Section **/
        // if missed last 3 remote contacts set sensor readings to zero
        if(millis() - last_Remote_Contact_millis > 3*atticFan.sleep_us)
        {atticFan.t3=0;atticFan.p3=0;atticFan.h3=0;atticFan.vcc = 0;}
        /** END lastRemoteContact **/

        startMillis=millis();
    }
  }
  void notFound(AsyncWebServerRequest *request){request->send_P(200, "text/html", MAIN);}
  //**********************************************************************
  void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){client->ping();webSock.cleanupClients();}
    if(type == WS_EVT_DISCONNECT){client->close();webSock.cleanupClients();}
//**************
    if(type != WS_EVT_DATA)return;
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && !info->index && info->len == len){
        if(info->opcode != WS_TEXT)return;
      }
      data[len]=0; // null terminate the char[]
      std::string s((char*)data, len);
      std::string src = valFromJson(s, "src");
      std::string what = valFromJson(s, "what");

#ifdef dbg
    Serial.printf("%i ws_in: %s\n", __LINE__, s.c_str());
#endif
      if(src=="main"){
        if(what=="system"){
          atticFan.systemEnabled = ::atoi(valFromJson(s, "system").c_str());
        }
        if(what=="timer"){
          atticFan.fanOnMs = ::atoi(valFromJson(s, "fanOnMs").c_str());
        }
        atticFan.setFanOn();
        digitalWrite(SSR_PIN, atticFan.fanOn);
        client->text(atticFan.mcuMain().c_str());
      }
      if(src=="settings"){
          if(what=="submit"){
            setAtticFanStruct(s);
            atticFan.updated = saveAtticFanStruct();
          }
          if(what=="system"){
            atticFan.systemEnabled = ::atoi(valFromJson(s, "systemEnabled").c_str());
          }
          digitalWrite(SSR_PIN, atticFan.setFanOn());
#ifdef dbg
  Serial.printf("\n%i %s\n", __LINE__, atticFan.mcuSettings().c_str());
#endif
          client->text(atticFan.mcuSettings().c_str());
          atticFan.updated = false;
        }
      if(valFromJson(s, "src")=="network"){
        if(what=="reboot")ESP.restart();
        if(what=="submit"){
          initWiFi(s); // <-- updated, but no reboot yet
          network.updated = saveWiFi();
        }
        client->text(network.toStr().c_str()); // include "updated" value
      }
#ifdef dbg2
    Serial.printf("\n%i mcuMain: %s\n\nmcuSettings: %s\n\n"
            , __LINE__, atticFan.mcuMain().c_str(), atticFan.mcuSettings().c_str());
#endif
  }
  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len){
trace
      memcpy(&now_Msg, incomingData, sizeof(now_Msg));
trace
#ifdef dbg
      Serial.printf("\n%i %s :Remote MAC: %s\t Local MAC: %s\n\n", __LINE__, __FUNCTION__, mac2string(mac).c_str(), WiFi.macAddress().c_str());
      Serial.printf("NOW IN MSG: %s\n", now_Msg.toStr().c_str());Serial.flush();
#endif
trace
      atticFan.t3  = .018 * now_Msg.temp + 32.0;
      atticFan.p3  = now_Msg.press; // pressure x 100
      atticFan.h3  = now_Msg.humid; // humidity x 100
      atticFan.vcc = now_Msg.vcc;   // vcc x 1000
trace
      last_Remote_Contact_millis=millis(); // used to decide whether the remote may have a communication problem

      webSock.textAll(atticFan.mcuMain().c_str());// send updated sensor data

      if(now_Msg.sleep_us != atticFan.sleep_us)
              esp_now_send(remote_Mac, (u8*) &atticFan.sleep_us, 8);
trace
      delay(25);
	}
  void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
#ifdef dbg
  Serial.printf("%i NOW Delivery %s. AtticFan.sleep_us: %llu\n\n", __LINE__, sendStatus?"ERROR":"SUCCESS", atticFan.sleep_us);Serial.flush();
#endif
  }
  void initAtticFanStruct(){
    File f = LittleFS.open("/settings", "r");
    if(f){
          const std::string s = f.readString().c_str();
          f.close();
          setAtticFanStruct(s);
    }
  }
  void setAtticFanStruct(const std::string &s){
#ifdef dbg
  Serial.printf("\n%i %s\n%s\n\n", __LINE__, __FUNCTION__, s.c_str());Serial.flush();
#endif
        atticFan.attic_on        = std::stoi(valFromJson(s, "attic_on"));
        atticFan.attic_off       = std::stof(valFromJson(s, "attic_off"));
        atticFan.AID_on          = std::stof(valFromJson(s, "AID_on"));
        atticFan.AID_off         = std::stof(valFromJson(s, "AID_off"));
        atticFan.AOD_on          = std::stof(valFromJson(s, "AOD_on"));
        atticFan.AOD_off         = std::stof(valFromJson(s, "AOD_off"));
        atticFan.IOD_on          = std::stof(valFromJson(s, "IOD_on"));
        atticFan.IOD_off         = std::stof(valFromJson(s, "IOD_off"));
        atticFan.systemEnabled   = std::stoi(valFromJson(s, "systemEnabled"))?1:0;
        atticFan.delayMs         = std::stoul(valFromJson(s, "delayMs"));
        atticFan.sleep_us        = std::stoull(valFromJson(s, "sleep_us"));
#ifdef dbg
  Serial.printf("%i %s\n\t%llu\n", __LINE__, s.c_str(),atticFan.sleep_us);
#endif
  }
  bool saveAtticFanStruct(){
      File f = LittleFS.open(F("/settings"), "w");
      if(f){
        f.print(atticFan.mcuSettings().c_str());
        f.close();
        return true;
    }
    else{return false;}
  }
  bool initWiFi(){
        File f = LittleFS.open("/network", "r");
        if(f){
               const std::string s = f.readString().c_str();
                f.close();
                initWiFi(s);
                return(true);
        }
        return(false);
  }
  void initWiFi(const std::string &s){
    network.ssid   = valFromJson(s, "ssid");
    network.ip     = valFromJson(s, "ip");
    network.pwd    = valFromJson(s, "pwd");
    network.isDHCP = valFromJson(s, "isDHCP")=="1";
    network.gw     = valFromJson(s, "gw");
    network.mask   = valFromJson(s, "mask");
  }
  // saveWiFi() resets "updated" flag before the save
  bool saveWiFi(){
    network.updated=0; // we never save "updated's" state
    File f = LittleFS.open(F("/network"), "w");
    if(f){
      f.print(network.toStr().c_str());
      f.close();
      return 1;
    }
    else
      return 0;
  }
  wl_status_t wifiConnect(WiFiMode_t m){
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);

    WiFi.mode(m);  // WIFI_AP_STA,WIFI_AP; WIFI_STA;
    switch(m){
        case WIFI_STA:
            WiFi.enableAP(false);
            WiFi.enableSTA(true);
            WiFi.hostname("Attic Fan Controller");
            if(network.isDHCP==false)WiFi.config(str2IP(network.ip), str2IP(network.gw), str2IP(network.mask));
            WiFi.begin(network.ssid.c_str(), network.pwd.c_str(), WiFiChannel);
          break;
        case WIFI_AP:
            WiFi.enableSTA(false);
            WiFi.enableAP(true);
            WiFi.config(ip_AP, ip_AP_GW, ip_subNet);
            WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
            WiFi.softAP(AP_SSID);//, "", 7, false, 4);
            WiFi.channel(WiFiChannel);
            WiFi.begin();
          break;
        case WIFI_AP_STA:
            delay(25);
            WiFi.hostname("Attic Fan Controller");
            if(!network.isDHCP)
              WiFi.config(str2IP(network.ip), str2IP(network.gw), str2IP(network.mask));
            WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
            WiFi.softAP(AP_SSID, "", 3, false, 4);
            WiFi.begin(network.ssid.c_str(), network.pwd.c_str(), WiFiChannel);
          break;
        case WIFI_OFF: return(WiFi.status());
    }
    WiFi.setOutputPower(20.5);  // range: 0dbm - 20.5dbm

#ifdef dbg2
  Serial.printf("%i networkToStr(): %s\n",__LINE__, network.toStr().c_str());Serial.flush();
#endif

    unsigned long startup = millis();
#ifdef dbg
            Serial.println("\n");
#endif
    while(WiFi.status() != WL_CONNECTED){
      ESP.wdtFeed();
      delay(750);
#ifdef dbg
      Serial.print(".");
#endif
      if(millis() - startup >= 15000) break;
    }
#ifdef dbg
      Serial.print("\n\n");
#endif

#ifdef dbg
    Serial.printf("%i ip: %s, gw: %s, mask: %s\n", __LINE__, WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.subnetMask().toString().c_str());Serial.flush();
    Serial.printf("%i AP_ssid: \"%s\", AP_ip: %s\n", __LINE__, WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());Serial.flush();
    Serial.printf("%i MAC: %s, RSSI: %i\n", __LINE__, WiFi.macAddress().c_str(), WiFi.RSSI());Serial.flush();
    Serial.printf("%i WiFi.Mode: %hhu, WiFi.Status(): %hhu, connect seconds: %lu\n\n", __LINE__, WiFi.getMode(), WiFi.status(), (millis() - startup)/1000);Serial.flush();
    delay(100);
#endif
    return(WiFi.status());
  }
	IPAddress str2IP(const std::string &s){
    unsigned int parts[4] ={0,0,0,0};
    int part=0;

    for( std::size_t i=0 ; i< s.length() ; i++){
    if(s[i] == '.')part++;
      else{
          parts[part]*=10;
          parts[part]+= (unsigned char)s[i]-'0';
      }
    }
    return (IPAddress(parts[0], parts[1], parts[2], parts[3]));
	}
  std::string valFromJson(const std::string &json, const std::string &element){
    uint32_t start, end;
    start = json.find(element);
    if(start == std::string::npos)return "";
    start = json.find(":", start)+1;
    if(json.substr(start,1) =="\"")start++;
    end  = json.find_first_of(",]}\"", start);

    if(start>json.length() || end-start<0)return "";
    return(json.substr(start, end-start));
  }
  String mac2string(const uint8_t *mac){
    char macStr[18];
    snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return(macStr);
  }
