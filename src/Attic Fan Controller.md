## Introduction

This ESP8266 web based application is intended to allow a user to set temperature difference thresholds, between 3 temperature probes, which will determine whether the attic fan will turn on or off.

## Architecture

The ESP8266 is an ESP01-1M.  The MCU hosts four web pages, readings, settings, wifi, and OTA (Over the Air firmware updates).  WebSockets are used to manage communication between the MCU and the web pages.  Web pages are hosted on the user's local wifi network, using provided ip4 and ssid/password information using the wifi web form.  Additionally the MCU simultaneously hosts an access point ("AtticFan WiFi: 10.0.1.23") on a fixed ip at 10.0.1.23.  The access point allows the user to set ip4 information for the local wifi network, read sensor and fan status values, set fan activation thresholds, and update the firmware over wifi - the same as the local WiFi ip would.  The Attic and inside temperature probes are dallas DS18B20 probes using the "one wire" protocol.  The outside probe is a BME280 using the i2c protocol and it transmits its values using the proprietary ESPNOW protocol.  Rem,ote sensor values are sent every "Remote Sensor Check-In Minutes" which is configurable on the setting web page.

Threshold and wifi values are stored in non volatile memory using JSON strings.  The string format is as follows:


### atticFan JSON:
***[{"t1":75.2,"t2":75.4,"t3":74.6,"p3":29.25,"h3":32,"dp3":42,"a3":1448,"vcc":3.0,"atticEpochTime":1638658867,"fanOn":false,"fanOnMs":96},{"attic_on":150,"attic_off":0,"AID_on":0,"AID_off":0,"AOD_on":0,"AOD_off":0,"IOD_on":7,"IOD_off":3,"fanEnabled":false,"delayMs":0,"sleepTIMEus":15000000}]***

###  WiFi JSON:
***{"ssid":"mySSID","pwd":"myPassword","isDHCP":false,"ip":"10.9.8.07","gw":"10.9.8.128","mask":"255.255.255.128"}***

The JSON strings can me modified using the associated web pages and uploaded over the connected wifi network.  

## Operation
  Network Setup
    1) after startup of the attic MCU (micro controller unit) connect to the WiFi access point "AtticFan WiFi: 10.0.1.23".
    2) Point your web browser to: http://10.0.1.23/wifi
        Note: lower case wifi.
    3) Fill out the WiFi settings with the values for your local WiFi network and submit, which will save  the values to non volatile memory (SPIFFS) in a JSON string.
    4) After the WiFi settings are saved the MCU will attempt to re initiate connection to local WiFi provided when filling out the WiFi web form.  An MCU restart is typically not required.

## Home readings web page (http://ip/)

Temperature are provided for all three temperature probes.  The outside probe also provided humidity and barometric pressure reading.  The Attic MCU uses the outside prob's val;ues to calculate outside dew point and density altitude.  The reading page provides "Fan On Timer" values and to disable or active the system.

    
## Settings (http://ip/settings)

Values are provided using the web form at: http://ip/settings, where ip is the local WiFi ip provided during step 1 of the Network Setup.  Alternatively you can use the Access Point ip, after connecting to the MCU's provided access point.
Settings "AOD, AID, IOD" are value of difference in temperature fahrenheit between temperature probes.  IOD_on (inside - outside = difference) turns on the fan if the difference between the inside temperature probe and the outside temperature probe is equal or greater than the value provided.  Similarly the fan will turn off if: there are no other fan on values whose thresholds have been met and the *inside temp probe* **-** *outside temp probe* **<** ***IOD_off***.  

Order of precedence for automating fan ***on***:
  Fan On:
    - Force Fan On Minutes
    - Fan On Attic Temp
    - IOD on
    - AID on
    - AOD on

Setting "fan System Enabled" to off will disable the fan.
Otherwise IOD off, AOD off, AID off, Attic Fan off temp will be honored with the first threshold met that doesn't conflict with ***any*** fan on threshold turning off the fan.  If no fan off threshold is set the fan will turn off when no "fan On" threshold is met.

Attic Sensor Check-In Seconds setting will change the frequency that the system looks at Attic and Inside temperature probe values and also the frequency that the settings are tested and sent to the web page.

Likewise the "Remote Sensor Check-In Minutes" setting will determine the frequency that the battery operated outside sensor wakes up and transmits the outside temperature.  Battery life will be extended with greater Remote Sensor Check-In values.
