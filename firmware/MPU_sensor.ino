
/* Reference: 
        https://github.com/VRomanov89/EEEnthusiast         
*/

#include <Wire.h>
#include <Arduino.h>

float ACCEL_THRESHOLD = 100;
float ROT_THRESHOLD = 100;

long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;

long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;

float prev_gForcex, prev_gForcey, prev_gForcez;
float prev_rotX, prev_rotY, prev_rotZ;

float delta_gForcex, delta_gForcey, delta_gForcez; 
float delta_rotX, delta_rotY, delta_rotZ;
float accel_mag, rot_mag;


void setupMPU(){
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4) 
  Wire.write(0x00000001); //Setting the gyro to full scale +/- 250deg./s 
  Wire.endTransmission(); 
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00000001); //Setting the accel to +/- 2g
  Wire.endTransmission(); 
}

void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Accel Registers (3B - 40)
  while(Wire.available() < 6);
  accelX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processAccelData();
}

void processAccelData(){
  gForceX = accelX / 8192.0;
  gForceY = accelY / 8192.0; 
  gForceZ = accelZ / 8192.0;
}

void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x43); //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Gyro Registers (43 - 48)
  while(Wire.available() < 6);
  gyroX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  gyroY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  gyroZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processGyroData();
}

void processGyroData() {
  rotX = gyroX / 65.5;
  rotY = gyroY / 65.5; 
  rotZ = gyroZ / 65.5;
}

void printData() {
  Serial.print("Gyro (deg)");
  Serial.print(" X=");
  Serial.print(rotX);
  Serial.print(" Y=");
  Serial.print(rotY);
  Serial.print(" Z=");
  Serial.print(rotZ);
  Serial.print(" Accel (g)");
  Serial.print(" X=");
  Serial.print(gForceX);
  Serial.print(" Y=");
  Serial.print(gForceY);
  Serial.print(" Z=");
  Serial.println(gForceZ);
}

void MPU_record() {
  recordAccelRegisters();
  recordGyroRegisters();
  prev_gForcex = gForceX;
  prev_gForcey = gForceY;
  prev_gForcez = gForceZ;
  prev_rotX = rotX;
  prev_rotY = rotY;
  prev_rotZ = rotZ;

  delay(10);

  recordAccelRegisters();
  recordGyroRegisters();

  delta_gForcex = gForceX - prev_gForcex;
  delta_gForcey = gForceY - prev_gForcey;
  delta_gForcez = gForceZ - prev_gForcez;
  delta_rotX = rotX - prev_rotX;
  delta_rotY = rotY - prev_rotY;
  delta_rotZ = rotZ - prev_rotZ;
}

float get_accelMag_MPU() {
  return sqrt(delta_gForcex*delta_gForcex + delta_gForcey*delta_gForcey + delta_gForcez*delta_gForcez);
}

float get_rotMag_MPU() {
  return sqrt(delta_rotX*delta_rotX + delta_rotY*delta_rotY + delta_rotZ*delta_rotZ);
}


