#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Stream.h>
#include <Wire.h>

//********************AWS Headers********************//
//AWS
#include "sha256.h"
#include "Utils.h"

//WEBSockets
#include <Hash.h>
#include <WebSocketsClient.h>

//MQTT PAHO
#include <SPI.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>

//AWS MQTT Websocket
#include "Client.h"
#include "AWSWebSocketClient.h"
#include "CircularByteBuffer.h"
//********************AWS Headers********************//


// ESP8266 Wifi Params
char wifi_ssid[]       = "Indoor";
char wifi_password[]   = "indoorpos";
ESP8266WiFiMulti WiFiMulti;

// AWS IOT Connectivity Params, Reference (1)
char aws_endpoint[]    = "a2pv8pigeo9wbh.iot.us-east-2.amazonaws.com";
char aws_key[]         = "AKIAJBDBZHG3VXU5TLQQ";
char aws_secret[]      = "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";
char aws_region[]      = "us-east-2";
int port = 443;
AWSWebSocketClient awsWSclient(1000);
IPStack ipstack(awsWSclient);
long connection = 0;
char* generateClientID () {
  char* cID = new char[23]();
  for (int i=0; i<22; i+=1)
    cID[i]=(char)random(1, 256);
  return cID;
}
int arrivedcount = 0;

// Collection of Topics
char *subscribeTopic[2] = {
  "im_good",
  "fall_detected"
};  //when adding topics, increase maxMQTTMessageHandlers
char currentTopic[sizeof(subscribeTopic[0])];

// MQTT Packet Size allocation
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 2;     // change when increasing # of topics
MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers> *client = NULL;

//********************Heart Rate Configs********************//
#define USE_ARDUINO_INTERRUPTS false
#include <PulseSensorPlayground.h>

const int OUTPUT_TYPE = SERIAL_PLOTTER;

const int PULSE_INPUT = 14;
const int PULSE_BLINK = 12;    // Pin 13 is the on-board LED
const int PULSE_FADE = 5;
const int THRESHOLD = 550;   // Adjust this number to avoid noise when idle

byte samplesUntilReport;
const byte SAMPLES_PER_SERIAL_SAMPLE = 20;
PulseSensorPlayground pulseSensor;
int myBPM;
int BPM_THRESHOLD = 100;
float ACCELERATION_THRESHOLD = 6.5;
float ROTATION_THRESHOLD = 1000.00;
//********************Heart Rate Configs********************//
float curr_accel;
float curr_rot;
boolean buzzer_flag;
boolean fall_detected;
boolean OTA_flag;
float startTime, endTime, time_diff;

void setup() {
  pinMode(2, OUTPUT);
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

  // AWS Configuration    
  awsWSclient.setAWSRegion(aws_region);
  awsWSclient.setAWSDomain(aws_endpoint);
  awsWSclient.setAWSKeyID(aws_key);
  awsWSclient.setAWSSecretKey(aws_secret);
  awsWSclient.setUseSSL(true);

  OTASetupProcedure();


  //************** MPU Setup **************//
  Wire.begin();
  setupMPU();

  // Configure the PulseSensor manager.
  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.blinkOnPulse(PULSE_BLINK);
  pulseSensor.fadeOnPulse(PULSE_FADE);

  pulseSensor.setSerial(Serial);
  //pulseSensor.setOutputType(OUTPUT_TYPE);
  pulseSensor.setThreshold(THRESHOLD);

  // Skip the first SAMPLES_PER_SERIAL_SAMPLE in the loop().
  samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

  // Now that everything is ready, start reading the PulseSensor signal.
  if (!pulseSensor.begin()) {
  }

  buzzer_flag = false;
  fall_detected = false;
  OTA_flag = false;
  //startTime = millis();
}

void loop() {

  if (OTA_flag == false) {
    if (awsWSclient.connected()) {      // keep the mqtt up and running
      digitalWrite(2, LOW);    
      if (fall_detected == false) {
        MPU_record();
        curr_accel = get_accelMag_MPU();
        curr_rot = get_rotMag_MPU();
        //endTime = millis();
        //time_diff = (endTime - startTime) / 1000;
        Serial.print("curr_accel(g): "); Serial.println(curr_accel);
        Serial.print("curr_rot(o/s): "); Serial.println(curr_rot);
        //printData();
        if (curr_accel >= ACCELERATION_THRESHOLD)
        {
          if (curr_rot >= ROTATION_THRESHOLD) {
            Serial.println("MOOOOOOOOOOOOOOOOOOOOOOOOOOOOM FELLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"); 
            sendmessage(1);
            buzzer_flag = true;
            fall_detected = true;
          }
        }
      }
      else {
        // check for im good 
        client->yield();
        subscribe(); 
      }
      
      if (buzzer_flag == true) {
        digitalWrite(12, LOW);
      } else {
        digitalWrite(12, HIGH);
      }
    }
    else {                              // handle reconnection
      digitalWrite(2, HIGH);
      connect ();
    }
  }
  else {
    Serial.println("IN PROGRAM MODE");
    digitalWrite(2, LOW);
    ArduinoOTA.handle();
  }
}

//*************************************** Helper Functions ***************************************//

//subscription to MQTT topics
void subscribe () {
  for (int i=0; i<2; i++) {  
    int rc = client->subscribe(subscribeTopic[i], MQTT::QOS0, messageArrived);
  }  
}

//send a message to a mqtt topic
void sendmessage (int answer_to_life) {
    //send a message
    MQTT::Message message;
    char buf[100];
    sprintf(buf, "%i", answer_to_life);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client->publish("upload_topic", message); 
}

void handle_incoming_message(char* incoming_msg) {
    int my_msg = atoi(incoming_msg);    
    if (my_msg == 1) {
      Serial.println("TURNING OFF BUZZ BUZZ");
      digitalWrite(12, HIGH);
      buzzer_flag = false;
      fall_detected = false;
    }
}

void OTASetupProcedure() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  //ArduinoOTA.setPassword((const char *)"IPS");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


