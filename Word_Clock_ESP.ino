//woordklok door Jeroen Jansen (Sept 2017)

#include <FastLED.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <Timezone.h>


ESP8266WebServer server(80);  //start webserver op poort 80

//LED-strip settings
#define NUM_LEDS 50
#define DATA_PIN 5

int kleurtjes = 0;
int teller = 0;
int cycle_snelheid = 100;
static uint8_t hue = 0;
int C_optie = 0;

//Time-modus settings
int TIME_MODE = 1; //in mode 0 worden de losse minuten opgeteld bij de tijd. In mode 1 worden de minuten van de huidige tijd afgetrokken

//WIFI settings
const char ssid[] = "ssid";                       // your network SSID (name)
const char pass[] = "pasword";                  // your network password
const char hostnamewifi[] = "Woordklok_ESP"; // hostname

// NTP Server / settings:
IPAddress timeServer;                         //NTP server address
const char* ntpServerName = "time.nist.gov";  //NTP adress
const int timeZone = 0;                       //Timezone correction
int sync;                                     //sync-teller (seconden)
int sync_interval = 43200;                    //sync interval (seconden) (43200 sec = 12h)

// Define UDP instance
WiFiUDP Udp;
unsigned int localPort = 123;  // local port to listen for UDP packets

// Define the array of leds
CRGB leds[NUM_LEDS];

//defineer lichtsensor
boolean autobright = true;        //automatische brightness aanpassing
int lichtSensorPin = A0;          //pin voor lichtsensor
int SensorValue = 0;
float lichtSensorFilter = 0.95;   //filter brightnesswaarde. 0 geen filter, 0.99 is max filter
float SensorValueFiltered;
int brightness = 96;              //initiele brightness; wordt ook gebruikt indien lichtsensor niet gebruikt wordt

// defineer klok eenheden
int uur;
int uur12;
int minuut;
int seconde;
int seconde2;


// defineer datum eenheden
int dag;
int maand;
int jaar;

//defineer led-arrays per woord
int woordHET[] = {47,48,49,-1};
int woordIS[] = {45,46,-1};
int woord5[] = {42,-1};
int woord10[] = {43,44,-1};
int woordKWART[] = {37,38,39,40,41,-1};
int woordOVER[] = {32,33,34,35,-1};
int woordVOOR[] = {28,29,30,31,-1};
int woordHALF[] = {20,21,22,23,-1};
int w1[] = {25,-1};
int w2[] = {26,-1};
int w3[] = {27,-1};
int w4[] = {19,-1};
int w5[] = {18,-1};
int w6[] = {17,-1};
int w7[] = {16,-1};
int w8[] = {15,-1};
int w9[] = {14,-1};
int w10[] = {12,13,-1};
int w11[] = {4,5,-1};
int w12[] = {6,7,-1};
int wmin1[] = {3,-1};
int wmin2[] = {2,-1};
int wmin3[] = {1,-1};
int wmin4[] = {0,-1};
int woordUUR[] = {9,10,11,-1};

//define colours
uint32_t Colour = CRGB::White;
String Colour_hex = "FFFFFF";

//timezone en DST functions
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, 120};    //Daylight time = UTC + 2 hours
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 2, 60};     //Standard time = UTC +1 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;

void setup () {
  delay(1000);
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  set_max_power_in_volts_and_milliamps(5, 500);  
  //FastLED.setDither( 0 );


  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.hostname(hostnamewifi);  
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("Waiting for sync");
  WiFi.hostByName(ntpServerName, timeServer);   
  setSyncProvider(getNtpTime);

  ArduinoOTA.setHostname("Woordklok ESP"); // give an name to our module
  ArduinoOTA.begin(); // OTA initialization  

  //start webserver
  server.on("/", handleRoot);
  server.on("/color", handleColor);
  server.on("/restart", handleRestart);
  server.on("/demo", handleDemo);
  server.on("/brightness", handleBrightness);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Setup completed");

    //toon test-animatie
//    test();
//    FastLED.delay(1000);

}

void loop () {

  ArduinoOTA.handle();  
  server.handleClient();

  utc = now();
  local = myTZ.toLocal(utc, &tcr);
  
  uur = hour(local);
  minuut = minute(local);
  seconde = second(local);
  dag = day(local);
  maand = month(local);
  jaar = year(local);

////paarse letters op verjaardag Donja
//  if (dag == 1 && maand == 2){
//    Colour = CRGB::Purple;
//  }
//  else{
//    Colour = White;
//  }

//lees lichtsensor en stel brightness in
  if (autobright == true){
    SensorValue = analogRead(lichtSensorPin);

    //filter sensorwaarde
    SensorValueFiltered = SensorValue * (1-lichtSensorFilter) + SensorValueFiltered * lichtSensorFilter;
    
    //remap brightness. Max sensorwaarde gecapped op 1023. Min helderheid is 5
    brightness = map(SensorValueFiltered, 0, 1023, 255,10);
  }

  FastLED.setBrightness (brightness);

//  Serial.print(SensorValue);
//  Serial.print(" ");
//  Serial.print(SensorValueFiltered);
//  Serial.print(" ");
//  Serial.println(brightness);

//maak led-array leeg
blank();

//standaard HET IS tonen
toon(woordHET, Colour);
toon(woordIS, Colour);

//zet uur om naar 12h format
    if (uur == 1 || uur == 13) uur12 = 1;
    if (uur == 2 || uur == 14) uur12 = 2;
    if (uur == 3 || uur == 15) uur12 = 3;
    if (uur == 4 || uur == 16) uur12 = 4;
    if (uur == 5 || uur == 17) uur12 = 5;
    if (uur == 6 || uur == 18) uur12 = 6;
    if (uur == 7 || uur == 19) uur12 = 7;
    if (uur == 8 || uur == 20) uur12 = 8;
    if (uur == 9 || uur == 21) uur12 = 9;
    if (uur == 10 || uur == 22) uur12 = 10;
    if (uur == 11 || uur == 23) uur12 = 11;
    if (uur == 12 || uur == 00) uur12 = 12;

//bepaal welk uur opgelicht moet worden  
    if ((TIME_MODE == 1 && minuut <= 15) || (TIME_MODE == 0 && minuut <= 19)){
      switch (uur12) {
        case 1: toon(w1, Colour); break; 
        case 2: toon(w2, Colour); break; 
        case 3: toon(w3, Colour); break;
        case 4: toon(w4, Colour); break;
        case 5: toon(w5, Colour); break;
        case 6: toon(w6, Colour); break;
        case 7: toon(w7, Colour); break;
        case 8: toon(w8, Colour); break;
        case 9: toon(w9, Colour); break;
        case 10: toon(w10, Colour); break;
        case 11: toon(w11, Colour); break;
        case 12: toon(w12, Colour); break;        
      }
    }
    else{
      switch (uur12) {
        case 1: toon(w2, Colour); break; 
        case 2: toon(w3, Colour); break; 
        case 3: toon(w4, Colour); break;
        case 4: toon(w5, Colour); break;
        case 5: toon(w6, Colour); break;
        case 6: toon(w7, Colour); break;
        case 7: toon(w8, Colour); break;
        case 8: toon(w9, Colour); break;
        case 9: toon(w10, Colour); break;
        case 10: toon(w11, Colour); break;
        case 11: toon(w12, Colour); break;
        case 12: toon(w1, Colour); break;        
      }      
    }

//behandel minuten
  if(TIME_MODE == 1){
    if(minuut >= 56 || minuut ==0) {
      toon(woordUUR, Colour);
    }
    else if(minuut >= 1 && minuut <=5) {
      toon(woord5, Colour);
      toon(woordOVER, Colour); 
    }
    else if(minuut >= 6 && minuut <=10) {
      toon(woord10, Colour);
      toon(woordOVER, Colour);  
    }
    else if(minuut >= 11 && minuut <=15) {
      toon(woordKWART, Colour);
      toon(woordOVER, Colour);  
    }
    else if(minuut >= 16 && minuut <=20) {
      toon(woord10, Colour);
      toon(woordVOOR, Colour);
      toon(woordHALF, Colour);
    }
    else if(minuut >= 21 && minuut <=25) {
      toon(woord5, Colour);
      toon(woordVOOR, Colour);
      toon(woordHALF, Colour);
    }
    else if(minuut >= 26 && minuut <=30) {
      toon(woordHALF, Colour);
    } 
    else if(minuut >= 31 && minuut <=35) {
      toon(woord5, Colour);
      toon(woordOVER, Colour);
      toon(woordHALF, Colour);
    }
    else if(minuut >= 36 && minuut <=40) {
      toon(woord10, Colour);
      toon(woordOVER, Colour);
      toon(woordHALF, Colour);
    }
    else if(minuut >= 41 && minuut <=45) {
      toon(woordKWART, Colour);
      toon(woordVOOR, Colour);
    }
    else if(minuut >= 46 && minuut <=50) {
      toon(woord10, Colour);
      toon(woordVOOR, Colour);
    }
    else if(minuut >= 51 && minuut <=55) {
      toon(woord5, Colour);
      toon(woordVOOR, Colour);
    }   
  
    if(minuut%5 == 1) {
    toon(wmin1, Colour);
    toon(wmin2, Colour);
    toon(wmin3, Colour);
    toon(wmin4, Colour);    
    }
    else if(minuut%5 == 2) {
    toon(wmin2, Colour);
    toon(wmin3, Colour);
    toon(wmin4, Colour);     
    }
    else if(minuut%5 == 3) {
    toon(wmin3, Colour);
    toon(wmin4, Colour);     
    }
    else if(minuut%5 == 4) {
    toon(wmin4, Colour);      
    } 
  }
  else if(TIME_MODE == 0){
    if(minuut == 00 || minuut <=4) {
      toon(woordUUR, Colour);
    }
    else if(minuut >= 5 && minuut <=9) {
      toon(woord5, Colour);
      toon(woordOVER, Colour); 
    }
    else if(minuut >= 10 && minuut <=14) {
      toon(woord10, Colour);
      toon(woordOVER, Colour);  
    }
    else if(minuut >= 15 && minuut <=19) {
      toon(woordKWART, Colour);
      toon(woordOVER, Colour);  
    }
    else if(minuut >= 20 && minuut <=24) {
      toon(woord10, Colour);
      toon(woordVOOR, Colour);
      toon(woordHALF, Colour);
    }
    else if(minuut >= 25 && minuut <=29) {
      toon(woord5, Colour);
      toon(woordVOOR, Colour);
      toon(woordHALF, Colour);
    }
    else if(minuut >= 30 && minuut <=34) {
      toon(woordHALF, Colour);
    } 
    else if(minuut >= 35 && minuut <=39) {
      toon(woord5, Colour);
      toon(woordOVER, Colour);
      toon(woordHALF, Colour);
    }
    else if(minuut >= 40 && minuut <=44) {
      toon(woord10, Colour);
      toon(woordOVER, Colour);
      toon(woordHALF, Colour);
    }
    else if(minuut >= 45 && minuut <=49) {
      toon(woordKWART, Colour);
      toon(woordVOOR, Colour);
    }
    else if(minuut >= 50 && minuut <=54) {
      toon(woord10, Colour);
      toon(woordVOOR, Colour);
    }
    else if(minuut >= 55 && minuut <=59) {
      toon(woord5, Colour);
      toon(woordVOOR, Colour);
    }   
  
    if(minuut%5 == 1) {
    toon(wmin1, Colour);
    }
    else if(minuut%5 == 2) {
    toon(wmin1, Colour);
    toon(wmin2, Colour);  
    }
    else if(minuut%5 == 3) {
    toon(wmin1, Colour);
    toon(wmin2, Colour);
    toon(wmin3, Colour);     
    }
    else if(minuut%5 == 4) {
    toon(wmin1, Colour);
    toon(wmin2, Colour);
    toon(wmin3, Colour);
    toon(wmin4, Colour);      
    }           
  }

  if(seconde != seconde2){
//    Serial.print(uur, DEC);
//    Serial.print(':');
//    Serial.print(minuut, DEC);
//    Serial.print(':');
//    Serial.print(seconde, DEC);
//
//    Serial.print("  ");
//    Serial.print(dag);
//    Serial.print(".");
//    Serial.print(maand);
//    Serial.print(".");
//    Serial.print(jaar); 
//    Serial.println(); 
//    Serial.println(); 
    
    seconde2 = seconde;
    sync++;
  }

  if(sync >= sync_interval){
    setSyncProvider(getNtpTime);
    sync = 0;    
  }


//laad led-array
  FastLED.show();

  delay(100);

    
}

void toon(int Woord[], uint32_t Kleur) {
  for (int x = 0; x < NUM_LEDS + 1; x++) {
    if(Woord[x] == -1) {
      break;
    } 
    else {
      if(kleurtjes == 1){
        leds[Woord[x]] = CHSV(hue, 255, 255);
         if(teller >= cycle_snelheid){
          hue++;
          teller = 0;
         }
        teller++;  
      }
      else{  
      leds[Woord[x]] = Kleur;
      }
    }
  }
}

void blank() {
//clear all
for (int x = 0; x < NUM_LEDS; ++x) {
  leds[x] = CRGB::Black;
}
//  FastLED.show();

}

//test-animatie
void test() {
  blank();
  fill_rainbow( leds, NUM_LEDS, 0, 5);
  FastLED.show();
  delay(5000);
  blank();
  uint32_t Colour_demo = CRGB::White;
  toon(woordHET, Colour_demo);
  delay(100); FastLED.show();
  toon(woordIS, Colour_demo);
  delay(100); FastLED.show();
  toon(woord5, Colour_demo);
  delay(100); FastLED.show();
  toon(woord10, Colour_demo);
  delay(100); FastLED.show();
  toon(woordKWART, Colour_demo);
  delay(100); FastLED.show();
  toon(woordOVER, Colour_demo);
  delay(100); FastLED.show();
  toon(woordVOOR, Colour_demo);
  delay(100); FastLED.show();
  toon(woordHALF, Colour_demo);
  delay(100); FastLED.show();
  toon(w1, Colour_demo);
  delay(100); FastLED.show();
  toon(w2, Colour_demo);
  delay(100); FastLED.show();
  toon(w3, Colour_demo);
  delay(100); FastLED.show();
  toon(w4, Colour_demo);
  delay(100); FastLED.show();
  toon(w5, Colour_demo);
  delay(100); FastLED.show();
  toon(w6, Colour_demo);
  delay(100); FastLED.show();
  toon(w7, Colour_demo);
  delay(100); FastLED.show();
  toon(w8, Colour_demo);
  delay(100); FastLED.show();
  toon(w9, Colour_demo);
  delay(100); FastLED.show();
  toon(w10, Colour_demo);
  delay(100); FastLED.show();
  toon(w11, Colour_demo);
  delay(100); FastLED.show();
  toon(w12, Colour_demo);
  delay(100); FastLED.show();
  toon(woordUUR, Colour_demo);
  delay(100); FastLED.show();
  toon(wmin1, Colour_demo);
  delay(100); FastLED.show();
  toon(wmin2, Colour_demo);
  delay(100); FastLED.show();
  toon(wmin3, Colour_demo);
  delay(100); FastLED.show();
  toon(wmin4, Colour_demo);
  delay(100); FastLED.show();  
}


//-------- NTP code ----------

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

//webserver
void handleRoot() {

  if(server.arg("c") != ""){
    Colour_hex = server.arg("c");
    Colour = StrToHex(Colour_hex.c_str());
  }
  
  if(server.arg("mode") == "1"){
    TIME_MODE = 1;
  }
  else if(server.arg("mode") == "0"){
    TIME_MODE = 0;
  }

  if(server.arg("cmode") == "true"){
    if(TIME_MODE == 1){
      TIME_MODE = 0;
    }
    else if(TIME_MODE == 0){
      TIME_MODE = 1;
    }
  }

  if(server.arg("crotate") == "true"){
    if(kleurtjes == 1){
      kleurtjes = 0;
    }
    else if(kleurtjes == 0){
      kleurtjes = 1;
    }
  }

  if(server.arg("cAB") == "true"){
    if(autobright == true){
      autobright = false;
    }
    else if(autobright == false){
      autobright = true;
    }
  }

  char* AB = "aan";
  char showslider[100]; 

  if(autobright == false){
    AB = "uit";
     
    snprintf(showslider, 100, "<input id='Bvalue' type='range' min='0' max='255' step='5' value='%d'>", brightness);
  }

  char* modus = "0: Tijd + losse minuten";
  if(TIME_MODE == 1){
    modus = "1: Tijd - losse minuten";
  }  

  char* c_rotate = "Vaste kleur";
  if(kleurtjes == 1){
    c_rotate = "Color cycle";
  } 

  char temp[3000];

  snprintf ( temp, 3000,

"<!DOCTYPE html PUBLIC '-//WAPFORUM//DTD XHTML Mobile 1.0//EN' 'http://www.wapforum.org/DTD/xhtml-mobile10.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml'>\n\
<head>\n\
 <title>Woordklok</title>\n\
    <style>\n\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\n\
    </style>\n\
  <script src='http://192.168.2.9:8000/iro.min.js'></script>\n\  
<script>\n\
function onColorChange(color) {\n\
  var colorhex = color.hexString.substring(1);\n\
  if (colorhex.length==3){colorhex=colorhex[0]+colorhex[0]+colorhex[1]+colorhex[1]+colorhex[2]+colorhex[2]};\n\  
  update(colorhex);\n\
}\n\
\n\
function update(jscolor) {\n\
var request  = new XMLHttpRequest(  );\n\
var params = encodeURI(\"c=\"+jscolor);\n\
request.open(\"GET\", \"color?\" + params);\n\
request.send(\"\");\n\
}\n\
function load(url) {\n\
var request  = new XMLHttpRequest(  );\n\
request.open(\"GET\", url);\n\
request.send(\"\");\n\
}\n\
</script> \n\   
</head>\n\
<body>\n\
    <h1>Woordklok</h1>\n\    
    <h2>Huidige gegevens</h2>\n\
      <table border='0'>\n\
    <tr><td><b>Tijd:</b></td><td>%02d:%02d:%02d</td></tr>\n\
    <tr><td><b>Datum:</b></td><td>%02d/%02d/%04d</td></tr>\n\
    <tr><td><b>Weergavemodus:</b></td><td>%s (<a href='?cmode=true'>Verander</a>)</td></tr>\n\
    <tr><td><b>Kleur:</b></td><td>%s (<a href='?crotate=true'>Verander</a>)</td></tr>\n\
    <tr><td><b>Brightness:</b></td><td><span id='Boutput'>%d </span>%s</td></tr>\n\
    <tr><td><b>Auto Brightness:</b></td><td>%s (<a href='?cAB=true'>Verander</a>)</td></tr>\n\
    <tr><td><b>NTP-server:</b></td><td>%s</td></tr>\n\
    <tr><td><a href='/restart'>Restart</a></td><td></td></tr>\n\   
    </table>\n\
  <h2>Demo</h2>\n\
  <p><a href='#' onclick='load(\"demo\");return false;'>Demo</a></p>\n\
  <h2>Kleur</h2>\n\
        <p>\n\
        <a href='#' onclick='load(\"?c=FFFFFF\");return false;'>Wit</a>\n\            
<div id=\"colorpick\"></div></p>\n\
<script>\n\
var CP = new iro.ColorPicker(\"#colorpick\", {\n\
  width: 320,\n\
  height: 320,\n\
  color: \"#%6s\",\n\
  markerRadius: 8,\n\
  padding: 4,\n\
  sliderMargin: 24,\n\
  borderWidth: 2,\n\
  borderColor: \"#000\",\n\
  });\n\
  CP.on(\"color:change\", onColorChange); \n\
var slider = document.getElementById(\"Bvalue\");\n\
var output = document.getElementById(\"Boutput\");\n\
output.innerHTML = slider.value;\n\
\n\
slider.oninput = function() {\n\
  output.innerHTML = this.value;\n\
} \n\
slider.onchange = function() {\n\
  load(\"brightness?B=\"+ this.value);\n\
}\n\
</script>\n\
</body>\n\
</html>",

    uur, minuut % 60, seconde % 60, dag, maand, jaar, modus, c_rotate, brightness, showslider, AB, ntpServerName, Colour_hex.c_str()
  );
  server.send ( 200, "text/html",  temp);
  
  if(server.arg("demo") == "1"){
    test();
    FastLED.delay(1000);
  }

}

void handleColor() {

  if(server.arg("c") != ""){
    Colour_hex = server.arg("c");
    Colour = StrToHex(Colour_hex.c_str());
  }
  
  server.send ( 200, "text/plain",  "OK");

}

void handleRestart() {
  server.send ( 200, "text/plain",  "OK; restart");
  ESP.restart();
}


void handleBrightness() {

  if(server.arg("B") != "" && autobright == false){
    brightness = server.arg("B").toInt();
  }
  
  server.send ( 200, "text/plain",  "OK");

}

void handleDemo() {
  test();
  FastLED.delay(1000);
  server.send ( 200, "text/plain",  "OK");
}


void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

uint32_t StrToHex(const char* str)
{
  return (uint32_t) strtol(str, 0, 16);
}
