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

void setup() {
  pinMode(16, INPUT);
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

  //************** MPU Setup **************//
  Wire.begin();
  setupMPU();

}

void loop() {
  if (awsWSclient.connected()) {      // keep the mqtt up and running      
    // client->yield();
    // subscribe();
    if (digitalRead(16) == LOW) {
      Serial.println ("\nDetected Logic HIGH on PIN 16\n");
      digitalWrite(2, LOW);

      MPU_delta();
      delay(100);
      
      //sendmessage(42);
    }
    else {
      Serial.println("nothing");
      digitalWrite(2, HIGH);
    }
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
void sendmessage (float answer_to_life) {
    //send a message
    MQTT::Message message;
    char buf[100];
    sprintf(buf, "%f", answer_to_life);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client->publish("upload_topic", message); 
}
