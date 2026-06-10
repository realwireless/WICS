/*nowRailV2_1_1
10/06/2026

 2_1_1 modifications to panel reponse to update buttons that press once for 0 press agin for 1
 These buttons now change state if a different source changes the accessory.
 Apllies to stdPinButtons, CD4021pinButtons and GT911 touch screen buttons.

2_1_1 bug fix nowRail.cpp
nowRail::sendPanelUpdate(accNum, accInst); moved from inside
if (recFifoBuffer[recReadFifoCounter][MESSRESPONSE] == MESSRESPREQ) { line 1800
Panel update should send whatever the state of MESSRESPREQ 

Additions
2_1_0 ability to set custom SDA/SCL pins using
#define CUSTOM_SDA 4 //SDA cutom pin number
#define CUSTOM_SCL 5 //SCL custom pin number
This allows the use os ESP32 C3 and S3 boards

2.0.1
nowChannelUpdate() in customFunctions.ino gives wifiChannelUpdate info
2.0.0
Adds delayed functions. This allows a function/s to be triggered after a period of time

//bug fix 
PCA9685 leds...flashing flickering at start up when not commanded...fixed
    
*/
//This line sets up the system
#include "nowRail.h"  //include the nowRail class
//This is your layout unique ID code. Can stay as is but change if you will be using near other nowRail layouts.
nowRail myLayout(0x00, 0x01, 0x02, 0x03);

// Non-nowRail function declarations so nowRail.ino can see them across tabs
extern void initWics();
extern void runWics();

//NON Nowrail variables
unsigned long currentMillis = millis();


void setup() {
  Serial.begin(115200);//Standard Serial Output to Serial monitor
  //Serial.begin(115200, SERIAL_8N2);//SERIAL_8N2 required if JMRICMRI connection
  Serial.println(F(__FILE__ " " __DATE__ " " __TIME__));  //File details
  
  //Start the system
  myLayout.init();//This functions sets up ESP-NOW as well as other items needed for the system to run
  initWics(); // This function sets up WICS addOn for nowRail 
}

//unsigned long currentMillis;
unsigned long timerMillis;
uint16_t timerTime = 5000;
byte timerState;

void loop() {
  currentMillis = millis();
  myLayout.runLayout();//This sits in the main loop and needs to run as often as possible
  runWics(); // Wics addOn to nowRail
}
