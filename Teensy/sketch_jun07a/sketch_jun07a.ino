#include <Servo.h>
#include <EEPROM.h>
#include <Wire.h>
#include <WireIMXRT.h>
#include <WireKinetis.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>

NAU7802 myScale; //Create instance of the NAU7802 class
Servo myservo; //Create servo object

#define DEBUG 0
#define LOCATION_SERVO_ENDPOINTS_ARRAY 20

int programButtonPin = 2;
int setButtonPin = 3;
int triggerPin = 4;
int potPin = A0;
int servoPin = 16;
int ledPin = 13;

byte servoEndPoints[2] = {50, 80};
bool previousButtonState = 1;
int scaleLowReading = -30000;
int scaleHighReading = 30000; // used to remap scale values to DAC output. Make this contingent on AFE gain setting.

void setup()
{
  pinMode(programButtonPin, INPUT_PULLUP);
  pinMode(setButtonPin, INPUT_PULLUP);
  pinMode(triggerPin, INPUT);
  pinMode(ledPin, OUTPUT);
  analogWriteResolution(12);
  
  Serial.begin(9600);
  Wire.begin();
  myservo.attach(servoPin);
  //myservo.write(37); //Safe starting position.

  if (myScale.begin() == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1);
  }
  Serial.println("Scale detected!");
  
  myScale.setGain(NAU7802_GAIN_4); //Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128.
  myScale.setSampleRate(NAU7802_SPS_80); //Sample rate can be set to 10, 20, 40, 80, or 320Hz
  myScale.calibrateAFE(); //Does an internal calibration. Recommended after power up, gain changes, sample rate changes, or channel changes.

  Serial.print("Default servo endpoints: ");
  Serial.print(servoEndPoints[0]);
  Serial.print(", ");
  Serial.println(servoEndPoints[1]);

  for(int i=0; i<2; i++){
     EEPROM.get(LOCATION_SERVO_ENDPOINTS_ARRAY+i, servoEndPoints[i]);
  }
  Serial.print("Read servo end point from EEPROM: ");
  Serial.print(servoEndPoints[0]);
  Serial.print(", ");
  Serial.println(servoEndPoints[1]);
}

void loop()
{
  if(myScale.available() == true){
    long currentReading = myScale.getReading();
    int scaledReading = map(currentReading, scaleLowReading, scaleHighReading, 0, 4095);
    if (DEBUG){
      Serial.print("Reading: ");
      Serial.println(currentReading);
      Serial.print("Scaled Reading: ");
      Serial.println(scaledReading);
    }
    analogWrite(A14, scaledReading);
  }
    if(buttonPress(programButtonPin)){
      Serial.println("Enterring Programming Mode");
      digitalWrite(ledPin, HIGH);
      updateEndPoints();
    }
    digitalWrite(ledPin, LOW);
    
  if(digitalRead(triggerPin)){
    myservo.write(servoEndPoints[0]);
  }
  else{
    myservo.write(servoEndPoints[1]);
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////
// Supporting Functions
void updateEndPoints(){ 
  int numEndPointsSet = 0;
  while (numEndPointsSet < 2){
    int potValue = analogRead(potPin);
    int servoAngle = map(potValue, 0, 1023, 180, 0);
    myservo.write(servoAngle);
    delay(10);
    
    if (buttonPress(setButtonPin)){
      Serial.print("Endpoint ");
      Serial.print(numEndPointsSet + 1);
      Serial.print(" set to ");
      Serial.println(servoAngle);
      servoEndPoints[numEndPointsSet] = servoAngle;
      numEndPointsSet++;
    }
  } 
  Serial.print("Programmed end points are: ");
  Serial.print(servoEndPoints[0]);
  Serial.print(", ");
  Serial.println(servoEndPoints[1]);
    
  for (int i = 0; i<2; i++){
    EEPROM.update(LOCATION_SERVO_ENDPOINTS_ARRAY+i, servoEndPoints[i] & 0xFF);//Grab and store only the least significant byte of the int.
    byte val = servoEndPoints[i] & 0xFF;
    Serial.print("Byte value to store in EEPROM: ");
    Serial.println(val);
  }
  Serial.println("Returning to normal mode");
}

bool buttonPress(int buttonPin){
  bool buttonState = digitalRead(buttonPin);
  if (buttonState != previousButtonState){
    previousButtonState = buttonState;
    if (buttonState == LOW){
      //Serial.println("Button Pressed");
      delay(50);
      return 1;
    }
    else{
      //Serial.println("Button Released");
      delay(50);
      return 0;
    }  
  }
  else{
  return 0;
  }
}
