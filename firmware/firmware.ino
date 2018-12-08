#include <Arduino.h>
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
char *subscribeTopic[1] = {
  "user_location"
};  //when adding topics, increase maxMQTTMessageHandlers
char currentTopic[sizeof(subscribeTopic[0])];

// MQTT Packet Size allocation
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;     // change when increasing # of topics
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
const byte SAMPLES_PER_SERIAL_SAMPLE = 10;
PulseSensorPlayground pulseSensor;
int myBPM;
int BPM_THRESHOLD = 90;
//********************Heart Rate Configs********************//

void setup() {
  pinMode(16, INPUT);
  pinMode(2, OUTPUT);
  pinMode(14, INPUT);
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  
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
  
}

void loop() {
  if (awsWSclient.connected()) {      // keep the mqtt up and running      
    // client->yield();
    // subscribe();
    
    myBPM = pulseSensor.getBeatsPerMinute();
    if (pulseSensor.sawNewSample()) {
      if (--samplesUntilReport == (byte) 0) {
        samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;
  
        pulseSensor.outputSample();
        if (pulseSensor.sawStartOfBeat()) {
          pulseSensor.outputBeat();
        }
      }
      if (myBPM >= BPM_THRESHOLD) {
        sendmessage(myBPM);
      }
    }
    
//    MPU_delta();
//    delay(100);  
  } 
  else {                              // handle reconnection
    connect ();
  }
}

//*************************************** Helper Functions ***************************************//

//subscription to MQTT topics
void subscribe () {
  for (int i=0; i<4; i++) {  
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
