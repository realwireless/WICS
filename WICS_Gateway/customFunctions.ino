/*nowRail V2_0_1 & WICS V1.0.1 for WICS GATEWAY Board
30/05/2026

   This tab contains custom functions that are called when nowrail receives commands.
   This allows users to write their own code driven by these events using the varibles passed.

   If not required all these functions can be commented out of the tab or deleted.

*/
// ============================================================================
// ### nowRail Custom Functions ### 
// ============================================================================

// ============================================================================
// WICS Helper: always create EXACT 21 char string for System Monitor
// ============================================================================
void formatLine21(char *out, const char *fmt, ...) {
  char temp[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf(temp, sizeof(temp), fmt, args);
  va_end(args);

  // Pad or trim to exactly 21 chars
  snprintf(out, 22, "%-21.21s", temp);
}

// ============================================================================
// ACCESSORY COMMAND RECEIVED
// ============================================================================
//myLayout.accProcessed(1);  //processed... required for panel update transmission
//myLayout.accProcessed(0);  //wasn't processed by this board so no panel update
// Serial.print("Accessory Command for AccNum:");
// Serial.print(accNum);
// Serial.print(" Dir:");
// Serial.println(accInst);

void nowAccComRec(int accNum, byte accInst) {
  char mLine[22];
  formatLine21(mLine, ">ACC %4d %d Requested", accNum, accInst);
  oledMonitor(mLine);
  // === Check if WICS GATEWAY.TXT have rule for channel change ===
  wics_checkAndTriggerChannelChange(accNum, accInst);
}

// ============================================================================
// PANEL UPDATE & Check if there is any Channel Change Rule active
// ============================================================================
  // Serial.print("Panel Update for AccNum:");
  // Serial.print(accNum);//The accessory number that has been processed
  // Serial.print(" Dir:");
  // Serial.println(accInst); //direction/instruction/direction completed

void nowPanelUpdate(int accNum, byte accInst) {
  char mLine[22];
  formatLine21(mLine, "<ACC %4d %d Processed", accNum, accInst);
  oledMonitor(mLine);
}

// ============================================================================
// SENSOR UPDATE
// ============================================================================
void nowSensorUpdate(int senNum, int32_t senInst) {
  char mLine[22];
  formatLine21(mLine, "SENS %4d %ld Received", senNum, senInst);
  oledMonitor(mLine);
}

// ============================================================================
// LOCO FUNCTION UPDATE
// ============================================================================
//Example loco decoder at https://youtu.be/GXlXMaAw16E
//receives loco function updates..for updating controllers
void nowLocoFuncUpdate(int locoAddr, byte nowFuncNum, byte nowFuncState) {
  char mLine[22];
  formatLine21(mLine, "LOCO %4d F%02d=%d", locoAddr, nowFuncNum, nowFuncState);
  oledMonitor(mLine);
}

// ============================================================================
// LOCO SPEED UPDATE
// ============================================================================
void nowLocoSpeedUpdate(int locoAddr, byte locoSpeed, byte locoDir) {
  char mLine[22];
  formatLine21(mLine, "LOCO %4d S%3d D%d", locoAddr, locoSpeed, locoDir);
  oledMonitor(mLine);
}

// ============================================================================
// nowTimeEvent 
// ============================================================================
//This function is called every layout second update (fast clock that can mean a lot of times per real world second )
//This function can be used to update layout clock either on controllers or layout scenic displays/clocks
//It can also be used to send commands at set times, this may control lights to come on/off at certain times in a building
//Function can be deleted
void nowTimeEvents(byte clockSpeed, byte clockHour, byte clockMinute, byte clockSecond, byte clockDay) {
  static byte lastSecond = 255;
  
  if (clockSecond != lastSecond) {
    lastSecond = clockSecond;
    
    // Check WICS Time Events
    checkTimeEvents(clockHour, clockMinute, clockSecond, clockDay);
  }
}

// ============================================================================
// nowChannelUpdate() - This function reports wifi channel updates
// ============================================================================
void nowChannelUpdate(uint8_t channelNum, uint8_t channelState) {
  Serial.print("wifi channel num: ");
  Serial.println(channelNum);
  
  if (currentWiFiChannel == channelNum) {
    return; 
  }

  if (channelState < 1) {
    Serial.println("Change command sent to Masterclock");
     currentWiFiChannel = channelNum; // This Board is MasterClock 
  } else {
    Serial.println("This board changing to new wifi channel");
    currentWiFiChannel = channelNum; // WICS System Monitor variable
    
    // Notify WICS WiFi (AP) to change channel
    if (wifiStatus == "on") {
      wics_configureWiFi(channelNum, false); // false = don't restart the webbservern
    }
  }
}



// ============================================================================

//Layout Clock----------------------------------------------------------------------------------------------------
//function called when clock speed change received... will usually be used for updating controller screens
void nowClockSpeedUpdate(){
  //update controller
  
  
}

//Layout Power Commands--------------------------------------------------------------
//Receives power commands that would usually be sent to DCC-EX by a line like myLayout.sendPowerCommand(DONEEMERGENCYSTOP);
//Function would be used by loco controllers to update power staus if needed.
void nowPowerCommand(byte Command) {
  Serial.print("nowPowerCommand: ");
  Serial.println(Command);
  //OPTIONS
  // #define TURNPOWEROFF 0...Turn power off
  // #define TURNPOWERON 1
  // #define TURNEMERGENCYSTOP 2
  // #define DONEPOWEROFF 3...power turned off
  // #define DONEPOWERON 4.
  // #define DONEEMERGENCYSTOP 5
}

//Loco controller data array--------------------------------------------------------------
//These functions allow data transfer between controllers usnig nowRail eeprom

//function called when bulk upload received and processed
void nowLocoBulkDataRX(){
  
}

//function called when single loco data has been received
void nowLocoDataSetRX(){
  
  
}

//GT911 Touch Screen-------------------------------------------
//This function shows the X and y pos of a GT911 screen touch
//Mainly used when setting up the GT911

//triggered by GT911 touch screen touches
//Allows custom functions to be written from GT911 screen presses
void nowGT911Touch(int xPos, int yPos) {
  Serial.println("custom function nowGT911Touch");
  Serial.print("xPos: ");
  Serial.print(xPos);
  Serial.print(" yPos: ");
  Serial.println(yPos);
}

//RFID data
//void nowRFIDDataRec(uint8_t incomingData[], int len){
void nowRFIDDataRec(uint8_t *incomingData, int len){
  Serial.print("RFID bytes received: ");
  Serial.println(len);
  
}

//Other buttons or code--------------------------------------------
//This function gets called every loop
//It allows you to check other buttons/sensors not built in
//Any button inherits nowRails debounce code
 void nowMomentButton(void) {
  /*
  Any custom button or sensor code can be placed in this function
  It is called everytime the void loop() runs.
  Code should control your own custom functions that can call nowRail functions
  */
 }


//Example custom function

//Example user written custom functions.

//CUSTOM Function
//this function allows users to trigger events based on times. It is not a built in function but shows how custom functions can be added.
//This is called from main loop and just calls various rtcClock...events to demonstrate some built in functions.
//Function can be deleted
/*
//Set up as an example of changing channels
int myCounter;
unsigned long timeMillis;
unsigned long timeTimer = 1000;
void myTimeEvents() {
  byte myHour;
  byte myMinute;
  byte mySeconds;
  byte myDay;
  byte myClockSpeed;

   if (currentMillis - timeMillis >= timeTimer) {
     timeMillis = currentMillis;
     myCounter++;
     Serial.println(myCounter);
     if(myCounter == 30){
      myLayout.changeWifiChannel(1);
     }
     if(myCounter == 60){
      myLayout.changeWifiChannel(6); 
     }
     if(myCounter == 90){
      myLayout.changeWifiChannel(11);
      myCounter = 0;
     }

    // myClockSpeed = myLayout.rtcClockSpeed();
    // Serial.print("Clock Speed: ");
    // Serial.print(myClockSpeed);

    // myHour = myLayout.rtcHours();
    // Serial.print(" Hours: ");
    // Serial.print(myHour);

    // myMinute = myLayout.rtcMinutes();
    // Serial.print(" Minutes: ");
    // Serial.print(myMinute);

    // mySeconds = myLayout.rtcSeconds();
    // Serial.print(" Seconds: ");
    // Serial.print(mySeconds);

    // myDay = myLayout.rtcDays();
    // Serial.print(" Day: ");
    // Serial.println(myDay); 
   }
}
*/

