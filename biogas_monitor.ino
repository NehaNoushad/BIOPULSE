
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ========== WIFI ACCESS POINT ==========
const char* AP_SSID = "Biogas_Monitor";
const char* AP_PASSWORD = "BioGas123";

// ========== SENSOR PINS ==========
#define DHTPIN 4
#define DHTTYPE DHT11

const int MQ5_PIN = 34;
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

#define SDA_PIN 21
#define SCL_PIN 22

// ========== DEVICES ==========
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// ========== CONFIGURATION ==========
const float TANK_HEIGHT_CM = 100.0;

// Example only: calibrate for your MQ-5 module.
const int GAS_THRESHOLD = 1800;
const bool GAS_HIGHER_READING = true;

const float FULL_LEVEL_PERCENT = 90.0;

// ========== SENSOR VARIABLES ==========
float temperatureC = NAN;
float distanceCM = -1;
float levelPercent = -1;

int gasADC = 0;
bool gasAlarm = false;
bool tankFull = false;

unsigned long lastSensorRead = 0;
unsigned long lcdPageStart = 0;

const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long LCD_INTERVAL = 3000;

int lcdPage = 0;

// ========== EMBEDDED DASHBOARD ==========
const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Biogas Monitor</title>

<style>
*{box-sizing:border-box}
body{
 margin:0;background:#0b1220;color:#edf4ff;
 font-family:Arial,sans-serif
}
header{
 padding:22px 18px;background:#111d31;
 border-bottom:1px solid #293a53
}
header h1{margin:0;font-size:24px}
header p{color:#aabbd3;margin:8px 0 0}
main{max-width:850px;margin:auto;padding:18px}
.status{
 padding:12px 14px;border-radius:10px;
 background:#17263b;margin-bottom:16px;color:#c1d1e5
}
.grid{
 display:grid;grid-template-columns:repeat(2,minmax(0,1fr));
 gap:14px
}
.card{
 background:#152238;border:1px solid #293b56;
 border-radius:16px;padding:18px;min-width:0
}
.card h2{font-size:15px;color:#b9c9df;margin:0 0 18px}
.value{font-size:30px;font-weight:bold;margin-bottom:12px;
 overflow-wrap:anywhere}
.unit{font-size:15px;color:#b9c9df}
.pill{
 display:inline-block;padding:7px 10px;border-radius:20px;
 font-size:12px;font-weight:bold;background:#344158;color:#e1eafa
}
.good{background:#164b3a;color:#85f0c0}
.warn{background:#624516;color:#ffdc88}
.bad{background:#632b35;color:#ffb4bc}
.bar{
 height:10px;background:#34435b;border-radius:10px;
 overflow:hidden;margin:14px 0
}
.fill{height:100%;width:0%;background:#53c6ed}
small{display:block;color:#aabbd3;line-height:1.5}
footer{padding:24px 0;color:#94a6bf;font-size:12px;line-height:1.6}
@media(max-width:480px){
 .grid{grid-template-columns:1fr}
}
</style>
</head>

<body>
<header>
 <h1>Biogas Monitor</h1>
 <p>ESP32 live monitoring dashboard</p>
</header>

<main>
 <div class="status" id="connection">
 Connecting to ESP32...
 </div>

 <div class="grid">

  <section class="card">
   <h2>Temperature</h2>
   <div class="value">
    <span id="temp">--</span>
    <span class="unit">°C</span>
   </div>
   <span class="pill" id="tempStatus">Waiting</span>
   <small>DHT11 sensor</small>
  </section>

  <section class="card">
   <h2>Gas leakage monitor</h2>
   <div class="value" id="gasValue">--</div>
   <span class="pill" id="gasStatus">Waiting</span>
   <small>MQ-6 raw ADC reading</small>
  </section>

  <section class="card">
   <h2>Slurry fill level</h2>
   <div class="value">
    <span id="level">--</span>
    <span class="unit">%</span>
   </div>
   <div class="bar">
    <div class="fill" id="levelBar"></div>
   </div>
   <span class="pill" id="levelStatus">Waiting</span>
   <small>Distance: <span id="distance">--</span> cm</small>
  </section>

  <section class="card">
   <h2>System status</h2>
   <div class="value" style="font-size:23px" id="system">
    Starting
   </div>
   <span class="pill" id="sensorStatus">Checking</span>
   <small>Sensor connection status</small>
  </section>

 </div>

 <footer>
  Refreshes every 2 seconds. Connect to ESP32 Wi-Fi.
  MQ-5 readings are indicative only, not certified gas alarms.
 </footer>
</main>

<script>
const $ = id => document.getElementById(id);

function pill(id,text,state){
 $(id).textContent=text;
 $(id).className='pill '+state;
}

async function update(){
 try{
  const response=await fetch('/data',{cache:'no-store'});
  if(!response.ok)throw Error('HTTP error');
  const d=await response.json();

  $('connection').textContent='Connected to ESP32 | Live data';

  // TEMPERATURE AND OPTIMUM-RANGE LABEL
  $('temp').textContent =
    d.temperature===null ? '--' : d.temperature.toFixed(1);

  if(d.temperature===null){
    pill('tempStatus','Sensor error','bad');
  }else if(d.temperature<30){
    pill('tempStatus','Below target','warn');
  }else if(d.temperature<=40){
    pill('tempStatus','Suitable range','good');
  }else{
    pill('tempStatus','High temperature','bad');
  }

  // MQ-5 GAS MONITOR
  $('gasValue').textContent=d.gas_adc;

  pill('gasStatus',
    d.gas_alarm?'GAS ALERT':'Below threshold',
    d.gas_alarm?'bad':'good');

  // ULTRASONIC SLURRY LEVEL
  $('level').textContent =
    d.level_pct===null?'--':d.level_pct;

  $('distance').textContent =
    d.distance_cm===null?'--':d.distance_cm;

  $('levelBar').style.width =
    (d.level_pct===null?0:d.level_pct)+'%';

  pill('levelStatus',
    d.level_pct===null?'Check sensor':
    d.full?'SLURRY FULL':
    d.level_pct>=80?'Level high':'Level normal',
    d.level_pct===null?'bad':
    d.full?'bad':d.level_pct>=80?'warn':'good');

  // OVERALL SYSTEM STATUS
  const problem =
    d.temperature===null || d.distance_cm===null;

  $('system').textContent =
    problem?'Check sensors':'Monitoring';

  pill('sensorStatus',
    problem?'Sensor issue':'Data available',
    problem?'warn':'good');

 }catch(error){
  $('connection').textContent='Connection lost. Reconnecting...';
  $('system').textContent='Offline';
  pill('sensorStatus','No connection','bad');
 }
}

update();
setInterval(update,2000);
</script>
</body>
</html>
)HTML";

// ========== ULTRASONIC MEASUREMENT ==========
float readDistanceCM(){
 digitalWrite(TRIG_PIN,LOW);
 delayMicroseconds(3);

 digitalWrite(TRIG_PIN,HIGH);
 delayMicroseconds(10);
 digitalWrite(TRIG_PIN,LOW);

 unsigned long duration=pulseIn(ECHO_PIN,HIGH,30000UL);

 if(duration==0)return -1;

 float cm=duration*0.0343f/2.0f;

 if(cm<2.0||cm>400.0)return -1;

 return cm;
}

// ========== READ SENSORS ==========
void readSensors(){
 temperatureC=dht.readTemperature();
 distanceCM=readDistanceCM();
 gasADC=analogRead(MQ5_PIN);

 gasAlarm=GAS_HIGHER_READING
          ?gasADC>=GAS_THRESHOLD
          :gasADC<=GAS_THRESHOLD;

 if(distanceCM>=0&&distanceCM<=TANK_HEIGHT_CM){
  levelPercent=
    (TANK_HEIGHT_CM-distanceCM)/TANK_HEIGHT_CM*100.0f;

  levelPercent=constrain(levelPercent,0.0f,100.0f);
  tankFull=levelPercent>=FULL_LEVEL_PERCENT;
 }else{
  levelPercent=-1;
  tankFull=false;
 }

 Serial.print("Temperature: ");
 if(isnan(temperatureC))Serial.print("ERROR");
 else Serial.print(temperatureC);

 Serial.print(" C | MQ5 ADC: ");
 Serial.print(gasADC);

 Serial.print(" | Distance: ");
 Serial.print(distanceCM);

 Serial.print(" cm | Slurry: ");
 Serial.print(levelPercent);
 Serial.println("%");
}

// ========== LCD DISPLAY ==========
void printLCDLine(byte row,const char* message){
 lcd.setCursor(0,row);
 lcd.print("                ");
 lcd.setCursor(0,row);

 for(byte i=0;i<16&&message[i]!='\0';i++){
  lcd.print(message[i]);
 }
}

void updateLCD(){
 char line1[17];
 char line2[17];

 if(lcdPage==0){
  if(isnan(temperatureC)){
   snprintf(line1,sizeof(line1),"Temp: Sensor Err");
  }else{
   snprintf(line1,sizeof(line1),"Temp: %.1f C",temperatureC);
  }

  if(gasAlarm){
   snprintf(line2,sizeof(line2),"GAS LEAK ALERT!");
  }else{
   snprintf(line2,sizeof(line2),"Gas ADC:%4d OK",gasADC);
  }
 }else{
  if(levelPercent<0){
   snprintf(line1,sizeof(line1),"Slurry: Sensor ?");
   snprintf(line2,sizeof(line2),"Check Ultrasonic");
  }else{
   snprintf(line1,sizeof(line1),"Slurry: %3.0f%%",levelPercent);

   if(tankFull){
    snprintf(line2,sizeof(line2),"!!! TANK FULL !!!");
   }else{
    snprintf(line2,sizeof(line2),"Dist:%5.1f cm",distanceCM);
   }
  }
 }

 printLCDLine(0,line1);
 printLCDLine(1,line2);
}

// ========== WEB API ==========
void handleData(){
 String json="{";

 json+="\"temperature\":";
 json+=isnan(temperatureC)?"null":String(temperatureC,1);

 json+=",\"gas_adc\":";
 json+=String(gasADC);

 json+=",\"gas_alarm\":";
 json+=gasAlarm?"true":"false";

 json+=",\"distance_cm\":";
 json+=levelPercent>=0?String(distanceCM,1):"null";

 json+=",\"level_pct\":";
 json+=levelPercent>=0
       ?String((int)roundf(levelPercent)):"null";

 json+=",\"full\":";
 json+=tankFull?"true":"false";

 json+="}";

 server.sendHeader("Cache-Control","no-store");
 server.send(200,"application/json",json);
}

void handleRoot(){
 server.send_P(200,"text/html",INDEX_HTML);
}

// ========== SETUP ==========
void setup(){
 Serial.begin(115200);

 pinMode(TRIG_PIN,OUTPUT);
 pinMode(ECHO_PIN,INPUT);
 analogReadResolution(12);

 dht.begin();

 Wire.begin(SDA_PIN,SCL_PIN);
 lcd.init();
 lcd.backlight();

 printLCDLine(0,"Biogas Monitor");
 printLCDLine(1,"Starting...");
 delay(1000);

 WiFi.mode(WIFI_AP);
 bool apStarted=WiFi.softAP(AP_SSID,AP_PASSWORD);

 if(apStarted){
  Serial.println("Access Point started");
  Serial.print("Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
 }else{
  Serial.println("Failed to start Access Point");
 }

 server.on("/",HTTP_GET,handleRoot);
 server.on("/data",HTTP_GET,handleData);
 server.begin();

 readSensors();
 updateLCD();

 lastSensorRead=millis();
 lcdPageStart=millis();
}

// ========== MAIN LOOP ==========
void loop(){
 server.handleClient();

 unsigned long now=millis();

 if(now-lastSensorRead>=SENSOR_INTERVAL){
  lastSensorRead=now;
  readSensors();
 }

 if(now-lcdPageStart>=LCD_INTERVAL){
  lcdPageStart=now;
  lcdPage=1-lcdPage;
  updateLCD();
 }
}
