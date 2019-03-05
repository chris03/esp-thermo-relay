#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <SI7021.h>

#include "debug.h"

#define SENSOR_SCL D5 // GPIO14
#define SENSOR_SDA D6 // GPIO12
#define RELAY_PIN D7  // GPIO13

const int MinTemp = 20;
const int MaxTemp = 26;
const int CheckIntervalSeconds = 10;

ESP8266WebServer webServer(80);
SI7021 sensor;

unsigned long nextCheck = 0;
bool isRelayOn = false;

void relayOn()
{
  DEBUGln("Set relay pin LOW/ON");
  digitalWrite(RELAY_PIN, LOW); // ON
  isRelayOn = true;
}

void relayOff()
{
  DEBUGln("Set relay pin HIGH/OFF");
  digitalWrite(RELAY_PIN, HIGH); // OFF
  isRelayOn = false;
}

void toggleRelay()
{
  if (isRelayOn)
  {
    relayOff();
  }
  else
  {
    relayOn();
  }
}

void setup()
{
  SERIALBEGIN();

  DEBUGln("Starting wifi...");
  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  wifiManager.autoConnect();

  DEBUG("Init sensor: ");
  bool sensorOk = sensor.begin(SENSOR_SDA, SENSOR_SCL);
  DEBUGln(sensorOk ? "OK" : "FAIL");

  // Init relay pin
  pinMode(RELAY_PIN, OUTPUT);
  relayOff();

  // Configure web server
  webServer.on("/", []() {
    const int temp = sensor.getCelsiusHundredths();
    const int hum = sensor.getHumidityPercent();
    const String jsonData = String("{") +
                            F("\"temp:\": ") + String(temp) +
                            F(", \"hum:\": ") + String(hum) +
                            F(", \"status:\": ") + String(isRelayOn) +
                            F("}");

    webServer.send(200, "application/json; charset=utf-8", jsonData);
  });

  webServer.on("/on", []() {
    relayOn();

    const String jsonData = String("{") +
                            F("\"status:\": ") + String(isRelayOn) +
                            F("}");
    webServer.send(200, "application/json; charset=utf-8", jsonData);
  });

  webServer.on("/off", []() {
    relayOff();

    const String jsonData = String("{") +
                            F("\"status:\": ") + String(isRelayOn) +
                            F("}");
    webServer.send(200, "application/json; charset=utf-8", jsonData);
  });
}

void loop()
{
  webServer.handleClient();

  const unsigned long now = millis();

  if (nextCheck <= now)
  {
    const int temp = sensor.getCelsiusHundredths();
    DEBUGln("Temp:" + String(temp));

    if (temp <= MinTemp)
    {
      relayOn();
    }
    else if (temp >= MaxTemp)
    {
      relayOff();
    }

    nextCheck = now + CheckIntervalSeconds * 1000;
  }
}
