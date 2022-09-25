/*
ESPPRTG

An PRTG Alarm light based on a WEMOS D1 Mini. 
See project at https://blog.boerzel.de/de/blog/esp-prtg-alarm-leuchte
Graps Alarms via the Paesler PRTG API and animates Neopixels.

Source: https://github.com/bboerzel/espprtg

Author: Benjamin Börzel 
Twitter: @boerzel
version: 0.1
last change: 2022.09.21

Changelog
v0.1
 Initial Script
*/
    
// import needed libraries
#include <ArduinoJson.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_NeoPixel.h>

// define settings
#define STASSID "MyWifiName"        // Wifi name
#define STAPSK  "password123"       // Wifi password
#define LED_PIN    2                // Pin for LEDs, Wemos D1 Mini, D4 = Pin 2
#define LED_COUNT 16                // How many Neopixels are you using? 2 Strips a 8 LEDs.
#define DEF_BRIGHT 50               // What is your default brightness? Value 0-255

// Settings for rgbBreathe function
int MinBrightness = 20;             // Minimal Brightness, value 0-255
int MaxBrightness = 100;            // Maximal Brightness, value 0-255
int fadeInWait = 20;                // lighting up speed, steps.
int fadeOutWait = 20;               // dimming speed, steps.

const char* ssid     = STASSID;
const char* password = STAPSK;
byte maxwait = 120;                 // maximal waiting time for wifi connection to establish
String mac = "";

// initiate Neopixel
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


void setup() {
    // Setup code that only runes once

    // initialize serial output  
    Serial.begin(9600);
    Serial.println("Setup"); 

    // initialize LEDs
    Serial.println("LED"); 
    strip.begin();              
    strip.show();               
    strip.setBrightness(DEF_BRIGHT);

    // initialize wifi connection
    Serial.println("Wifi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    if (waitWifi()) {
        // wait for wifi connection to be established
        // slow green animation        
        colorWipe(strip.Color( 0, 125, 0), 80);
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        mac += WiFi.macAddress();
        Serial.print("MAC: ");
        Serial.println (mac);
        Serial.print("Hostname: ");  
        Serial.println(WiFi.hostname());
    }else{
        // blink red
        theaterChase(strip.Color(127,   0,   0), 200);
        Serial.println("Wifi ERROR");
    }  
}

void loop() {
    // Main code
    if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

        // initialize https client
        std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
        client->setInsecure();
        HTTPClient https;

        Serial.print("[HTTPS] begin...\n");
        // call PRTG API and get result as JSON, set username and passhash (See PRTG documentation how to generate it)
        if (https.begin(*client, "https://myprtgsite.com/api/getstatus.htm?id=0&username=PRTGUSER&passhash=PRTGPASSHASH")) {  // Add here your PRTG Site, user and password

            Serial.print("[HTTPS] GET...\n");
            // start connection and send HTTP header
            int httpCode = https.GET();

            // httpCode will be negative on error
            if (httpCode > 0) {
                // HTTP header has been send and server response header has been handled
                Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

                // file found at server                
                if (httpCode == 200) {

                    // get the site content
                    String payload = https.getString();

                    // initialize JSON object
                    // Check out https://arduinojson.org/v6/assistant
                    DynamicJsonDocument doc(1536);
          
                    // deserialize JSON
                    DeserializationError error = deserializeJson(doc, payload);
                    if (error) {
                        Serial.print(F("deserializeJson() failed: "));
                        Serial.println(error.f_str());           
                    }

                    // define variable with json content of the alarm field
                    const char* Alarms = doc["Alarms"];

                    Serial.print("\nAlarms:" + String(Alarms));
          
                    if(String(Alarms) == ""){ // check if there is an alarm
                        Serial.println("no Alarm");
                        // everything is fine       
                        // fade slowly from a darker green to a brighter green
                        colorWipe(strip.Color( 0, 100, 0), 80); 
                        colorWipe(strip.Color( 0, 150, 0), 80); 
                        colorWipe(strip.Color( 0, 100, 0), 80); 
                        colorWipe(strip.Color( 0, 150, 0), 80); 
                        colorWipe(strip.Color( 0, 100, 0), 80); 
                        colorWipe(strip.Color( 0, 150, 0), 80); 
                        colorWipe(strip.Color( 0, 100, 0), 80); 
                        colorWipe(strip.Color( 0, 150, 0), 80);
                    }else{
                        Serial.println("!!! ALARM !!!");
                        // something is broken call startAlarm animation
                        // red blue police car blinking and red moving light
                        startAlarm(); 
                    }
                }
            } else {
                // http error e.g. login failed
                // blink red
                theaterChase(strip.Color(127,   0,   0), 200); 
                Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
            }

            https.end();
        } else {
            // connection problem
            // blink red
            theaterChase(strip.Color(127,   0,   0), 200);
            Serial.printf("[HTTPS] Unable to connect\n");
        }
    }
    Serial.println("Wait 10s before next round...");
    delay(10000);     
}

bool waitWifi() {
    // waiting for wifi connection
    while((WiFi.status() != WL_CONNECTED) && maxwait > 0) {
        // look till wifi is connected or maximal wait count is arrived
        Serial.println("Waiting for Wifi...");
        // wipe blue while waiting
        colorWipe(strip.Color( 0, 0, 125), 80); 
        colorWipe(strip.Color( 0, 0, 64), 80);       
        maxwait--;
    }
    if(WiFi.status() == WL_CONNECTED) return true;
    return false;
}


void startAlarm(){
    // LED animation for alarm
    // blink red blue like a us police car
    blinkAlarm(255,200);    
    blinkAlarm(255,200);
    
    // for 8 times, wipe from a darker red to a brighter red ad full brightness
    for(int i=0; i<8; i++) {
        strip.setBrightness(255);
        colorWipe(strip.Color(255, 0, 0), 50);
        colorWipe(strip.Color(125, 0, 0), 50); 
        strip.setBrightness(DEF_BRIGHT);
    }
}

void colorWipe(uint32_t color, int wait) {
    //wipe from one color to another
    for(int i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, color);        
        strip.show();                         
        delay(wait);                          
    }
}

void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {                         // Repeat 10 times...
    for(int b=0; b<3; b++) {                        //  'b' counts from 0 to 2...
      strip.clear();                                //   Set all pixels in RAM to 0 (off)
                                                    // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color);              // Set pixel 'c' to value 'color'
      }
      strip.show();                                 // Update strip with new contents
      delay(wait);                                  // Pause for a moment
    }
  }
}

void blink(uint32_t color,int bright, int wait){
    // blink once
    strip.setBrightness(bright);                    // set brightness
    for(int i=0;i<LED_COUNT;i++){                   // for each LED
        strip.setPixelColor(i, color);              // set coor 
        strip.show();                               // update strip   
    }              
    delay(wait);                                    // wait
    for(int i=0;i<LED_COUNT;i++){                   //for each LED
        strip.setPixelColor(i, strip.Color(0,0,0)); // turn LEDs off
        strip.show();                               // update strip 
    }              
    strip.setBrightness(DEF_BRIGHT);                // set brightness back to default
 }

 void rgbBreathe(uint32_t c, uint8_t x, uint8_t y) {
    // very nice breath like animation
    // thanks to kit-ho
    // https://github.com/kit-ho/NeoPixel-WS2812b-Strip-Breathing-Code-with-Arduino/blob/master/Breather_v1.1.ino
    for (int j = 0; j < x; j++) {
        for (uint8_t b = MinBrightness; b < MaxBrightness; b++) {
            strip.setBrightness(b * 255 / 255);
            for (uint16_t i = 0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, c);
            }
            strip.show();
            delay(fadeInWait);
        }
        strip.setBrightness(MaxBrightness * 255 / 255);
        for (uint16_t i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, c);
            strip.show();
            delay(y);
        }
        for (uint8_t b = MaxBrightness; b > MinBrightness; b--) {
            strip.setBrightness(b * 255 / 255);
            for (uint16_t i = 0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, c);
            }
            strip.show();
            delay(fadeOutWait);
        }
    }
 }

 void blinkAlarm(int bright, int wait){
     // blink like a us police car in red blue
    strip.setBrightness(bright);                        // set to brightnes
  
    for(int i=0;i<(LED_COUNT/2);i++){                   // for first strip
        strip.setPixelColor(i, strip.Color(255,0,0));   // set to red
        strip.show();                                   // update strip   
    }           
    for(int i=(LED_COUNT/2);i<LED_COUNT;i++){           // for second strip
        strip.setPixelColor(i, strip.Color(0,0,0));     // set off
        strip.show();                                   // update strip
    } 
    
    delay(wait);                                        // wait
    
    for(int i=0;i<(LED_COUNT/2);i++){                   // for first strip
        strip.setPixelColor(i, strip.Color(0,0,0));     // set off
        strip.show();                                   // update strip
    }          
    for(int i=(LED_COUNT/2);i<LED_COUNT;i++){           // for second strip
        strip.setPixelColor(i, strip.Color(0,0,255));   // set to blue
        strip.show();                                   // update strip
    } 
    delay(wait);                                        // wait

    strip.setBrightness(DEF_BRIGHT);                    // set brightnes back to default
 }
