/*nowRailV2_0_1
30/05/2026

Additions
2.0.1
nowChannelUpdate() in customFunctions.ino gives wifiChannelUpdate info
2.0.0
Adds delayed functions. This allows a function/s to be triggered after a period of time

//bug fix 
PCA9685 leds...flashing flickering at start up when not commanded...fixed

MIT License
Copyright (c) 2026 Simon Coward
Copyright (c) 2026 Digital Town

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
    
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

void loop() {
  currentMillis = millis();
  myLayout.runLayout();//This sits in the main loop and needs to run as often as possible
  runWics(); // Wics addOn to nowRail
}
