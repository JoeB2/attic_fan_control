// pgm_read_byte() bug Stack Dumps: https://github.com/esp8266/Arduino/issues/3140 
#undef pgm_read_byte(addr)
#define pgm_read_byte(addr)                                                             \
(__extension__({                                                                        \
    PGM_P __local = (PGM_P)(addr);  /* isolate varible for macro expansion */           \
volatile ptrdiff_t __offset = ((uint32_t)__local & 0x00000003); /* byte aligned mask */ \
    const uint32_t* __addr32 = (const uint32_t*)((const uint8_t*)(__local)-__offset);   \
    uint8_t __result = ((*__addr32) >> (__offset * 8));                                 \
    }))
// END pgm_read_byte()

const char MAIN[] PROGMEM =  R"=====(
<!DOCTYPE html>
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <link
            rel="stylesheet"
            href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"
            integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr"
            crossorigin="anonymous"
        />
        <title>Attic Fan Controller</title>
        <style>
            @viewport{width:device-width;zoom:1.0;}
            @-ms-viewport {width: device-width ;}
            html{
                font-family: Arial;
                display: inline-block;
                margin: 0px auto;
                text-align: left;
                min-width: 544px;
                min-height:777px;
                background-color: dodgerblue;
            }
            h2 {
                font-size: 4.0vw;
                text-align: center;
            }
            h3 {
                font-size: 2.8vw;
                font-weight: bold;
                text-align: center;
                background-color: rgb(255,255,255);
                color: blue;
                vertical-align: top;
                margin-top:0px;
            }
            .help {
                border: 0;
                line-height: 3.5vw;

                padding: 0 1vw;
                text-align: center;
                font-weight:bold;
                font-size: 2.2vw;

                cursor: pointer;
                background-color: rgb(255, 255, 255);
                box-shadow: inset 1vw 1vw 1vw rgba(255, 255, 255, 0.6),
                    inset -.3vw -.25vw .3vw rgba(0, 0, 0, 0.6);

                border:19vw;

                margin-left:.3vw;
                margin-top: 4vw;
                padding: 1vw;

                width:67.9vw;
                margin-inline: 1vw;
            }
            .vcc {
                color: #242121;
                font-weight: bold;
                font-family: Verdana;

                text-align: center;
                vertical-align: top;
                padding-left: 1vw;
                padding-top: -1vw;
                font-size: 2.5vw;
                grid-column:5;
                grid-row:1/2;
            }
            .blink {
                animation: blinker 1.9s linear infinite;
                color: #a81a1a;
                font-weight: bold;
                font-family: Verdana;

                padding-left: 1vw;
                font-size: 2.5vw;
                grid-column:5;
                grid-row:1/2;
            }
            @keyframes blinker {
                99% {
                    opacity: 0;
                }
            }
            .sensors-grid{
                display:grid;
                grid-template-columns:25vw 13vw 21vw 13vw;
                grid-template-rows: 6vw 4vw 4vw 4vw 7vw;

                box-sizing: border-box;
                width: min(100%, 72.5vw);
                min-width: 39vw;
                height: 25vw;

                margin-left: 5vw;
                margin-right: 5vw;

                background-color: #ced3c7;
                justify-items: left;

                padding: 1.3vw 2.3vw;

                font-size:2.8vw;
                font-weight: bold;
            }
            .status-grid{
                display:grid;
                grid-template-columns:7vw 18vw 15vw 17vw 8vw;
                grid-template-rows: 4vw 9.5vw;

                box-sizing: border-box;
                width: min(100%, 72.5vw);
                min-width: 39vw;
                height: 16.5vw;

                margin-left: 5vw;
                margin-right: 5vw;

                background-color: #ced3c7;
                justify-items: center;

                padding: 1vw 2.8vw;

                font-size:2.5vw;
                font-weight: bold;
                text-align:center;
            }
            .time {
                grid-column: 4 / 5;
                color: blue;
                font-size:2.0vw;
            }
        </style>
    </head>
        <h3 style="margin: 1vw 5vw;width: 72.5vw;">ESP8266 Attic Fan Manager</h3>
        <main class="sensors-grid" style="font-size:2.4vw;">
            <div>
                <span id="bt_help" class="help" role="submit"
                    onclick="f_help()" value="Help" title="Hide/Show WebSock Messages">Help
                </span>
            </div>
            <div id="clock" class="time"></div>
            <div>
                <i class="fas fa-thermometer-three-quarters" style="color: #ff0022; margin-left: 1.6vw;font-size: 2.3vw"></i>
                <span title="Inside House Temp.  Dallas Sensor (DS18B20)">Inside Temp</span>
            </div>
            <div>
                <span id="t2">%T2%</span>
                <sup class="units">&deg;F</sup>
            </div>
            <div>
                <i class="fas fa-thermometer-half" style="color: #1e68f3; margin-left: 1.4vw;font-size: 2.3vw"></i>
                <span title="Remote Sensor Temperature. (BME280)" >Outside Temp</span>
            </div>
            <div>
                <span id="t3">%T3%</span>
                <sup class="units">&deg;F</sup>
            </div>
            <div>
                <i class="fas fa-thermometer-full" style="color: #ff2002cc; margin-left: 1.6vw;font-size: 2.3vw;"></i>
                <span title="Controller Box's Sensor Temp.  Dallas Sensor (DS18B20)">Attic Temp</span>
            </div>
            <div>
                <span id="t1">%T1%</span>
                <sup class="units">&deg;F</sup>
            </div>
            <div>
                <i class="fas fa-tint" style="color:#00add6; margin-left: 1.2vw;font-size: 2.3vw"></i>
                <span title="Relative Humidity.  Percent moisture possible at the indicated temperature.">Humidity</span>
            </div>
            <div>
                <span id="h3">%H3%</span>
                <sup class="units">%</sup>&nbsp&nbsp
            </div>
            <div><i class="fas fa-thermometer-empty" style="color:#00add6; margin-left: 1.5vw;font-size: 2.3vw"></i>
                <span title="Temperature water comes out of the air.">Dew Point</span>
            </div>
            <div>
                <span id="dp3">%DP3%</span>
                <sup class="units">&degF</sup>&nbsp&nbsp
            </div>
            <div>
                <i class="fas fa-tachometer-alt" style="color: #06b106; margin-left: .50vw; font-size:2.3vw;"></i>
                <span title="Air pressure :  NOTE: Standard is 14.7 PSI at Sea Level. (29.92 in Hg)">Barometer</span>
            </div>
            <div>
                <span id="p3">%P3%</span>
                <sup class="units">Hg</sup>
            </div>
            <div>
                <svg enable-background='new 0 0 58.422 40.639'height=2vw id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=4vw x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px>
                    <g>
                        <path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902
                            c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004
                            c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994
                            C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5
                        />
                        <path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0
                            c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004
                            C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813
                            C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5
                        />
                    </g>
                </svg>
                <span title="Functional Altitude.  Altitude you feel & engines & planes feel.">Density Altitude</span>
            </div>
            <div>
                <span id="a3">%A3%</span>
                <sup class="units">ft</sup>&nbsp&nbsp
            </div>
        </main>
        <main class="status-grid" style="background:yellow;">
            <div style="width: 10vw;">Fan</div>
            <div>System</div>
            <div>Fan On</div>
            <div>Minutes</div>
            <div id="fanOn"
                style= "padding: 3.3vw 1.2vw;
                        border-radius: 25%;
                        box-shadow: inset 2px 2px 3px rgba(153, 150, 150, .6),
                        inset -2px -2px 3px rgba(0, 0, 0, 0.6);
                        background-color: rgb(248, 113, 135);"
                title="Indicator for showing whether the Fan is ON or OFF"
            >OFF</div>
            <div id="system"
                style=" padding: 3.2vw 1.0vw;
                        text-transform: capitalize;
                        border-radius: 50%;
                        font-weight: bold;
                        box-shadow: inset 2px 2px 3px rgba(153, 150, 150, .6),
                        inset -2px -2px 3px rgba(0, 0, 0, 0.6);
                        cursor: pointer;"
                onclick="f_system('main')"
                role="submit"

                title="Enable / Disable Fan Control System"
                >Enable
            </div>
            <div>
                <span
                    class="help"
                    style="width: 4vw;padding:0.8vw;"
                    role="submit"
                    onclick="f_fanTimer('main', +5)"
                    title="add 5 minutes to fan on override"
                    >+ 5mins
                </span><br><br>
                  <span
                    class="help"
                    style="width: 4vw;padding:.7vw;"
                    role="submit"
                    onclick="f_fanTimer('main', -5)"
                    title="remove 5 minutes from fan on override"
                    >-- 5mins
                </span>
            </div>
            <div>
                <a id="fanOnMs">0</a><br><br>
            </div>
            <div class="vcc" id="battery" >
                <span id="vcc0" name="bat" hidden>Charge<br>Remote<br>Battery<br></span>
                <span id="vcc1" name="bat" hidden style="font-size: 1.5vw;">3.0</span>
    <sup                        name="bat" hidden style="font-size: 1.5vw;">vcc</sup>
            </div>
        </main>
        <main>
            <!-- comment: NOTES -->
            <div id="help" hidden="true">
                <p class="settings-grid">
                    <span class="help"
                        value="Controller Limits"
                        type="button"
                        onclick="f_redirect('settings')"
                        style="margin-left:14vw;background: rgb(155, 219, 149)"
                        title="Web Page for setting Fan ON/OFF Parameters"
                        >Controller Limits
                    </span>
                    <span class="help"
                        value="WiFi Settings"
                        type="button"
                        onclick="f_redirect('wifi')"
                        style="background: orange;"
                        title="Web Page for setting WiFi SSID/PWD & IP"
                        >WiFi Settings
                    </span>
                    <span class="help"
                        value="Firmware"
                        type="button"
                        onclick="f_redirect('update')"
                        style="background:rgb(249, 253, 0);color: red;"
                        title="Web Page for updating firmware"
                        >Firmware
                    </span>
                    <details><hr>JSON Inbound
                        <aside id="json1" style="background-color:white;font-weight: normal; font-size: 1.5vw; word-wrap:  break-word;">{"src":"mcuMain","what":"in bound"}</aside>
                        <br>JSON Outbound
                        <aside id="json2" style="background-color:white;font-weight: normal; font-size: 1.5vw; word-wrap:  break-word;">{"src":"main","what":"out bound"}</aside>
                        <hr>
                    </details>
                </p>
            </div>
        </main>
    <script language="javascript" type="text/javascript">
        // Logic: Minutes is everything. i.e. 0 minutes & system is off >0 mins & system is on
        window.onload = function() {setInterval( f_clock, 1000);lastWebSock = Date.now();}
        let webSock = new WebSocket("ws://" + window.location.hostname + ":80");
        webSock.onopen = function(){ 
            let json = {src:"main",what:"refresh"};
            webSock.send(JSON.stringify(json));
        };
        webSock.onmessage = function(evt) {f_webSockOnMessage(evt);};
        webSock.onerror = function(evt) {console.warn(evt);};
        webSock.onclose = function(){};
        let sleep_s = Number(30); // no WebSock inbound timeout; send refresh signal
        let lastDataRefresh = Number(1);
        let lastPageRefresh = Number(1);
        

        const ALTITUDE = Number(394);
        let j = {
              src: "main"
            , what: "refresh" // system, timer
            , t1: 75.60
            , t2: 78.33
            , t3: 71.40
            , p3: 99722
            , h3: 8823
            , vcc: 2850
            , fanOn: 1
            , fanOnMs: 138000
            , systemEnabled: 1
        };
        function f_webSockOnMessage(evt){
            if (typeof evt.data === "string"){
                let s = evt.data;
                j = JSON.parse(s);
                if(j.src!="mcuMain")return;
                lastWebSock = Date.now();
                document.getElementById("json1").innerHTML = s;
                for (let key in j){
                    switch (key){
                        case "src": continue;
                          break;
                        case "t1":
                            document.getElementById(key).innerHTML = j.t1?j.t1.toFixed(1):"n/a";
                          break;
                        case "t2":
                            document.getElementById(key).innerHTML = j.t2?j.t2.toFixed(1):"n/a";
                          break;
                        case "t3":
                            document.getElementById(key).innerHTML = j.t3?j.t3.toFixed(1):"n/a";
                          break;
                        case "p3":
                            if(j.p3 == 0){
                                document.getElementById(key).innerHTML = "n/a";
                                document.getElementById("a3").innerHTML = "n/a";
                                break;
                            }
                            document.getElementById(key).innerHTML = Pa2Hg(j.p3/100.0).toFixed(2);
                            document.getElementById("a3").innerHTML = f_densityAlt(j.t3, j.p3);
                          break;
                        case "h3":
                            if(j.h3 == 0){
                                document.getElementById("h3").innerHTML = "n/a";
                                document.getElementById("dp3").innerHTML = "n/a";
                                break;
                            }
                            document.getElementById("h3").innerHTML = (j.h3/100).toFixed(1);
                            document.getElementById("dp3").innerHTML = c2f( f_dewPoint( j.t3, j.h3/100) ).toFixed(1);
                          break;
                        case "vcc":
                            f_battery(j[key]);
                          break;
                        case "fanOn":
                            f_fan(j[key]);
                          break;
                        case "fanOnMs":
                                f_fanTimer('mcuMain', f_ms2min(j[key]));
                          break;
                          case "systemEnabled":
                            f_system(j.src);
                          break;
                          case "sleep_us":  // refresh data thrice per remote's sleep
                            sleep_s = j[key]/1000000;
                          break;
                        default:
                            document.getElementById(key).innerHTML =j[key];
                    }
                }
            }
        }
        function f_dewPoint(t, h){return(Math.round(f2c(t)-((100-h)/5)));} // t:celsius
        function f_densityAlt(t, p){
            tempC=f2c(t);pHg=Pa2Hg(p/100);
            PA=((29.92-pHg)*1000)+ALTITUDE;
            sTemp=15-(.002*PA);
            DA = PA + (118.8 * (tempC-sTemp));
            return Math.round(DA);
        }
        function Hg2Mb(p){return(33.86389*p);}
        function f2k(f){return((f-32)/1.80 + 273.15);}
        function f2c(f){return((f-32)/1.80)}
        function c2f(c){return(1.80*c+32)}
        function Pa2Hg(p){return(p*0.029529980164712);}
        function f_ms2min(ms){
            const minutes = Math.floor(ms/60000);
            const tenths  = Math.round(((ms/60000)-minutes)*10)/10;
            return(minutes+tenths);
        }
        function f_redirect(where) {window.location.href = "/" + where;}
        function f_fanTimer(src, val){ // p1 : src
            if(src=="main"){
                j.fanOnMs+=(val*60*1000);
                if(j.fanOnMs<0)j.fanOnMs=0;
                let j2 = {"src":"main","what":"timer","fanOnMs":0};
                j2.fanOnMs = j.fanOnMs;
                webSock.send(JSON.stringify(j2));
                document.getElementById("json2").innerHTML = JSON.stringify(j2);
            }
            if(src == "mcuMain")document.getElementById("fanOnMs").innerHTML=val;
        }
        function f_system(p1) {
            // src: mcuMain or main
            const system = document.getElementById("system");
            if(p1=="main"){
                if(j.systemEnabled)j.systemEnabled=0;
                else j.systemEnabled=1;

                let j2={"src":"main","what":"system","systemEnabled":0};
                j2.systemEnabled = j.systemEnabled;
                webSock.send(JSON.stringify(j2));
                document.getElementById("json2").innerHTML = JSON.stringify(j2);
            }
            if(p1=="mcuMain"){
                system.style.backgroundColor = j.systemEnabled ? "rgb(250, 0, 0)" : "rgb(0, 250, 0)";
                system.innerHTML = j.systemEnabled ? "STOP" :"Enable" ;
            }
        }
        function f_battery(batt){
            const battery = document.getElementById("battery");
            let elements = document.getElementsByName("bat");
            elements.forEach(function(index){
                if(batt)index.removeAttribute("hidden");
                else index.setAttribute("hidden", true);
            })
           if(batt<3000){ // HY7333 outputs ~3.3; if vcc drops too low then
                battery.className = "blink";
                var bat = "Charge&nbsp<br>Remote<br>Battery<br>";
                }else{
                battery.className = "vcc";
                bat = "Remote<br>Battery<br>";
            }
            document.getElementById("vcc0").innerHTML = bat;
            document.getElementById("vcc1").innerHTML = (batt/1000).toFixed(2);
        }
        function f_fan(){
            fan = Number(arguments[0]);
            doc = document.getElementById("fanOn");
            doc.innerHTML = fan ? "ON" : "OFF";
            doc.style.color = fan?"white":"black";
            doc.style.backgroundColor = fan? "green":" rgb(248, 113, 135)";
        };
        let f_help = () => {
            if (document.getElementById("help").getAttribute("hidden")){
                document.getElementById("help").removeAttribute("hidden");
                document.getElementById("bt_help").innerHTML = "Un-Help";
            }else{
                document.getElementById("help").setAttribute("hidden", true);
                document.getElementById("bt_help").innerHTML = "Help";
            }
        };
        function f_clock(){
            const clock = document.getElementById("clock");
            d = new Date();
            clock.innerHTML = d.toLocaleDateString() + "<br>"
            + d.toLocaleTimeString(
                'en-US', {
                    timeZone:'America/Los_Angeles'
                , hour12:false
                , hour:'numeric'
                , minute:'numeric'
                , second:'numeric'
                }
            );
        }
        function stringToFloat(s){
            if (s.search(/^\s*(\+|\-)?\d*(\.\d*)?\s*$/)==-1)
                throw new Error("'"+s+"' is not a valid number", "'"+s+"' is not a valid number");
            return parseFloat(s);
        }
</script>
</html>
)=====";

  const char SETTINGS[] PROGMEM =  R"=====(
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta name="file" content="set_bounds.html" />
        <meta name="author" content="Joe Belson 20210822" />
        <meta
            content="esp8266 html for WiFi connected temp/humidity bounds."
        />
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>AtticFan Controller: Settings</title>
    </head>
    <style type="text/css">
        body {
            font-size: 12px;
            background-color: #f6f6ff;
            font-family: Calibri, Myriad;
            text-align: center;
        }
        table {
            text-align: center;
            margin-left: 5px;
            margin-right: auto;
            width: 350px;
            border: 1px solid black;
            border-style: solid;
            font-weight: bold;
            column-width: 15vw;
        }
        table td {
            background-color:#f6f6ff;
            color: black;
            border-style: solid;
            border-width: 1px;
        }
        table caption {
            border-width: 3px;
            border-style: solid;
            border-color: #bdc7bd #a2a5a3;

            font-size:medium;
            font-weight: bold;
            letter-spacing: 0.3em;
        }
        table.calibrate caption {
            background-color: #0d09ff;
            color: #fff;
        }
        table thead th {
            font-size: 16px;
            border-width: 6px;
            border-style: solid;
        }
        table.calibrate thead th {
            background-color: #98c2e9;
            border-color: #4666f7 #9cabf0;
        }
        table.override caption {
            background-color: #f74646;
            color: #fff;
        }
        table.override thead th {
            background-color: #f8d3de;
            border-color: #f79646 rgb(247, 191, 87);
        }
        .help {
                font-size: 1.5vw;
                text-align: center;
                font-weight: bold;

                background-color: rgb(248, 241, 241);
                box-shadow: inset 1vw 1vw 1vw rgba(225, 238, 225, 0.6),
                    inset -.3vw -.25vw .3vw rgba(16, 17, 16, 0.6);

                padding: .3vw .7vw;

                height:2.5vw;
                width:5%;
                cursor: pointer;
            }
        .fanOn{
                text-align: center;
                background-color: rgb(248, 241, 241);
                box-shadow: inset 1vw 1vw 1vw rgba(225, 238, 225, 0.6),
                    inset -.3vw -.25vw .3vw rgba(16, 17, 16, 0.6);

                height:2.0vw;
                padding:.1vw;
                background-color:yellow;
                font-weight: bolder;
                font-size: 18px;
        }
    </style>
    <body id="body" style="text-align: left">
        <span id="bt_help" class="help" role="submit" onclick="f_help()" >Help</span>
        <hr />
        <b><i>Note: Values are updated every "Attic Sensor Check-in Seconds". All values are in &degF</i></b>
        <table class="calibrate" id="settings">
            <caption>
                Temp & Humidity<br>ON/OFF Limits
            </caption>
            <thead>
                <tr>
                    <th>*Inside<br>Off &degf</th>
                    <th>Attic ON*<br>Absolute Temp&degf</th>
                    <th>Attic off<br>Temp&degf</th>
                </tr></thead>
                <tbody>
                    <tr>
                        <td><input id="inside_off" type="number" name="limit" min="0" max="120" onclick()="updating=1"/></td>
                        <td><input id="attic_on"   type="number" name="limit" min="0" max="120"/></td>
                        <td><input id="attic_off"  type="number" name="limit" min="0" max="120"/></td>
                    </tr>
                </tbody>
                <thead><tr>
                    <th>IOD on<br>Value*</th>
                    <th>AOD on<br>Value</th>
                    <th>AID on<br>Value</th>
                </tr></thead>
                <tbody>
                    <tr>
                        <td><input id="IOD_on" type="number" name="limit" min="0" max="120"/></td>
                        <td><input id="AOD_on" type="number" name="limit" min="0" max="120"/></td>
                        <td><input id="AID_on" type="number" name="limit" min="0" max="120"/></td>
                    </tr>
                </tbody>
    

                <thead><tr>
                    <th>IOD off<br>Value*</th>
                    <th>AOD off<br>Value</th>
                    <th>AID off<br>Value</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><input id="IOD_off" type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="AOD_off" type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="AID_off" type="number" name="limit" min="0" max="120"/></td>
                </tr>
            </tbody>
        </table>
        <p style=" font: 14px/17px Monospace ;">
            <span style=" font: bold 18px/20px Monospace ;">Values to trigger the Fan ON/OFF</span><br>
            <span style="font-size: 8pt;">
            AID: Attic minus Inside in degrees fahrenheit &degf<br>
            AOD: Attic minus Outside in degrees fahrenheit &degf<br>
            IOD: Inside minus Outside in degrees fahrenheit &degf<br>
            Attic off: Less than &degf Fan OFF. Unless on setting overrides.
        </span>
        </p>
        <p style="font-weight: bold; font-size: large">
            NOTE: Zero or Empty settings are ignored.
        </p>
        <hr/>
        <br/>
        <table class="override">
            <caption>Set Override Values</caption>
            <thead>
              <tr>
                <th>Fan System<br>Enabled</th>
                <th>Attic Sensor<br>Check-In Seconds</th>
              </tr>
            </thead>
            <tbody>
              <tr>
                <td>
                  <a id="systemEnabled" enabled="1"></a>
                  <input id="systemEnabled_true"  type="radio" name="systemEnabled" value="1" onclick="f_systemEnabled('radio', this.value)" autocomplete="off" style="align-items: center;display:inline-flex" checked>ON
                  <input id="systemEnabled_false" type="radio" name="systemEnabled" value="0" onclick="f_systemEnabled('radio', this.value)" autocomplete="off" style="align-items: center;display:inline-flex">OFF
                </td>
                <td><input id="delayMs"     type="number" name="limit" min="0" max="600"/></td>
              </tr>
            </tbody>
            <thead>
              <tr>
                <th>Remote Sensor<br>Check-In Seconds</th>
                <th>
                  <input id="refresh" type="submit" value="Refresh" onclick="f_refresh()" title="Reset all values with those on the server."
                         style=" margin: 1px; color: black; background-color: yellow; font-weight: bold;"/>
                  <input id="Submit" type="submit" value="Submit" onclick="f_submit()"
                         style="margin: 2px; color: black; background-color: rgb(248, 138, 171); font-weight: bold;"/>
                    </th>
                </tr>
            </thead>
            <tbody>
                <td><input id="sleep_us" type="number" name="limit"/></td>
                <td id="fanOn" class="fanOn">Fan is On</td>
</tbody>
        </table>
        <a id="updated" hidden style="padding: 1px 1px 1px 1px;font-size: 17px; background-color: green">UPDATED</a>
        <a style="font-weight: bold; font-size: 10pt">
            <br>
            A refresh occurs every "Attic Sensor Check-In Minutes"
        </a>
        <hr /><a>
            <input type="submit" onclick="history.back()" value="Home">
        </a><br>
        <div id="help" hidden="true" style="font-weight: bold;font-size: 13pt; word-wrap: break-word;">
            <a>JSON Inbound</a><br><a id="json1"></a>
            <br><br>
            <a>JSON Outbound</a><br><a id="json2"></a>
        </div>
    </body>

    <script language="javascript" type="text/javascript">

    let webSock = new WebSocket("ws://" + window.location.hostname + ":80");
    webSock.onopen = function (evt) {webSock.send('{"src":"settings","what":"refresh"}');}
    webSock.onmessage = function (evt) {f_webSockOnMessage(evt);}
    webSock.onerror = function (evt) {}
    document.getElementById("body").onchange = function() {updating=1;}

    let updating = 0;

    let settings = {
            src : "settings"
        , fanOn : 0
        , attic_on : 150.0
        , attic_off : 0.0
        , AID_on : 0
        , AID_off : 0
        , AOD_on : 0
        , AOD_off : 0
        , IOD_on : 7
        , IOD_off : 3
        , inside_off : 70
        , systemEnabled : 1
        , delayMs : 360000
        , sleep_us : Number(2*1e6+2)
        , updated : 0
    };
    function f_webSockOnMessage(evt) {
        if (typeof evt.data === "string") {
            r = JSON.parse(evt.data);
            if(r.src != "mcuSettings")return;
            settings=r;console.log(settings);
            if(updating ^ settings.updated)return; // ...updated == bool return from mcu save of the struct.
            document.getElementById("json1").innerHTML = JSON.stringify(settings);
            for (let key in settings){
                if(key == "src" || key == "what")continue;
                dv = document.getElementById(key);
                if(dv.value=="" || dv.value==null){
                    switch (key){
                        case "updated":
                            f_updated(settings.updated);  // set the Green Updated Flag On/Off
                            updating ^= settings.updated; // if this is an "updated mcu return" we are done with the last local update
                          break;
                        case "fanOn":
                            doc = document.getElementById("fanOn");
                            doc.innerHTML = settings.fanOn&settings.systemEnabled?"Fan is ON":"Fan is OFF";
                            doc.style.backgroundColor = settings.fanOn&settings.systemEnabled?"#03c04A":"red";
                          break;
                        case "delayMs":
                            dv.value = settings.delayMs/(1000);break;// milliseconds to Seconds
                          break;
                          case "systemEnabled":
                              f_systemEnabled("mcuMain", settings.systemEnabled);
                          break;
                          case "sleep_us":
                              dv.value = Math.round(settings[key]/1e6);
                          break;
                        default :
                            dv.value = settings[key];
                    }
                }
            }
            document.getElementById("settings").setAttribute("updated","1");
        }
    }
    function f_ms2min(ms){
        const minutes = Math.floor(ms/60000);
        const tenths  = Math.round(((ms/60000)-minutes)*10)/10;
        return(minutes+tenths);
    }
    function f_submit() {
        settings.src=settings;
        for (let key in settings){
            if(key == "src" || key == "what" || key == "fanOn")continue;
            let dv = document.getElementById(key);
            switch (key) {
                case "delayMs":
                    settings[key] = dv.value * 1e3; // milliseconds to seconds
                  break;
                case "systemEnabled":
                     settings[key] = dv.getAttribute("enabled")==1?1:0;
                     dv.setAttribute("updated", "0");
                   break;
                case "sleep_us":
                    settings[key] = Number(dv.value * 1e6);
                  break;
                default:
                    settings[key] = dv.value;
            }
        }
        settings.src = "settings"; settings.what = "submit"; settings.updated=updating;
        document.getElementById("json2").innerHTML=JSON.stringify(settings);
        webSock.send(JSON.stringify(settings));
    }
    function f_refresh() {
        const x = document.getElementsByName("limit");
        for (let i = 0; i < x.length; i++) x[i].value = null;
        updating = 0;
        document.getElementById("json2").innerHTML = '{"src":"settings","what":"refresh"}';
        webSock.send('{"src":"settings","what":"refresh"}');
    }
    let f_help = () => {
            if (document.getElementById("help").getAttribute("hidden")){
                document.getElementById("help").removeAttribute("hidden");
                document.getElementById("bt_help").innerHTML = "Un-Help";
            }else{
                document.getElementById("help").setAttribute("hidden", true);
                document.getElementById("bt_help").innerHTML = "Help";
            }
        };
    function f_updated(p1){
        const doc = document.getElementById("updated");
        if(p1){
            doc.removeAttribute("hidden");
            updating = 0;
        }
        else doc.setAttribute("hidden", true);
    }
    const buttons = document.querySelectorAll("input[name='systemEnabled']");
    function f_systemEnabled(val){
        buttons.forEach(button => {
            if(button.value == val)button.checked = true;
        });
    }
    buttons.forEach(button => {button.onclick = () => {
        if (button.checked) {
            const json = '{"src":"settings","what":"system","systemEnabled":'+Number(button.value)+'}';
            webSock.send(json);
        }
    }})
    </script>
</html>
)=====";

  const char NETWORK[] PROGMEM =  R"=====(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="file" content="set_bounds.html">
    <meta name="author" content="Joe Belson 20220422">
    <meta name="what" content="esp8266 html for WiFi connected temp/humidity bounds.">
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title id="title" updated="0">Todd's Sprinkler: Network Setup</title>
  </head>
<style>
  .reboot{
    background-color: red;
    color:rgb(255, 255, 255);
    padding:1.5vw 1.5vw 1.5vw 1.5vw;
    border-radius: 17%;
    box-shadow: inset 2px 2px 3px rgba(153, 150, 150, .6),
    inset -2px -2px 3px rgba(0, 0, 0, 0.6);
    border-bottom: black solid 3px;
    border-spacing: 45px;
    cursor: pointer;
    font: size 2vw;
    font-weight: bold;
    font-size: 17px;
  }
  .updated{
    background-color: green;
    color:rgb(255, 255, 255);
    padding:1vw 1vw 1vw 1vw;
    border-radius: 0%;
    box-shadow: inset 2px 2px 3px rgba(153, 150, 150, .6),
    inset -2px -2px 3px rgba(0, 0, 0, 0.6);
    border-bottom: black solid 3px;
    border-spacing: 45px;
    cursor: pointer;
    font-weight: bold;
  }
</style>
  <body>
    <hr>
    <div onclick="updating=true">
      <table class="calibrate">
        <caption style="font-weight: bolder;padding-bottom:1rem">
          <h1>WiFi SSID & PWD & IP</h1>
          <h2 id="updated" class="updated" hidden>UPDATED</h2>
        </caption>
         <thead>
            <tr>
              <th>SSID (Case Sensitive)</th>
              <th>PWD</th>
            </tr>
          </thead>
            <tr>
              <td><input  id="ssid"   type="text"   name="cred"></td>
              <td><input  id="pwd"    type="text"   name="cred"></td>
            </tr>
      </table>
      <table>
            <tr><td><br></td></tr>
            <tr>
              <td>DHCP: <input type="checkbox" id="isDHCP" name="isDHCPServer" onclick="f_dhcp(this.checked)"/></td>
            </tr>
            <tr id="dhcp">
              <td><label for="ip"   name="dhcp">IP:     </label><input id="ip"    type="text" name="dhcp" style="width: 5rem;"></td>
              <td><label for="gw"   name="dhcp">Gateway:</label><input id="gw"    type="text" name="dhcp" style="width: 5rem;"></td>
              <td><label for="mask" name="dhcp">Mask:   </label><input id="mask"  type="text" name="dhcp" style="width: 5rem;"></td>
            </tr>
      </table>
    </div>
    <p><input  id="submit2" type="submit" value="Submit Changes" onclick="f_submit()">
      <input type="submit" onclick="history.back()" value="Back to Main">
      <input id="reboot" type="submit" onclick="f_reboot('reboot')" class="reboot" value="REBOOT" hidden>
    </p>
    <hr>
  </body>
    <script language = "javascript" type = "text/javascript">
  
        let webSock       = new WebSocket("ws://" + window.location.hostname + ":80?source=wifi");
        webSock.onopen    = function(evt){webSock.send('{"src":"network","what":"refresh"}');}
        webSock.onmessage = function(evt){f_webSockOnMessage(evt);}
        webSock.onerror   = function(evt){f_webSockOnError(evt);}
  
        let updating = false;  // local flag: don't refresh text boxes using mcu json.  "We are updating values"
        let j = {
              src : "mcuNetwork"
            , what : "refresh"
            , ssid : "ssid"
            , pwd : "pwd"
            , isDHCP : false
            , ip : "11.1.1.11"
            , gw : "11.1.1.254"
            , mask : "255.255.255.0"
            , updated : false         // flag did MCU just save WiFi credentials?
            , reboot : false          // flag - tell the MCU to reboot.  Try AP_Mode STA
          };
        function f_webSockOnMessage(evt){
          if (typeof evt.data === "string") {
            r = JSON.parse(evt.data);
            if(r.src != "mcuNetwork")return;
            j=r;console.log(j);
            if(updating ^ j.updated)return; // ...updated == bool return from mcu save of the struct.
            for (key in j){
              if(key == "src" || key == "what")continue;
              dv = document.getElementById(key);
              if(dv.value=="" || dv.value==null){
                switch(key){
                  case "ssid":dv.value = j.ssid;break;
                  case "isDhcp":f_dhcp(j.isDHCP);break;
                  case "ip":dv.value = j.ip;break;
                  case "gw":dv.value = j.gw;break;
                  case "mask":dv.value = j.mask;break;
                  case "updated":
                    if(j.updated){
                      updating ^= j.updated; // if this is an "updated mcu return" we are done with the last local update
                      f_reboot('updated');
                      dv.removeAttribute("hidden");
                    }break;
                }
              }
            }
          }
         }
        function f_webSockOnError(evt){}
        function f_submit(){
          for(var key in j){
            switch(key){
              case "src":break;
              case "isDHCP":j[key]=document.getElementById(key).checked;break;
              case "updated":j.updated = 1; break;
              case "reboot":j.reboot = false;break;
              default: j[key]=document.getElementById(key).value;
            }
          }
          j.src = "network";
          j.what = "submit";
          console.log(j);
          webSock.send(JSON.stringify(j));
        }
        function f_dhcp(tf){
          console.log(tf);
          const dhcp = document.getElementById("dhcp");
          if(tf)dhcp.setAttribute("hidden", "hidden");
        else
        dhcp.removeAttribute("hidden");
       }
       function f_reboot(what){
        switch (what){
          case 'reboot':
              webSock.send('{"src":"network","what":"reboot"}');
              window.location.href='http://' + j.ip;
            break;
          case 'updated':
            // make visible REBOOT
              document.getElementById("reboot").removeAttribute("hidden");
            break;
        }
       }
     </script>
  </html>
  )=====";
