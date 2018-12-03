
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

char wifi_ssid[]       = "Indoor";
char wifi_password[]   = "indoorpos";
ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin (115200);
  delay (2000);
  Serial.setDebugOutput(1);

  WiFiMulti.addAP(wifi_ssid, wifi_password);
  Serial.println ("connecting to wifi");
  while(WiFiMulti.run() != WL_CONNECTED) {
      delay(100);
      Serial.print (".");
  }
  Serial.println ("\nconnected");

}

void loop() {
  // put your main code here, to run repeatedly:

}
