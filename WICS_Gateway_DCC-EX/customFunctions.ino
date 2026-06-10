// ============================================================================
//                 ### WICS using nowRail Custom Functions ### 
// ============================================================================

// ============================================================================
// GLOBAL VARIABLES FOR ACC TIMEOUT AND STATUS TRACKING
// ============================================================================
int lastReqAccNum = -1;
byte lastReqAccInst = 0;
bool lastAccWasProcessed = true; 
unsigned long lastReqTime = 0;
const unsigned long ACC_TIMEOUT_MS = 2000; // 2 seconds timeout for response
int channelChangeAccNum = -1;             // Tracks DCC address for channel changes
int lastOledSenNum = -1;
int32_t lastOledSenInst = -1;
int lastOledLocoAddrFunc = -1;
byte lastOledFuncNum = 255;
byte lastOledFuncState = 255;
int lastOledLocoAddrSpeed = -1;
byte lastOledLocoSpeed = 255;
byte lastOledLocoDir = 255;

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
void nowAccComRec(int accNum, byte accInst) {
  char mLine[22]; 

  // Check if previous timed out
  if (!lastAccWasProcessed && lastReqAccNum != -1 && lastReqAccNum != accNum) {
    formatLine21(mLine, "<ACC %4d %d Failed", lastReqAccNum, lastReqAccInst);
    oledMonitor(mLine);
  }

  // Log new request
  if (lastReqAccNum != accNum || lastReqAccInst != accInst || lastAccWasProcessed) {
    lastReqAccNum = accNum;
    lastReqAccInst = accInst;
    lastAccWasProcessed = false; 
    lastReqTime = millis(); 

    formatLine21(mLine, ">ACC %4d %d Requested", accNum, accInst);
    oledMonitor(mLine);
  }

  // Save the address in case this request triggers a channel change
  channelChangeAccNum = accNum;

  // Check channel change rule (this will eventually trigger nowChannelUpdate)
  wics_checkAndTriggerChannelChange(accNum, accInst);

  myLayout.accProcessed(0); 
}

// ============================================================================
// PANEL UPDATE
// ============================================================================
void nowPanelUpdate(int accNum, byte accInst) {
  if (accNum == lastReqAccNum && accInst == lastReqAccInst) {
    lastAccWasProcessed = true; 
  }

  char mLine[22]; 
  formatLine21(mLine, "<ACC %4d %d Processed", accNum, accInst);
  oledMonitor(mLine);
}

// ============================================================================
// WICS TIMEOUT CHECKER (Called from WICS main loop)
// ============================================================================
void wics_checkAccTimeout() {
  if (!lastAccWasProcessed && lastReqAccNum != -1) {
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastReqTime >= ACC_TIMEOUT_MS) {
      lastAccWasProcessed = true; 
      
      Serial.print("[WICS TIMEOUT] No response for AccNum: ");
      Serial.println(lastReqAccNum);
      
      char mLine[22]; 
      formatLine21(mLine, "<ACC %4d %d Failed", lastReqAccNum, lastReqAccInst);
      oledMonitor(mLine);
    }
  }
}

// ============================================================================
// SENSOR UPDATE
// ============================================================================
void nowSensorUpdate(int senNum, int32_t senInst) {
  // Only print if the sensor number or its state actually changed
  if (senNum != lastOledSenNum || senInst != lastOledSenInst) {
    lastOledSenNum = senNum;
    lastOledSenInst = senInst;

    char mLine[22];
    formatLine21(mLine, "SENS %4d %ld Received", senNum, senInst);
    oledMonitor(mLine);
  }
}

// ============================================================================
// LOCO FUNCTION UPDATE
// ============================================================================
void nowLocoFuncUpdate(int locoAddr, byte nowFuncNum, byte nowFuncState) {
  // Only print if it is a new locomotive, a different function pin, or a state change
  if (locoAddr != lastOledLocoAddrFunc || nowFuncNum != lastOledFuncNum || nowFuncState != lastOledFuncState) {
    lastOledLocoAddrFunc = locoAddr;
    lastOledFuncNum = nowFuncNum;
    lastOledFuncState = nowFuncState;

    char mLine[22];
    formatLine21(mLine, "LOCO %4d F%02d=%d", locoAddr, nowFuncNum, nowFuncState);
    oledMonitor(mLine);
  }
}

// ============================================================================
// LOCO SPEED UPDATE
// ============================================================================
void nowLocoSpeedUpdate(int locoAddr, byte locoSpeed, byte locoDir) {
  // Only print if the locomotive address, speed value, or direction changed
  if (locoAddr != lastOledLocoAddrSpeed || locoSpeed != lastOledLocoSpeed || locoDir != lastOledLocoDir) {
    lastOledLocoAddrSpeed = locoAddr;
    lastOledLocoSpeed = locoSpeed;
    lastOledLocoDir = locoDir;

    char mLine[22];
    formatLine21(mLine, "LOCO %4d S%3d D%d", locoAddr, locoSpeed, locoDir);
    oledMonitor(mLine);
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
    currentWiFiChannel = channelNum; 
  } else {
    Serial.println("This board changing to new wifi channel");
    currentWiFiChannel = channelNum; 
    
    if (wifiStatus == "on") {
      wics_configureWiFi(channelNum, false); 
    }
  }

  // If a channel change was initiated by a DCC address, clear timeout and update OLED
  if (channelChangeAccNum != -1) {
    lastAccWasProcessed = true; // Stops the "Failed" timer instantly!
    
    char mLine[22]; 
    formatLine21(mLine, "<ACC %4d %d Processed", channelChangeAccNum, lastReqAccInst);
    oledMonitor(mLine);
    
    channelChangeAccNum = -1; // Reset tracking variable
  }
}
// ============================================================================
// Power Command to DCC-EX 
// ============================================================================
void nowPowerCommand(byte Command) {
  Serial.print("nowPowerCommand: ");
  Serial.println(Command);

  char mLine[22];
  bool validCommand = true;

  // Map the nowRail power states to clean 21-character monitor strings
  switch (Command) {
    case 0: // TURNPOWEROFF
      formatLine21(mLine, "PWR  Request OFF");
      break;
    case 1: // TURNPOWERON
      formatLine21(mLine, "PWR  Request ON");
      break;
    case 2: // TURNEMERGENCYSTOP
      formatLine21(mLine, "PWR  Request ESTOP");
      break;
    case 3: // DONEPOWEROFF
      formatLine21(mLine, "PWR  Status OFF");
      break;
    case 4: // DONEPOWERON
      formatLine21(mLine, "PWR  Status ON");
      break;
    case 5: // DONEEMERGENCYSTOP
      formatLine21(mLine, "PWR  Status ESTOP!!");
      break;
    default:
      validCommand = false;
      break;
  }

  // Only push to the OLED if it was a recognized power command
  if (validCommand) {
    oledMonitor(mLine);
  }
}

// ============================================================================
// nowTimeEvent 
// ============================================================================
void nowTimeEvents(byte clockSpeed, byte clockHour, byte clockMinute, byte clockSecond, byte clockDay) {
  static byte lastSecond = 255;
  
  if (clockSecond != lastSecond) {
    lastSecond = clockSecond;
    
    // Check WICS Time Events
    checkTimeEvents(clockHour, clockMinute, clockSecond, clockDay);
  }
}


// ============================================================================
// ============================================================================
// BELOW NOT USED BY WICS TODAY
// ============================================================================
// ============================================================================

//Layout Clock----------------------------------------------------------------------------------------------------
//function called when clock speed change received... will usually be used for updating controller screens
void nowClockSpeedUpdate(){
  //update controller
  
  
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

