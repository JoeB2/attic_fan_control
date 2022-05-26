#define dbg

#include <Arduino.h>
#include <LittleFS.h>
#include <string>

bool saveAtticFanStruct();
void initAtticFanStruct();
void setAtticFanStruct(const std::string &s);
std::string valFromJson(const std::string &json, const std::string &element);

const char s0[] = "{\"src\":\"mcuSettings\",\"fanOn\":0,\"attic_on\":155,\"attic_off\":0,\"inside_off\":70,\"AID_on\":0,\"AID_off\":0,\"AOD_on\":0,\"AOD_off\":0,\"IOD_on\":10,\"IOD_off\":3,\"systemEnabled\":1,\"delayMs\":15000,\"sleep_us\":5000000,\"updated\":0}";

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
  u64 sleep_us;                 // sent to the remote sensor; sets remote esp01 deep sleep interval
  bool updated = false;

    std::string mcuMain(){      // make JSON string: #1 Readings, #2 Settings
      char c[250];
      size_t n;
      n = sprintf(c, "{\"src\":\"mcuMain\",\"t1\":%# 6.1f,\"t2\":%# 6.1f,\"t3\":%# 6.1f,\"p3\":%u,\"h3\":%u,\"vcc\":%hu,\"fanOn\":%hhu,\"fanOnMs\":%i,\"systemEnabled\":%hhd}"
      , t1, t2, t3, p3, h3, vcc, fanOn?1:0, fanOnMs, systemEnabled?1:0);

      return(std::string(c, n));
    }
    std::string mcuSettings(){ // make JSON string: #1 Readings, #2 Settings
      char c[285];
      size_t n;

      n = sprintf(c, "{\"src\":\"mcuSettings\",\"fanOn\":%hhu,\"attic_on\":%hhu,\"attic_off\":%hhu,\"inside_off\":%hhu,\"AID_on\":%hhu,\"AID_off\":%hhu,\"AOD_on\":%hhu,\"AOD_off\":%hhu,\"IOD_on\":%hhu,\"IOD_off\":%hhu,\"systemEnabled\":%hhu,\"delayMs\":%lu,\"sleep_us\":%u,\"updated\":%hhu}"
      , fanOn?1:0, attic_on, attic_off, inside_off, AID_on, AID_off, AOD_on, AOD_off, IOD_on, IOD_off, systemEnabled?1:0, delayMs, (u32)(sleep_us/1e6), updated);

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


void setup(){
    Serial.begin(74400); // initial boot message is at 74400
    delay(500);
    Serial.printf("\n%i\n", __LINE__);Serial.flush();

    LittleFS.begin();
    saveAtticFanStruct();
    initAtticFanStruct();
}

void loop(){
  delay(2e3);
  Serial.printf("%i\n%s\n\n", __LINE__, atticFan.mcuSettings().c_str());Serial.flush();
}

  bool saveAtticFanStruct(){
      File f = LittleFS.open(F("/settings"), "w");
      if(f){
Serial.printf("%i File: %s\tbytes:%i\nstring:%s\n", __LINE__, f?"opened":"failed", f.print(s0), s0);Serial.flush();
        f.close();
        return true;
    }
    else{return false;}
  }
  void initAtticFanStruct(){
    File f = LittleFS.open("/settings", "r");
    if(f){
          const std::string s = f.readString().c_str();
          f.close();
Serial.printf("%i File: %s\n", __LINE__, s.c_str());Serial.flush();
          setAtticFanStruct(s);
    }
  }
  void setAtticFanStruct(const std::string &s){
#ifdef dbg
  Serial.printf("%i init atticFan::settings: %s\n", __LINE__, s.c_str());Serial.flush();
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
