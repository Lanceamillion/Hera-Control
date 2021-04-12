#include "timer.h"
#include <EEPROM.h>
#include "Adafruit_LEDBackpack.h"
#include <Adafruit_SSD1306.h>
#include "Adafruit_TCS34725.h"

//Hex codes for Digits
#define DIGIT_0 0x4037
#define DIGIT_1 0x4002
#define DIGIT_2 0x00d7
#define DIGIT_3 0x40c7
#define DIGIT_4 0x40e2
#define DIGIT_5 0x40e5
#define DIGIT_6 0x40f5
#define DIGIT_7 0x4003
#define DIGIT_8 0x40f7
#define DIGIT_9 0x40e7
#define DIGIT_LOE 0x2404
#define DIGIT_DOT 0x0008
#define DIGIT_CLEAR 0x0000

//Pin Definitionst
#define buttonYellow 1//PD1
#define buttonBlue 15 //PC1
#define buttonBlack 0 //PD0
#define trigger 3     //PD3
#define magSensor 2   //PD2
#define rev 4         //PD4
#define mosfet 11     //PB2
#define threePosOne 21//ADC7
#define threePosTwo 20//ADC6
#define OLED_RESET 16 //PC2
#define irChamber 17  //PC3
#define voltagePin 14 //PC0

//Stepper Pins
#define bridgeEnable 5 //PD5
#define pinA 6 //PD6
#define pinB 7 //PD7
#define pinC 8 //PB0
#define pinD 9 //PB1

//LED Display
#define DIGIT_ADDR_HIGH 0
#define DIGIT_ADDR_LOW  1

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

//OLED Definition
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Color Sensor Definition
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_60X);

uint16_t r, g, b, c;

#define RED_THRESH 530

//Stepper Perameraters
#define loadingStepMaximum 80
#define shootStepMinimum 16

int loadingStepsLeft = 0;
int shootStepLeft = 0;

//Buttons and switches
bool buttonState[9];
bool buttonPrevious[9];

enum MAG_ROUND_STATE {MANY,LUNKNOWN,KNOWN};
MAG_ROUND_STATE magState = MANY;

enum FIRE_MODE {SEMI,AUTO,BURST};
FIRE_MODE fireMode = SEMI;

enum CHAMBER_STATE {EMPTY,CHAMBERED,WCHAMBERED,WEMPTY,LOADING,SHOOTING};
CHAMBER_STATE chamberState = EMPTY;

//General Iterator
int i;
//
bool initialChambering = false;
//Safety
bool saftey = false;
//Shooting PWM
int speedPWM = 100;
//NumberOfRounds
int numberOfRounds = 0;

//StepperControl
int stepCount = 0;

//Shotcount
int shotCount = 0;

//Timers
Timer stepIntervalMagLoad(millis);

//Time Constants
#define timeBetweenSteps 2
#define magLoadDelay 100

void setup() {
  //OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  
  pinMode(mosfet,OUTPUT);
  digitalWrite(mosfet,LOW);

  //StepperDriver
  pinMode(bridgeEnable,OUTPUT);
  pinMode(pinA,OUTPUT);
  pinMode(pinB,OUTPUT);
  pinMode(pinC,OUTPUT);
  pinMode(pinD,OUTPUT);

  digitalWrite(bridgeEnable,HIGH);
  digitalWrite(pinA,HIGH);
  digitalWrite(pinB,HIGH);
  digitalWrite(pinC,LOW);
  digitalWrite(pinD,LOW);
  
  pinMode(buttonYellow,INPUT_PULLUP);
  pinMode(buttonBlue,INPUT_PULLUP);
  pinMode(buttonBlack,INPUT_PULLUP);
  pinMode(trigger,INPUT_PULLUP);
  pinMode(rev,INPUT_PULLUP);
  pinMode(magSensor,INPUT_PULLUP);
  pinMode(irChamber,INPUT_PULLUP);
  pinMode(threePosOne,INPUT_PULLUP);
  pinMode(threePosTwo,INPUT_PULLUP);

  //Start LED Display
  alpha4.begin(0x71);

  //Start colorSensor
  tcs.begin();

  //StartTimers
  stepIntervalMagLoad.begin();
  
  //Determine system states
  buttonState[0] = !digitalRead(buttonYellow);
  buttonState[1] = !digitalRead(buttonBlue);
  buttonState[2] = !digitalRead(buttonBlack);
  buttonState[3] = !digitalRead(trigger);
  buttonState[4] = !digitalRead(rev);
  buttonState[5] = !digitalRead(magSensor);
  buttonState[6] = !digitalRead(irChamber);
  buttonState[7] = (analogRead(threePosOne) < 500);
  buttonState[8] = (analogRead(threePosTwo) < 500);

  //Copy current to previous
  for(i = 0; i < 9; i++){
    buttonPrevious[i] = buttonState[i];
  }
  
  //Run Update Functions
  handleFiremodeChange();
  updateLEDDisplay();
  updateOLED();
}

void loop() {
  checkSwitches();
  checkTimers();
}

void checkSwitches(){
  //Read in current data
  buttonState[0] = !digitalRead(buttonYellow);
  buttonState[1] = !digitalRead(buttonBlue);
  buttonState[2] = !digitalRead(buttonBlack);
  buttonState[3] = !digitalRead(trigger);
  buttonState[4] = !digitalRead(rev);
  buttonState[5] = !digitalRead(magSensor);
  buttonState[6] = !digitalRead(irChamber);
  buttonState[7] = (analogRead(threePosOne) < 500);
  buttonState[8] = (analogRead(threePosTwo) < 500);
  
  //Compare to previous state to run functions
  if(buttonState[0] > buttonPrevious[0]) handleYellowButtonPress();
  if(buttonState[1] > buttonPrevious[1]) handleBlueButtonPress();
  if(buttonState[2] > buttonPrevious[2]) handleBlackButtonPress();
  if(buttonState[3] > buttonPrevious[3]) handleTriggerPress();
  else if(buttonState[3]) handleTriggerDown();
  if(buttonState[4] > buttonPrevious[4]) handleRevPress();
  else if(buttonState[4] < buttonPrevious[4]) handleRevRelease();
  if(buttonState[5] > buttonPrevious[5]) handleMagInsertion();
  else if(buttonState[5] < buttonPrevious[5]) handleMagRemoval();
  if(buttonState[6] > buttonPrevious[6]) handleGateClose();
  if((buttonState[7] != buttonPrevious[7]) || 
  (buttonState[8] != buttonPrevious[8])) handleFiremodeChange();
  
  //Copy current to previous
  for(i = 0; i < 9; i++){
    buttonPrevious[i] = buttonState[i];
  }
}

void checkTimers(){
  //stepIntervalMagLoad Mag Load Part
  if(chamberState == WCHAMBERED || chamberState == WEMPTY){
    //If working on chambering
    if(stepIntervalMagLoad > magLoadDelay){
      //If delay time has passed raise the initialChambering flag
      if(chamberState == WCHAMBERED){
        //If it was already chambered
        handleColorCheck();
        chamberState = CHAMBERED;
      }else{
        //If it wasnt already chambered start the chambering
        chamberState = LOADING;
        //Begin Stepping
        stepIntervalMagLoad.reset();
        loadingStepsLeft = loadingStepMaximum;
      }
    }
  }else if(chamberState == LOADING){
    if(stepIntervalMagLoad > timeBetweenSteps){
      if(loadingStepsLeft > 0){
        doStep();
        loadingStepsLeft--;
        stepIntervalMagLoad.reset();
      }else{
        chamberState = EMPTY;
        numberOfRounds = 0;
        shotCount = 0;
        digitalWrite(bridgeEnable,LOW);
        updateLEDDisplay();
      }
    }
  }else if(chamberState == SHOOTING){
    if(stepIntervalMagLoad > timeBetweenSteps){
      if(shootStepLeft > 0){
        doStep();
        shootStepLeft--;
        stepIntervalMagLoad.reset();
      }else{
        numberOfRounds--;
        updateLEDDisplay();
        chamberState = LOADING;
        loadingStepsLeft = loadingStepMaximum;
      }
    }
  }
}

void doStep(){
  switch(stepCount){
    case 0:
      stepCount++;
      digitalWrite(pinA,LOW);
      digitalWrite(pinB,HIGH);
      digitalWrite(pinC,LOW);
      digitalWrite(pinD,LOW);
      break;
    case 1:
      stepCount++;
      digitalWrite(pinA,LOW);
      digitalWrite(pinB,LOW);
      digitalWrite(pinC,HIGH);
      digitalWrite(pinD,LOW);
      break;
    case 2:
      stepCount++;
      digitalWrite(pinA,LOW);
      digitalWrite(pinB,LOW);
      digitalWrite(pinC,LOW);
      digitalWrite(pinD,HIGH);
      break;
    case 3:
      stepCount = 0;
      digitalWrite(pinA,HIGH);
      digitalWrite(pinB,LOW);
      digitalWrite(pinC,LOW);
      digitalWrite(pinD,LOW);
      break;
  }
}

void handleColorCheck(){
  //Dont check if rounds are less than 6
  if(numberOfRounds >= 6){
    //tcs.getRawData(&r, &g, &b, &c);
    //If rounds equal 6 then trasition just happened. Check for that
    if(numberOfRounds == 6){
      //Check if round is there
      if (r < RED_THRESH){
        // Round is no there
        magState = KNOWN;
      }else{
        //This happens only if there should be only 6 rounds left but somehow there is more
        numberOfRounds++;
      }
    }else{
      //Check to make sure the rounds are still there
      if (r < RED_THRESH){
        //This means the round is gone
        if(initialChambering == true){
          //Mag was just being checked for the first time
          magState = LUNKNOWN;
          numberOfRounds = 6;
        }else{
          //Previous number was wrong but now we know this one is right
          magState = KNOWN;
          numberOfRounds = 6;
        }
      }
    }
    initialChambering = false;
  }
  updateLEDDisplay();
}

void handleStepperPower(){
  if(chamberState == EMPTY){
    digitalWrite(bridgeEnable,LOW);
  }else{
    digitalWrite(bridgeEnable,LOW);
  }
}

void handleYellowButtonPress(){
  r=100;
}

void handleBlueButtonPress(){
  r=1000; 
}

void handleBlackButtonPress(){
  handleMagInsertion();
}

void handleTriggerPress(){
  if(fireMode == SEMI){
    if(chamberState == CHAMBERED){
      if(!saftey){
        chamberState = SHOOTING;
      }
    }
  }else if(fireMode == BURST){
    if(!saftey){
      chamberState = SHOOTING;
      shotCount = 1;
    }
  }
}

void handleTriggerDown(){
  if(fireMode == AUTO){
    if(chamberState == CHAMBERED){
      if(!saftey){
        chamberState = SHOOTING;
      }
    }
  }
}

void handleRevPress(){
  if(!saftey){
    analogWrite(mosfet,speedPWM);
  }
}

void handleRevRelease(){
  digitalWrite(mosfet,LOW);
}

void handleMagInsertion(){
  digitalWrite(bridgeEnable,HIGH);
  magState = MANY;
  initialChambering = true;
  if(buttonState[6]){
    numberOfRounds = 13;
    chamberState = WCHAMBERED;
  }else{
    numberOfRounds = 12;
    chamberState = WEMPTY;
  }
  stepIntervalMagLoad.reset();
  updateLEDDisplay();
}

void handleMagRemoval(){
  magState = KNOWN;
  if(buttonState[6]){
    numberOfRounds = 1;
    chamberState = CHAMBERED;
  }else{
    numberOfRounds = 0;
    chamberState = EMPTY;
    shotCount = 0;
    digitalWrite(bridgeEnable,LOW);
  }
  updateLEDDisplay();
}

void handleGateClose(){
  chamberState = CHAMBERED;
  if(shotCount > 0){
    chamberState = SHOOTING;
    shotCount--;
  }
  handleColorCheck();
}

void handleFiremodeChange(){
  if (buttonState[7]){
    fireMode = SEMI;
  }else if (buttonState[8]){
    fireMode = BURST;
  }else{
    fireMode = AUTO;
  }
  updateOLED();
}

void updateOLED(){
  display.clearDisplay();
  
  // Draw white text
  display.setTextColor(SSD1306_WHITE);
   
  //Display fire select
  display.setTextSize(2);
  display.setCursor(0,0);
  if (fireMode == SEMI){  
    display.println(F("SEMI"));
  }else if (fireMode == AUTO){
    display.println(F("AUTO"));
  }else{
    display.println(F("BRST"));
  }
  
  display.setTextSize(1);
  

  //if (modeSetState) display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);//Inverse for PRG
  
  display.setCursor(0,16);//PWM
  display.println(F("PWM:  %"));
  display.setCursor(24,16);
  display.println(100*speedPWM/256);
  /*
  display.setCursor(0,25);//Debug
  display.println(F("PWM:"));
  display.setCursor(24,25);
  display.print(r);*/
  
  display.display();
}

void updateLEDDisplay(){
  //
  if (numberOfRounds > 9){
    //If there are two digits write the sencond digit
    writeSevenSegment(numberOfRounds / 10, DIGIT_ADDR_HIGH);
  }else if (magState == KNOWN){
    //If the mag state is known add a dot to notify the user
    alpha4.writeDigitRaw(DIGIT_ADDR_HIGH, DIGIT_DOT);
  }else if (magState == LUNKNOWN){
    //Add =< symbool if unkown
    alpha4.writeDigitRaw(DIGIT_ADDR_HIGH, DIGIT_LOE);
  }else{
    //If no digit needs to be printed clear the didit
    alpha4.writeDigitRaw(DIGIT_ADDR_HIGH, DIGIT_CLEAR);
  }
  //Print the least significant digit
  writeSevenSegment(numberOfRounds % 10, DIGIT_ADDR_LOW);
  alpha4.writeDisplay();
}

void writeSevenSegment(uint8_t digit, uint8_t address) {
  switch (digit) {
    case 0:
      alpha4.writeDigitRaw(address, DIGIT_0);
      break;
    case 1:
      alpha4.writeDigitRaw(address, DIGIT_1);
      break;
    case 2:
      alpha4.writeDigitRaw(address, DIGIT_2);
      break;
    case 3:
      alpha4.writeDigitRaw(address, DIGIT_3);
      break;
    case 4:
      alpha4.writeDigitRaw(address, DIGIT_4);
      break;
    case 5:
      alpha4.writeDigitRaw(address, DIGIT_5);
      break;
    case 6:
      alpha4.writeDigitRaw(address, DIGIT_6);
      break;
    case 7:
      alpha4.writeDigitRaw(address, DIGIT_7);
      break;
    case 8:
      alpha4.writeDigitRaw(address, DIGIT_8);
      break;
    case 9:
      alpha4.writeDigitRaw(address, DIGIT_9);
      break;
  }
}
