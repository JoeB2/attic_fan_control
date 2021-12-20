const char index_html[] PROGMEM =  R"=====(
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
        <title>AtticFan Controller : Readings</title>
        <style>
            html {
                font-family: Arial;
                display: inline-block;
                margin: 0px auto;
                text-align: left;
            }
            h2 {
                font-size: 2rem;
                text-align: center;
            }
            h3 {
                font-size: 2rem;
                text-align: center;
            }
            .units {
                font-size: 1.0rem;
            }
            input[type=button]{
                text-align: center;

                font-weight: bold;
                cursor: pointer;
                box-shadow: inset 1px 1px 1px rgba(255, 255, 255, 0.6),
                    inset -1px -1px 1px rgba(0, 0, 0, 0.6);

            }
            input[type="submit"] {
                border: 0;
                line-height: 1.5;
                padding: 0 7px;
                font-size: 13px;
                text-align: center;

                font-weight: bold;
                cursor: pointer;
                box-shadow: inset 2px 2px 3px rgba(255, 255, 255, 0.6),
                    inset -2px -2px 3px rgba(0, 0, 0, 0.6);
                }
            .c1 {
                text-align: center;
                padding-left: 0rem;
                padding-right: 0rem;
                padding-top: 0rem;
                padding-bottom: 0rem;
                height: auto;
            }
            .c2 {
                text-align: center;
                padding-left: 5px;
                padding-right: 5px;
                padding-top: 0;
                padding-bottom: 0;
                height: 1rem;
                font-weight: bold;
                font-size: 17px;
                width: 3rem;
                height: 3rem;
            }
            .c3 {
                text-align: center;
                padding-left: 3px;
                padding-right: 3px;
                padding-top: 0rem;
                padding-bottom: 0rem;
                font-weight: bold;
                height: auto;
            }
            .c4 {
                width: 1rem;
                padding-left: 1rem;
                padding-right: 1rem;
                padding-top: 0rem;
                padding-bottom: 0rem;
                font-weight: bold;
                text-align: center;
                width: 3rem;
                height:2rem;
            }
            .c5 {
                text-align: center;
                padding-left: 1rem;
                padding-right: 1.5rem;
                padding-top: 0rem;
                padding-bottom: 0rem;
                font-size: smaller;
                font-weight: bolder;
                line-height: 1.5;
                height: auto;
            }
            .blink {
                animation: blinker 1.9s linear infinite;
                color: #a81a1a;
                font-size: 25px;
                font-weight: bold;
                font-family: Verdana;
                text-align: center;
                font-weight: bold;
                padding-left: 2rem;
                height: 1rem;
            }
            @keyframes blinker {
                99% {
                    opacity: 0;
                }
            }
            ul.no-bullets {
                list-style-type: none; /* Remove bullets */
                padding: 0; /* Remove padding */
                margin: 0; /* Remove margins */
            }
            .sensors-grid{
                display:grid;
                gap: 1.5rem;
                grid-template-columns:repeat(2, 1fr);
                padding-block:2rem;
                width: min(95%, 70rem);
                margin-inline: auto;
                font-size:21pt;
            }
        </style>
        <input id="bt_help" type="button" onclick="f_help()" value="Help" title="Hide/Show WebSock Messages"/>
        <title>Attic Fan Switch</title>
    </head>
    <body>
        <h2>ESP8266 Attic Fan Manager</h2>
        <h3 id="atticEpochTime" style="font-size: 170%; color: rgb(0, 4, 255) ; font-weight: bold">
                00:00:00
        </h3>
        <main class="sensors-grid">
        <article>
            <img src="https://img.icons8.com/color/30/000000/thermometer.png"/>
            <span>Attic Temp:</span>
            <span id="t1">%T1%</span>
            <sup class="units">&deg;F</sup>
        </article>
        <article>
            <img src="https://img.icons8.com/color/30/000000/thermometer.png"/>
            <span>Inside Temp:</span>
            <span id="t2">%T2%</span>
            <sup class="units">&deg;F</sup>
        </article>

        <article>
            <i class="fas fa-thermometer-half" style="color: #059e8a"></i>
            <span>Outside Temp:</span>
            <span id="t3">%T3%</span>
            <sup class="units">&deg;F</sup>
        </article>
        <article>
            <img src="https://img.icons8.com/external-justicon-blue-justicon/30/000000/external-barometer-science-justicon-blue-justicon.png"/>
            <span>Barometer</span>
            <span id="p3">%P3%</span>
            <sup class="units">Hg</sup>
        </article>

        <article>
            <i class="fas fa-tint" style="color:#00add6;"></i>
            <span>Humidity:</span>
            <span id="h3">%H3%</span>
            <sup class="units">%</sup>&nbsp&nbsp
        </article>
        <article>
            <img src="https://img.icons8.com/external-vitaliy-gorbachev-blue-vitaly-gorbachev/30/000000/external-thermometer-weather-vitaliy-gorbachev-blue-vitaly-gorbachev-5.png"/>
            <span>Dew Point:</span>
            <span id="dp3">%DP3%</span>
            <sup class="units">&degF</sup>&nbsp&nbsp
        </article>
        <article>
            <img src="https://img.icons8.com/office/30/000000/alps.png"/>
            <span>Density Altitude:</span>
            <span id="a3">%A3%</span>
            <sup class="units">ft</sup>&nbsp&nbsp
        </article>
        </main>
    
    <table>
        <th>
            <tr style="font-weight: bold;">
                <td class="c1"></td>
                <td class="c2">Fan</td>
                <td style="width: 10px;"></td>
                <td class="c3">System</td>
                <td class="c3">Fan On<br>Timer</td>
                <td class="c4">Minutes</td>
                <td class="blink" id="vcc0">Charge</td>
            </tr>
        </th>
        <tr>
            <td class="c1"></td>
            <td class="c2" id="fanOn"
                style=  "padding: 1 3px;
                        border-radius: 30%;
                        background-color: rgb(248, 113, 135);
                        box-shadow: inset 2px 2px 3px rgba(255, 255, 255, 0.6),
                    inset -2px -2px 3px rgba(0, 0, 0, 0.6);"
            >OFF</td>
            <td></td>
            <td class="c3" id="system"
                style="padding: 0 0px;
                        border-radius: 50%;
                        font-weight: bold;
                        box-shadow: inset 2px 2px 3px rgba(255, 255, 255, 0.6),
                    inset -2px -2px 3px rgba(0, 0, 0, 0.6);
                    cursor: pointer;"
                onclick="f_system('button', this.innerHTML=='STOP')"
                updated="0"

                title="Enable / Disable Fan Control System"
            >Enable</td>
            <td class="c4">
                <input
                type="button"
                value="+ 5mins"
                onclick="f_fanTimer('button', +5)"
                title="add 5 minutes to fan on override"
            />
            <input
                type="button"
                value="- 5mins"
                onclick="f_fanTimer('button', -5)"
                title="remove 5 minutes from fan on override"
                style="margin-top: 5px;width: 100%;"
            />
            </td>
            <td class="c5" style="vertical-align:top">
                <label id="fanOnMs" updated="0">0</label>
                <input
                    type="submit"
                    onclick="f_fanTimer('submit' ,-1)"
                    title="***OVERRIDE***&#x0a;  Turn on fan for&#x0a; specified minutes."
                    style="margin-top: 10px; height: 25px; ;background: rgb(57, 216, 110)"
                />
            </td>
            <td class="blink" id="vcc1">Remote Battery<br>
                <span id="vcc" class="units">3.00</span>
                <sup class="units" id="vcc2">vcc</sup>
            </td>
    </tr>
        <tr>
            <td class="c1"></td>
            <td class="c2"></td>
            <td></td>
            <td class="c3"></td>
            <td class="c4">
            </td>
            <td class="c5">
            </td>
        </tr>
    </table>
        <br/><br/>
        <div id="help" hidden="true">
            <input
                value="Set Controller Limits"
                type="button"
                onclick="f_redirect('settings')"
                style="background: rgb(155, 219, 149)"
            />
            <input
                value="Update WiFi settings"
                type="button"
                onclick="f_redirect('wifi')"
                style="background: orange;"
            />
            <input
                value="Firmware / Startup Settings"
                type="button"
                onclick="f_redirect('update')"
                style="background:rgb(249, 253, 0);color: red;"
            />
            <br><br>
            <p id="json1" style="font-weight: normal; font-size: 12pt"></p>
            <p id="json2" style="font-weight: normal; font-size: 12pt"></p>
        </div>
    </body>
    <script language="javascript" type="text/javascript">
        let webSock = new WebSocket("ws://" + window.location.hostname + ":80?json=1");
        webSock.onopen = function(){ 
            let json = {json:atticEpochTime,atticEpochTime:0};
            json.atticEpochTime = Math.floor(Date.now() / 1000);
            webSock.send(JSON.stringify(json));
        };
        webSock.onmessage = function(evt) {f_webSockOnMessage(evt);};
        webSock.onerror = function(evt) {console.warn(evt);};
        webSock.onclose = function(){ console.log("webSock Connection Closed.")};

        const empty          = 0  // 0b00000000
            , readings       = 1  // 0b00000001
            , settings       = 2  // 0b00000010
            , net            = 4  // 0b00000100
            , atticEpochTime = 9  // 0b00001001
            , fanOnMs        = 17 // 0b00010001
            , systemEnabled  = 33 // 0b00100001
            , ping_readings  = 65 // 0b01000001
            , ping_settings  = 66 // 0b01000010
            , ping_network   = 68 // 0b01000100

        ;

        let j = {
              json: readings
            , t1: 98.60
            , t2: 82.33
            , t3: 87.30
            , p3: 29.92
            , h3: 88
            , dp3: 75
            , a3: 994
            , vcc: 2.85
            , atticEpochTime: 1637713842
            , fanOn: 1
            , fanOnMs: 138000
            , systemEnabled: 1
        };
        function f_webSockOnMessage(evt){
            if (typeof evt.data === "string"){
                let s = evt.data;
                j = JSON.parse(s);
                for (let key in j){
                    document.getElementById("json2").innerHTML = s;
                    switch (key){
                        case "json": continue;
                          break;
                        case "vcc":
                            f_battery(j[key]);
                          break;
                        case "atticEpochTime":
                            const options = { hourCycle: 'h24', year: 'numeric', month: '2-digit', day: '2-digit'
                                            , hour: '2-digit', minute: '2-digit', second: '2-digit', timeZoneName: 'short'
                                  };
                            const dt = new Date();
                            document.getElementById("atticEpochTime").innerHTML
                                = dt.toLocaleTimeString("en-US", options);
                                j[key]= dt.valueOf()/1000;
                          break;
                        case "fanOn":
                            f_fan(j[key]);
                          break;
                        case "fanOnMs":
                                f_fanTimer('ws', f_ms2min(j[key]));
                          break;
                        case "dp3":
                            document.getElementById(key).innerHTML = j.h3?j.dp3:"n/a";
                          break;
                        case "a3":
                            document.getElementById(key).innerHTML = j.h3?j.a3:"n/a";
                          break;
                        case "systemEnabled":
                            f_system("ws", j.systemEnabled);
                          break;
                        default:
                            document.getElementById(key).innerHTML =j[key];
                    }
                }
            }
        }
        function gHbi(e){return(document.getElementById(e).innerHTML);} // return innerHTML by id
        function sHbi(e,v){document.getElementById(e).innerHTML=v;}     // set value by id
        function gVAbi(e,a){return(document.getElementById(e).getAttribute("a"));} // get value by id
        function sVAbi(e,a,v){document.getElementById(e).setAttribute(a,v);}     // set value by id
        function f_ms2min(ms){
            const minutes = Math.floor(ms/60000);
            const tenths  = Math.round(((ms/60000)-minutes)*10)/10;
            return(minutes+tenths);
        }
        function f_redirect(where) {window.location.href = "/" + where;}
        // need to allow json updates unless +5 mins or -5min have been pressed
        function f_fanTimer(){
            let who = arguments[0];
            let value = Number(arguments[1]);
            const doc = document.getElementById("fanOnMs");
            if(who == "ws" && doc.getAttribute("updated") == "1")return;
            if(who == "ws"){doc.innerHTML=Number(value);return;}
            else doc.setAttribute("updated", "1");

            if(value === -1){
                jsn = {
                    json : fanOnMs
                  , fanOnMs : doc.innerHTML*60*1000
                };
                webSock.send(JSON.stringify(jsn));
                doc.setAttribute("updated", "0");
                document.getElementById("json1").innerHTML = JSON.stringify(jsn);
                return;
            }
            let val = Number(doc.innerHTML) + Number(value);
            if(val<0)val=0;
            doc.innerHTML = val;
    }
        function f_system() {
            let who = arguments[0];
            let value = Number(arguments[1]);

            const system = document.getElementById("system");
            let updated = system.getAttribute("updated");
            if(who=="ws" && updated=="1")return;
            if(who=="button"){
                value=!value;
                system.setAttribute("updated", "1");
            }
            system.style.backgroundColor = value ? "rgb(250, 0, 0)" : "rgb(0, 250, 0)";
            system.innerHTML = value ? "STOP" :"Enable" ;

            if(who=="button"){
                let jsn = {
                    json : systemEnabled
                  , systemEnabled : value?1:0
                };
                document.getElementById("json1").innerHTML = JSON.stringify(jsn);
                webSock.send(JSON.stringify(jsn));
                system.setAttribute("updated", "0");
            }
        }
        function f_battery(batt){
            let blink = document.getElementsByClassName("blink");
            for(i=0;i<blink.length;i++){
                if(batt<3.00)
                    blink[i].removeAttribute("hidden");
                else
                    blink[i].setAttribute("hidden", true);
            }
            document.getElementById("vcc").innerHTML = batt;
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
                document.getElementById("bt_help").value = "un-Help";
            }else{
                document.getElementById("help").setAttribute("hidden", true);
                document.getElementById("bt_help").value = "Help";
            }
        };
    </script>
</html>
)=====";

const char settings_html[] PROGMEM =  R"=====(
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
        .a {
            font: italic small-caps bold 12px/30px Georgia, serif;
        }
        table.calibrate th {
            text-align: center;
            width: 23pt;
            border-bottom: 2px solid rgb(8, 8, 8);
        }
        table.calibrate {
            font-size: smaller;
            margin-left: 10px;
            margin-right: auto;
            width: 300px;
            border: 1px solid black;
            border-style: solid;
        }
        table.calibrate caption {
            background-color: #56970c;
            color: #fff;
            font-size: large;
            font-weight: bold;
            letter-spacing: 0.3em;
        }
        table.calibrate thead th {
            padding: 3px;
            background-color: #98e99d;
            font-size: 16px;
            border-width: 1px;
            border-style: solid;
            border-color: #f79646 #ccc;
        }
        table.calibrate td {
            font-size: 12px;
            font-weight: bold;
            text-align: center;
            padding: 2px;
            background-color: shite;
            color: black;
            font-weight: bold;
            min-width: 6.5em;
            max-width: 6em;

        }
        table.override th {
            text-align: center;
        }
        table.override {
            margin-left: 5px;
            margin-right: auto;
            width: 650px;
            border: 1px solid black;
            border-style: solid;
        }
        table.override caption {
            background-color: #f74646;
            color: #fff;
            font-size: large;
            font-weight: bold;
            letter-spacing: 0.3em;
        }
        table.override thead th {
            padding: 3px;
            background-color: #f8d3de;
            font-size: 16px;
            border-width: 1px;
            border-style: solid;
            border-color: #f79646 #ccc;
            column-width: 7in;
        }
        table.override td {
            font-size: 12px;
            font-weight: bold;
            text-align: center;
            padding: 2px;
            background-color:#f6f6ff;
            color: black;
            font-weight: bold;
            column-width: 1.2in;
        }
    </style>
    <body style="text-align: left">
        <input id="bt_help" type="button" onclick="f_help()" value="Help" />
        <hr />
        <b><i>Note: Values are updated every "Attic Sensor Check-in Seconds". All values are in &degF</i></b>
        <table class="calibrate" id="settings" updated="false">
            <caption>
                Set Temp & Humidity Bounds
            </caption>
            <thead>
                <tr>
                    <th>*Inside<br>Off &degf</th>
                    <th>IOD on Value*</th>
                    <th>IOD off Value*</th>
                    <th>Fan on Attic* Temp&degf</th>
                    <th>Fan off Attic Temp&degf</th>
                    <th>AOD on Value</th>
                    <th>AOD off Value</th>
                    <th>AID on Value</th>
                    <th>AID off Value</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><input id="inside_off" type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="IOD_on"     type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="IOD_off"    type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="attic_on"   type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="attic_off"  type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="AOD_on"     type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="AOD_off"    type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="AID_on"     type="number" name="limit" min="0" max="120"/></td>
                    <td><input id="AID_off"    type="number" name="limit" min="0" max="120"/></td>
                </tr>
            </tbody>
        </table>
        <p style=" font: 14px/17px Monospace ;">
            <span style=" font: bold 17px/25px Monospace ;">Values to trigger the Fan ON/OFF</span><br>
            AID: <b><i>Difference</i></b> of Attic minus the Inside temp in degrees fahrenheit <a style="font-size: small;">(A-I)&degf</a><br>
            AOD: <b><i>Difference</i></b> of Attic minus the Outside temp in degrees fahrenheit <a style="font-size: small;">(A-O)&degf</a><br>
            IOD: <b><i>Difference</i></b> of Inside minus the Outside temp in degrees fahrenheit <a style="font-size: small;">(I-O)&degf</a><br>
        </p>
        <p style="font-weight: bold; font-size: large">
            NOTE: Value of zero or empty causes the setting to be ignored
        </p>
        <hr />
        <br />

        <table class="override">
            <caption>
                Set Override Values
            </caption>
            <thead>
                <tr>
                    <th>Fan System<br>Enabled</th>
                    <th>Attic Sensor Check-In Seconds</th>
                    <th>Remotes Sensor Check-In Seconds</th>
                    <th>
                        <input id="refresh" type="submit" value="Refresh" onclick="f_refresh()"
                                style=" margin: 1px; color: black; background-color: yellow; font-weight: bold;"/>
                        <input id="Submit" type="submit" value="Submit" onclick="f_submit()"
                                style="margin: 2px; color: black; background-color: rgb(248, 138, 171); font-weight: bold;"
                        />
                    </th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>
                        <a id="systemEnabled" updated="false" enabled="1"></a>
                        <input id="systemEnabled_true"  type="radio" name="systemEnabled" value="1" onclick="f_systemEnabled('radio', this.value)" autocomplete="off" style="align-items: center;display:inline-flex" checked>ON
                        <input id="systemEnabled_false" type="radio" name="systemEnabled" value="0" onclick="f_systemEnabled('radio', this.value)" autocomplete="off" style="align-items: center;display:inline-flex">OFF
                    </td>
                    <td><input id="delayMs"     type="number" name="limit" min="0" max="600"/></td>
                    <td><input id="remoteSleepSeconds" type="number" name="limit" min="0" max="3600"/></td>
                    <td id="fanOn" style="font-size: 12px; font-size: 18px;">Fan is On</td>
                </tr>
            </tbody>
        </table>
        <p style="font-weight: bold; font-size: large">
            NOTE: Value of zero or empty causes the setting to be ignored<br>
            Refresh: Resets all values with those already in play. Refresh happens every "Attic Sensor Check-In Minutes"
        </p>
        <hr /><br />
        <div id="help" hidden="true">
            <p id="json1" style="font-weight: normal; font-size: 12pt">test</p>
            <p id="json2" style="font-weight: normal; font-size: 12pt">test:</p>
        </div>
    </body>

    <script language="javascript" type="text/javascript">

        let webSock = new WebSocket("ws://" + window.location.hostname + ":80?json=2");
        webSock.onopen = function (evt) {webSock.send('{"json":' + ping_settings + '}');};
        webSock.onmessage = function (evt) {f_webSockOnMessage(evt);};
        webSock.onerror = function (evt) {};

        const empty          = 0  // 0b00000000
            , readings       = 1  // 0b00000001
            , settings       = 2  // 0b00000010
            , net            = 4  // 0b00000100
            , atticEpochTime = 9  // 0b00001001
            , fanOnMs        = 17 // 0b00010001
            , systemEnabled  = 33 // 0b00100001
            , ping_readings  = 65 // 0b01000001
            , ping_settings  = 66 // 0b01000010
            , ping_network   = 68 // 0b01000100

        ;

        let j = {
              json : settings
            , fanOn : 0
            , fanOnMs : 318000
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
            , remoteSleepSeconds : 120000000
        };

    function f_webSockOnMessage(evt) {
        if (typeof evt.data === "string") {
            s=evt.data;
            document.getElementById("json2").innerHTML = s;
            j = JSON.parse(s);
            if(j.json != settings)return;
            for (let key in j){
                if(key == "json" || key == "fanOnMs")continue;
                dv = document.getElementById(key);
                    if(dv.value=="" || dv.value==null){
                    switch (key){
                        case "fanOn":
                            doc = document.getElementById("fanOn");
                            doc.innerHTML = j.fanOn&j.systemEnabled?"Fan is ON":"Fan is OFF";
                            doc.style.backgroundColor = j.fanOn&j.systemEnabled?"#98e99d":"#ff0953";
                          break;
                        case "delayMs":
                            dv.value = j.delayMs/(1000);break;// milliseconds to Seconds
                          break;
                        case "systemEnabled":
                            if(document.getElementById("systemEnabled").getAttribute("updated")=='0')
                                f_systemEnabled(j.systemEnabled, "ws");
                          break;
                        case "fanOnMs":
                            f_fanTimer(f_ms2min(j.fanOnMs));
                          break;
                        default :
                            dv.value = j[key];
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
        j.json=settings;
        for (let key in j) {
            if(key == "json" || key == "fanOn")continue;
            let dv = document.getElementById(key);
            switch (key) {
                case "delayMs":
                    j[key] = dv.value * 1000; // milliseconds to seconds
                  break;
                case "fanOnMs":
                    j.fanOnMs = dv.value * 1000 * 60; // milliseconds to seconds to minutes
                  break;
                case "systemEnabled":
                     j[key] = dv.getAttribute("enabled")=="true"?1:0;
                     dv.setAttribute("updated", "false");
                   break;
                default:
                    j[key] = dv.value;
            }
        }
        document.getElementById("json1").innerHTML=JSON.stringify(j);
        webSock.send(JSON.stringify(j));
        f_refresh();
    }
    function f_refresh() {
        const x = document.getElementsByName("limit");
        for (let i = 0; i < x.length; i++) x[i].value = null;
        document.getElementById("systemEnabled").setAttribute("updated", "false");
        document.getElementById("settings").setAttribute("updated","false");
        document.getElementById("fanOn").innerHTML ="";
        document.getElementById("fanOn").style.backgroundColor = "#ffffff";
        webSock.send('{"json":' + ping_settings + '}');
    }
    let f_help = () => {
            if (document.getElementById("help").getAttribute("hidden"))
                document.getElementById("help").removeAttribute("hidden");
            else document.getElementById("help").setAttribute("hidden", true);
        };
    function f_systemEnabled(source, value){ // if 'ws' we update it once
            let link = document.querySelector('#systemEnabled');

            if(source=='radio')link.setAttribute("updated", '1');

            if(value=='1'){
                document.getElementById('systemEnabled_true').checked=true;
                link.setAttribute('enabled', '1');
            }else{
                document.getElementById('systemEnabled_false').checked=true;
                link.setAttribute('enabled', '0');
            }
        }
    function f_fanTimer(source, value){
            doc = document.getElementById("fanOnMs");
            if(source == "ws" && doc.getAttribute("updated") == "true")return;
            if(source == "input" && value == -1)doc.setAttribute("updated", "true");
            if(source == "ws")doc.value=Number(value);
        }
    </script>
</html>
)=====";

  const char wifi_html[] PROGMEM =  R"=====(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="file" content="set_bounds.html">
    <meta name="author" content="Joe Belson 20210822">
    <meta name="what" content="esp8266 html for WiFi connected temp/humidity bounds.">
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AtticFan Controller : Network Setup</title>
  </head>
  <body style="text-align: left;">
    <hr>
      <table class="calibrate"   id="settings" updated="0">
        <caption style="font-weight: bolder;padding-bottom:1rem">
          <h1>Set WiFi SSID & PWD & IP</h1>
        </caption>
          <thead>
              <tr>
                  <th>SSID</th>
                  <th>PWD</th>
              </tr>
          </thead>
          <tbody>
              <tr>
                  <td><input  id="ssid"   type="text"   name="cred"></td>
                  <td><input  id="pwd"    type="text"   name="cred"></td>
                  <td><input  id="submit" type="submit" onclick="f_submit()"></td>
              </tr>
          </tbody></table>


      <br>
      <label for="dhcp">DHCP:</label>
        <input type="checkbox" id="isDhcp" onclick="f_dhcp(this.checked)">
      <br><br>

      <label for="ip"   name="dhcp">IP:     </label><input id="ip"    type="text" name="dhcp" style="width: 7rem;">
      <label for="gw"   name="dhcp">Gateway:</label><input id="gw"    type="text" name="dhcp" style="width: 7rem;">
      <label for="mask" name="dhcp">Mask:   </label><input id="mask"  type="text" name="dhcp" style="width: 7rem;">
    </body>
    <script language = "javascript" type = "text/javascript">
  
        let webSock       = new WebSocket("ws://" + window.location.hostname + ":80?source=wifi");
        webSock.onopen    = function(evt){webSock.send('{"json":' + ping_network + '}');};
        webSock.onmessage = function(evt){f_webSockOnMessage(evt);}
        webSock.onerror   = function(evt){f_webSockOnError(evt);}
  
        const empty          = 0  // 0b00000000
            , readings       = 1  // 0b00000001
            , settings       = 2  // 0b00000010
            , net            = 4  // 0b00000100
            , atticEpochTime = 9  // 0b00001001
            , fanOnMs        = 17 // 0b00010001
            , systemEnabled  = 33 // 0b00100001
            , ping_readings  = 65 // 0b01000001
            , ping_settings  = 66 // 0b01000010
            , ping_network   = 68 // 0b01000100

        ;

        let j = {
              json : net
            , ssid : "ssid"
            , pwd : "pwd"
            , isDHCP : 1
            , ip : "11.1.1.11"
            , gw : "11.1.1.254"
            , mask : "255.255.255.0"
          };

        function f_webSockOnMessage(evt){
          if(document.getElementById("settings").getAttribute("updated")=="1")return;
          if(typeof evt.data === "string"){
              const j=JSON.parse(evt.data);
              if(j.json != net)return;
              for(key in j){
                if(key == "json")continue;
                dv = document.getElementById(key);
                switch(key){
                  case "ssid":dv.value = j.ssid;break;
                  case "isDhcp":f_dhcp(j.isDHCP);break;
                  case "ip":dv.value = j.ip;break;
                  case "gw":dv.value = j.gw;break;
                  case "mask":dv.value = j.mask;break;
                }
              }
          }document.getElementById("settings").setAttribute("updated", "1");
        }
        function f_webSockOnError(evt){}
        function f_submit(){
          for(var key in j){
            if(key == "json")continue;
            if(key=="isDHCP")
              j[key]=document.getElementById(key).checked;
            else j[key]=document.getElementById(key).value;
          }
          j.json = net;
          webSock.send(JSON.stringify(j));
          document.getElement.getElementById("title").setAttribute("updated", "0");
        }
        function f_dhcp(){
          chk = arguments[0];console.log(chk);
          document.getElementById("isDhcp").checked = chk;
          const n = document.getElementsByName("dhcp");
          if(chk){
            for(var i=0;i<n.length;i++)
              n[i].setAttribute("hidden", "hidden");
          }else{
            for(var i=0;i<n.length;i++)
              n[i].removeAttribute("hidden");
          }
       }
     </script>
  </html>
  )=====";
