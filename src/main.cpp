#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ESP8266Wifi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define ss 15 // GPIO15 / D8
#define rst 5 // GPIO5 / D1
#define dio0 4 // GPIO4 / D2 (INTERRUP)

#define DHTTYPE DHT11
const uint8_t DHTpin = 2; // GPIO2 / D4
DHT dht(DHTpin, DHTTYPE);

float temperature;
float humidity;
String data = "";

const char *ssid =  "MiFibra-F2FE";
const char *pass =  "G4hp7ngJ";
const char *mqttServer = "raspberry.lan";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// put function declarations here:
void onReceive(int);
void connectMqtt();

void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.println("");

    Serial.println("LoRa init");
    LoRa.setPins(ss, rst, dio0);

    if (!LoRa.begin(433E6)) {
      Serial.println("LoRa Start failed");
      while (1);
    }

    LoRa.onReceive(onReceive);
    LoRa.receive();
    Serial.println("Receiving...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    Serial.println("");
    Serial.println("WiFi connected"); 
    Serial.print("AP:\t");
    Serial.println(WiFi.BSSIDstr());
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address:\t");
    Serial.println(WiFi.macAddress());

    Serial.println("Inicializando...");
    mqttClient.setServer(mqttServer, 1883);
    pinMode(DHTpin, INPUT_PULLUP);
    dht.begin();
}

void loop() {
    if (!mqttClient.connected()) {
        connectMqtt();
    }

    delay(3000);
    mqttClient.loop();

    temperature = dht.readTemperature(false);
    humidity = dht.readHumidity();

    Serial.print("Tmp: ");
    Serial.println(temperature);
    Serial.print("Hum: ");
    Serial.println(humidity);

    data = "";
    data = WiFi.macAddress();
    data.concat(";T:");
    data.concat(temperature);
    data.concat(";H:");
    data.concat(humidity);
    Serial.print("Sending data: ");
    Serial.println(data);

    mqttClient.publish("casa/temperature", data.c_str());

    Serial.println("");
    Serial.println("---------------");
}

void onReceive(int packetSize) {
    Serial.print("Received LoRa packet: ");
    if (!mqttClient.connected()) {
            connectMqtt();
        }
    String dataLoRa = LoRa.readString();
    String debugData = "";
    debugData.concat(dataLoRa);
    debugData.concat(" | RSSI: ");
    debugData.concat(LoRa.packetRssi());
    Serial.println(debugData);
    mqttClient.publish("casa/LoRa", debugData.c_str());

    if (dataLoRa.length() == 35) {
        Serial.print("Sending data: ");
        Serial.println(dataLoRa);
        mqttClient.publish("casa/temperature", dataLoRa.c_str());
    } else {
        Serial.println("Invalid data size");
    }
    
}

void connectMqtt() {
    while (!mqttClient.connected()) {
        Serial.println("Connecting to MQTT broker...");
        if (mqttClient.connect(WiFi.macAddress().c_str())) {
            Serial.println("Connected, sending hello");
            data = "";
            data.concat(WiFi.macAddress());
            data.concat(":HELLO");
            mqttClient.publish("casa/hello", data.c_str());
        } else {
            Serial.println("Connection failed, retrying in 5 seconds");
            delay(5000);
        }
    }
}