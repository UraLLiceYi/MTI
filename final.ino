//Parts of this coding is modified from "Capacitive touch" by Karen and "About MPU-6050" by Vivek 
//Link:https://github.com/karenanndonnachie/MAKETHINGSINTERACTIVE_SEM1_2023/tree/main/Week3
//Link:https://electrosome.com/interfacing-mpu-6050-gy-521-arduino-uno/

#include <CapacitiveSensor.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h> 
#include <math.h> 
#include <FastLED.h> 

// create an instance of the library
// pin 8 sends electrical energy, pin 7 senses senses a change
CapacitiveSensor capSensor = CapacitiveSensor(8, 7);
int threshold = 1100;// threshold for turning the lamp on

#define ringPIN 9 //pin for control LED RGB ring
#define NUMPIXELS 24 //number of LEDs on the ring
Adafruit_NeoPixel pixels(NUMPIXELS, ringPIN,NEO_GRB + NEO_KHZ800);

const int MPU=0x68; //I2C address of the MPU-6050
int16_t AcX,AcY,AcZ; //16-bit integers for receiving the data from gyro
int balanceTreshold = 10;// the tilted angle for balance state 

int breathTime = 120;
double pitch,roll;
bool rest = 0; //default the rest flag to 0
bool flag;
int tiltedAngle, ledId;
int i,ledi; //declear varibles for loops and data
long v;


void setup() {
// open a serial connection(for monitering and debuging)
Serial.begin(9600);
pixels.begin();
Wire.begin(); //initiate wire library and I2C
Wire.beginTransmission(MPU); //begin transmission to I2C slave device
Wire.write(0x6B); // PWR_MGMT_1 register
Wire.write(0); // set to zero (wakes up the MPU-6050)  
Wire.endTransmission(true); //ends transmission to I2C slave device
}

void loop() {
// store the value reported by the sensor in a variable
long sensorValue = capSensor.capacitiveSensor(30);
// if the value is greater than the threshold
if (sensorValue > threshold) {
rest = 1;//change the state of rest flag

//if the bowl is not balanced
if(checkBalance() == 1){
  balance();
}
//if the bowl is balanced
else{
  breath(10500); //+timer
}
}
// if it's lower than the threshold
else {
//check if the bowl is just been used
if(rest == 1){
  bye();
}
}
delay(10);
}

//set the ring in one color
void setAllColor(long c)
{
  for(int i =0; i < 24; i++)
  {
    pixels.setPixelColor(i,pixels.gamma32(pixels.ColorHSV(c)));
  }
}


void breath(long c){
pixels.setBrightness(0);
pixels.show();

int good = 0;
flag = 1;
while(flag == 1){
if(good < 3){
  pixels.setBrightness(0);
  pixels.show();
    for(i = 0; i < 20; i++)
  {
    setAllColor(c);
    pixels.setBrightness(i);
    pixels.show();
    delay(breathTime); 

    long v = capSensor.capacitiveSensor(30);
    //break the loop if the bowl is not being touched
    if (v < threshold){
      flag = 0; //exit the while loop
      break;
    }
    //break if the bowl is unbalanced
    if(checkBalance() == 1){
      trans();
      flag = 0;
      break;
    }
  }
  for(i = 20; i > 0; i--)
  {
    setAllColor(c);
    pixels.setBrightness(i);
    pixels.show();
    delay(breathTime); 

    long v = capSensor.capacitiveSensor(30);
    if (v < threshold){
      break;
    }
    if(checkBalance() == 1){
      trans();
      flag = 0;
      break;
    }
  }
  good++;
}
else{
  rainbow();
  flag = 0;
}
}  
}

//calculate the tilted angle base on the pitch and roll, retuen a int degree
int getAngle(int Ax,int Ay,int Az) 
{
    double x = Ax;
    double y = Ay;
    double z = Az;
    int angle;

    pitch = atan(x/sqrt((y*y) + (z*z))); //pitch calculation
    roll = atan(y/sqrt((x*x) + (z*z))); //roll calculation

    //converting radians into degrees
    pitch = pitch * (180.0/3.14);
    roll = roll * (180.0/3.14) ;
    angle = atan(roll/pitch)* (180.0/3.14);
    return angle;
}

//function for balance stage
void balance()
{
  pixels.setBrightness(20);
  int rgbId = remapAngle();
  setAllColor(15600);
  pixels.setPixelColor(rgbId,pixels.gamma32(pixels.ColorHSV(500)));
  pixels.show();
}

//function for showing user the bowl is turning off
void bye(){
  rest = 0;
  for(i = 0; i < 6; i++)
  {
    setAllColor(8200);
    pixels.setBrightness(10-i*2);
    pixels.show();
    long v = capSensor.capacitiveSensor(30);
    if (v > threshold){
      break;
    }
    delay(500);   
  }
}

//remapping the angle to LED ring LED id
int remapAngle(){
  if(AcX < -200){
  ledId = map(tiltedAngle,-120,90,11,0);
}
else if(AcX > 200) {
  ledId = map(tiltedAngle,90,-90,12,23);
}
return ledId;
}

//get reading from gyro and calculate the angle, return value is to identy if the bowl is stabe
int checkBalance(){
Wire.beginTransmission(MPU); //begin transmission to I2C slave device
Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
Wire.endTransmission(false); //restarts transmission to I2C slave device
Wire.requestFrom(MPU,14,true); //request 14 registers in total  
//read accelerometer data
AcX=Wire.read()<<8|Wire.read(); // 0x3B (ACCEL_XOUT_H) 0x3C (ACCEL_XOUT_L)  
AcY=Wire.read()<<8|Wire.read(); // 0x3D (ACCEL_YOUT_H) 0x3E (ACCEL_YOUT_L) 
AcZ=Wire.read()<<8|Wire.read(); // 0x3F (ACCEL_ZOUT_H) 0x40 (ACCEL_ZOUT_L)

tiltedAngle = getAngle(AcX,AcY,AcZ);//get pitch/roll, calculate tilted angle
  if(abs(pitch) > balanceTreshold|| abs(roll) > balanceTreshold){
  return 1;
}
else{
  return 0;
}
}

//transition animation
void trans(){
  for(int i = 4; i > 0 ; i--){
    pixels.setBrightness(i);
    pixels.show();
    delay(100);
  }
}

//function for telling user they are calm
void rainbow(){
  pixels.setBrightness(10);
  flag = 1;
  while(flag == 1){
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256)
  {
    for(int i = 0;i < pixels.numPixels();i++)
    {
      int pixelHue = firstPixelHue + (i*65536L / pixels.numPixels());
      pixels.setPixelColor(i,pixels.gamma32(pixels.ColorHSV(pixelHue)));
    }
    pixels.show();
    delay(10);

    long v = capSensor.capacitiveSensor(30);
    if (v < threshold){
      flag = 0;
      break;
    }
    if(checkBalance() == 1){
      trans();
      flag = 0;
      break;
    }
  }
  }
}