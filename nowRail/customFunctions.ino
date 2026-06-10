/*nowRailV2_1_1
10/06/2026

   This tab contains custom functions that are called when nowrail receives commands.
   This allows users to write their own code driven by these events using the varibles passed.

   If not required all these functions can be commented out of the tab or deleted.

*/

//Accessory Commands------------------------------------------------------------------

//Accessory Command Received.. this function allows you to process NON built in commands
//or add extra custom features
//Accnum and accInst received. Custom fetures, animations can be called from this function
//Function can be deleted
void nowAccComRec(int accNum, byte accInst) {
  //myLayout.accProcessed(1);  //processed... required for panel update transmission
  //myLayout.accProcessed(0);  //wasn't processed by this board so no panel update
  // Serial.print("Accessory Command for AccNum:");
  // Serial.print(accNum);
  // Serial.print(" Dir:");
  // Serial.println(accInst);

  // switch (accNum) {
  //   case 70:  //accessory number 70
  //     if (accInst > 0) {
  //       //      Serial.println("Acc 70 moved 1");
  //     } else {
  //       //      Serial.println("Acc 70 moved 0");
  //     }
  //     break;
  //   default:
  //     break;
  // }
}

//Control panel response to Accessory commands----------------------------------------------------

//control panel update command received after an Accessory command has been completed
//The accNum and accInst are received and can be used for custom control panel functions
//Function can be deleted
void nowPanelUpdate(int accNum, byte accInst) {
  // Serial.print("Panel Update for AccNum:");
  // Serial.print(accNum);//The accessory number that has been processed
  // Serial.print(" Dir:");
  // Serial.println(accInst); //direction/instruction/direction completed

  //switch (accNum) {
    /*
    case 25: //accessory number 25
      if(accInst > 0){
          digitalWrite(ledLeftPin,HIGH);//control panel leds
          digitalWrite(ledRightPin,LOW);//control panel leds
      }else{
          digitalWrite(ledLeftPin,LOW);
          digitalWrite(ledRightPin,HIGH);
      }
      break;
    default:
      break;
      */
  //}
}

//Sensors---------------------------------------------------------------------------------

//As sensors are triggered this function allows users to trigger animations or anything the user requires.
//Outputs sensor updates
//1.8.1 now allows int32_t senInst
void nowSensorUpdate(int senNum, int32_t senInst) {
  //Uncomment to see sensor traffic
  // Serial.print("sensor: ");
  // Serial.print(senNum);
  // Serial.print(" : ");
  // Serial.println(senInst);
  //example of how to use sensor
  // switch(senNum){
  //   case 112:
  //     myLayout.sensorProcessed();//let system know I've dealt with it and stop repeating
  //     break;
  //   case 115:
  //     myLayout.sensorProcessed();//let system know I've dealt with it and stop repeating
  //     break;
  //   default:
  //     break;
  // }
}

//Layout Clock----------------------------------------------------------------------------------------------------

//function called when clock speed change received... will usually be used for updating controller screens
void nowClockSpeedUpdate(){
  //update controller
  
  
}


//This function is called every layout second update (fast clock that can mean a lot of times per real world second )
//This function can be used to update layout clock either on controllers or layout scenic displays/clocks
//It can also be used to send commands at set times, this may control lights to come on/off at certain times in a building
//Function can be deleted
void nowTimeEvents(byte clockSpeed, byte clockHour, byte clockMinute, byte clockSecond, byte clockDay) {
  
}

//Loco commands...................................................................
//Example loco decoder at https://youtu.be/GXlXMaAw16E
//receives loco function updates..for updating controllers
void nowLocoFuncUpdate(int locoAddr, byte nowFuncNum, byte nowFuncState){
 
}

//receives speed updates
void nowLocoSpeedUpdate(int locoAddr, byte locoSpeed, byte locoDir){
  //update controller or loco?
  // Serial.println("nowLocoSpeedUpdate: ");
  // Serial.println(locoAddr);
  // Serial.println(locoSpeed);
  // Serial.println(locoDir);

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
//This function reports wifi channel updates
void nowChannelUpdate(uint8_t channelNum, uint8_t channelState){ //channelNum = 1,6 or 11
  Serial.print("wifi channel num: ");
  Serial.println(channelNum);
  if(channelState < 1){
    Serial.println("Change command sent to Masterclock");
  }else{
    Serial.println("This board changing to new wifi channel");
  }
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

