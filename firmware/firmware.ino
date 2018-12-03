/* Library References: 
        https://github.com/odelot/aws-mqtt-websockets         (1)

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Stream.h>

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


}

void loop() {
  if (awsWSclient.connected()) {      // keep the mqtt up and running      
    // client->yield();
    // subscribe();
    if (digitalRead(16) == LOW) {
      Serial.println ("\nDetected Logic HIGH on PIN 16\n");
      digitalWrite(2, LOW);
      sendmessage(42);
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
// Attribution Reference (1)
bool connect () {         // connects to websocket layer and mqtt layer
  if (client == NULL) {
    client = new MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers>(ipstack);
  } else {

    if (client->isConnected ()) {    
      client->disconnect ();
    }  
    delete client;
    client = new MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers>(ipstack);
  }
  
  //delay is not necessary... it just help us to get a "trustful" heap space value
  delay (1000);
  Serial.print (millis ());
  Serial.print (" - conn: ");
  Serial.print (++connection);
  Serial.print (" - (");
  Serial.print (ESP.getFreeHeap ());
  Serial.println (")");


  int rc = ipstack.connect(aws_endpoint, port);
  if (rc != 1)
  {
    Serial.println("error connection to the websocket server");
    return false;
  } else {
    Serial.println("websocket layer connected");
  }


  Serial.println("MQTT connecting");
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 3;
  char* clientID = generateClientID ();
  data.clientID.cstring = clientID;
  rc = client->connect(data);
  delete[] clientID;
  if (rc != 0)
  {
    Serial.print("error connection to MQTT server");
    Serial.println(rc);
    return false;
  }
  Serial.println("MQTT connected");
  digitalWrite(2, LOW);
  return true;
}

//subscription to MQTT topics
void subscribe () {
  for (int i=0; i<4; i++) {  
    int rc = client->subscribe(subscribeTopic[i], MQTT::QOS0, messageArrived);
  }  
}

//callback to handle mqtt messages
void messageArrived(MQTT::MessageData& md)
{
  MQTT::Message &message = md.message;
  
  int len = md.topicName.lenstring.len;
  memcpy(currentTopic, md.topicName.lenstring.data, (size_t)len);
  
  Serial.print("Message from ");
  Serial.print(currentTopic);
  Serial.print("\n arrived: qos ");
  Serial.print(message.qos);
  Serial.print(", retained ");
  Serial.print(message.retained);
  Serial.print(", dup ");
  Serial.print(message.dup);
  Serial.print(", packetid ");
  Serial.println(message.id);
  Serial.print("Payload ");
  char* msg = new char[message.payloadlen+1]();
  memcpy (msg,message.payload,message.payloadlen);
  Serial.println(msg);
  //sendmessage(msg);
  delete msg;
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