#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
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
#define PUMP     2

Adafruit_PCD8544 display = Adafruit_PCD8544(SCLK,MOSI,DC,CS_LCD,RST_LCD);
Adafruit_MCP3008 adc;

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);

Ticker soilMoistureTicker;

const int AIR_VALUE   = 875;
const int WATER_VALUE = 463;

const int icon_wifi_pos_x = 0;  
const int icon_wifi_pos_y = 1; 
const int icon_data_pos_x = icon_wifi_pos_x;  
const int icon_data_pos_y = icon_wifi_pos_y + 22;  
const int data_pos_x = 30;

const char *description[] = { "saturated", 
                              "adequately wet", 
                              "irrigation advice", 
                              "irrigation", 
                              "dangerously dry"};

IPAddress ip;  

void setup()   {

    pinMode(PUMP, OUTPUT);     // pump pin to output
    digitalWrite(PUMP, LOW);   // pump off
    
    Serial.begin(115200);
    delay(10);
    Serial.println('\n');
    
    display.begin();
    display.setContrast(50);
    display.setFont(NULL);
    display.clearDisplay();
    display.display();

    //display.setTextSize(1);
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
    ip = WiFi.localIP();
    display.println("");

    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP  : ");
    Serial.println(ip);

    display.println(WiFi.SSID());
    display.println(ip);
    display.display();

    if (MDNS.begin("soilmoisture")) {
        Serial.println("mDNS responder started");
    } else {
        Serial.println("Error setting up MDNS responder!");
    }
    adc.begin(SCLK,MOSI,MISO,CS_MCP);

    displayMoisture();
    
    server.on("/soilmoisture", HTTP_GET, []() {

        StaticJsonDocument<200> doc;
        int moisture = getMoisture();
        doc["value"] = moisture;
        doc["description"] = getDescription(moisture);
        char buffer[512];
        serializeJson(doc, buffer);
        server.send(200, "application/json", buffer);
    });

    server.on("/pump", HTTP_PUT, []() {

        StaticJsonDocument<200> doc;

        deserializeJson(doc,server.arg("plain"));
        if (doc.containsKey("status")) {
            if ((bool)doc["status"]) {
                digitalWrite(PUMP, HIGH);  // pump on
            } else {
                digitalWrite(PUMP, LOW);  // pump off
            }
        } else if (doc.containsKey("duration")) {
            digitalWrite(PUMP, HIGH);  // pump on
            delay(doc["duration"]);
            digitalWrite(PUMP, LOW);  // pump off
        }
        server.send(200);
    });
    
    server.onNotFound([]() {
        server.send(404, "text/plain", "404: Not found");
    });

    server.begin();
    Serial.println("HTTP server started");

    
    soilMoistureTicker.attach(120, displayMoisture);    
}


void loop() {
    server.handleClient();
}

void displayMoisture() {
    int moisture =  getMoisture();
    display.clearDisplay();
    
    display.setFont(&FreeSans9pt7b);
    display.setCursor(0,15);
    display.setTextSize(1);
    display.print("Dry: ");
    display.print(moisture);
    display.print("%");
    //display.drawBitmap(icon_data_pos_x,icon_data_pos_y, humidity_icon_bmp, humidity_icon_width, humidity_icon_height, WHITE); 
    display.setFont();
    display.println();
    display.print(ip);
    display.display();  
}

/**
 * Get a text vor the moisture
 * @param moisture the relative moisture in percent
 */
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

/**
 * Calculates the moisture by reading some values and using the median to reduce the possiblity of inaccurate readings 
 */
int getMoisture() {

    int moistureArray[5];
    int moistureArrayLenght = sizeof(moistureArray)/sizeof(int);
    // read the values
    for (int i = 0; i < moistureArrayLenght; i++) {
        moistureArray[i] = adc.readADC(0);
        delay(50);
    }
    
    // sort the array, a simple bubblesort is enougth for this amount of samples
    int tmp;
    for (int i = 0; i < moistureArrayLenght - 1; i++) {
        for (int j = i + 1; j < moistureArrayLenght; j++) {
            if (moistureArray[i] > moistureArray[j]) {
                tmp = moistureArray[i];
                moistureArray[i] = moistureArray[j];
                moistureArray[j] = tmp; 
            }
        }
    }
    // the element in the middle of the array is the median
    int value = max(moistureArray[moistureArrayLenght >> 1], WATER_VALUE);
    value = min(value, AIR_VALUE);

    value = (value - WATER_VALUE) * 100 / (AIR_VALUE - WATER_VALUE);
    // return the moisture in [%]
    return value;
}
