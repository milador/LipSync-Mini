/*
//                                                                                                  
//  +++         .+++:    /++++++++/:.     .:/+++++/: .+++/`     .+++/  ++++.      ++++.     `-/++++++/:
//  oooo         .ooo:    +ooo:--:+ooo/   :ooo/:::/+/  -ooo+`   .ooo+`  ooooo:     .o-o`   `/ooo+//://+:
//  oooo         .ooo:    +ooo`    :ooo-  oooo`     `   .ooo+` .ooo+`   oooooo/`   .o-o`  .oooo-`       
//  oooo         .ooo:    +ooo`    -ooo-  -ooo+:.`       .ooo+.ooo/`    ooo:/oo+.  .o-o`  +ooo.         
//  oooo         .ooo:    +ooo.`..:ooo+`   `:+oooo+:`     `+ooooo/      ooo: :ooo- .o-o`  oooo          
//  oooo         .ooo:    +ooooooooo+:`       `-:oooo-     `+ooo/       ooo/  .+oo/.o-o`  +ooo.         
//  oooo         .ooo:    +ooo-...``             `oooo      /ooo.       ooo/   `/oo-o-o`  .oooo-        
//  oooo::::::.  .ooo:    +ooo`           :o//:::+ooo:      /ooo.       ooo/     .o-o-o`   ./oooo/:::/+/
//  +ooooooooo:  .ooo:    /ooo`           -/++ooo+/:.       :ooo.       ooo:      `.o.+      `-/+oooo+/-
//
//An assistive technology device which is developed to allow quadriplegics to use touchscreen mobile devices by manipulation of a mouth-operated joystick with integrated sip and puff controls.
*/

//Developed by : MakersMakingChange
//Modified by: Milad Hajihassan
//Firmware : LipSync_Mini_Firmware
//VERSION: 1.0 (21 March 2020) 



#include "Mouse.h"
//#include <StopWatch.h>
#include <Adafruit_NeoPixel.h>
#include <FlashAsEEPROM.h>
#include <math.h>

//***INPUT/OUTPUT PIN CONFIGURATION***//


#define BUTTON_UP_PIN 12                           // Cursor Control Button 1: UP - digital input pin 12 (internally pulled-up)
#define BUTTON_DOWN_PIN 11                         // Cursor Control Button 2: DOWN - digital input pin 11 (internally pulled-up)
#define LED_PIN A4                               // LipSync RGB LED : analog output pin A4

#define PIO4_PIN 10                               // PIO4_PIN Command Pin - digital output pin 10

#define X_DIR_HIGH_PIN A2                         // X Direction High (Cartesian positive x : right) - analog input pin A2
#define X_DIR_LOW_PIN A3                          // X Direction Low (Cartesian negative x : left) - analog output pin A3
#define Y_DIR_HIGH_PIN A5                         // Y Direction High (Cartesian positive y : up) - analog input pin A5
#define Y_DIR_LOW_PIN A1                         // Y Direction Low (Cartesian negative y : down) - analog input pin A1
#define PRESSURE_PIN A0                           // Sip & Puff Pressure Transducer Pin - analog input pin A0

//***CUSTOMIZE VALUES***//

#define PRESSURE_THRESHOLD 0.4                    //puff threshold 
#define DEBUG_MODE true                           //Debug mode ( Enabled = true and Disabled = false )
#define CURSOR_DEFAULT_SPEED 30                   //Maximum default USB cursor speed                  
#define CURSOR_DELTA_SPEED 5                      //Delta value that is used to calculate USB cursor speed levels
#define CURSOR_RADIUS 50.0                        //Constant joystick radius
#define LED_BRIGHTNESS 125                         //The mode led color brightness

//***VARIABLE DECLARATION***//

int cursorSpeedCounter = 5;                       //Cursor speed counter which is set to 4 by default 


int xHigh, yHigh, xLow, yLow;                       
int xRight, xLeft, yUp, yDown;                    //Individual neutral starting positions for each FSR

int xHighMax, xLowMax, yHighMax, yLowMax;         //Max FSR values which are set to the values from EEPROM


float xHighYHighRadius, xHighYLowRadius, xLowYLowRadius, xLowYHighRadius;
float xHighYHigh, xHighYLow, xLowYLow, xLowYHigh;

int cursorDeltaBox;                               //The delta value for the boundary range in all 4 directions about the x,y center
int cursorDelta;                                  //The amount cursor moves in some single or combined direction
int cursorClickStatus = 0;                        //The value indicator for click status, ie. tap, back and drag ( example : 0 = tap )
int configDone;                                   // Binary check of completed configuration
unsigned int puffCount, sipCount;                 //The puff and long sip incremental counter variables
int pollCounter = 0;                              //Cursor poll counter

int cursorDelay;
float cursorFactor;
int cursorMaxSpeed;

float yHighComp = 1.0;
float yLowComp = 1.0;
float xHighComp = 1.0;
float xLowComp = 1.0;

float yHighDebug, yLowDebug, xHighDebug, xLowDebug;
int yHighMaxDebug, yLowMaxDebug, xHighMaxDebug, xLowMaxDebug;

//Pressure sensor variables
float sipThreshold, puffThreshold, cursorClick;

//RGB LED Color code structure 

struct rgbColorCode {
    int r;    // red value 0 to 255
    int g;   // green value
    int b;   // blue value
 };

//Color structure 
typedef struct { 
  uint8_t colorNumber;
  String colorName;
  rgbColorCode colorCode;
} colorStruct;

//Color properties 
const colorStruct colorProperty[] {
    {1,"GREEN",{0,100,0}},
    {2,"PINK",{100,0,40}},
    {3,"YELLOW",{100,100,0}},    
    {4,"ORANGE",{100,40,0}},
    {5,"BLUE",{0,0,100}},
    {6,"RED",{100,0,0}},
    {7,"PURPLE",{100,0,100}},
    {8,"TEAL",{0,128,128}}       
};

//Action structure 
typedef struct { 
  uint8_t actionNumber;
  String actionName;
  uint8_t actionColorNumber;
} actionStruct;

//Action properties 
const actionStruct actionProperty[] {
    {0,"Initialization",8},
    {1,"Calibration",5},
    {2,"Calibration_Start",1},
    {3,"Calibration_End",6},
    {4,"Calibration_Home",5},
    {5,"Swipe",3},
    {6,"Scroll",3},
    {7,"Speed_Increase",1},
    {8,"Speed_Decrease",6},
    {9,"Speed_Max",3}
};

//Cursor Speed Level structure 
typedef struct {
  int _delay;
  float _factor;
  int _maxSpeed;
} _cursor;

//Define characteristics of each speed level ( Example: 5,-1.0,10)
_cursor setting1 = {5, -1.1, CURSOR_DEFAULT_SPEED - (5 * CURSOR_DELTA_SPEED)};
_cursor setting2 = {5, -1.1, CURSOR_DEFAULT_SPEED - (4 * CURSOR_DELTA_SPEED)};
_cursor setting3 = {5, -1.1, CURSOR_DEFAULT_SPEED - (3 * CURSOR_DELTA_SPEED)};
_cursor setting4 = {5, -1.1, CURSOR_DEFAULT_SPEED - (2 * CURSOR_DELTA_SPEED)};
_cursor setting5 = {5, -1.1, CURSOR_DEFAULT_SPEED - (CURSOR_DELTA_SPEED)};
_cursor setting6 = {5, -1.1, CURSOR_DEFAULT_SPEED};
_cursor setting7 = {5, -1.1, CURSOR_DEFAULT_SPEED + (CURSOR_DELTA_SPEED)};
_cursor setting8 = {5, -1.1, CURSOR_DEFAULT_SPEED + (2 * CURSOR_DELTA_SPEED)};
_cursor setting9 = {5, -1.1, CURSOR_DEFAULT_SPEED + (3 * CURSOR_DELTA_SPEED)};
_cursor setting10 = {5, -1.1, CURSOR_DEFAULT_SPEED + (4 * CURSOR_DELTA_SPEED)};
_cursor setting11 = {5, -1.1, CURSOR_DEFAULT_SPEED + (5 * CURSOR_DELTA_SPEED)};

_cursor cursorParams[11] = {setting1, setting2, setting3, setting4, setting5, setting6, setting7, setting8, setting9, setting10, setting11};

int single = 0;
int puff1, puff2;

bool settingsEnabled = false; 

//Declare the RGB LED's
Adafruit_NeoPixel ledPixel = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

float pressure_test;

//-----------------------------------------------------------------------------------//

//***MICROCONTROLLER AND PERIPHERAL MODULES CONFIGURATION***//

void setup() {
  
  Serial.begin(115200);                           //Setting baud rate for serial communication which is used for diagnostic data returned from Bluetooth and microcontroller

  pinMode(LED_PIN, OUTPUT);                       //Set the LED pin 1 as output(GREEN LED)
  pinMode(PRESSURE_PIN, INPUT);                   //Set the pressure sensor pin input
  pinMode(X_DIR_HIGH_PIN, INPUT);                 //Define Force sensor pinsas input ( Right FSR )
  pinMode(X_DIR_LOW_PIN, INPUT);                  //Define Force sensor pinsas input ( Left FSR )
  pinMode(Y_DIR_HIGH_PIN, INPUT);                 //Define Force sensor pinsas input ( Up FSR )
  pinMode(Y_DIR_LOW_PIN, INPUT);                  //Define Force sensor pinsas input ( Down FSR )


  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);           //Set increase cursor speed button pin as input
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);         //Set decrease cursor speed button pin as input

  pinMode(2, INPUT_PULLUP);                       //Set unused pins as inputs with pullups
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);


  //while(!Serial1);
  delay(2000);
  joystickInitialization();                       //Set the Home joystick and generate movement threshold boundaries
  delay(10);
  pressureSensorInitialization();                 //Set the pressure sensor threshold boundaries
  delay(10);

  
  setDefault();                                   //Set the default values that are stored in EEPROM
  delay(10);
  mouseConfigure();                               //Configure the HID mouse functions
  delay(10);
  usbConfigure();                                 //USB configuration 
  delay(10);
  readCursorSpeed();                              //Read the saved cursor speed parameter from EEPROM
  delay(10);
  initLed(LED_BRIGHTNESS);                        //RGB Led's initialization 
  delay(10);
  displayVersion();                               //Display firmware version number
  delay(10);
  initLedFeedback();                              //Show led feedback that the device is ready to use and indicate current mode
  delay(10);
  forceCursorDisplay();                           //Display cursor on screen by moving it
  

  cursorDelay = cursorParams[cursorSpeedCounter]._delay;
  cursorFactor = cursorParams[cursorSpeedCounter]._factor;
  cursorMaxSpeed = cursorParams[cursorSpeedCounter]._maxSpeed;

  //Functions below are for diagnostic feedback only
  if(DEBUG_MODE==true) {
      puff1=EEPROM.read(0);
      delay(5);
      Serial.println((String)"configDone: "+puff1);
      delay(5);
      puff2=EEPROM.read(2);
      delay(5);
      Serial.println((String)"cursorSpeedCounter: "+puff2);
      delay(5);
      Serial.println((String)"cursorDelay: "+cursorParams[puff2]._delay);
      delay(5);
      Serial.println((String)"cursorFactor: "+cursorParams[puff2]._factor);
      delay(5);
      Serial.println((String)"cursorMaxSpeed: "+cursorParams[puff2]._maxSpeed);
      delay(5);
      yHighDebug=EEPROM.read(6);
      delay(5);
      Serial.println((String)"yHighComp: "+yHighDebug);
      delay(5);
      yLowDebug=EEPROM.read(10);
      delay(5);
      Serial.println((String)"yLowComp: "+yLowDebug);
      delay(5);
      xHighDebug=EEPROM.read(14);
      delay(5);
      Serial.println((String)"xHighComp: "+xHighDebug);
      delay(5);
      xLowDebug=EEPROM.read(18);
      delay(5);
      Serial.println((String)"xLowComp: "+xLowDebug);
      delay(5);
      xHighMaxDebug=EEPROM.read(22);
      delay(5);
      Serial.println((String)"xHighhMax: "+xHighMaxDebug);
      delay(5);
      xLowMaxDebug=EEPROM.read(24);
      delay(5);
      Serial.println((String)"xLowMax: "+xLowMaxDebug);
      delay(5);
      yHighMaxDebug=EEPROM.read(26);
      delay(5);
      Serial.println((String)"yHighMax: "+yHighMaxDebug);
      delay(5);
      yLowMaxDebug=EEPROM.read(28);
      delay(5);
      Serial.println((String)"yLowMax: "+yLowMaxDebug);
      delay(5);
  }
  
}

//-----------------------------------------------------------------------------------//

//***START OF MAIN LOOP***//

void loop() {
  
  settingsEnabled=serialSettings(settingsEnabled);       //Check to see if setting option is enabled in Lipsync

  xHigh = averageAnalogRead(3,X_DIR_HIGH_PIN);             //Read analog values of FSR's : A0
  xLow = averageAnalogRead(3,X_DIR_LOW_PIN);               //Read analog values of FSR's : A1
  yHigh = averageAnalogRead(3,Y_DIR_HIGH_PIN);             //Read analog values of FSR's : A0
  yLow = averageAnalogRead(3,Y_DIR_LOW_PIN);               //Read analog values of FSR's : A10


  xHighYHigh = sqrt(sq(((xHigh - xRight) > 0) ? (float)(xHigh - xRight) : 0.0) + sq(((yHigh - yUp) > 0) ? (float)(yHigh - yUp) : 0.0));     //The sq() function raises thr input to power of 2 and is returning the same data type int->int
  xHighYLow = sqrt(sq(((xHigh - xRight) > 0) ? (float)(xHigh - xRight) : 0.0) + sq(((yLow - yDown) > 0) ? (float)(yLow - yDown) : 0.0));    //The sqrt() function raises input to power 1/2, returning a float type
  xLowYHigh = sqrt(sq(((xLow - xLeft) > 0) ? (float)(xLow - xLeft) : 0.0) + sq(((yHigh - yUp) > 0) ? (float)(yHigh - yUp) : 0.0));          //These are the vector magnitudes of each quadrant 1-4. Since the FSRs all register
  xLowYLow = sqrt(sq(((xLow - xLeft) > 0) ? (float)(xLow - xLeft) : 0.0) + sq(((yLow - yDown) > 0) ? (float)(yLow - yDown) : 0.0));         //a larger digital value with a positive application force, a large negative difference

  //Check to see if the joystick has moved
  if ((xHighYHigh > xHighYHighRadius) || (xHighYLow > xHighYLowRadius) || (xLowYLow > xLowYLowRadius) || (xLowYHigh > xLowYHighRadius)) {
    //Add to the poll counter
    pollCounter++;
    delay(20); 
    //Perform cursor movment actions if joystick has been in active zone for 3 or more poll counts
    if (pollCounter >= 3) {
        if ((xHighYHigh >= xHighYLow) && (xHighYHigh >= xLowYHigh) && (xHighYHigh >= xLowYLow)) {
          Serial.println("quad1");
          Mouse.move(xCursorHigh(xHigh), yCursorHigh(yHigh), 0);
          delay(cursorDelay);
          pollCounter = 0;
        } else if ((xHighYLow > xHighYHigh) && (xHighYLow > xLowYLow) && (xHighYLow > xLowYHigh)) {
          Serial.println("quad4");
          Mouse.move(xCursorHigh(xHigh), yCursorLow(yLow), 0);
          delay(cursorDelay);
          pollCounter = 0;
        } else if ((xLowYLow >= xHighYHigh) && (xLowYLow >= xHighYLow) && (xLowYLow >= xLowYHigh)) {
          Serial.println("quad3");
          Mouse.move(xCursorLow(xLow), yCursorLow(yLow), 0);
          delay(cursorDelay);
          pollCounter = 0;
        } else if ((xLowYHigh > xHighYHigh) && (xLowYHigh >= xHighYLow) && (xLowYHigh >= xLowYLow)) {
          Serial.println("quad2");
          Mouse.move(xCursorLow(xLow), yCursorHigh(yHigh), 0);
          delay(cursorDelay);
          pollCounter = 0;
        }
    }
  }

  //Cursor speed control push button functions below
  if (digitalRead(BUTTON_UP_PIN) == LOW) {
    delay(200);
    clearLedColor();
    if (Mouse.isPressed(MOUSE_LEFT)) {
      Mouse.release(MOUSE_LEFT);
    } else if (Mouse.isPressed(MOUSE_RIGHT)) {
      Mouse.release(MOUSE_RIGHT);
    } 
    delay(50);
    if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      joystickCalibration();                      //Call joystick calibration if both push button up and down are pressed 
    } else {
      increaseCursorSpeed();                      //Call increase cursor speed function if push button up is pressed 
    }
  }

  if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
    delay(200);
    clearLedColor();
    if (Mouse.isPressed(MOUSE_LEFT)) {
      Mouse.release(MOUSE_LEFT);
    } else if (Mouse.isPressed(MOUSE_RIGHT)) {
      Mouse.release(MOUSE_RIGHT);
    }
    delay(50);
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      joystickCalibration();                      //Call joystick calibration if both push button up and down are pressed 
    } else {
      decreaseCursorSpeed();                      //Call increase cursor speed function if push button up is pressed 
    }
  }

  //Perform pressure sensor sip and puff functions
  cursorClick = averageAnalogRead(10,PRESSURE_PIN);   //Read the pressure transducer analog value and convert it using ADC to a value between [0.0V - 5.0V]

  //Check if the pressure is under puff pressure threshold 
  if (cursorClick < puffThreshold) {             
    while (cursorClick < puffThreshold) {
      cursorClick = averageAnalogRead(10,PRESSURE_PIN);
      puffCount++;                                //Count how long the pressure value has been under puff pressure threshold
      delay(5);
    }
    //USB puff actions 
      if (puffCount < 40) {
        //Serial.println("Puff");
        //Perform mouse left click action if puff counter value is under 40 ( 1 Second Short Puff )
        clearLedColor();
        if (Mouse.isPressed(MOUSE_LEFT)) {
          Mouse.release(MOUSE_LEFT);
        } else {
          Mouse.click(MOUSE_LEFT);
          delay(5);
        }
      } else if (puffCount > 40 && puffCount < 150) {
        //Perform mouse left press action ( Drag Action ) if puff counter value is under 150 and more than 40 ( 3 Second Long Puff )
        //Serial.println("Long Puff");
        if (Mouse.isPressed(MOUSE_LEFT)) {
          Mouse.release(MOUSE_LEFT);
          clearLedColor();
        } else {
          updateLedColor(actionProperty[5].actionColorNumber,LED_BRIGHTNESS);
          Mouse.press(MOUSE_LEFT);
          delay(5);
        }
      } else if (puffCount > 150) {
        //Serial.println("Very Long Puff");
        //Perform joystick manual calibration to reset default value of FSR's if puff counter value is more than 150 ( 5 second Long Puff )
        clearLedColor();
        delay(5);
        joystickHomeCalibration();
      }
    puffCount = 0;                                //Reset puff counter
  }


  //Check if the pressure is above sip pressure threshold 
  if (cursorClick > sipThreshold) {
    while (cursorClick > sipThreshold) {
      cursorClick = averageAnalogRead(10,PRESSURE_PIN);
      sipCount++;                                 //Count how long the pressure value has been above sip pressure threshold
      delay(5);
    }

    //USB Sip actions 
      if (sipCount < 40) {
        //Serial.println("Sip");
        //Perform mouse right click action if sip counter value is under 150 ( 1 Second Short Sip )
        clearLedColor();
        Mouse.click(MOUSE_RIGHT);
        delay(5);
      } else if (sipCount > 40 && sipCount < 150) {
        //Serial.println("Long Sip");
        //Perform mouse scroll action if sip counter value is under 150 and more than 40 ( 3 Second Long Sip )
        updateLedColor(actionProperty[6].actionColorNumber,LED_BRIGHTNESS);
        delay(5);
        mouseScroll();
        delay(5);
      } else if (sipCount > 150) {
        //Serial.println("Very Long Sip");
        clearLedColor();
        delay(5);
      }
    sipCount = 0;                                 //Reset sip counter
  }
  
}

//***END OF Main LOOP***//

//-----------------------------------------------------------------------------------//

//***SERIAL SETTINGS FUNCTION TO CHANGE SPEED AND COMMUNICATION MODE USING SOFTWARE***//

bool serialSettings(bool enabled) {

    String inString = "";  
    bool settingsFlag = enabled;                   //Set the input parameter to the flag returned. This will help to detect that the settings actions should be performed.
    
     if (Serial.available()>0)  
     {  
       inString = Serial.readString();            //Check if serial has received or read input string and word "settings" is in input string.
       if (settingsFlag==false && inString=="settings") {
       Serial.println("Actions:");                //Display list of possible actions 
       Serial.println("S,(+ or -)");
       Serial.println("C,0");
       settingsFlag=true;                         //Set the return flag to true so settings actions can be performed in the next call to the function
       }
       else if (settingsFlag==true && inString.length()==((2*2)-1)){ //Check if the input parameter is true and the received string is 3 characters only
        inString.replace(",","");                 //Remove commas 
        if(inString.length()==2) {                //Perform settings actions if there are only two characters in the string.
          writeSettings(inString);
          Serial.println("Successfully changed.");
        }   
        Serial.println("Exiting the settings.");
        settingsFlag=false;   
       }
       else if (settingsFlag==true){
        Serial.println("Exiting the settings.");
        settingsFlag=false;         
       }
       Serial.flush();  
     }  
    return settingsFlag;
}

//***PERFORM SETTINGS FUNCTION TO CHANGE SPEED AND COMMUNICATION MODE USING SOFTWARE***//

void writeSettings(String changeString) {
    char changeChar[changeString.length()+1];
    changeString.toCharArray(changeChar, changeString.length()+1);

    //Increase the cursor speed if received "S+" and decrease the cursor speed if received "S-"
    if(changeChar[0]=='S' && changeChar[1]=='+') {
      increaseCursorSpeed();
      delay(5);
    } else if (changeChar[0]=='S' && changeChar[1]=='-') {
      decreaseCursorSpeed();
      delay(5);
    } else if (changeChar[0]=='C' && changeChar[1]=='0') {
      joystickCalibration();
      delay(5);
    } 
}

//***DISPLAY VERSION FUNCTION***//

void displayVersion(void) {

  Serial.println(" --- ");
  Serial.println("LipSync Mini USB V1.0 (21 March 2020)");
  Serial.println(" --- ");

}


//***MOUSE SCROLL FUNCTION***//

void mouseScroll(void) {
  while (1) {
    int scrollUp = analogRead(Y_DIR_HIGH_PIN);                      // A2
    int scrollDown = analogRead(Y_DIR_LOW_PIN);                     // A10

    float scrollRelease = (((float)analogRead(PRESSURE_PIN)) / 1023.0) * 5.0;
    
    if (scrollUp > yUp + 30) {
      Mouse.move(0, 0, -1 * yCursorHigh(scrollUp));
      delay(cursorDelay * 35);
    } else if (scrollDown > yDown + 30) {
      Mouse.move(0, 0, -1 * yCursorLow(scrollDown));
      delay(cursorDelay * 35);
    } else if ((scrollRelease > sipThreshold) || (scrollRelease < puffThreshold)) {
      break;
    }
  }
  delay(250);
}

//***Initialization completion RGB LED color effect function***//

void initLedFeedback(){
  makeLedRainbow(20);
  delay(5);
  setLedBlink(2,500,actionProperty[0].actionColorNumber,LED_BRIGHTNESS);
  delay(5);
}

//***RGB LED Initialization function***//

void initLed(int ledBrightness) {

  //Initialize NeoPixel RGB LED 
  ledPixel.begin();  
  ledPixel.setBrightness(ledBrightness);
  ledPixel.show(); 
}

//***Update RGB LED color using color number defined in colorProperty, and brightness value function***//

void updateLedColor(int ledColor, uint8_t ledBrightness) {
  ledColor--;
  ledPixel.setPixelColor(0, ledPixel.Color(colorProperty[ledColor].colorCode.g,colorProperty[ledColor].colorCode.r,colorProperty[ledColor].colorCode.b));
  ledPixel.setBrightness(ledBrightness);
  ledPixel.show();   
}

//***Set RGB LED blink Function***//

void setLedBlink(int ledBlinkNum, int ledBlinkDelay, int ledColor, uint8_t ledBrightness) {
  if (ledBlinkNum < 0) ledBlinkNum *= -1;
      ledColor--;
      for (int i = 0; i < ledBlinkNum; i++) {
        ledPixel.setPixelColor(0, ledPixel.Color(colorProperty[ledColor].colorCode.g,colorProperty[ledColor].colorCode.r,colorProperty[ledColor].colorCode.b));
        ledPixel.setBrightness(ledBrightness);
        ledPixel.show();  
        
        delay(ledBlinkDelay);
        clearLedColor();
        delay(ledBlinkDelay);
      }
}

//*** Create RGB LED Rainbow effect color function***//

void makeLedRainbow(uint8_t wait) {
  uint16_t i, j;
 
  for(j=0; j<256; j++) {
    for(i=0; i<ledPixel.numPixels(); i++) {
      ledPixel.setPixelColor(i, getLedColorWheel((i+j) & 255));
    }
    ledPixel.show();
    delay(wait);
  }
}

//*** get RGB LED 32 bits color code from 0 to 255 bye value  function***//

uint32_t getLedColorWheel(byte WheelPos) {
  if(WheelPos < 85) {
   return ledPixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return ledPixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return ledPixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

//***Clear RGB LED function***//

void clearLedColor(void) {
   ledPixel.setPixelColor(0, ledPixel.Color(0,0,0));
   ledPixel.show();  
}

//***READ THE CURSOR SPEED LEVEL FUNCTION***//

void readCursorSpeed(void) {
  int var;
  var=EEPROM.read(2); 
  delay(5);
  cursorSpeedCounter = var;
}

//***INCREASE CURSOR SPEED LEVEL FUNCTION***//

void increaseCursorSpeed(void) {
  cursorSpeedCounter++;
  if (cursorSpeedCounter == 11) {
    setLedBlink(6,50,actionProperty[9].actionColorNumber,LED_BRIGHTNESS);
    cursorSpeedCounter = 10;
  } else {
    setLedBlink(cursorSpeedCounter,100,actionProperty[7].actionColorNumber,LED_BRIGHTNESS);
    
    cursorDelay = cursorParams[cursorSpeedCounter]._delay;
    cursorFactor = cursorParams[cursorSpeedCounter]._factor;
    cursorMaxSpeed = cursorParams[cursorSpeedCounter]._maxSpeed;

    if (EEPROM.isValid()) {
      EEPROM.write(2, cursorSpeedCounter);
      EEPROM.commit();
      delay(25);
      Serial.println("+");
    }
  }
  Serial.print("Speed level : ");
  Serial.println(cursorSpeedCounter+1);
}

//***DECREASE CURSOR SPEED LEVEL FUNCTION***//

void decreaseCursorSpeed(void) {
  cursorSpeedCounter--;
  if (cursorSpeedCounter == -1) {
    setLedBlink(6,50,actionProperty[9].actionColorNumber,LED_BRIGHTNESS);
    cursorSpeedCounter = 0;
  } else if (cursorSpeedCounter == 0) {
    setLedBlink(1,350,actionProperty[8].actionColorNumber,LED_BRIGHTNESS);
    cursorDelay = cursorParams[cursorSpeedCounter]._delay;
    cursorFactor = cursorParams[cursorSpeedCounter]._factor;
    cursorMaxSpeed = cursorParams[cursorSpeedCounter]._maxSpeed;

    if (EEPROM.isValid()) {
      EEPROM.write(2, cursorSpeedCounter);
      EEPROM.commit();
      delay(25);
      Serial.println("-");
    }
  } else {
    setLedBlink(cursorSpeedCounter+1,100,actionProperty[8].actionColorNumber,LED_BRIGHTNESS);

    cursorDelay = cursorParams[cursorSpeedCounter]._delay;
    cursorFactor = cursorParams[cursorSpeedCounter]._factor;
    cursorMaxSpeed = cursorParams[cursorSpeedCounter]._maxSpeed;

    if (EEPROM.isValid()) {
      EEPROM.write(2, cursorSpeedCounter);
      EEPROM.commit();
      delay(25);
      Serial.println("-");
    }
  }
   Serial.print("Speed level : ");
   Serial.println(cursorSpeedCounter+1);
}

//***Y HIGH CURSOR MOVEMENT MODIFIER FUNCTION***//

int yCursorHigh(int j) {

  if (j > yUp) {
    //Calculate Y up factor ( 1.25 multiplied by Y high comp multiplied by ratio of Y value to Y High Maximum value )
    float yUp_factor = 1.25 * (yHighComp * (((float)(j - yUp)) / (yHighMax - yUp)));

    //Use the calculated Y up factor to none linearize the maximum speeds
    int k = (int)(round(-1.0 * pow(cursorMaxSpeed, yUp_factor)) - 1.0);

    //Select maximum speed
    int maxSpeed = round(-1.0 * pow(cursorMaxSpeed, 1.25*yHighComp)) - 1.0;

    //Map the value to a value between 0 and the selected maximum speed
    k = map(k, 0, (round(-1.0 * pow(cursorMaxSpeed, 1.25*yHighComp)) - 1.0), 0, maxSpeed); 

    //Set a constrain
    k = constrain(k,-1 * cursorMaxSpeed, 0);

    return k;
  } else {
    return 0;
  }
}

//***Y LOW CURSOR MOVEMENT MODIFIER FUNCTION***//

int yCursorLow(int j) {

  if (j > yDown) {
    //Calculate Y down factor ( 1.25 multiplied by Y low comp multiplied by ratio of Y value to Y Low Maximum value )
    float yDown_factor = 1.25 * (yLowComp * (((float)(j - yDown)) / (yLowMax - yDown)));

    //Use the calculated Y down factor to none linearize the maximum speeds
    int k = (int)(round(1.0 * pow(cursorMaxSpeed, yDown_factor)) - 1.0);

    //Select maximum speed
    int maxSpeed = round(1.0 * pow(cursorMaxSpeed, 1.25*yLowComp)) - 1.0;

    //Map the values to a value between 0 and the selected maximum speed
    k = map(k, 0, (round(1.0 * pow(cursorMaxSpeed, 1.25*yLowComp)) - 1.0), 0, maxSpeed); 
    
    //Set a constrain   
    k = constrain(k,0,cursorMaxSpeed);
    
    return k;
  } else {
    return 0;
  }
}

//***X HIGH CURSOR MOVEMENT MODIFIER FUNCTION***//

int xCursorHigh(int j) {

  if (j > xRight) {
    //Calculate X right factor ( 1.25 multiplied by X high comp multiplied by ratio of X value to X High Maximum value )
    float xRight_factor = 1.25 * (xHighComp * (((float)(j - xRight)) / (xHighMax - xRight)));

    //Use the calculated X down factor to none linearize the maximum speeds
    int k = (int)(round(1.0 * pow(cursorMaxSpeed, xRight_factor)) - 1.0);

    //Select maximum speed
    int maxSpeed = round(1.0 * pow(cursorMaxSpeed, 1.25*xHighComp)) - 1.0;

    //Map the values to a value between 0 and the selected maximum speed
    k = map(k, 0, (round(1.0 * pow(cursorMaxSpeed, 1.25*xHighComp)) - 1.0), 0, maxSpeed); 
    
    //Set a constrain
    k = constrain(k,0,cursorMaxSpeed);
    
    return k;
  } else {
    return 0;
  }
}

//***X LOW CURSOR MOVEMENT MODIFIER FUNCTION***//

int xCursorLow(int j) {

  if (j > xLeft) {
    //Calculate X left factor ( 1.25 multiplied by X low comp multiplied by ratio of X value to X low Maximum value )
    float xLeft_factor = 1.25 * (xLowComp * (((float)(j - xLeft)) / (xLowMax - xLeft)));

    //Use the calculated X down factor to none linearize the maximum speeds
    int k = (int)(round(-1.0 * pow(cursorMaxSpeed, xLeft_factor)) - 1.0);

    //Select maximum speed
    int maxSpeed = round(-1.0 * pow(cursorMaxSpeed, 1.25*xLowComp)) - 1.0;

    //Map the values to a value between 0 and the selected maximum speed
    k = map(k, 0, (round(-1.0 * pow(cursorMaxSpeed, 1.25*xLowComp)) - 1.0), 0, maxSpeed); 
    
    //Set a constrain 
    k = constrain(k,-1 * cursorMaxSpeed, 0);
     
    return k;
  } else {
    return 0;
  }
}

//***JOYSTICK INITIALIZATION FUNCTION***//

void joystickInitialization(void) {
  xHigh = analogRead(X_DIR_HIGH_PIN);               //Set the initial neutral x-high value of joystick
  delay(10);

  xLow = analogRead(X_DIR_LOW_PIN);                 //Set the initial neutral x-low value of joystick
  delay(10);

  yHigh = analogRead(Y_DIR_HIGH_PIN);               //Set the initial neutral y-high value of joystick
  delay(10);

  yLow = analogRead(Y_DIR_LOW_PIN);                 //Set the initial Initial neutral y-low value of joystick
  delay(10);

  xRight = xHigh;
  xLeft = xLow;
  yUp = yHigh;
  yDown = yLow;

  yHighComp=EEPROM.read(0); 
  delay(10);
  yLowComp=EEPROM.read(10); 
  delay(10);
  xHighComp=EEPROM.read(14);
  delay(10);
  xLowComp=EEPROM.read(18);
  delay(10);
  xHighMax=EEPROM.read(22);
  delay(10);
  xLowMax=EEPROM.read(24);
  delay(10);
  yHighMax=EEPROM.read(26);
  delay(10);
  yLowMax=EEPROM.read(28);
  delay(10);

  xHighYHighRadius = CURSOR_RADIUS;
  xHighYLowRadius = CURSOR_RADIUS;
  xLowYLowRadius = CURSOR_RADIUS;
  xLowYHighRadius = CURSOR_RADIUS;

}

//***PRESSURE SENSOR INITIALIZATION FUNCTION***//

void pressureSensorInitialization(void) {
  float nominalPressure = averageAnalogRead(50,PRESSURE_PIN);               //Set the initial neutral pressure transducer analog value [0.0V - 5.0V]
   delay(50);
//PRESSURE_THRESHOLD
  sipThreshold = nominalPressure*3;                                         //Create sip pressure threshold value

  puffThreshold = nominalPressure/1.65;                                     //Create puff pressure threshold value
}

//***AVERAGE ANALOG PIN FUNCTION***//

float averageAnalogRead(int number,int pin) {
  float total=0.0;
  for (int i = 0; i < number ; i++)
  {
    total += analogRead(pin);
    delay(2);
  }
  return (total/number);
}

//***USB CONFIGURATION STATUS FUNCTION***//

void usbConfigStatus(void) {
  int eepromVal = 3;                                //Local integer variable initialized and defined for use with EEPROM GET function
  eepromVal=EEPROM.read(0);                         //Assign value of EEPROM memory at index zero (0)
  delay(10);
  configDone = (eepromVal == 1) ? eepromVal : 0;    //Define the configDone to 0 if the device is set for the first time
  delay(10);
}

//***USB CONFIGURATION FUNCTION***//

void usbConfigure(void) {
  usbConfigStatus();                                  //Check if device has previously been configured
  delay(10);
  if (configDone == 0) {                              //If device has not been configured
    int val0 = 1;
    int val1 = cursorSpeedCounter;

    //Save the default cursor counter value and configuration completed value at EEPROM. This action will happen once when a new device is configured 

    EEPROM.write(0, val0);                            //Save the configuration completed value at EEPROM address location 0
    delay(15);
    EEPROM.write(2, val1);                            //Save the default cursor speed counter value at EEPROM address location 2
    delay(15);
    EEPROM.commit();
    delay(15);
  } else {
    Serial.println("LipSync has previously been configured.");
    delay(10);
  }
}


//***ARDUINO/GENUINO HID MOUSE INITIALIZATION FUNCTION***//

void mouseConfigure(void) {                         //USB mode is commMode == 0, this is when the jumper on J13 is installed
  Mouse.begin();                                  //Initialize the HID mouse functions
  delay(25);                                      //Allow extra time for initialization to take effect
}

//***FORCE DISPLAY OF CURSOR***//

void forceCursorDisplay(void) {
  Mouse.move(1, 0, 0);
  delay(25);
  Mouse.move(-1, 0, 0);
  delay(25);
}

//***JOYSTICK SPEED CALIBRATION FUNCTION***//

void joystickCalibration(void) {
  Serial.println("Prepare for joystick calibration!");       //Start the joystick calibration sequence 
  Serial.println(" ");
  setLedBlink(4,300,actionProperty[1].actionColorNumber,LED_BRIGHTNESS);

  Serial.println("Move mouthpiece to the furthest vertical up position and hold it there until the LED turns SOLID RED, then release the mouthpiece.");
  setLedBlink(6,500,actionProperty[2].actionColorNumber,LED_BRIGHTNESS);
  yHighMax = analogRead(Y_DIR_HIGH_PIN);
  setLedBlink(1,1000,actionProperty[3].actionColorNumber,LED_BRIGHTNESS);
  Serial.println(yHighMax);

  Serial.println("Move mouthpiece to the furthest horizontal right position and hold it there until the LED turns SOLID RED, then release the mouthpiece.");
  setLedBlink(6,500,actionProperty[2].actionColorNumber,LED_BRIGHTNESS);
  xHighMax = analogRead(X_DIR_HIGH_PIN);
  setLedBlink(1,1000,actionProperty[3].actionColorNumber,LED_BRIGHTNESS);
  Serial.println(xHighMax);

  Serial.println("Move mouthpiece to the furthest vertical down position and hold it there until the LED turns SOLID RED, then release the mouthpiece.");
  setLedBlink(6,500,actionProperty[2].actionColorNumber,LED_BRIGHTNESS);
  yLowMax = analogRead(Y_DIR_LOW_PIN);
  setLedBlink(1,1000,actionProperty[3].actionColorNumber,LED_BRIGHTNESS);
  Serial.println(yLowMax);

  Serial.println("Move mouthpiece to the furthest horizontal left position and hold it there until the LED turns SOLID RED, then release the mouthpiece.");
  setLedBlink(6,500,actionProperty[2].actionColorNumber,LED_BRIGHTNESS);
  xLowMax = analogRead(X_DIR_LOW_PIN);
  setLedBlink(1,1000,actionProperty[3].actionColorNumber,LED_BRIGHTNESS);
  Serial.println(xLowMax);

  int maxX = (xHighMax > xLowMax) ? xHighMax : xLowMax;
  int yMax = (yHighMax > yLowMax) ? yHighMax : yLowMax;
  float finalMax = (maxX > yMax) ? (float)maxX : (float)yMax;

  Serial.print("finalMax: ");
  Serial.println(finalMax);

  yHighComp = (finalMax - yUp) / (yHighMax - yUp);
  yLowComp = (finalMax - yDown) / (yLowMax - yDown);
  xHighComp = (finalMax - xRight) / (xHighMax - xRight);
  xLowComp = (finalMax - xLeft) / (xLowMax - xLeft);

  EEPROM.write(6, yHighComp);
  delay(10);
  EEPROM.write(10, yLowComp);
  delay(10);
  EEPROM.write(14, xHighComp);
  delay(10);
  EEPROM.write(18, xLowComp);
  delay(10);
  EEPROM.write(22, xHighMax);
  delay(10);
  EEPROM.write(24, xLowMax);
  delay(10);
  EEPROM.write(26, yHighMax);
  delay(10);
  EEPROM.write(28, yLowMax);
  delay(10);
  EEPROM.commit();
  delay(10);

  setLedBlink(4,250,actionProperty[1].actionColorNumber,LED_BRIGHTNESS);

  Serial.println(" ");
  Serial.println("Joystick speed calibration procedure is complete.");
}

//***HOME JOYSTICK POSITION CALIBRATION FUNCTION***///

void joystickHomeCalibration(void) {

  setLedBlink(4,250,actionProperty[4].actionColorNumber,LED_BRIGHTNESS);

  xHigh = analogRead(X_DIR_HIGH_PIN);               //Set the initial neutral x-high value of joystick
  delay(10);
  Serial.println(xHigh);

  xLow = analogRead(X_DIR_LOW_PIN);                 //Set the initial neutral x-low value of joystick
  delay(10);
  Serial.println(xLow);

  yHigh = analogRead(Y_DIR_HIGH_PIN);               //Set the Initial neutral y-high value of joystick
  delay(10);
  Serial.println(yHigh);

  yLow = analogRead(Y_DIR_LOW_PIN);                 //Set the initial neutral y-low value of joystick
  delay(10);
  Serial.println(yLow);

  xRight = xHigh;
  xLeft = xLow;
  yUp = yHigh;
  yDown = yLow;

  int xMax = (xHighMax > xLowMax) ? xHighMax : xLowMax;
  int yMax = (yHighMax > yLowMax) ? yHighMax : yLowMax;
  float finalMax = (xMax > yMax) ? (float)xMax : (float)yMax;

  yHighComp = (finalMax - yUp) / (yHighMax - yUp);
  yLowComp = (finalMax - yDown) / (yLowMax - yDown);
  xHighComp = (finalMax - xRight) / (xHighMax - xRight);
  xLowComp = (finalMax - xLeft) / (xLowMax - xLeft);

  EEPROM.write(6, yHighComp);
  delay(10);
  EEPROM.write(10, yLowComp);
  delay(10);
  EEPROM.write(14, xHighComp);
  delay(10);
  EEPROM.write(18, xLowComp);
  delay(10);
  EEPROM.commit();
  delay(10);

  Serial.println("Home calibration complete.");

}

//***SET DEFAULT VALUES FUNCTION***///

void setDefault(void) {

  int defaultConfigSetup;
  int defaultCursorSetting;
  int defaultIsSet;
  float defaultCompFactor = 1.0;

  defaultIsSet=EEPROM.read(4);
  delay(10);

  if (defaultIsSet != 1) {

    defaultConfigSetup = 0;
    EEPROM.write(0, defaultConfigSetup);
    delay(10);

    defaultCursorSetting = 4;
    EEPROM.write(2, defaultCursorSetting);
    delay(10);

    EEPROM.write(6, defaultCompFactor);
    delay(10);

    EEPROM.write(10, defaultCompFactor);
    delay(10);

    EEPROM.write(14, defaultCompFactor);
    delay(10);

    EEPROM.write(18, defaultCompFactor);
    delay(10);

    defaultIsSet = 1;
    EEPROM.write(4, defaultIsSet);
    delay(10);
    EEPROM.commit();
    delay(10);
  }
}
