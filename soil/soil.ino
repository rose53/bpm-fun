#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMono9pt7b.h>
#include "Adafruit_PCD8544.h"
#include <Adafruit_MCP3008.h>
#include <ArduinoJson.h>
#include "credentials.h"

#define SCLK    12
#define MOSI    13
#define MISO     5
#define DC       0
#define CS_LCD   4
#define CS_MCP  15
#define RST_LCD 16

Adafruit_PCD8544 display = Adafruit_PCD8544(SCLK,MOSI,DC,CS_LCD,RST_LCD);
Adafruit_MCP3008 adc;

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);

Ticker soilMoistureTicker;

const int AIR_VALUE   = 875;
const int WATER_VALUE = 463;

const char *description[] = { "saturated", 
                              "adequately wet", 
                              "irrigation advice", 
                              "irrigation", 
                              "dangerously dry"};

void setup()   {
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');

    display.begin();
    display.setContrast(90);
    display.clearDisplay();
    display.display();

    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(0, 0);
    display.println("Soil");

    display.display();

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
    display.println("");

    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP  : ");
    Serial.println(WiFi.localIP());

    display.println(WiFi.SSID());
    display.println(WiFi.localIP());
    display.display();

    if (MDNS.begin("soilmoisture")) {
        Serial.println("mDNS responder started");
    } else {
        Serial.println("Error setting up MDNS responder!");
    }
    adc.begin(SCLK,MOSI,MISO,CS_MCP);
    
    server.on("/soilmoisture", HTTP_GET, []() {

        StaticJsonDocument<200> doc;
        int moisture = getMoisture();
        doc["value"] = moisture;
        doc["description"] = getDescription(moisture);
        char buffer[512];
        serializeJson(doc, buffer);
        server.send(200, "application/json", buffer);
    });
    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Not found");
    });

    server.begin();
    Serial.println("HTTP server started");

    soilMoistureTicker.attach(30, readSoilMoisture);
    
}


void loop() {
    server.handleClient();
}

void readSoilMoisture() {
    Serial.println(adc.readADC(0));
}

const char* getDescription(int moisture) {
    const char* retVal = NULL;
    if (moisture <= 9) {
        retVal = description[0];
    } else if (moisture > 9 && moisture <= 19){
        retVal = description[1];
    } else if (moisture > 19 && moisture <= 59){
        retVal = description[2];
    } else if (moisture > 59 && moisture <= 99){
        retVal = description[3];
    } else {
        retVal = description[4];
    }
    return retVal;
}

int getMoisture() {

    int value = adc.readADC(0);

    value = max(value, WATER_VALUE);
    value = min(value, AIR_VALUE);

    value = (value - WATER_VALUE) * 100 / (AIR_VALUE - WATER_VALUE);

    return value;
}
