// Pulse Monitor Test Script
int ledPin = 12;
int sensorPin = 14;
double alpha = 0.75;
int period = 20;
double change = 0.0;
void setup ()
{
pinMode (ledPin, OUTPUT);
Serial.begin (9600);
}void loop ()
{
 digitalWrite(ledPin, HIGH);
static double oldValue = 0;
static double oldChange = 0;
int rawValue = analogRead (sensorPin);
double value = alpha * oldValue + (1 - alpha) * rawValue;
Serial.print ((((value*value)-100000)-942000)/10);
Serial.println ((((value*value)-100000)-942000)/10);
oldValue = value;
delay (period);
}
