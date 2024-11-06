
String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

//html for the website
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Texas A&M Wind Energy Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
    <link rel="icon" href="data:,">
    <style>
      html {
        font-family: Arial; 
        display: inline-block; 
        text-align: center;
      }
      p { 
        font-size: 1.2rem;
      }
      body {  
        margin: 0;
      }
      .topnav { 
        overflow: hidden; 
        background-color: #500000; 
        color: white; 
        font-size: 1rem; 
      }
      .content { 
        padding: 20px; 
      }
      .card { 
        background-color: white; 
        box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); 
      }
      .cards { 
        max-width: 800px; 
        margin: 0 auto; 
        display: grid; 
        grid-gap: 2rem; 
        grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
      .reading { 
        font-size: 1.4rem;  
      }
    </style>
    <script src="smoothie.js"></script>
  </head>

  <body>
    <div class="topnav">
      <h1>A&M Wind Energy Monitor</h1>
    </div>
    <div class="content">
      <div class="cards">
        <div class="card">
          <p>Batery1 Charge</p>
          <p><span class="reading"><span id="bat1">%bat1%</span> &percnt;</span></p>
        </div>
        <div class="card">
          <p>Batery2 Charge</p>
          <p><span class="reading"><span id="bat2">%bat2%</span> &percnt;</span></p>
        </div>
        <div class="card">
          <p>Power Generated</p>
          <p><span class="reading"><span id="pwr">%pwr%</span> W</span></p>
        </div>
        <div class="card">
          <p>Power Usage</p>
          <p><span class="reading"><span id="use">%use%</span> W</span></p>
        </div>
      </div>
    </div>
    <canvas id="mycanvas" width="300" height="100" style="border:1px solid #000000;"></canvas>
    <script>
      if (!!window.EventSource) {
        var source = new EventSource("/events");
        source.addEventListener("open", function(e) {
        console.log("Events Connected");
      }, false);
      source.addEventListener("error", function(e) {
        if (e.target.readyState != EventSource.OPEN) {
          console.log("Events Disconnected");
        }
      }, false);
      source.addEventListener("message", function(e) {
        console.log("message", e.data);
      }, false);
      
      source.addEventListener("bat1", function(e) {
        console.log("level1", e.data);
        document.getElementById("bat1").innerHTML = e.data;
      }, false);
      source.addEventListener("bat2", function(e) {
        console.log("level2", e.data);
        document.getElementById("bat2").innerHTML = e.data;
      }, false);
      source.addEventListener("pwr", function(e) {
        console.log("pwr", e.data);
        document.getElementById("pwr").innerHTML = e.data;
      }, false);
      source.addEventListener("use", function(e) {
        console.log("use", e.data);
        document.getElementById("use").innerHTML = e.data;
      }, false);
      var line1 = new TimeSeries();
      var line2 = new TimeSeries();
      
        source.addEventListener("avgPwr", function(e) {
          console.log("avgPwr", e.data);
          var temp = JSON.parse(e.data);
          console.log(parseFloat(temp));
          line1.append(new Date().getTime(), parseFloat(temp));
        }, false);
        source.addEventListener("avgUse", function(e) {
          console.log("avgUse", e.data);
          var temp = JSON.parse(e.data);
          console.log(parseFloat(temp));
          line2.append(new Date().getTime(), parseFloat(temp));
        }, false);
      
      
      var smoothie = new SmoothieChart({millisPerPixel:1000,grid:{millisPerLine:50000, verticalSections:5}});

      smoothie.addTimeSeries(line1, { strokeStyle:'rgb(0, 255, 0)', fillStyle:'rgba(0, 255, 0, 0.4)', lineWidth:3 });
      smoothie.addTimeSeries(line2, { strokeStyle:'rgb(255, 0, 0)', fillStyle:'rgba(255, 0, 0, 0.3)', lineWidth:3 });
      smoothie.streamTo(document.getElementById("mycanvas"), 1000); 
      }
      </script>
  </body>
</html>
)rawliteral";

/*
   <script>
    function refresh(refreshPeriod){
      setTimeout('location.reload(true)', refreshPeriod);
    } 
    window.onload = refresh(10000);
    </script>
 */

// Load Wi-Fi library
#include <WiFi.h>
// Load INA219 library
#include <INA219.h>
// IFTTT Webhook
#include <IFTTTWebhook.h>

// SPIFFS
#include "SPIFFS.h"
//AsyncWebServer
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"

// Replace with your network credentials
char ssid[] = "Maxwell’s SE";       //  your network SSID (name)
char pass[] = "orangeviolin270";          //  your network password

// For linking with IFTTT
const char* resource = "/trigger/Wind_Powered_Building_Readings/with/json/key/dRopscmb3FOKFIntbfvffh";

// Webhook IFTTT
const char* WebHookserver = "maker.ifttt.com";
IFTTTWebhook webhook(resource, "Wind_Powered_Building_Reading");

// Set web server port number to 80
AsyncWebServer server(80);
//WiFiServer server(80);

// Event Source
AsyncEventSource events("/events");

// Intizilizing INA219"s
INA219 ina219_A;
//INA219 ina219_B;

// Tracking time to calcualte and average AH
unsigned long startMillis;
unsigned long currentMillis;
unsigned long startSecMillis;
unsigned long currentSecMillis;
unsigned long startHrMillis;
unsigned long currentHrMillis;
unsigned long startRstMillis;
unsigned long currentRstMillis;
const unsigned long period = 60000; // time for one minute
const unsigned long secPeriod = 1000; // time for one sec
const unsigned long hrPeriod = 3600000; //time for one hour
//const unsigned long hrPeriod = 60000;

// Battery Current Values
float batCurIn = 0;
float totbatCurIn = 0;
float maxBat1Q = 0;
float maxBat2Q = 0;

// Value to Track what is being used
int batCharge = 0; // 0 = Bat1 charging, 1 = Bat2 charging, 2 = none
int batUse = 1; // 0 = Bat1 use, 1 = Bat2 use, 2 = none
String status1 = "";
String status2= "";

//Current Values
float curOut = 0;
float voltageOut = 0;
float batCurOut = 0;
float totbatCurOut = 0;
float curCnt = 0;
float curOutCnt = 0;

// Battery %
// Need to set these values as the initial percetanges of the battery
float batPer1 = 0.85;
float batPer2 = 0.25;

// Battery Health
float batHealth1 = 0;
float batHealth2 = 0;
float batLevel1 = 0;
float batLevel2 = 0;
float maxBatVol = 13; //Maximum battery voltage
//Convert to AMin 7.2*60 = 432
float batRating = 7.2; //Battery rating of 7.2 AH

String tmpStrng = "";

// Assign output variables to GPIO pins
const int batPin1 = 39;
const int batPin2 = 34;

//Battery Volatges
float vol1 = 0;
float vol2 = 0;
float batVol1 = 0;
float totBatVol1 = 0;
float prevbatVol1 = 0;
float batVol2 = 0;
float totBatVol2 = 0;
float prevbatVol2 = 0;
float batCnt = 0;

//Battery Percentage
float level1 = batPer1 * 100;
float level2 = batPer2 * 100;

//Health
float health1 = 0;
float health2 = 0;

//Usage/Generation
float genLevel = 0;
float useLevel = 0;

//PowerGen/Used Values
float pwrGen = 0;
float prevPwrGen = 0;
float pwrUse = 0;
float prevPwrUse = 0;
float totPwrGen = 0;
float totPwrUse = 0;
float avgPwrGen = 0;
float avgPwrUse = 0;
float pwrCnt = 0;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

String processor(const String& var){
  //Serial.println(var);
  if(var == "bat1"){
    return String(level1);
  }
  else if(var == "bat2"){
    return String(level2);
  }
//  else if(var == "heal1"){
//    return String(health1);
//  }
//  else if(var == "heal2"){
//    return String(health2);
//  }
  else if(var == "pwr"){
    return String(genLevel);
  }
  else if(var == "use"){
    return String(useLevel);
  }
  else if(var == "avgPwr"){
    return String(avgPwrGen);
  }
  else if(var == "avgUse"){
    return String(avgPwrUse);
  }
}

void setup() {
  Serial.begin(115200);
    Serial.println();
    Serial.println("Serial started at 115200");
    Serial.println();
    //start time
    startMillis = millis();
    startSecMillis = millis();
    startHrMillis = millis();
    startRstMillis = millis();
    // Connect to a WiFi network
    Serial.print(F("Connecting to "));  Serial.println(ssid);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) 
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("");
    Serial.println(F("[CONNECTED]"));
    Serial.print("[IP ");              
    Serial.print(WiFi.localIP()); 
    Serial.println("]");

    //loading spiffs files
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, processor);
    });

    server.serveStatic("/", SPIFFS, "/");

//    server.on("/smoothie.js", HTTP_GET, [](AsyncWebServerRequest *request){
//      request->send(SPIFFS, "/smoothie.js", "text/javascript");
//    });

    Serial.println(SPIFFS.exists("/smoothie.js"));

    //handling event reausests
    events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // testing event request
    client->send("hello!", NULL, millis(), 10000);
    });
    server.addHandler(&events);
    
    // start a server
    server.begin();
    Serial.println("Server started");

    //Starting INA219
    ina219_A.begin();
    //ina219_B.begin();

    //battery percentage setup
    
    
    //setting up pins for sending signals
    //39 34 35 32 being used already
    //first using two add more later
    //Battery 1 Charge
    pinMode(33, OUTPUT);
    //Battery 2 Charge
    pinMode(25, OUTPUT);
    //Dumpload
    pinMode(26, OUTPUT);
    //DisBattery1
    pinMode(27, OUTPUT);
    //DisBattery2
    pinMode(14, OUTPUT);
    //setting up default incase batteries are inbetween 30 and 80%
    batUse = 1;
    batCharge = 0;
    digitalWrite(25, LOW);
    digitalWrite(26, LOW);
    digitalWrite(27, LOW);
    digitalWrite(14, HIGH);
    digitalWrite(33, HIGH);
    status1 = "Battery 1 Charging";
    status2 = "Battery 2 Powering Building";
}

void loop(){
  //WiFiClient client = server.available(); // Listen for incoming clients

  //BatteryVoltageMonitor
  //need to remap
  vol1 = analogRead(batPin1);
  batVol1 = map(vol1, 0, 4095, 0, 17100); // mapping 0-4095 to 0-25000 mV
  batVol1 /= 1000; //convert mV to V
  //level1 = String(batVol1);
  totBatVol1 = totBatVol1 + batVol1;
  Serial.print("Battery 1: ");
  Serial.print(batVol1);
  Serial.println("V");
  Serial.println(vol1);

  vol2 = analogRead(batPin2);
  batVol2 = map(vol2, 0, 4095, 0, 17100); // mapping 0-4095 to 0-25000 mV
  batVol2 /= 1000; //covert mV to V
  //level2 = String(batVol2);
  totBatVol2 = totBatVol2 + batVol2;
  Serial.print("Battery 2: ");
  Serial.print(batVol2);
  Serial.println("V");

  //Current Monitor
  batCurIn = ina219_A.shuntCurrent();
  totbatCurIn = totbatCurIn + batCurIn;
  Serial.print("Current In: ");
  Serial.print(batCurIn);
  Serial.println("A");
  
  /*batCurOut = ina219_b.getCurrent_mA();
  totbatCurOut = totbatCurOut + batCurOut;
  Serial.print("Current 2: ");
  Serial.print(batCurOut);
  Serial.println("%");*/
  curCnt = curCnt + 1;
  batCnt = batCnt + 1;
  int curOutRead;
  int curOutPin;
  int maxCurOut = 0;
  int minCurOut = 4096;
  //logic to pick between which current sensor to pick from
  if(batUse == 0){
    curOutPin = 35;
  }
  else if(batUse == 1){
    curOutPin = 32;
  }
  startSecMillis = millis();
  while(millis() - startSecMillis < 1000){
    curOutRead = analogRead(curOutPin);
    if(curOutRead > maxCurOut){
      maxCurOut = curOutRead;
    }
    if(curOutRead < minCurOut){
      minCurOut = curOutRead;
    }
  }
  //curOut = analogRead(35);
  curOut = maxCurOut - minCurOut;
  voltageOut = curOut*3.3/4095.0;
  batCurOut = (((voltageOut/2*0.707*1000)/100 - 0.2) * 4); //0.6 offset need to adjust
  if(batCurOut < 0){
    batCurOut = 0;
  }
  else{
    curOutCnt = curOutCnt + 1;
  }
  totbatCurOut = totbatCurOut + batCurOut;
  Serial.print("Current Out: ");
  Serial.print(batCurOut);
  Serial.println("A");
  Serial.println(curOut);

  //Logic for Usage and Charging switching
  //Need to change for BatterLevel
  if(batPer1 > .8){
    if(batPer2 > .8){
      //Discharge power to dumpload
      Serial.println("Both batteries are fully charged, sending power to dumpload");
      batCharge = 2; 
      batUse = 0;
      digitalWrite(33, LOW);
      digitalWrite(25, LOW);
      digitalWrite(14, LOW);
      digitalWrite(27, HIGH);
      digitalWrite(26, HIGH);
      status1 = "Battery 1 Powering Building";
      status2 = "Sending excess power to dumpload";
    }
    else{
      //Start Discharing Battery1
      Serial.println("Battery1 is almost fully charged, now being used to power the building");
      batUse = 0;
      batCharge = 1;
      digitalWrite(33, LOW);
      digitalWrite(26, LOW);
      digitalWrite(14, LOW);
      digitalWrite(27, HIGH);
      digitalWrite(25, HIGH);
      status1 = "Battery 1 Powering Building";
      status2 = "Battery 2 Charging";
    }
  }
  else if(batPer2 > .8){
    //Start Discharging Battery2
    Serial.println("Battery2 is almost fully charged, now being used to power the building");
    batUse = 1;
    batCharge = 0;
    digitalWrite(25, LOW);
    digitalWrite(26, LOW);
    digitalWrite(27, LOW);
    digitalWrite(14, HIGH);
    digitalWrite(33, HIGH);
    status1 = "Battery 1 Charging";
    status2 = "Battery 2 Powering Building";
  }
  //Start Charging at 4V
  else if(batPer1 < .3){
    if(batPer2 < .3){
      //no batteries have power discount from building
      batCharge = 0;
      batUse = 2;
      digitalWrite(25, LOW);
      digitalWrite(26, LOW);
      digitalWrite(27, LOW);
      digitalWrite(14, LOW);
      digitalWrite(33, HIGH);
      status1 = "Battery 1 Charging";
      status2 = "Not enough power for building, using external source";
    }
    else{
      //Start Charging Battery1
      Serial.println("Battery1 is almost out of power, now charging with the wind turbine");
      batCharge = 0;
      batUse = 1;
      digitalWrite(25, LOW);
      digitalWrite(26, LOW);
      digitalWrite(27, LOW);
      digitalWrite(14, HIGH);
      digitalWrite(33, HIGH);
      status1 = "Battery 1 Charging";
      status2 = "Battery 2 Powering Building";
    }
  }
  else if(batPer2 < .3){
    //Start Charging Battery2
    Serial.println("Battery2 is almost out of power, now charging with the wind turbine");
    batCharge = 1;
    batUse = 0;
    digitalWrite(33, LOW);
    digitalWrite(26, LOW);
    digitalWrite(14, LOW);
    digitalWrite(27, HIGH);
    digitalWrite(25, HIGH);
    status1 = "Battery 1 Powering Building";
    status2 = "Battery 2 Charging";
  }
  //If Turbine not generating enough power
  if(batCurIn <= 0){
    batCharge = 2;
    digitalWrite(33, LOW);
    digitalWrite(25, LOW);
    digitalWrite(26, HIGH);
  }
  else if(batCharge == 2){
    if(batUse == 0){
      batCharge = 1;
      digitalWrite(33, LOW);
      digitalWrite(25, HIGH);
      digitalWrite(26, LOW);
    }
    else if(batUse == 1){
      batCharge = 0;
      digitalWrite(33, HIGH);
      digitalWrite(25, LOW);
      digitalWrite(26, LOW);
    }
  }


  //PowerGenerated/Used
//  if(prevbatVol1 - batVol1 > 200 && prevbatVol1 < 0){
//    pwrUse = pwrUse + (prevbatVol1 - batVol1);
//  }
//  else if(prevbatVol1 - batVol1 <  -200 && prevbatVol1 < 0){
//    pwrGen = pwrGen + (batVol1 - prevbatVol1);
//  }
//
//  if(prevbatVol2 - batVol2 > 200 && prevbatVol2 < 0){
//    pwrUse = pwrUse + (prevbatVol2 - batVol2);
//  }
//  else if(prevbatVol2 - batVol2 < -200 && prevbatVol2 < 0){
//    pwrGen = pwrGen + (batVol2 - prevbatVol2);
//  }
  //Serial.println(startMillis);
  currentMillis = millis(); //current time
    if(currentMillis - startMillis >= period){ //if current time - start time is over a minute
      //Calcualting State of Health with Charge(AH) divide by rated capacity
      Serial.println("One Minute has Passed");
      Serial.print("Total Current In:");
      Serial.println(totbatCurIn);
      Serial.print("Average Current In:");
      Serial.println(totbatCurIn/curCnt);      
      float temp1 = (totbatCurIn/curCnt)/60.0/batRating;
      float curOutAvg = 0.0;
      if(curOutCnt != 0.0){
        curOutAvg = totbatCurOut/curOutCnt;
      }
      float temp2 = (curOutAvg)/60.0/batRating;
      Serial.println(temp1, 8);
      if(batCharge == 0){
        batPer1 = batPer1 + temp1; 
//        if((totbatCurIn/curCnt)*(1/60) >= maxBat1Q){
//          maxBat1Q = (totbatCurIn/curCnt)*(1/60);
//        }
      }
      else if(batCharge == 1){
        batPer2 = batPer2 + temp1;
//        if((totbatCurIn/curCnt)*(1/60) >= maxBat2Q){
//          maxBat2Q = (totbatCurIn/curCnt)*(1/60);
//        }
      }

      if(batUse == 0){
        batPer1 = batPer1 - temp2; 
        pwrUse = batVol1 * curOutAvg;
//        if((totbatCurIn/curCnt)*(1/60) >= maxBat1Q){
//          maxBat1Q = (totbatCurIn/curCnt)*(1/60);
//        }
      }
      else if(batUse == 1){
        batPer2 = batPer2 - temp2;
        pwrUse = batVol2 * curOutAvg;
//        if((totbatCurIn/curCnt)*(1/60) >= maxBat2Q){
//          maxBat2Q = (totbatCurIn/curCnt)*(1/60);
//        }
      }
//      batHealth1 = (totbatCurIn/curCnt)*(1/60)/batRating; 
//      batHealth2 = (totbatCurOut/curCnt)*(1/60)/batRating;
//      if(batHealth1 < (totBatVol1/batCnt)/maxBatVol){
//        batHealth1 = (totBatVol1/batCnt)/maxBatVol;
//        health1 = batHealth1*100;
//      }
//      if(batHealth2 < (totBatVol2/batCnt)/maxBatVol){
//        batHealth2 = (totBatVol2/batCnt)/maxBatVol;
//        health2 = batHealth2*100;
//      }
      //testing switching swapping values to force swithing
      float holder = batPer1;
      batPer1 = batPer2;
      batPer2 = holder;
      level1 = batPer1*100;
      level2 = batPer2*100;
      startMillis = currentMillis;
      totbatCurIn = 0;
      totbatCurOut = 0;
      curCnt = 0;
      curOutCnt = 0;
      Serial.print("Battery1 Charge: ");
      Serial.println(batPer1, 8);
      Serial.print("Battery2 Charge: ");
      Serial.println(batPer2, 8);
    }

    pwrGen = ina219_A.busPower(); //reading in power generated in W
    //for pwr used voltage of which every battery is being used * the current out

    //uselevel need to be fixed
    //printing to monitor
    Serial.print("Power Generated: ");
    genLevel = pwrGen;
    totPwrGen = totPwrGen + genLevel;
    Serial.println(pwrGen);
    Serial.print("Power Consumed: ");
    useLevel = pwrUse;
    if(useLevel < 0){
      useLevel = 0;
    }
    totPwrUse = totPwrUse + useLevel;
    Serial.println(pwrUse);
    pwrCnt = pwrCnt + 1;

    // if its been one hour
    currentHrMillis = millis();
    if(currentHrMillis - startHrMillis >= period){
      avgPwrGen = totPwrGen / pwrCnt;
      avgPwrUse = totPwrUse / pwrCnt;
      if(avgPwrUse < 0){
        avgPwrUse = 0;
      }
      
//      Serial.print("Connecting to "); 
//      Serial.print(WebHookserver);
//      webhook.trigger("10", "10");
      
//      WiFiClient iftclient;
//      int retries = 10;
//      while(!!!iftclient.connect(WebHookserver, 80) && (retries-- > 0)) {
//        Serial.print(".");
//      }
//      Serial.println();
//      if(!!!iftclient.connected()) {
//        Serial.println("Failed to connect...");
//      }
//      Serial.print("Request resource: "); 
//      Serial.println(resource);
////        char* temp1;
////        sprintf(temp1, "%f", maxBat1Q);
////        char* temp2;
////        sprintf(temp2, "%f", maxBat2Q);        
////        webhook.trigger(temp1, temp2);
//      String jsonObject = String("{\"value1\":\"") + avgPwrGen + "\",\"value2\":\"" + avgPwrUse + "\"}";
//      iftclient.println(String("POST ") + resource + " HTTP/1.1");
//      iftclient.println(String("Host: ") + WebHookserver); 
//      iftclient.println("Connection: close\r\nContent-Type: application/json");
//      iftclient.print("Content-Length: ");
//      iftclient.println(jsonObject.length());
//      iftclient.println();
//      iftclient.println(jsonObject);
//
//      int timeout = 5 * 10; // 5 seconds             
//      while(!!!iftclient.available() && (timeout-- > 0)){
//        delay(100);
//      }
//      
//      if(!!!iftclient.available()) {
//        Serial.println("No response...");
//      }
//      while(iftclient.available()){
//        Serial.write(iftclient.read());
//      }
//  
//      Serial.println("\nclosing connection");
//      iftclient.stop(); 
      
      startHrMillis = currentHrMillis;
      maxBat1Q = 0;
      maxBat2Q = 0;

      batHealth1 = 0;
      batHealth2 = 0;

      //resetting cnt and tot for average power generated and used
      totPwrGen = 0;
      totPwrUse = 0;
      pwrCnt = 0;
  }  
  currentRstMillis = millis();
  //Reseting the battery percertange after a week to account for possible loss in circuits and from heat
  // 168*hrPeriod is one week
  if(currentRstMillis = startRstMillis >= 168*hrPeriod){
    /* Caclauting Percentage using only voltage since its independent of everything but the batter
     * Considering 13V to be 100% and 12V to be 0%
     */
    batPer1 = (batVol1 - 12);
    batPer2 = (batVol2 - 12);
  }
  
  delay(500);
//
//  if (!client)  {  return;  }

//    if (batVol1 != prevbatVol1){
//      level1 = prevLevel1;
//    }
//    if (batVol2 != prevbatVol2){
//      level2 = prevLevel2;
//    }
//    if(pwrGen != prevPwrGen){
//      genLevel = prevGenLevel;
//    }
//    if(pwrUse != prevPwrUse){
//      useLevel = prevUseL evel;
//    }

    //updating values in webpage
    events.send("ping",NULL,millis());
    events.send(String(level1).c_str(),"bat1",millis());
    events.send(String(level2).c_str(),"bat2",millis());
//    events.send(String(health1).c_str(),"heal1",millis());
//    events.send(String(health2).c_str(),"heal2",millis());
    events.send(String(genLevel).c_str(),"pwr",millis());  
    events.send(String(useLevel).c_str(),"use",millis());  
    events.send(String(avgPwrGen).c_str(),"avgPwr",millis());
    events.send(String(avgPwrUse).c_str(),"avgUse",millis());

    //old format using WifiServer
//    tmpStrng = html_1;
//    tmpStrng.replace("%stat1%", status1);
//    tmpStrng.replace("%stat2%", status2);
//    tmpStrng.replace("%bat1%", level1 );
//    tmpStrng.replace("%bat2%", level2 );
//    tmpStrng.replace("%heal1%", health1);
//    tmpStrng.replace("%heal2%", health2);
//    tmpStrng.replace("%pwr%", genLevel);
//    tmpStrng.replace("%use%", useLevel);
//
//    //refreshing values on webpage
//    client.flush();
//    client.print( header );
//    client.print( tmpStrng );   

    delay(5);

  //recording prev Battery Values
//  prevbatVol1 = batVol1;
//  prevLevel1 = String(prevbatVol1);
//  prevbatVol2 = batVol2;
//  prevLevel2 = String(prevbatVol2);
//  prevPwrGen = pwrGen;
//  prevGenLevel = String(prevPwrGen);
//  prevPwrUse = pwrUse;
//  prevUseLevel = String(prevPwrUse);
}
