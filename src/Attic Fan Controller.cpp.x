  #include <ESP8266WiFi.h>
  #include <ESP8266WiFiMulti.h>
  #include <ArduinoOTA.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #include <FS.h>
  #include <WebSocketsServer.h>

  #include <OneWire.h>
  #include <DallasTemperature.h>
  #include <index.h>


  // PROTOTYPES
  void startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  void startOTA();                  // Start the OTA service
  void startSPIFFS();               // Start the SPIFFS and list all contents
  void startWebSocket();            // Start a WebSocket server
  void startMDNS();                 // Start the mDNS responder
  void startServer();
  void setHue(int hue);
  String getContentType(String filename);
  String formatBytes(size_t bytes);
  void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);
  void handleFileUpload();
  bool handleFileRead(String path);
  void handleNotFound();
/******  PROTOTYPES  *******/
  // web
  void notFound(AsyncWebServerRequest *request);
  void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

  void initAtticFanStruct(); // load struct atticFan from LittleFS
  void setAtticFanStruct(const std::string &s); // set struct atticFan's differential switching values from WebSock string
  bool saveAtticFanStruct(); // save struct atticFan's JSON string in LittleFS
  bool initCreds(); // load struct creds from LittleFS
  bool saveCreds(); // save struct creds JSON string in LittleFS 
  void initCreds(const std::string &s); // load struct creds from WebSock string
  bool wifiConnect(WiFiMode m);
  IPAddress str2IP(const std::string &s);
  std::string valFromJson(const std::string &json, const std::string &element);// Parse JSON string
  int getDewPoint();
  int getDensityAlt();
  long long int printLocalTime();

	// ESP_NOW
	void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus);
	void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len);


  ESP8266WiFiMulti wifiMulti;      // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

  ESP8266WebServer webServer(80);       // Create a webserver object that listens for HTTP request on port 80
  WebSocketsServer webSocket(81);    // create a websocket server on port 81

  File fsUploadFile;                 // a File variable to temporarily store the received file


  const char *OTAName = "ESP8266";           // A name and a password for the OTA service
  const char *OTAPassword = "esp8266";

  const char* mdnsName = "Attic Fan"; // Domain name for the mDNS responder

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

IPAddress  // soft AP IP info
          ip_AP(10,0,1,23)
        , ip_GW(10,0,0,125)
        , ip_AP_GW(10,0,1,23)
        , ip_subNet(255,255,255,128);

enum json:byte{empty=0, readings=1, settings=2, wifi=4, atticEpochTime=9, fanOnMs=17, fanEnabled=33};

typedef struct wsClient{
  uint32_t clientID;
  json source = json::empty; // enum values
  char url[100];
}connection;
connection connections[4];

struct {
      std::string ssid="JRJAG";
      std::string pwd="GeorgeTheDogy";
      bool isDHCP;
      std::string ip="10.0.0.117";
      std::string gw="10.0.0.125";
      std::string mask="255.255.255.128";

      std::string toStr(){
        char s[125];
        size_t n;
        n = sprintf(s, "{\"json\":%i,\"ssid\":%s,\"pwd\":%s,\"isDHCP\":%s,\"ip\":%s,\"gw\":%s,\"mask\":%s"
        , json::wifi, ssid.c_str(), pwd.c_str(), isDHCP?"true":"false", ip.c_str(), gw.c_str(), mask.c_str());
        return(std::string(s, n));
      }
} net;

struct remote_Data{
  float temp;
  double press;
  byte humid;
  uint64_t sleepTIMEus=10000000;//10sec     5*60*1000000; // 5 minutes
  float vcc;
  unsigned long remoteTime=0;
  unsigned long atticTime=0;
 
  String toStr(){
    char c[150];
    sprintf(c, "{\"temp\":%.1f,\"press\":%.2f,\"humid\",:%u,\"vcc:%.2f,\"sleepTIMEus\":%llu, \"remoteTime\":%lu,\"atticTime\":%lu}", temp, press, humid, vcc, sleepTIMEus, remoteTime, atticTime);
    return c;
  }
}now_Msg;

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
  unsigned long fanOnMs=0;
  byte attic_on;   // Attic temp to force Fan ON: 0 to disable (Master Override)
  byte attic_off;  // Attic temp to turn fan OFF: 0 to dissable
  byte inside_off=70;  // Fan is off if t2 (inside) is below 70 (default) degrees;
  byte AID_on;     // Attic - Inside temp delta for fan ON: 0 to dissable   (precedence 3)
  byte AID_off;    // Attic - Inside temp delta for fan OFF: 0 to dissable
  byte AOD_on;     // Attic - Outside temp delta for fan ON: 0 to dissable  (precedence 2)
  byte AOD_off;    // Attic - Outside temp delta for fan OFF: 0 to dissable
  byte IOD_on;     // Inside - Outside temp delta for fan ON: 0 to dissable (precedence 1)
  byte IOD_off;    // Inside - Outside temp delta for fan OFF: 0 to dissable
  bool fanEnabled;  // true: ok to turn on fan; // logic simplifies some c++ later on
  unsigned long delayMs = 15000; // how often are temps checked in milliseconds
  uint64_t sleepTIMEus;// sent to the remote sensor; sets remote esp01 deep sleep interval
  bool RemoteOK;

    std::string toStr(json source){ // make JSON string
      char c[230];
      size_t n;
      if(source==json::readings) n = sprintf(c, "{\"json\":%i,\"t1\":%.1f,\"t2\":%.1f,\"t3\":%.1f,\"p3\":%.2f,\"h3\":%i,\"dp3\":%i,\"a3\":%i,\"vcc\":%.1f,\"atticEpochTime\":%lu,\"fanOn\":%hd,\"fanOnMs\":%lu,\"fanEnabled\":%hd}"
      , json::readings, t1, t2, t3, p3, h3, dp3, a3, vcc, atticEpochTime, fanOn?1:0, fanOnMs, fanEnabled);

      if(source==json::settings) n = sprintf(c, "{\"json\":%i,\"fanOn\":%i,\"fanOnMs\":%lu,\"attic_on\":%i,\"attic_off\":%i,\"AID_on\":%i,\"AID_off\":%i,\"AOD_on\":%i,\"AOD_off\":%i,\"IOD_on\":%i,\"IOD_off\":%i,\"fanEnabled\":%hd,\"delayMs\":%lu,\"sleepTIMEus\":%llu}"
      , json::settings, fanOn?1:0, fanOnMs, attic_on, attic_off, AID_on, AID_off, AOD_on, AOD_off, IOD_on, IOD_off, fanEnabled, delayMs, sleepTIMEus);

      return std::string(c, n);
    }
    // true==turn on fan
    bool turn_on_fan_due_to_temp_differences(){
      return((((t1-t3 >= AOD_on)&AOD_on) | ((t1-t2 >= AID_on)&AID_on) | ((t2-t3 >= IOD_on)&IOD_on) | ((t1>=attic_on)&attic_on)) | (bool)fanOnMs);
    }
    //false==turn off fan: temp differences
    bool turn_off_fan_due_to_temp_differences(){

      if(IOD_off && t1-t3<IOD_off)return false;
      if(AOD_off && t1-t3<AOD_off)return false;
      if(AID_off && t1-t3<AID_off)return false;
      if(attic_off && t1<attic_off)return false;

      return(AID_off | AOD_off | IOD_off | attic_off);
    }
  }atticFan;


  void setup() {

    Serial.begin(115200);        // Start the Serial communication to send messages to the computer
    delay(10);
    Serial.println("\r\n");

    startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
    startOTA();                  // Start the OTA service
    startSPIFFS();               // Start the SPIFFS and list all contents
    startWebSocket();            // Start a WebSocket server
    startMDNS();                 // Start the mDNS responder
    startServer();               // Start a HTTP server with a file read handler and an upload handler
  }

  void loop() {
    webSocket.loop();                          // constantly check for websocket events
    webServer.handleClient();                  // run the server
    ArduinoOTA.handle();                       // listen for OTA events
  }
  void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) { // When a WebSocket message is received
    switch (type) {
      case WStype_DISCONNECTED:             // if the websocket is disconnected
        Serial.printf("[%u] Disconnected!\n", num);
        break;
      case WStype_CONNECTED: {              // if a new websocket connection is established
          IPAddress ip = webSocket.remoteIP(num);
          Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        }
        break;
      case WStype_TEXT:                     // if new text data is received
        Serial.printf("[%u] get Text: %s\n", num, payload);
        break;
    }
  }
  String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
    if (bytes < 1024) {
      return String(bytes) + "B";
    } else if (bytes < (1024 * 1024)) {
      return String(bytes / 1024.0) + "KB";
    } else if (bytes < (1024 * 1024 * 1024)) {
      return String(bytes / 1024.0 / 1024.0) + "MB";
    }
    return String("x");
  }
  String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
  void startWiFi() {                         // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
    WiFi.softAP(ssid, password);             // Start the access point
    Serial.print("Access Point \"");
    Serial.print(ssid);
    Serial.println("\" started\r\n");

    wifiMulti.addAP("ssid_from_AP_1", "your_password_for_AP_1");                 // add Wi-Fi networks you want to connect to
    wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
    wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

    Serial.println("Connecting");
    while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
      delay(250);
      Serial.print('.');
    }
    Serial.println("\r\n");
    if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
      Serial.print("Connected to ");
      Serial.println(WiFi.SSID());             // Tell us what network we're connected to
      Serial.print("IP address:\t");
      Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
    }else{                                     // If a station is connected to the ESP SoftAP
      Serial.print("Station connected to ESP8266 AP");
    }
    Serial.println("\r\n");
  }
  void startOTA() { // Start the OTA service
    ArduinoOTA.setHostname(OTAName);
    ArduinoOTA.setPassword(OTAPassword);

    ArduinoOTA.onStart([]() {Serial.println("OTA Start");});
    ArduinoOTA.onEnd([]() {Serial.println("\r\nOTA End");});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){
                  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
                });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("OTA ready\r\n");
  }
  void startSPIFFS() { // Start the SPIFFS and list all contents
    SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
    Serial.println("SPIFFS started. Contents:");{
      Dir dir = SPIFFS.openDir("/");
      while (dir.next()){                      // List the file system contents
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
      }
      Serial.printf("\n");
    }
  }
  void startWebSocket() { // Start a WebSocket server
    webSocket.begin();                          // start the websocket server
    webSocket.onEvent(webSocketEvent);          // if there's an incoming websocket message, go to function 'webSocketEvent'
    Serial.println("WebSocket server started.");
  }
  void startMDNS() { // Start the mDNS responder
    MDNS.begin(mdnsName);                        // start the multicast domain name server
    Serial.print("mDNS responder started: http://");
    Serial.print(mdnsName);
    Serial.println(".local");
  }
  void startServer() { // Start a HTTP server with a file read handler and an upload handler
    webServer.on("/", HTTP_GET, [](){webServer.send_P(200, "text/http", index_html)});
    webServer.on("/settings", HTTP_GET, [](){webServer.send_P(200, "text/http", settings_html)});
    webServer.on("/wifi", HTTP_GET, [](){webServer.send_P(200, "text/http", wifi_html)});

    webServer.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
      webServer.send(200, "text/plain", "");
    }, handleFileUpload);                       // go to 'handleFileUpload'

    webServer.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                                // and check if the file exists

    webServer.begin();                             // start the HTTP server
    Serial.println("HTTP server started.");
  }
//  These are the function definitions of the functions used in the setup. Nothing new here, apart from the startWebSocket function. You just have to start the WebSocket server using the begin method, and then give it a callback function that is executed when the ESP receives a WebSocket message.
//  Server handlers
//  This is the code that is executed on certain server-related events, like when an HTTP request is received, when a file is being uploaded, when there's an incoming WebSocket message ... etc.
  void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
    if(!handleFileRead(webServer.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
      webServer.send(404, "text/plain", "404: File Not Found");
    }
  }
  bool handleFileRead(String path) { // send the right file to the client (if it exists)
    Serial.println("handleFileRead: " + path);
    if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
    String contentType = getContentType(path);             // Get the MIME type
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
      if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
        path += ".gz";                                         // Use the compressed verion
      File file = SPIFFS.open(path, "r");                    // Open the file
      size_t sent = webServer.streamFile(file, contentType);    // Send it to the client
      file.close();                                          // Close the file again
      Serial.println(String("\tSent file: ") + path);
      return true;
    }
    Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
    return false;
  }
  void handleFileUpload(){ // upload a new file to the SPIFFS
    HTTPUpload& upload = webServer.upload();
    String path;
    if(upload.status == UPLOAD_FILE_START){
      path = upload.filename;
      if(!path.startsWith("/")) path = "/"+path;
      if(!path.endsWith(".gz")) {                          // The file server always prefers a compressed version of a file 
        String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
        if(SPIFFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
          SPIFFS.remove(pathWithGz);
      }
      Serial.print("handleFileUpload Name: "); Serial.println(path);
      fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
      path = String();
    } else if(upload.status == UPLOAD_FILE_WRITE){
      if(fsUploadFile)
        fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
    } else if(upload.status == UPLOAD_FILE_END){
      if(fsUploadFile) {                                    // If the file was successfully created
        fsUploadFile.close();                               // Close the file again
        Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
        webServer.sendHeader("Location","/success.html");      // Redirect the client to the success page
        webServer.send(303);
      } else {
        webServer.send(500, "text/plain", "500: couldn't create file");
      }
    }
  }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
        memcpy(&now_Msg, incomingData, sizeof(now_Msg));  // maybe need len or sizeof incoming data?? (TBD)
        atticFan.t3=now_Msg.temp;
        atticFan.p3=now_Msg.press;
				atticFan.h3=now_Msg.humid;
				atticFan.dp3=getDewPoint();
        atticFan.a3=getDensityAlt();
        atticFan.vcc=now_Msg.vcc;
        atticFan.RemoteOK = atticFan.atticEpochTime < 5*atticFan.delayMs+now_Msg.remoteTime;

				// if(now_Msg.remoteTime>atticFan.atticEpochTime)atticFan.atticEpochTime=now_Msg.remoteTime;
			  now_Msg.atticTime=atticFan.atticEpochTime;
        last_Remote_Contact_millis=millis();
        // send atticFan to all "readings clients"
        for(int i = 0;i<4;i++){
          if(connections[i].source==json::readings)
            webSock.text(connections[i].clientID, atticFan.toStr(json::readings).c_str());
        }

        webSock.cleanupClients();
Serial.printf("\n%i OnDataRecv now_Msg: %s\n", __LINE__, now_Msg.toStr().c_str());Serial.flush();
				esp_now_send(peer, (uint8_t *)&now_Msg, sizeof(now_Msg)); delay(25);
	}
  void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
    if (sendStatus == 0){Serial.printf("\n%i NOW: Delivery success\n", __LINE__);Serial.flush();}
    else{Serial.printf("\n%i NOW: Delivery fail\n", __LINE__);Serial.flush();}
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
        atticFan.attic_on     = std::stoi(valFromJson(s, "attic_on"));
        atticFan.attic_off    = std::stof(valFromJson(s, "attic_off"));
        atticFan.AID_on       = std::stof(valFromJson(s, "AID_on"));
        atticFan.AID_off      = std::stof(valFromJson(s, "AID_off"));
        atticFan.AOD_on       = std::stof(valFromJson(s, "AOD_on"));
        atticFan.AOD_off      = std::stof(valFromJson(s, "AOD_off"));
        atticFan.IOD_on       = std::stof(valFromJson(s, "IOD_on"));
        atticFan.IOD_off      = std::stof(valFromJson(s, "IOD_off"));
        atticFan.fanEnabled   = std::stoi(valFromJson(s, "fanEnabled"))?1:0;
        atticFan.fanOnMs      = std::stoi(valFromJson(s, "fanOnMs"));
        atticFan.delayMs      = std::stoul(valFromJson(s, "delayMs"));
        atticFan.sleepTIMEus  = std::stoull(valFromJson(s, "sleepTIMEus"));
  }
  bool saveAtticFanStruct(){
        File f = LittleFS.open(F("/settings"), "w");
        if(f){
                f.print(atticFan.toStr(json::settings).c_str());
                f.close();
                return true;
        }
        else{return false;}
  }
  bool initCreds(){
        File f = LittleFS.open("/net", "r");
        if(f){
               const std::string s = f.readString().c_str();
                f.close();
                initCreds(s);
                return(true);
        }
        return(false);
  }
  void initCreds(const std::string &s){
        net.ssid=valFromJson(s, "ssid");
        net.pwd=valFromJson(s, "pwd");
        net.isDHCP = valFromJson(s, "isDHCP")=="true"?1:0;
        net.ip     = valFromJson(s, "ip");
        net.gw = valFromJson(s, "gw");
        net.mask = valFromJson(s, "mask");
  }
  bool saveCreds(){
        File f = LittleFS.open(F("/net"), "w");
        if(f){
                f.print(net.toStr().c_str());
                f.close();
                wifiConnect(WIFI_AP_STA);
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
              if(net.isDHCP==false)
                if(WiFi.config(str2IP(net.ip), str2IP(net.gw), str2IP(net.mask)))
                  WiFi.begin(net.ssid.c_str(), net.pwd.c_str());
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
              if(!net.isDHCP)
              	WiFi.config(str2IP(net.ip), str2IP(net.gw), str2IP(net.mask));
              WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
              WiFi.softAP(AP_SSID, "", 3, false, 4);
							WiFi.begin(net.ssid.c_str(), net.pwd.c_str(), 1);//, 7, (const uint8_t *)__null, true);
           	break;
           case WIFI_OFF: return(true);
        }
        unsigned int startup = millis();
        while(WiFi.status() != WL_CONNECTED){
              delay(500);
              Serial.print(".");
              if(millis() - startup >= 5000) break;
        }Serial.println("");

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
    size_t start, end;
    start = json.find(element);
    start = json.find(":", start)+1;
    if(json.substr(start,1) =="\"")start++;
    end  = json.find_first_of(",]}\"", start);

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

