const char INDEX_HTML[] PROGMEM =  R"=====(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: left;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 2.5rem; }
    .units { font-size: 1.2rem; }
    .bmp-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    input[type=submit] {
      border: 0;
      line-height: 2.5;
      padding: 0 5px;
      font-size: 17px;
      text-align: center;
            
      font-weight: bolder;
      padding: 3px 3x;
      text-decoration: none;
      margin: 0px 0px;
      cursor: pointer;
      width: auto;
      height: auto;
      box-shadow: inset 2px 2px 3px rgba(255, 255, 255, .6),
                  inset -2px -2px 3px rgba(0, 0, 0, .6);
   }
  </style>
  <title>Attic Fan Switch</title>
</head>
<body>
  <h2>ESP8266 Attic Fan Manager</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="bmp-labels">Temp-Attic</span>
    <span id="t1">%T1%</span>
    <sup class="units">&deg;F</sup>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="bmp-labels">Humid-Attic</span>
    <span id="h1">%H1%</span>
    <sup class="units">%</sup>
    <i class="fas fa-hourglass-half" style="color: #00add6;"></i>
    <span class="bmp-labels">Press-Attic</span>
    <span id="p1">%P1%</span>
    <sup class="units">Hg</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="bmp-labels">Temp-Inside</span> 
    <span id="t2">%T2%</span>
    <sup class="units">&deg;F</sup>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="bmp-labels">Humid-Inside</span>
    <span id="h2">%H2%</span>
    <sup class="units">%</sup>
    <i class="fas fa-hourglass-half" style="color: #00add6;"></i>
    <span class="bmp-labels">Press-Attic</span>
    <span id="p2">%P2%</span>
    <sup class="units">Hg</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="bmp-labels">Temp-Outside</span> 
    <span id="t3">%T3%</span>
    <sup class="units">&deg;F</sup>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="bmp-labels">Humid-Outside</span>
    <span id="h3">%H3%</span>
    <sup class="units">%</sup>
    <i class="fas fa-hourglass-half" style="color: #00add6;"></i>
    <span class="bmp-labels">Press-Attic</span>
    <span id="p3">%P3%</span>
    <sup class="units">Hg</sup>
  </p>
  <table>
    <th><tr style="width: 90px; font-weight: bold;" >
      <td style="width:90px; font-weight: bold;">Fan</td>
      <td style="width: 90px; font-weight: bold;">Fan Timer</td>
      <td style="width: 90px; font-weight: bold;">Minutes</td>
      <td></td><td></td>
      <td style="font-size: large;">SYSTEM STATUS</td>
    </tr>
  </th>
    <tr>
      <td style="width: 60px; font-weight: bolder;" id="fan"></td>
      <td><input type="button" value="+ 5mins" onclick="f_fan(+1)"></td>
      <td><label for="fan" id="lbl_fan" style="width: 90px; font-weight: bold; text-align: right;"></label></td>
    </tr>
    <tr>
      <td><input type="submit" onclick="f_fan(0)" style="background:rgb(57, 216, 110); height: 30px; font-size: smaller;font-weight: bolder;"></td>
      <td><input type="button" value="- 5mins" onclick="f_fan(-1)"></td>
      <td></td>
      <td><input type="submit" id="bt_overide_OFF" onclick="f_overide_OFF(true)" style="background-color: red; font-weight: bolder;" value="Stop FAN"></td>
      <td></td><td style="width: 200px;"><label id="overide_OFF" style="font-weight: bolder; font-size: 30pt; color: green;">ON LINE</label></td>

    </tr>

  </table>
  <br><br>
  <input value="Set Bounds" type="submit" onclick="f_redirect('Set_Bounds')" style="background:moccasin">
  <p id="json"></p>
</body>

<script language = "javascript" type = "text/javascript">

  let webSock       = new WebSocket('ws://'+window.location.hostname+':80');
  webSock.onopen    = function(evt){f_webSockOnOpen(evt);}
  webSock.onmessage = function(evt){f_webSockOnMessage(evt);}
  webSock.onerror   = function(evt){f_webSockOnError(evt);}

  var s='[{"t1":0,"h1":0,"p1":0,"t2":0,"h2":0,"p2":0,"t3":0,"h3":0,"p3":0,"fan":false,"onMins":0},{"attic_on":75,"attic_off":82,"AID_on":75,"AID_off":85,"AOD_on":75,"AOD_off":80,"IOD_on":0,"IOD_on":20,"IOD_off":20,"overide_OFF":false,"delay":7}]';
  var j = JSON.parse(s);

  function f_webSockOnMessage(evt){
      if(typeof evt.data === "string"){
        var t='<i class="fas fa-toggle-on" style="color: green;"></i> on';
        var f='<i class="fas fa-toggle-off" style="color:red;"></i> off',
        s=evt.data;
        j=JSON.parse(evt.data);
          for(var key in j[0]){
              if(j[0].hasOwnProperty(key)){
                if(key!="fan"){document.getElementById(key).innerHTML=j[0].key;}
                else{
                  document.getElementById(key).innerHTML=j[0].key?t:f;
                  if(document.getElementById("onMins").innerHTML==""){
                        document.getElementById("onMins").innerHTML=j[1].onMins;
                  }
                  j[1].overide_OFF=!j[1].overide_OFF;
                  f_overide_OFF(false);
                }
              }
          }
      }
  }
  function f_webSockOnOpen(evt){}
  function f_webSockOnError(evt){}
  function f_redirect(where){;
                      window.location.href = "/"+where;
  }
  function f_fan(value){
    if(!value){ // send fan timer value to MCU
    j[0].onMins=Number(document.getElementById("lbl_fan").innerHTML);
    console.log(JSON.stringify(j));
    webSock.send(JSON.stringify(j));
    return;
    }
    // increment/decrement fan timer value
    var m=Number(document.getElementById("lbl_fan").innerHTML)+value*5;
    if(m<0)m=0
    document.getElementById("lbl_fan").innerHTML=m;
  }
  function f_overide_OFF(send){
    j[1].overide_OFF=!j[1].overide_OFF;
    document.getElementById("bt_overide_OFF").style.backgroundColor=j[1].overide_OFF?'green':'red';
    document.getElementById("bt_overide_OFF").value=j[1].overide_OFF?'Enable FAN':'STOP Fan';
    document.getElementById("overide_OFF").style.color=j[1].overide_OFF?'red':'green';
    document.getElementById("overide_OFF").innerHTML=j[1].overide_OFF?'OFF LINE':'ON LINE';
    
    if(send)webSock.send(JSON.stringify(j));
  }
  </script>
</html>
)=====";

  const char SETTINGS_HTML[] PROGMEM =  R"=====(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="file" content="set_bounds.html">
    <meta name="author" content="Joe Belson 20210822">
    <meta name="what" content="esp8266 html for WiFi connected temp/humidity bounds.">
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Set Temp / Humidity Bounds for 110VAC relays</title>
  </head>
<style type="text/css">
      body {font-size: 12px; background-color: #f6f6ff;font-family: Calibri, Myriad;text-align: center;}
      body p{text-align: left;}
      table th{text-align: center;width: 23pt;border-bottom:2px solid rgb(8, 8, 8);}
      table.calibrate {font-size:smaller ; margin-left:10px; margin-right: auto;  width: 300px;  border: 1px solid black; border-style:solid;}
      table.calibrate caption {	background-color: #f79646;	color: #fff;	font-size: large;	font-weight: bold;	letter-spacing: .3em;}
      table.calibrate thead th {
                  padding: 3px;
                  background-color: #fde9d9;
                  font-size:16px;
                  border-width: 1px;
                  border-style: solid;
                  border-color: #f79646 #ccc;
      }
      table.calibrate td {
                  font-size: 12px;
                  font-weight: bold;
                  text-align: center;
                  padding: 2px;
                  background-color:shite;
                  color:black;
                  font-weight: bold;
      }
</style>
<body style="text-align: left;">
  <hr><b><i>Current setttings are sent from the MCU every 7 seconds.</i></b>
    <table class="calibrate">
      <caption>Set Temp & Humidty Bounds</caption>
        <thead>
            <tr>
                <th>Fan on Attic Temp</th>
                <th>Fan off Attic Temp</th>
                <th>AID On Value</th>
                <th>AID Off Value</th>
                <th>AOD On Value</th>
                <th>AOD Off Value</th>
                <th>IOD On Value</th>
                <th>IOD Off Value</th>
                <th>Sensor Delay Minutes</th>
                <th>
                  <input id="refresh" type="submit" value="Refresh" onclick="f_refresh()" style="margin: 1px; color: black; background-color: yellow; font-weight: bold;">
                  <input id="Submit" type="submit" value="Submit" onclick="f_submit()" style="margin: 2px; color: black; background-color: rgb(248, 138, 171); font-weight: bold;">
                </th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><input type="number" id="attic_on"  name="limit" min="0" max="120" ></td>
                <td><input type="number" id="attic_off" name="limit" min="0" max="120"></td>
                <td><input type="number" id="AID_on"    name="limit" min="0" max="120" ></td>
                <td><input type="number" id="AID_off"   name="limit" min="0" max="120"></td>
                <td><input type="number" id="AOD_on"    name="limit" min="0" max="120" ></td>
                <td><input type="number" id="AOD_off"   name="limit" min="0" max="120" ></td>
                <td><input type="number" id="IOD_on"    name="limit" min="0"  max="120" ></td>
                <td><input type="number" id="IOD_off"   name="limit" min="0"  max="120" ></td>
                <td><input type="number" id="delay"     name="limit" min="3"  max="999" ></td>
            </tr>
        </tbody>
    </table>
    <hr>
    <p style="font-size:inherit;">AID: Attic - Inside Temp Delta</p>
    <p style="font-size: inherit;"> AOD: Attic - Outside Temp Delta</p>
    <p style="font-size: inherit;">IOD: Inside - Outside Temp Delta</p>
    <p style="font-weight: bold; font-size: large;">NOTE: Value of zero causes the setting to be ignored</p>
    <hr>
    <br><p id="settings">Current MCU Settings</p>
  </body>

<script language = "javascript" type = "text/javascript">

      let webSock       = new WebSocket('ws://'+window.location.hostname+':80');
      webSock.onopen    = function(evt){f_webSockOnOpen(evt);}
      webSock.onmessage = function(evt){f_webSockOnMessage(evt);}
      webSock.onerror   = function(evt){f_webSockOnError(evt);}


      const s='[{"t1":0,"h1":0,"t2":0,"h2":0,"t3":0,"h3":0,"fan":0,"onMins":0},{"attic_on":0,"attic_off","AID_on":0,"AID_off":0,"AOD_on":0,"AOD_off":0,"IOD_on":0,"IOD_off":0,"delay":0}]';


      function f_webSockOnMessage(evt){
        if(typeof evt.data === "string"){
          j=JSON.parse(evt.data);
          document.getElementByName("settings").innerHTML=evt.data;

            for(var key in j[1]){
                if(j[1].hasOwnProperty(key)){
                    if(document.getElementById(key).value=="")
                      document.getElementById(key).value=key!="delay"?j[1][key]:j[1][key]/1000;
                }
            }
        }
      }
      function f_webSockOnOpen(evt){}
      function f_webSockOnError(evt){}
      function f_submit(){
        var str= '[{"t1":0,"h1":0,"t2":0},{"t1_on":75,"t1_off":82,"h1_on":75,"h1_off":85,"t2_on":75,"t2_off":80,"delay":7}]';
        j=JSON.parse(str);

        for(var key in j[1]){
          console.log(document.getElementById(key).value);
          j[1][key]=document.getElementById(key).value*1;
          if(key=="delay")j[1][key]=j[1][key]*1000;
        }
        console.log("sending:", JSON.stringify(j));
        webSock.send(JSON.stringify(j));
      }
      function f_refresh(){
        var x=document.getElementsByName("limit");
        for(var i=0;i<x.length;i++)x[i].value="";
      }
  </script>
</html>
)=====";

  const char SSIDPWD_HTML[] PROGMEM =  R"=====(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="file" content="set_bounds.html">
    <meta name="author" content="Joe Belson 20210822">
    <meta name="what" content="esp8266 html for WiFi connected temp/humidity bounds.">
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Attic Fan Controller - WiFi SSID/PWD</title>
  </head>
  <caption style="font-weight: bolder;">Attic Fan Controller - WiFi SSID/PWD</caption>
  <p></p>

  <body style="text-align: left;">
      <table class="calibrate">
          <thead>
              <tr>
                  <th>SSID</th>
                  <th>PWD</th>
              </tr>
          </thead>
          <tbody>
              <tr>
                  <td><input type="text" id="SSID" name="cred"></td>
                  <td><input type="text" id="PWD" name="cred"></td>
                  <td><input type="submit" id="submit" onclick="f_submit()"></td>
              </tr>
          </tbody></table>


      <br>
      <label for="dhcp">DHCP:</label><input type="checkbox" id="isDHCP" onclick="f_dhcp(checked)">
      <br><br>

      <label for="ip"      name="dhcp">IP: </label>     <input type="tel"  id="ip"      name="dhcp" style="width: 7rem;">
      <label for="gateway" name="dhcp">Gateway: </label><input type="text" id="gateway" name="dhcp" style="width: 7rem;">
      <label for="mask"    name="dhcp">Mask: </label>   <input type="text" id="mask"    name="dhcp" style="width: 7rem;">
      <br><p id="status"></p>
    </body>
    <script language = "javascript" type = "text/javascript">
  
        let webSock       = new WebSocket('ws://'+window.location.hostname+':80');
        webSock.onopen    = function(evt){f_webSockOnOpen(evt);}
        webSock.onmessage = function(evt){f_webSockOnMessage(evt);}
        webSock.onerror   = function(evt){f_webSockOnError(evt);}

        var str= '{"SSID":"0","PWD":"0", "ip":"0","gateway":"0","mask":"0","isDHCP":"0"}';
        j=JSON.parse(str);

        function f_webSockOnMessage(evt){
          if(typeof evt.data === "string"){
              document.getElementById("status").innerHTML=evt.data;
              j=JSON.parse(evt.data);
              n=document.getElementsByName("dhcp");
              for(var i=0;i<n.length;i++)
                if(n[i].value="")n[i].value=j[n[i].id];
          }
        }
        function f_webSockOnOpen(evt){}
        function f_webSockOnError(evt){}
        function f_submit(){
            for(var key in j){
              if(key=="isDHCP")j[key]=document.getElementById(key).checked;
              else j[key]=document.getElementById(key).value;
            }
          webSock.send(JSON.stringify(j));
        }
        function f_dhcp(checked){
          const n=document.getElementsByName("dhcp");
          if(checked)for(var i=0;i<n.length;i++)n[i].setAttribute("hidden", "hidden");
          else for(var i=0;i<n.length;i++)n[i].removeAttribute("hidden");
        }
     </script>
  </html>
      )=====";
