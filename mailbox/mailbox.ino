
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Adafruit_SSD1306.h>
#include "credentials.h"

#define OLED_RESET 0

ESP8266WiFiMulti wifiMulti;

ESP8266WebServer server(80);

Adafruit_SSD1306 display(OLED_RESET);
Servo flagServo;

boolean flagUpFlag    = false;
const int flagUpPos   = 90;
const int flagDownPos = 10;

void flagUp() {
  if (flagUpFlag) {
    return;
  }
  for (int pos = flagDownPos; pos <= flagUpPos; pos += 2) {
    flagServo.write(pos);
    delay(15);
  }
  flagUpFlag = true;
}

void flagDown() {
  if (!flagUpFlag) {
    return;
  }
  for (int pos = flagUpPos; pos >= flagDownPos; pos -= 2) {
    flagServo.write(pos);
    delay(15);
  }
  flagUpFlag = false;
}

void handleRoot();              
void handleFlag();


void setup() {

  Serial.begin(115200);        
  delay(10);
  Serial.println('\n');

  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.clearDisplay();
  display.display();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Mailbox");

  display.display();
    
  flagServo.attach(5);

  flagServo.write(flagDownPos);
  flagUpFlag = false;
  delay(2000);

  wifiMulti.addAP(ssid1, password);
  wifiMulti.addAP(ssid2, password);
  
  display.setTextSize(1);
  display.print("Connecting .");
  display.display();
  
  while (wifiMulti.run() != WL_CONNECTED) { 
    delay(250);
    Serial.print('.');
    display.print('.');
    display.display();
  }

  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());        

  display.println("");
  display.print("Connected to ");
  display.println(WiFi.SSID()); 
  
  display.print("IP:");
  display.println(WiFi.localIP());
  display.display();
  
  if (MDNS.begin("mailbox")) {         
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);    
  server.on("/flag", HTTP_POST, handleFlag);  
  server.on("/flag", HTTP_PUT, [](){
    Serial.println(server.arg("plain"));
    StaticJsonDocument<200> doc;

    deserializeJson(doc,server.arg("plain"));
    if ((bool)doc["status"]) {
        flagUp();
    } else {
        flagDown();
    }
    server.send(200);
  });
  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Not found");
  });

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

void loop() {

    server.handleClient();
}


void handleRoot() {                         // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", "<form action=\"/flag\" method=\"POST\"><input type=\"submit\" value=\"Toggle Flag\"></form>");
}

void handleFlag() {                          // If a POST request is made to URI /LED
  if (flagUpFlag) {
    flagDown();
  } else {
    flagUp();
  }

  server.sendHeader("Location", "/");       // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}
