# attic_fan_control
Abstract:
  In the summer, Seattle Area, it can get hot.  Last summer, 2021 we saw several days well over 100f.
  Most homes in the Seattle area are not equiped with AC.  Attic space in my home can reach 130f degrees.
  If I could programatically manage an attic fan to turn on/off depending on Attic Temps & outside temps & inside house temps 
  it may reduce inside house temps.  (TBD)
  
  Using two ESP8266 -  ESP01:
  Control a solid state "relay" SSR-25 DA to turn on/off a box fan that covers an attic access in the ceiling.
  Using two Dallas DS18B20 temp sensors, one in the Attic and one in the hallway AND a remote esp01 sensor with a DS18B20
  the Attic esp01's C++ will determine whether to turn on the solid state module for the fan. 
  
Implementation:
  The Remote esp01 will be located in the shade outside in the back yard.  Its purpose will be to send outside
  temperatures to the Attic situated esp01, providing the c++ an opportunity to "imagine what's possible."
  The remote esp01 will have a single DS18B20 and a DS3231.
  
  Attic: esp01 handles 2x DS18B20 and a 2n2222 which excites a SSR-25 DA solid state relay.
    An NTP pool is contacted once every hour to get a current time value.
    Temperatures are checked and delta and threshold values are computed periodically based on a user provided value.
    A status web page is hosted which shows the various temperature values and the Fan On/Off status
    A setting page is hosted where a user can input threshold values for managing when the Fan is On/Off.
    Attic Master uses: WiFi STA mode, ESPNOW, WebSock, Relay, 2x DS18B20, WiFi AP_Mode when unable to connect to the SPIFFS stored SSID
    
    Fan on/Off Logic:
      1) Attic sensor: Threshold to force Fan on or off
      2) Attic sensor: compare self to inside to turn on/off fan
      3) Attic sensor: compare self to remote sensor to turn on/off fan
      4) Inside sensor: compare self to remote sensor to turn on/off fan
      Order of precidence: 1, 3, 4, 2.
      Setting any threshold value to zero dissables that check.
      
      NOTE:
        WiFi connectivity: The attic esp01 uses a JSON string stored in its SPIFFS {SSID, PWD, IP, GW, Mask, isDHCP}.
        If the esp01 failes to connect or if the SPIFFS JSON credential string fails the Attic esp01 goes into "AP_MODE"
        whereby a software AP SSID: "Attic Fan WiFi Setup: 10.0.1.23" is hosted.  Pointing a browser to the AP's IP will provide a page
        where an SSID, PWD, and preferred IP:GW:Mask can be entered.  If use DHCP is checked IP,GW, Mask are not used.

    Remote: esp01 handles a single DS18B20 temp sensor and a DS3231 clock.
      The temperature reading is entered into a struct along with the current time and the struct's binary bits are broadcast using ESPNOW.
      Upon reciving the remote's espnow broadcast the Attic esp01 will broadcast, using espnow, the same struct's bit with an updated ack bit and an updated time vale.
