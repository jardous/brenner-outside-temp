// Replacing the Oil heater outside temperature wired thermomenter
// Using PmodPOT  from DIGILENT to fake the thermometer readings.
// Temperature is reported from Home Assisntant and translated to
// the variable resistance that the heater control unit reads.
// 
// Board used: Seeed Studio XIAO ESP32C3
//
// https://www.hackster.io/56149/using-the-pmod-dpot-with-arduino-uno-599aa5

#include <Arduino.h>

// Data measurements
/*
 Temp 
-21.6 218
-19.2 193
-16.7 167
-12.9	137
 -6.6	100
 -4.3	89
  1.3	66
 10.3	45
 13	  40
 14.2	38
 17.3	33
 20.2	29
 22.7	26
 24.6	24
 29.1	20
 39.5	13
 50    8

https://arachnoid.com/polysolve/

Mode: normal x,y analysis
Polynomial degree 4, 17 x,y data pairs.
Correlation coefficient = 0.9998828943211641
Standard error = 0.7459147587921936

Output form: C function:
*/
double regress(double x) {
  double terms[] = {
     7.1198043291249064e+001,
    -3.4765543064929418e+000,
     1.0305520205537699e-001,
    -1.9706202226396794e-003,
     1.5940374387768800e-005
};
  
  size_t csz = sizeof terms / sizeof *terms;
  
  double t = 1;
  double r = 0;
  for (int i = 0; i < csz;i++) {
    r += terms[i] * t;
    t *= x;
  }
  return r;
}
/*
Copyright (c) 2019, P. Lutus -- http://arachnoid.com. All Rights Reserved.
*/

#include <WiFi.h>
#include <ArduinoHA.h>
#include <SPI.h>
#include "secrets.h"

// https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/
#define PIN_CS   D2
// XIAO ESP32C3 SPI: D8 SCK, D10 MOSI, D9 MISO
#define PIN_SCK  D8
#define PIN_MISO D9
#define PIN_MOSI D10


WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);
HANumber outsideTemp("outside-temp", HABaseDeviceType::PrecisionP1);
HASensorNumber rssiSensor("rssi");


void onNumberCommand(HANumeric number, HANumber* sender)
{
    if (!number.isSet()) {
        // the reset command was send by Home Assistant
    } else {
        float numberFloat = number.toFloat();
        Serial.print("received: ");
        Serial.println(numberFloat);

        uint16_t val = round(regress(numberFloat));
        Serial.print("Setting to ");
        Serial.println(val);

        // begin transmission
        digitalWrite(PIN_CS, LOW); // activation of CS line
        delayMicroseconds(15);

        SPI.transfer(val);

        // end transmission
        digitalWrite(PIN_CS, HIGH); // deactivation of CS line 
    }

    sender->setState(number); // report the selected option back to the HA panel
}

void connect() {
    byte mac[6];

    Serial.print("checking wifi...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    WiFi.macAddress(mac);
    device.setUniqueId(mac, sizeof(mac));

    // set device's details (optional)
    device.setName("Oil Heater");
    device.setSoftwareVersion("1.0.0");

    // handle command from the HA panel
    outsideTemp.onCommand(onNumberCommand);

    // Optional configuration
    outsideTemp.setIcon("mdi:thermometer");
    outsideTemp.setName("Outside Temp");
    outsideTemp.setMin(-20);
    outsideTemp.setMax(50);
    outsideTemp.setStep(0.5f); // minimum step: 0.001f
    outsideTemp.setUnitOfMeasurement("°C");
    //outsideTemp.setMode(HANumber::ModeBox);
    outsideTemp.setMode(HANumber::ModeSlider);
    

    // You can set retain flag for the HA commands
    outsideTemp.setRetain(true);

    // You can also enable optimistic mode for the HASelect.
    // In this mode you won't need to report state back to the HA when commands are executed.
    // outsideTemp.setOptimistic(true);

    // reporting RSSI
    rssiSensor.setIcon("mdi:wifi");
    rssiSensor.setName("RSSI");
    rssiSensor.setUnitOfMeasurement("dB");

    mqtt.begin(BROKER_ADDR);
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // init the SPI interface
    SPI.begin(PIN_SCK, PIN_MOSI, PIN_MISO); // initialization of SPI interface
    SPI.setDataMode(SPI_MODE0); // configuration of SPI communication in mode 0
    SPI.setClockDivider(SPI_CLOCK_DIV16); // configuration of clock at 1MHz
    pinMode(PIN_CS, OUTPUT); //setting chip select as output

    connect();
}


unsigned long lastUpdateAt = 0;

void loop() {
    //client.loop();

    mqtt.loop();

    //sleep(500);

    if (!client.connected()) {
        Serial.println("reconnecting ");
        connect();
    }

    if ((millis() - lastUpdateAt) > 10000) { // update in 10s interval
        unsigned long uptimeValue = millis() / 1000;
        rssiSensor.setValue(WiFi.RSSI());
        lastUpdateAt = millis();
    }
}
