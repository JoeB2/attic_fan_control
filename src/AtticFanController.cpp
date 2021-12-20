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
*/

//toDo: remote ESP: 1) solder connection from GPIO16 to rst: for deep sleep wakeup

//#define dbg

#include <Arduino.h>
#include <espnow.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <string>

// DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

// Web Server
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <index.h>

// OTA
#include <AsyncElegantOTA.h>


// bit macros
#define SET_BIT(x, pos) (x |= (1U << pos))
#define CLEAR_BIT(x, pos) (x &= (~(1U<< pos)))
#define CHECK_BIT(x, pos) (x & (1UL << pos) )

// mutex bitpos defines
#define WS_LOCK  1
#define NOW_LOCK 0

#define READINGS_DELAY 60000  // 1 minutes: this is only used if no ack received from attic ESP
#define ONE_WIRE_BUS 2
#define RELAY_PIN    0
#define ALTITUDE 394 // MSL altitude at JJ's house
#define MAX_WS_CLIENT 5
#define CLIENT_NONE     0
#define CLIENT_ACTIVE   1

const char AP_SSID[] = "AtticFan WiFi: 10.0.1.23";
const DeviceAddress // DS18B20 Addresses
          T1={0x28, 0x31, 0xCA, 0xC7, 0x3E, 0x19, 0x01, 0xCE}
        , T2={0x28, 0xEF, 0x94, 0xC7, 0x3E, 0x19, 0x01, 0x2E};

uint8_t peer[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // MAC: MultiCast"
uint8_t remote_Mac[] = {0x5C, 0xCF, 0x7F, 0xE3, 0xC0,0xC0};

// t3 is most accurate.  1) put all three sensors in same temp area 2)start MCU 3) json will calc & store t1-t3 & t2-t3
float tSync[] = {0.7,0.10};

unsigned long startMillis=millis();
unsigned long last_Remote_Contact_millis=millis();

AsyncWebServer webServer(80);
AsyncWebSocket webSock("/");

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

IPAddress  // soft AP IP info
          ip_AP(10,0,1,23)
        , ip_GW(10,0,0,125)
        , ip_AP_GW(10,0,1,23)
        , ip_subNet(255,255,255,128);

const byte
      empty = 0             // 0b00000000
    , readings = 1          // 0b00000001
    , settings = 2          // 0b00000010
    , net = 4               // 0b00000100
    , atticEpochTime = 9    // 0b00001001
    , fanOnMs = 17          // 0b00010001
    , systemEnabled = 33    // 0b00100001
    , ping_readings = 65    // 0b01000001
    , ping_setings = 66     // 0b01000010
    , ping_wifi = 68        // 0b01000100
  ;

  typedef struct {
    uint32_t  id;
    uint8_t   state;
    byte what;
  } _ws_client; 

  _ws_client ws_client[MAX_WS_CLIENT];

  struct {
      std::string ssid = "myWiFi";
      std::string pwd  = "notAPassword";
      bool isDHCP;
      std::string ip   = "198.162.1.117";
      std::string gw   = "198.162.1.117";
      std::string mask = "255.255.255.128";

      std::string toStr(){
        char s[125];
        size_t n;
        n = sprintf(s, "{\"json\":%i,\"ssid\":\"%s\",\"pwd\":\"%s\",\"isDHCP\":%hd,\"ip\":\"%s\",\"gw\":\"%s\",\"mask\":\"%s\"}"
        , net, ssid.c_str(), pwd.c_str(), isDHCP?1:0, ip.c_str(), gw.c_str(), mask.c_str());
        return(std::string(s, n));
      };
  } network;

typedef struct remote_Data{
  float temp;
  double press;
  byte humid;
  uint64_t remoteSleepSeconds=10;
  float vcc;
  }remote_data_t;

  struct{
  float t1;		// attic
  float t2;		// inside
  float t3;		// outside - remote sensor temp
  double p3;	// outside air pressure inHg
  byte h3;		// humidity outside
  int dp3;		// dew point outside
  int a3;			// density altitude outside
  float vcc;	// battery voltage Vcc at remote sensor
  unsigned long atticEpochTime;
  bool fanOn;	// fan on/off indicator
  long fanOnMs=0;
  byte attic_on;   // Attic temp to force Fan ON: 0 to disable (Master Override)
  byte attic_off;  // Attic temp to turn fan OFF: 0 to dissable
  byte inside_off=70;  // Fan is off if t2 (inside) is below 70 (default) degrees;
  byte AID_on;     // Attic - Inside temp delta for fan ON: 0 to dissable   (precedence 3)
  byte AID_off;    // Attic - Inside temp delta for fan OFF: 0 to dissable
  byte AOD_on;     // Attic - Outside temp delta for fan ON: 0 to dissable  (precedence 2)
  byte AOD_off;    // Attic - Outside temp delta for fan OFF: 0 to dissable
  byte IOD_on;     // Inside - Outside temp delta for fan ON: 0 to dissable (precedence 1)
  byte IOD_off;    // Inside - Outside temp delta for fan OFF: 0 to dissable
  bool systemEnabled;  // true: ok to turn on fan; // logic simplifies some c++ later on
  unsigned long delayMs = 15000; // how often are temps checked in milliseconds
  uint64_t remoteSleepSeconds;// sent to the remote sensor; sets remote esp01 deep sleep interval


    std::string toStr(byte source){ // make JSON string
      char c[230];
      size_t n;
      if(source==readings) n = sprintf(c, "{\"json\":%i,\"t1\":%.1f,\"t2\":%.1f,\"t3\":%.1f,\"p3\":%.2f,\"h3\":%i,\"dp3\":%i,\"a3\":%i,\"vcc\":%.1f,\"atticEpochTime\":%lu,\"fanOn\":%hd,\"fanOnMs\":%li,\"systemEnabled\":%hd}"
      , readings, t1, t2, t3, p3, h3, dp3, a3, vcc, atticEpochTime, fanOn?1:0, fanOnMs, systemEnabled?1:0);

      if(source==settings) n = sprintf(c, "{\"json\":%i,\"fanOn\":%i,\"attic_on\":%i,\"attic_off\":%i,\"inside_off\":%i,\"AID_on\":%i,\"AID_off\":%i,\"AOD_on\":%i,\"AOD_off\":%i,\"IOD_on\":%i,\"IOD_off\":%i,\"systemEnabled\":%hd,\"delayMs\":%lu,\"remoteSleepSeconds\":%llu}"
      , settings, fanOn?1:0, attic_on, attic_off, inside_off, AID_on, AID_off, AOD_on, AOD_off, IOD_on, IOD_off, systemEnabled?1:0, delayMs, remoteSleepSeconds);

      return std::string(c, n);
    }
    // atticFan struct: true==turn on fan
void setFanOn(){
    // systemEnabled, inside_off take precident for the OFF state
    if(inside_off && t2 <= inside_off){fanOn=0;return;}
    if(systemEnabled==0){fanOn=0;return;}

    // default off if no OFF setting is set and if nothing below turns it on
    if(!(AID_off || AOD_off || IOD_off || attic_off || inside_off))fanOn=0;

    // fan OFF tests
    if(IOD_off && t2-t3<IOD_off)fanOn=0;
    if(AOD_off && t1-t3<AOD_off)fanOn=0;
    if(AID_off && t1-t2<AID_off)fanOn=0;
    if(attic_off && t1<attic_off)fanOn=0;

    // fanOn ON checks
    fanOn |= ( ((t1-t3 >= AOD_on)&&AOD_on) | ((t1-t2 >= AID_on)&&AID_on) | ((t2-t3 >= IOD_on)&&IOD_on) | ((t1>=attic_on)&&attic_on) | (fanOnMs!=0) );
}

    bool turn_on_fan_due_to_temp_differences(){

      return ( ((t1-t3 >= AOD_on)&AOD_on) | ((t1-t2 >= AID_on)&AID_on) | ((t2-t3 >= IOD_on)&IOD_on) | ((t1>=attic_on)&attic_on) | (bool)fanOnMs );
    }
    //false==turn off fan: temp differences
    bool turn_off_fan_due_to_temp_differences(){

      if(inside_off && t2<=inside_off)return false;
      if(IOD_off && t1-t3<IOD_off)return false;
      if(AOD_off && t1-t3<AOD_off)return false;
      if(AID_off && t1-t3<AID_off)return false;
      if(attic_off && t1<attic_off)return false;

      return(AID_off | AOD_off | IOD_off | attic_off | inside_off);
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
  bool wifiConnect(WiFiMode m);
  IPAddress str2IP(const std::string &s);
  std::string valFromJson(const std::string &json, const std::string &element);// Parse JSON string
  int getDewPoint();
  int getDensityAlt();
  long long int printLocalTime();

  void setup() {
delay(500);
#ifdef dbg
    Serial.begin(115200);
    Serial.println();
#endif

    LittleFS.begin();
    initWiFi(); // get WiFi credentials from LittleFS

    wifi_set_phy_mode(PHY_MODE_11B);

    if(!wifiConnect(WIFI_AP_STA))
      wifiConnect(WIFI_AP);

		initAtticFanStruct();
		pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

		esp_now_init();
		esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
		esp_now_register_send_cb(OnDataSent);
		esp_now_register_recv_cb(OnDataRecv);
		esp_now_add_peer(remote_Mac, ESP_NOW_ROLE_COMBO, (uint8_t)WiFi.channel(), NULL, 0);

		// init Websock
		webSock.onEvent(onWsEvent);
		webServer.addHandler(&webSock);

		// Web Pages
		webServer.onNotFound(notFound);

		webServer.on("/"         , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", index_html);});
		webServer.on("/settings" , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", settings_html);});
		webServer.on("/wifi"     , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", wifi_html);});
		AsyncElegantOTA.begin(&webServer);    // Start ElegantOTA
		webServer.begin();
		startMillis=millis()-atticFan.delayMs; // get temps immediately after startup
  }

  void loop() {
   if(millis()-startMillis > atticFan.delayMs){
#ifdef dbg
  Serial.print("#");Serial.flush();
#endif
      sensors.requestTemperatures();
      atticFan.t1 = sensors.getTempF(T1)+tSync[0];
      atticFan.t2 = sensors.getTempF(T2)+tSync[1];
      atticFan.dp3=getDewPoint();
      atticFan.a3=getDensityAlt();
#ifdef dbg
  Serial.print("*");Serial.flush();
#endif
      //fanOnMinutes count down
      if(atticFan.fanOnMs)atticFan.fanOnMs-=(millis()-startMillis);
      if(atticFan.fanOnMs>120*60000)atticFan.fanOnMs=120*60000;
      if(atticFan.fanOnMs<0)atticFan.fanOnMs=0;
      atticFan.setFanOn();
#ifdef dbg
  Serial.print("$");Serial.flush();
#endif
      digitalWrite(RELAY_PIN, atticFan.turn_off_fan_due_to_temp_differences() | atticFan.turn_on_fan_due_to_temp_differences());

      for (uint8_t i=0; i<MAX_WS_CLIENT ; i++) {
        if (ws_client[i].what != empty) {
          switch (ws_client[i].what & 7){
            case 1: webSock.text(ws_client[i].id, atticFan.toStr(1).c_str());break;
            case 2: webSock.text(ws_client[i].id, atticFan.toStr(2).c_str());break;
            case 4: webSock.text(ws_client[i].id, network.toStr().c_str());break;
          }
        }
      }

#ifdef dbg
  Serial.print("^");Serial.flush();
#endif

    startMillis=millis();
    if(!millis()%5000)webSock.cleanupClients(); // call every 5 seconds
    }
  }
  void notFound(AsyncWebServerRequest *request){request->send_P(200, "text/html", index_html);}
  void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){
      uint8_t index;
#ifdef dbg
  Serial.printf("%i connect: srv_url[%s]ID[%u]remotePort[%u] connect\n", __LINE__, server->url(), client->id(), client->remotePort());
#endif
      for (index=0; index<MAX_WS_CLIENT ; index++) {
        if (ws_client[index].id == 0 ) {
          ws_client[index].id = client->id();
          ws_client[index].state = CLIENT_ACTIVE;
#ifdef dbg
  Serial.printf("added #%u at index[%d]\n", client->id(), index);
#endif
          client->ping();
          break; // Exit for loop
        }
      }
    }
    if(type == WS_EVT_DISCONNECT){
#ifdef dbg
        Serial.printf("ws[%s] disconnect: %u\n", server->url(), client->id());
#endif

      for (uint8_t i=0; i<MAX_WS_CLIENT ; i++) {
        if (ws_client[i].id == client->id() ) {
          ws_client[i].id = 0;
          ws_client[i].state = CLIENT_NONE;
          ws_client[i].what = empty;
#ifdef dbg
          Serial.printf("freed[%d]\n", i);
#endif

          break; // Exit for loop
        }
      }
    }
//*************************************************************************************************
    if(type == WS_EVT_DATA){
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && !info->index && info->len == len){
        if(info->opcode == WS_TEXT){
          std::string s((char*)data, len);

          uint8_t source = std::stoi(valFromJson(s, "json")); // which "webSock.send" sent this?

#ifdef dbg
  Serial.printf("url[%s]client[%u]source[%u]root[%u] msg: %s\n"
      , server->url(), client->id(), source, source&0b00000111, s.c_str());Serial.flush();
#endif

          uint8_t i = 0;
          // track clients; we want to send only the required sock's json
          for (i=0; i<MAX_WS_CLIENT ; i++){
            if (ws_client[i].id == client->id() ){
                ws_client[i].what = ((byte)source & 0b00000111);// readings, settings, network
                break; // Exit for loop
            }
          }// loop ws_client
#ifdef dbg
  Serial.printf("%i Sock Msg: %s, root: %u\n",__LINE__, s.c_str(), ws_client[i].what);Serial.flush();
#endif
          switch (source){
            case readings: break;
            case settings:  // settings : Update
                  setAtticFanStruct(s);
                  saveAtticFanStruct();
                  webSock.text(client->id(), atticFan.toStr(settings).c_str());
              break;
            case ping_setings:
#ifdef dbg
  Serial.printf("%i\n",__LINE__);Serial.flush();
#endif
                  webSock.text(client->id(), atticFan.toStr(settings).c_str());
#ifdef dbg
  Serial.printf("%i\n",__LINE__);Serial.flush();
#endif
              break;
            case net: // network : Update
                  initWiFi(s);
                  saveWiFi();
                  webSock.text(client->id(), network.toStr().c_str());
              break;
            case ping_wifi:
                  webSock.text(client->id(), network.toStr().c_str());
             break;
            case atticEpochTime: // Readings : "ping" with atticEpochTime
                  atticFan.atticEpochTime = std::stoul(valFromJson(s, "atticEpochTime"));
                  webSock.text(client->id(), atticFan.toStr(readings).c_str());
              break;
            case fanOnMs: //readings : "ping" with fanOnMs update
                 atticFan.fanOnMs = std::stol(valFromJson(s, "fanOnMs"));
                 webSock.text(client->id(), atticFan.toStr(readings).c_str());
              break;
            case systemEnabled: //readings : "ping" with systemEnabled update
                atticFan.systemEnabled = std::stoi(valFromJson(s, "systemEnabled"));
                saveAtticFanStruct();
                webSock.text(client->id(), atticFan.toStr(readings).c_str());
              break;
          }// send correct json to the client if client exists
        }
      }
    }
  }
  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
      {
        remote_data_t now_Msg;
        memcpy(&now_Msg, incomingData, sizeof(now_Msg));  // maybe need len or sizeof incoming data?? (TBD)

        atticFan.t3  = now_Msg.temp;
        atticFan.p3  = now_Msg.press;
        atticFan.h3  = now_Msg.humid;
        atticFan.vcc = now_Msg.vcc;

        atticFan.dp3 = getDewPoint();   // called after temp & humid & pressure are set
        atticFan.a3  = getDensityAlt(); // called after temp & humid & pressure are set

        last_Remote_Contact_millis=millis();
      }
      for(int i = 0;i<MAX_WS_CLIENT;i++){
        if(ws_client[i].what == readings)
          webSock.text(ws_client[i].id, atticFan.toStr(readings).c_str());
      }
      webSock.cleanupClients();
      esp_now_send(peer, (uint8_t *)&atticFan.remoteSleepSeconds, 8); delay(25);
	}
  void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
#ifdef dbg
      if (sendStatus == 0){Serial.printf("\n%i NOW: Delivery success\n", __LINE__);Serial.flush();}
    else{Serial.printf("\n%i NOW: Delivery fail\n", __LINE__);Serial.flush();}
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
  Serial.printf("%i init atticFan::settings: %s\n", __LINE__, s.c_str());Serial.flush();
#endif
        atticFan.attic_on            = std::stoi(valFromJson(s, "attic_on"));
        atticFan.attic_off           = std::stof(valFromJson(s, "attic_off"));
        atticFan.AID_on              = std::stof(valFromJson(s, "AID_on"));
        atticFan.AID_off             = std::stof(valFromJson(s, "AID_off"));
        atticFan.AOD_on              = std::stof(valFromJson(s, "AOD_on"));
        atticFan.AOD_off             = std::stof(valFromJson(s, "AOD_off"));
        atticFan.IOD_on              = std::stof(valFromJson(s, "IOD_on"));
        atticFan.IOD_off             = std::stof(valFromJson(s, "IOD_off"));
        atticFan.systemEnabled       = std::stoi(valFromJson(s, "systemEnabled"))?1:0;
        atticFan.delayMs             = std::stoul(valFromJson(s, "delayMs"));
        atticFan.remoteSleepSeconds  = std::stoull(valFromJson(s, "remoteSleepSeconds"));
  }
  bool saveAtticFanStruct(){
      File f = LittleFS.open(F("/settings"), "w");
      if(f){
        f.print(atticFan.toStr(settings).c_str());                
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
        network.pwd    = valFromJson(s, "pwd");
        network.isDHCP = valFromJson(s, "isDHCP")=="1";
        network.ip     = valFromJson(s, "ip");
        network.gw     = valFromJson(s, "gw");
        network.mask   = valFromJson(s, "mask");
  }
  bool saveWiFi(){
      File f = LittleFS.open(F("/network"), "w");
      if(f){
        f.print(network.toStr().c_str());
        f.close();
        return true;
      }
      else
        return false;
  }
  bool wifiConnect(WiFiMode m){
    WiFi.disconnect();
    WiFi.softAPdisconnect();

    WiFi.mode(m);  // WIFI_AP_STA,WIFI_AP; WIFI_STA;

    switch(m){
        case WIFI_STA:
          WiFi.hostname("Attic Fan Controller");
          if(network.isDHCP==false)
            if(WiFi.config(str2IP(network.ip), str2IP(network.gw), str2IP(network.mask)))
              WiFi.begin(network.ssid.c_str(), network.pwd.c_str());
            else
              return(false);
          break;
        case WIFI_AP:
          WiFi.disconnect();
          WiFi.softAPdisconnect();
          WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
          WiFi.softAP(AP_SSID);//, "", 7, false, 4);
          WiFi.begin();
        break;
        case WIFI_AP_STA:
          WiFi.disconnect();
          WiFi.softAPdisconnect();
          delay(25);
          WiFi.hostname("Attic Fan Controller");
          if(!network.isDHCP)
            WiFi.config(str2IP(network.ip), str2IP(network.gw), str2IP(network.mask));
          WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
          WiFi.softAP(AP_SSID, "", 3, false, 4);
          WiFi.begin(network.ssid.c_str(), network.pwd.c_str(), 1);//, 7, (const uint8_t *)__null, true);
        break;
        case WIFI_OFF: return(true);
    }
    unsigned int startup = millis();
#ifdef dbg
            Serial.println("\n");
#endif
    while(WiFi.status() != WL_CONNECTED){
          delay(750);
#ifdef dbg
            Serial.print(".");
#endif
          if(millis() - startup >= 5000) break;
    }
#ifdef dbg
      Serial.println("");
#endif


#ifdef dbg
    Serial.printf("mode: %i, ip: %s, gw: %s, mask: %s\n", m, WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.subnetMask().toString().c_str());Serial.flush();
    Serial.printf("%i AP_ssid: \"%s\", AP_ip: %s\n", __LINE__, WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());Serial.flush();
    delay(100);
#endif
    return((WiFi.status()== WL_CONNECTED)?true:false);
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
  int getDewPoint(){
		double L, M, N, B, D, T, RH;

		RH = atticFan.h3;
		T = (atticFan.t3-32)/1.8;

		L = log(RH/100);
		M = 17.27 * T;
		N = 237.3 + T;
		B = (L + (M/N)) / 17.27;
		D = (237.3 * B) / (1- B);

		return(1.8*D+32);
	}
  int getDensityAlt(){
			return(61.7 * (atticFan.t3 - 59) + 1.236 * ALTITUDE);
	}
