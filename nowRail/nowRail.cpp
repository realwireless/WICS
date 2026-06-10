/*nowRailV2_1_1
10/06/2026
*/
#include "Arduino.h"
#include "nowRail.h"

uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  //broadcast to all boards

byte recWriteFifoCounter;
byte recFifoBuffer[256][PACKETLENGTH];  //Way more than needed but better safe than sorry



String nowRail::getnowRailAddr() {
  String myString;
  //_transmissionPrefix[0] = addr1;
  // _transmissionPrefix[1] = addr2;
  //_transmissionPrefix[2] = addr3;
  //_transmissionPrefix[3] = addr4;
  myString = "0x" + String(_transmissionPrefix[0], HEX) + " 0x" + String(_transmissionPrefix[1], HEX) + " 0x" + String(_transmissionPrefix[2], HEX) + " 0x" + String(_transmissionPrefix[3], HEX);
  return myString;
}

void nowRail::setnowRailAddr(byte element, int changeVal) {
  if (element < 4) {  //make sure valid segment
    _transmissionPrefix[element] = _transmissionPrefix[element] + changeVal;
  }
}

#if defined(MP3BUSYPIN)
void nowRail::addMP3PlayTrack(int accNum, int dirOn, int trackNum, int maxTrackNum, int mode) {
  if (_mp3NumAccs < MP3NUMTRACKS) {  //check that full number not used up
    _mp3Accessories[_mp3NumAccs][0] = accNum;
    _mp3Accessories[_mp3NumAccs][1] = dirOn;
    _mp3Accessories[_mp3NumAccs][2] = trackNum;
    _mp3Accessories[_mp3NumAccs][3] = maxTrackNum;
    _mp3Accessories[_mp3NumAccs][4] = mode;

    _mp3NumAccs++;
  } else {
    Serial.println("ERROR:addMP3PlayTrack Too many MP3Access added, increase the value of MP3NUMTRACKS in nowrail_user_setup.h");
  }
};

#endif

//delayed accessories
#if defined(NUMDELAYEDACCS)
//uint8_t _DelayAccs[NUMDELAYEDACCS][7];  //TriggerAccNum,TriggerAccDir,DelaAccNum,DelayAccDir,DelayTime,Triggerstate,TriggerMillis
//int _DelayAccsCount;
void nowRail::addDelayedAccTrigger(int triggerAccNum, byte triggerAccDir, int delayedAccNum, byte delayedAccDir, int delayTime) {

  if (_DelayAccsCount < NUMDELAYEDACCS) {  //check that full number not used up
    _DelayAccs[_DelayAccsCount][0] = triggerAccNum;
    _DelayAccs[_DelayAccsCount][1] = triggerAccDir;
    _DelayAccs[_DelayAccsCount][2] = delayedAccNum;
    _DelayAccs[_DelayAccsCount][3] = delayedAccDir;
    _DelayAccs[_DelayAccsCount][4] = delayTime;

    _DelayAccsCount++;
  } else {
    Serial.println("ERROR:addDelayedAccTrigger Too many delayed accessories added, increase the value of NUMDELAYEDACCS in nowrail_user_setup.h");
  }
}
#endif

//1.4.2  only runs if WIFIMASTERCLOCKCHANGE defined
#if defined(WIFIMASTERCLOCKCHANGE)
void nowRail::changeWifiChannel(byte newChannel) {
  //new channel must have a value of 1,6 or 11
  if (newChannel == 1 || newChannel == 6 || newChannel == 11) {
    memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);
    sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;  //Cannot be responded to because channel will change
    sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;             //0 as new message
    sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = WIFICHANNELCMD;   //Message type
    sendFifoBuffer[sendWriteFifoCounter][LOCOSPEED] = newChannel;
    nowRail::incsendWriteFifoCounter();
  }
}
#endif


void nowRail::sendPowerCommand(byte Command, int dccAddr, byte dccSpeed, byte dccDir) {  //sends power commands to basestations...1.2.2 int dccAddr, byte dccSpeed, byte dccDir added for NCE
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPREQ;         //Must get through
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;                 //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = LAYOUTPOWERCOMMANDS;  //14

  sendFifoBuffer[sendWriteFifoCounter][LOCOSPEED] = dccSpeed;
  sendFifoBuffer[sendWriteFifoCounter][LOCODIR] = dccDir;
  sendFifoBuffer[sendWriteFifoCounter][LOCOADDRHIGH] = (dccAddr >> 8) & 0xFF;  //get High Byte
  sendFifoBuffer[sendWriteFifoCounter][LOCOADDRLOW] = dccAddr & 0xFF;
  sendFifoBuffer[sendWriteFifoCounter][POWERCOMMAND] = Command;  //power command 16
  nowRail::incsendWriteFifoCounter();                            //new function to add highbyte to messageID
}

//adds a panel led 74HC595N
void nowRail::add74HC595NPPanelLed(int chip, int pin, int accNum, int dir) {
  if (_S74HC595NPanelLedPinCount < NUM74HC595NCHIPS * 8) {
    _S74HC595NPanelLedPins[_S74HC595NPanelLedPinCount][0] = chip;
    _S74HC595NPanelLedPins[_S74HC595NPanelLedPinCount][1] = pin;
    _S74HC595NPanelLedPins[_S74HC595NPanelLedPinCount][2] = accNum;
    _S74HC595NPanelLedPins[_S74HC595NPanelLedPinCount][3] = dir;
    _S74HC595NPanelLedPinCount++;
  } else {
    Serial.println("ERROR:add74HC595NPPanelLed to many stdPin LED's");
  }
}


//stdPanelLED
// int _StdPanelLed[20][2];//only 20 allowed...not many pins
//   int _StdPanelLedCount;//keeps track of how many stdPanelLed's have been added
void nowRail::addStdPanelLed(int pin, int accNum, int dir) {
  if (_StdPanelLedCount < 20) {  //max std pins is 20
    _StdPanelLed[_StdPanelLedCount][0] = pin;
    _StdPanelLed[_StdPanelLedCount][1] = accNum;
    _StdPanelLed[_StdPanelLedCount][2] = dir;
    pinMode(pin, OUTPUT);
    _StdPanelLedCount++;
  } else {
    Serial.println("ERROR:addStdPanelLed to many stdPin LED's, max 20");
  }
}


//sends out a DCC speed command
void nowRail::sendDCCLocoSpeed(int dccAddr, byte dccSpeed, byte dccDir) {
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageLOW BYTE ID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;          //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = DCCLOCOSPEED;  //DCCLOCOSPEED 14
  sendFifoBuffer[sendWriteFifoCounter][LOCOSPEED] = dccSpeed;
  sendFifoBuffer[sendWriteFifoCounter][LOCODIR] = dccDir;
  sendFifoBuffer[sendWriteFifoCounter][LOCOADDRHIGH] = (dccAddr >> 8) & 0xFF;  //get High Byte
  sendFifoBuffer[sendWriteFifoCounter][LOCOADDRLOW] = dccAddr & 0xFF;
  nowRail::incsendWriteFifoCounter();  //new function to add highbyte to messageID
}

//send DCC loco function command
void nowRail::sendDCCLocoFunc(byte locoID, int dccAddr, byte funcNum, byte funcState) {
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageLOW BYTE ID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;             //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = DCCLOCOFUNCTION;  //DCCLOCOFUNCTION
  sendFifoBuffer[sendWriteFifoCounter][LOCOFUNC] = funcNum;
  sendFifoBuffer[sendWriteFifoCounter][LOCOFUNCSTATE] = funcState;
  sendFifoBuffer[sendWriteFifoCounter][LOCOADDRHIGH] = (dccAddr >> 8) & 0xFF;  //get High Byte
  sendFifoBuffer[sendWriteFifoCounter][LOCOADDRLOW] = dccAddr & 0xFF;
  //version 7 mods to function... sending extra data so that NCE system can send correct function updates, requires multiple items per call
  sendFifoBuffer[sendWriteFifoCounter][20] = _locoData[locoID][4];   //function data
  sendFifoBuffer[sendWriteFifoCounter][21] = _locoData[locoID][5];   //function data
  sendFifoBuffer[sendWriteFifoCounter][22] = _locoData[locoID][6];   //function data
  sendFifoBuffer[sendWriteFifoCounter][23] = _locoData[locoID][7];   //function data
  sendFifoBuffer[sendWriteFifoCounter][24] = _locoData[locoID][8];   //function data
  sendFifoBuffer[sendWriteFifoCounter][25] = _locoData[locoID][9];   //function data
  sendFifoBuffer[sendWriteFifoCounter][26] = _locoData[locoID][10];  //function data
  sendFifoBuffer[sendWriteFifoCounter][27] = _locoData[locoID][11];  //function data
  // end mods
  nowRail::incsendWriteFifoCounter();  //new function to add highbyte to messageID
}




#if defined(NOWDisc)  //These functions are only needed for eeprom
//EEPROM Functions
void nowRail::useEEPROM(byte discAddress) {  //sets up system to use EEPROM for time storage
  int q;
  int w;
  byte myByte;
  byte byteArray[32];
  int junk;
#if defined(CUSTOM_SDA)
  Wire.begin(CUSTOM_SDA, CUSTOM_SCL);  //custom pins
#else
  Wire.begin();        //standard pins
#endif
  delay(100);

  q = 0;
  Serial.println("Read Wire");
  junk = nowRail::EEPROMRead(NOWDisc, 0, 32);  //Read from disk1 starting at an address and read 32 bytes of data
  while (Wire.available()) {                   // slave may send less than requested
    byteArray[q] = Wire.read();
    q++;
  }
  for (q = 0; q < 32; q++) {
    Serial.println(byteArray[q]);
  }

  Serial.println("End Read Wire");



  if (byteArray[10] != EEPROMVERSION) {  //Check if EEPROM is running latest version in byte 10
    Serial.print("EEPROMVERSION: ");
    Serial.println(byteArray[10]);
    //Version 1 Sets first 32 bytes to Zero apart from
    //0 Clock Speed = 1
    //10 EEPROM Version = EEPROMVERSION
    // EEPROM 0-128 System reserved
    // 0 clock speed
    // 1 hour
    // 2 min
    // 3 second
    // 4 day
    // 5 //reserved date day
    // 6 // reserved date month
    // 7 //reserved date year HIGH
    // 8 //reserved date year LOW

    // 10 Check byte... EEPROM Version

    //The sets up LOCO data array starting at pos 1000
    // 0:16 reserved...possible loco ID HIGH
    // 1:17 reserved...possible loco ID LOW
    // 2:18 Loco High
    // 3:19 Loco Low
    // 4:20 func 0-7
    // 5:21 func 8-15
    // 6:22 func 16-23
    // 7:23 func
    // 8:24 func
    // 9:25 func
    // 10:26 func
    // 11:27 func...allows 64 functions...DCC may change in future
    // 12:28 consist 0 none fwd 1 none rev 2 const fwd 3 const rev
    // 13 - 31:29-49 LocoName




    //if (byteArray[10] != EEPROMVERSION) {  //probably a new chip so set up as set of data
    Serial.println("EEPROM being configured for first use");
    for (q = 0; q < 32; q++) {
      nowRail::EEPROMWrite(NOWDisc, q, q);
    }
    // nowRail::EEPROMWrite(NOWDisc, 0, 1);
    nowRail::EEPROMWrite(NOWDisc, 0, 3);
    nowRail::EEPROMWrite(NOWDisc, 1, 6);
    nowRail::EEPROMWrite(NOWDisc, 2, 9);
    nowRail::EEPROMWrite(NOWDisc, 10, EEPROMVERSION);

    Serial.println("EEPROM LocoArray being set up, be patient");
    for (q = 0; q < 200; q++) {                                              //work through 200 locos
      for (w = 0; w < 13; w++) {                                             //write first parts as 0's
        nowRail::EEPROMWrite(NOWDisc, EEPROMLOCODATASTART + q * 32 + w, 0);  //set value to Zero
      }
      for (w = 13; w < 32; w++) {                                             //write first parts as 0's
        nowRail::EEPROMWrite(NOWDisc, EEPROMLOCODATASTART + q * 32 + w, 32);  //Ascii for Space
      }
      Serial.print(q);
      Serial.println(" of 200");
    }

    Serial.println("EEPROM configured");
    //}
    //no data so put some info into the array
    _timeArray[TIMEARRAYCLOCKSPEED] = 1;  //set the clock speed to 1 so it's seen to be working


  } else {  //if valid data use it
    _timeArray[TIMEARRAYCLOCKSPEED] = byteArray[TIMEARRAYCLOCKSPEED];
    _timeArray[TIMEARRAYCLOCKHOUR] = byteArray[TIMEARRAYCLOCKHOUR];
    if (_timeArray[TIMEARRAYCLOCKHOUR] > 23) {
      _timeArray[TIMEARRAYCLOCKHOUR] = 0;
    }
    _timeArray[TIMEARRAYCLOCKMIN] = byteArray[TIMEARRAYCLOCKMIN];
    _timeArray[TIMEARRAYCLOCKSECONDS] = byteArray[TIMEARRAYCLOCKSECONDS];
    _timeArray[TIMEARRAYCLOCKDAY] = byteArray[TIMEARRAYCLOCKDAY];
    if (_timeArray[TIMEARRAYCLOCKDAY] > 6) {
      _timeArray[TIMEARRAYCLOCKDAY] = 0;
    }

    //get LocoReCallData
    Serial.println("Getting loco ReCall Data from EEPROM");
    w = 0;
    for (q = 0; q < 10; q++) {                                             //work through
      junk = nowRail::EEPROMRead(NOWDisc, EEPROMLOCORECALLDATASTART, 32);  //get 32 bytes...last 32 locos
      while (Wire.available()) {                                           // slave may send less than requested
        _locoReCallData[w][0] = Wire.read();
        _locoReCallData[w][1] = 128;        //set the speed to 0 forwards                              //0.9 mod                               //read the bytes into the array
        if (_locoReCallData[w][0] > 199) {  ////0.9 mod
          _locoReCallData[w][0] = 0;
        }
        w++;
      }
    }


    //Now get the loco data and put in _locoData[200][32]
    Serial.println("Getting loco Data from EEPROM");
    for (q = 0; q < 200; q++) {
      w = 0;                                                                  //rest for next loco
      junk = nowRail::EEPROMRead(NOWDisc, q * 32 + EEPROMLOCODATASTART, 32);  //Read from disk1 starting at an address and read 32 bytes of data
      while (Wire.available()) {                                              // slave may send less than requested
        _locoData[q][w] = Wire.read();
        w++;
      }
      Serial.print(q);
      Serial.println(" of 200");
    }
  }
}

void nowRail::timeUpdateEEPROM() {
  if (_locoReCallPos > 0) {                             //if not 0
    if (_currentMillis - _locoReCallMillis >= 15000) {  //and 15 secs of no activity
      _locoReCallPos = 0;                               //reset to 0
    }
  }
#if defined(NOWDisc)
  int q;
  if (_currentMillis - _EEPROMMillis >= _EEPROMTimer) {
    _EEPROMMillis = _currentMillis;
    for (q = 0; q < 5; q++) {
      nowRail::EEPROMWrite(NOWDisc, q, _timeArray[q]);
    }
  }
  //if the _locoReCallDataFlag is set update the eeprom
  if (_locoReCallDataFlag > 0) {
    for (q = 0; q < 32; q++) {
      nowRail::EEPROMWrite(NOWDisc, EEPROMLOCORECALLDATASTART + q, _locoReCallData[q][0]);  //update the eeprom
    }
    _locoReCallDataFlag = 0;
  }
#endif
}
//updates the loco data in eeprom
void nowRail::locoEEPROMUpdate(byte locoID) {
#if defined(NOWDisc)
  int q;
  for (q = 0; q < 32; q++) {
    // Serial.print(q);
    // Serial.print(" F: ");
    // Serial.println(_locoData[locoID][q]);
    nowRail::EEPROMWrite(NOWDisc, EEPROMLOCODATASTART + (32 * locoID) + q, _locoData[locoID][q]);
  }
#endif
}

//0.9 modded speed added update the locoReCallUpdate array
//the goal of this function is to move the latest selected loco to position 0
void nowRail::locoReCallDataUpdate(byte theLoco, byte theLocoSpeed) {  ////function that updates _locoReCallData[32] array
  int q;
  int moveSteps = 30;
  if (theLoco != _locoReCallData[0][0]) {  //only update if latest loco is not the last loco
    //work out how many items need to be moved... does it already exist
    for (q = 0; q < 30; q++) {
      if (_locoReCallData[q][0] == theLoco) {
        moveSteps = q - 1;
        q = 30;  //to exir for loop
      }
    }
    //if it already exists move only items up to that loco
    for (q = moveSteps; q > -1; q--) {
      _locoReCallData[q + 1][0] = _locoReCallData[q][0];  //loco id
      _locoReCallData[q + 1][1] = _locoReCallData[q][1];  //loco speed
    }
    //insert the new item in position zero
    _locoReCallData[0][0] = theLoco;
    _locoReCallData[0][1] = theLocoSpeed;
    _locoReCallDataFlag = 1;
    _locoReCallMillis = _currentMillis;  //reset timer so eeprom will get updated


  } else {
    _locoReCallData[0][1] = theLocoSpeed;  //if just the speed is updated eeprom not updated
  }
}

//0.9 modded gets the next loco to be recalled and puts in locoReCallLocoData[]
//data on which loco was last recalled moved outside function
//This allows people to display the recall list if required
byte nowRail::locoReCallGetLocoID(byte ReCallPos) {
  byte returnLocoID;
  if (ReCallPos < 30) {
    returnLocoID = _locoReCallData[ReCallPos][0];
  }
  return returnLocoID;
}

//0.9 mod
byte nowRail::locoReCallGetLocoSpeed(byte ReCallPos) {
  byte returnLocoSpeed;
  if (ReCallPos < 30) {
    returnLocoSpeed = _locoReCallData[ReCallPos][1];
  }
  return returnLocoSpeed;
}


int nowRail::EEPROMRead(int disk, int startdatareadaddress, int numbytes) {  //numbytes cannot be bigger than 32
  if (numbytes < 33) {
    Wire.beginTransmission(disk);
    //splits the address into 2 bytes for transmission
    Wire.write(startdatareadaddress >> 8);    // MSBthese two lines make sure the address gets sent correctly
    Wire.write(startdatareadaddress & 0xFF);  // LSB
    Wire.endTransmission();
    Wire.requestFrom(disk, numbytes);  // request bytes from slave device
    return 1;
  } else {
    Serial.println("EEPROM Read error, too many bytes requested");
    return 0;
  }
}

//this function writes to an eeprom and location with some passed data.
void nowRail::EEPROMWrite(int disk, int eepromaddress, byte eepromdata) {
  Wire.beginTransmission(disk);
  Wire.write((eepromaddress >> 8));    // MSBthese two lines make sure the address gets sent correctly
  Wire.write((eepromaddress & 0xFF));  // LSB
  Wire.write(eepromdata);              //data
  Wire.endTransmission();
  delay(10);  //required to give it time to write
}
//end eeprom functions

#endif




#if defined(MAXPCA9685SERVOBOARDS)


//function to set led brightness..TOWRITE
void nowRail::setPCA695Led(byte boardAddress, byte port, int brightness) {
  int error;
  if (port < 16) {
    if (brightness > 4095) {
      brightness = 4095;
    }
    Wire.beginTransmission(boardAddress);  //Start transmissions to board
    Wire.write((port * 4) + 6);            //6 = port 0.............               sets the start transmission address
    Wire.write(0x00);                      //pulse start time..using fixed on time... writes date to addrsss and increments...start time low byte
    Wire.write(0x00);                      //pulse start time                         writes date to addrsss and increments.... high byte...both zero in this example
    Wire.write(byte(brightness));          //value between 0 and 4095..writes date to addrsss and increments...finish time low byte
    Wire.write(byte(brightness >> 8));     //high byte              writes date to addrsss and increments...finish time high byte byte
    error = Wire.endTransmission();        //end transmission, check if any errores occured with the writes...0 = good news.


  } else {
    Serial.println("invalid PCA9685 port Max = 15");
  }
}

//led control
void nowRail::pca9685LedControl(void) {  //controls all leds effects..turns on off...also for panel leds
  int q;
  int effectBrightness;
  //deals with turning on/off
  for (q = 0; q < _pca9685LEDCount; q++) {                                                  //work through all leds
    if (_pca9685LEDStates[q][0] != _pca9685LEDStates[q][1]) {                               //change taken place so do something
      if (_pca9685LEDStates[q][0] == 0) {                                                   //turn off led
        nowRail::setPCA695Led(_pca9685LEDS[q][0], _pca9685LEDS[q][1], 0);                   //set LED off
      } else {                                                                              //turn it on
        nowRail::setPCA695Led(_pca9685LEDS[q][0], _pca9685LEDS[q][1], _pca9685LEDS[q][5]);  //set to max brightness
        _pca9685LEDStates[q][2] = 0;                                                        //1_9_0 mod resets state as falshing pair can be offset
      }
      _pca9685LEDStates[q][1] = _pca9685LEDStates[q][0];
    }

    //now for effects
    // if (_pca9685LEDStates[q][1] > 0) {  //if the LED is ON
    //bug fix version 2_0_0 stops effects activatiing at start up without command
    if (_pca9685LEDStates[q][1] > 0 && _pca9685LEDStates[q][1] != 255) {  //if the LED is ON and NOT in start up mode (255)
      switch (_pca9685LEDS[q][4]) {
        case 1:  //fire flicker
          if (_currentMillis - _pca9685LEDTimers[q][0] >= _pca9685LEDTimers[q][1]) {
            _pca9685LEDTimers[q][0] = _currentMillis;
            _pca9685LEDTimers[q][1] = random(500);                                            //set a random change timer
            effectBrightness = random(_pca9685LEDS[q][6], _pca9685LEDS[q][5]);                //create random brightness
            nowRail::setPCA695Led(_pca9685LEDS[q][0], _pca9685LEDS[q][1], effectBrightness);  //set led brightness
          }
          break;
        case 2:  //gas lights
          if (_currentMillis - _pca9685LEDTimers[q][0] >= _pca9685LEDTimers[q][1]) {
            _pca9685LEDTimers[q][0] = _currentMillis;
            if (_pca9685LEDStates[q][2] > 0) {             //0 = bright..primary, 1 = secondary
              _pca9685LEDTimers[q][1] = random(200, 500);  //if going dull short flicker
              _pca9685LEDStates[q][2] = 0;
              effectBrightness = random(_pca9685LEDS[q][6], _pca9685LEDS[q][5]);  //random between values
            } else {
              _pca9685LEDTimers[q][1] = random(3000, 10000);  //if bright long constant
              _pca9685LEDStates[q][2] = 1;
              effectBrightness = _pca9685LEDS[q][5];  //max brightness
            }
            nowRail::setPCA695Led(_pca9685LEDS[q][0], _pca9685LEDS[q][1], effectBrightness);  //set led brightness
          }
          break;
        case 3:  //arc welder
          if (_currentMillis - _pca9685LEDTimers[q][0] >= _pca9685LEDTimers[q][1]) {
            _pca9685LEDTimers[q][0] = _currentMillis;
            _pca9685LEDTimers[q][1] = random(10, 40);  //fast flicker
            if (_pca9685LEDStates[q][2] > 0) {         //0 = bright..primary, 1 = secondary
              _pca9685LEDStates[q][2] = 0;
              effectBrightness = _pca9685LEDS[q][6];  //effect value
            } else {
              _pca9685LEDStates[q][2] = 1;
              effectBrightness = _pca9685LEDS[q][5];  //max brightness
            }
            nowRail::setPCA695Led(_pca9685LEDS[q][0], _pca9685LEDS[q][1], effectBrightness);  //set led brightness
          }
          break;
          //1_9_0 additions
          //working here... why flashing together?
        case 4:  //flashing LED for reverse flash swap min/max
          if (_currentMillis - _pca9685LEDTimers[q][0] >= _pca9685LEDTimers[q][1]) {
            _pca9685LEDTimers[q][0] = _currentMillis;
            _pca9685LEDTimers[q][1] = PCA9685FLASHTIMER;  //next change interval
            if (_pca9685LEDStates[q][2] > 0) {            //0 = bright..primary, 1 = secondary
              _pca9685LEDStates[q][2] = 0;
              effectBrightness = _pca9685LEDS[q][6];  //effect value
            } else {
              _pca9685LEDStates[q][2] = 1;
              effectBrightness = _pca9685LEDS[q][5];  //max brightness
            }
            nowRail::setPCA695Led(_pca9685LEDS[q][0], _pca9685LEDS[q][1], effectBrightness);  //set led brightness
          }
          break;

          //end 1_9_0 mods
        default:
          break;
      }
    }
  }
  //panel LED's
  for (q = 0; q < _pca9685PanelLEDCount; q++) {                                                            //work through all leds
    if (_pca9685PanelLEDStates[q][0] != _pca9685PanelLEDStates[q][1]) {                                    //change taken place so do something
      if (_pca9685PanelLEDStates[q][0] == 0) {                                                             //turn off led
        nowRail::setPCA695Led(_pca9685PanelLEDS[q][0], _pca9685PanelLEDS[q][1], 0);                        //set LED off
      } else {                                                                                             //turn it on
        nowRail::setPCA695Led(_pca9685PanelLEDS[q][0], _pca9685PanelLEDS[q][1], _pca9685PanelLEDS[q][5]);  //set to max brightness
      }
      _pca9685PanelLEDStates[q][1] = _pca9685PanelLEDStates[q][0];
    }

    //now for effects
    if (_pca9685PanelLEDStates[q][1] > 0) {  //if the LED is ON
      if (_pca9685PanelLEDS[q][4] > 0) {
        //1.9.0 mod
        if (_currentMillis - _pca9685PanelLEDTimers[q] >= PCA9685FLASHTIMER) {  //1 sec iontervals for flashing

          _pca9685PanelLEDTimers[q] = _currentMillis;
          if (_pca9685PanelLEDStates[q][2] > 0) {  //0 = bright..primary, 1 = secondary
            _pca9685PanelLEDStates[q][2] = 0;
            effectBrightness = 0;  //random between values
          } else {
            _pca9685PanelLEDStates[q][2] = 1;
            effectBrightness = _pca9685PanelLEDS[q][5];  //max brightness
          }
          nowRail::setPCA695Led(_pca9685PanelLEDS[q][0], _pca9685PanelLEDS[q][1], effectBrightness);  //set led brightness
        }
      }
    }
  }
}

//just like addPCA9685Led but for panel leds
void nowRail::addPCA9685PanelLed(byte board, byte port, int accNum, int dirOn, int effect, int maxBright) {  //panel led
  byte validLed = 0;
  byte existingBoard = 0;
  int q;

  _pca9685PanelLEDS[_pca9685PanelLEDCount][0] = board;
  _pca9685PanelLEDS[_pca9685PanelLEDCount][1] = port;
  _pca9685PanelLEDS[_pca9685PanelLEDCount][2] = accNum;
  _pca9685PanelLEDS[_pca9685PanelLEDCount][3] = dirOn;
  _pca9685PanelLEDS[_pca9685PanelLEDCount][4] = effect;
  _pca9685PanelLEDS[_pca9685PanelLEDCount][5] = maxBright;

  if (port < 0 || port > 15) {
    Serial.println("ERROR:addPCA9685Led invalid port address");
    validLed = 1;
    existingBoard = 1;  //dont add new board
  }

  for (q = 0; q < _pca9685AddressesCount; q++) {
    if (_pca9685Addresses[q][0] == _pca9685PanelLEDS[_pca9685PanelLEDCount][0]) {  //0_9_2 mod
      existingBoard = 1;
      //now to check if this is a servo board 0_9_2 mod
      if (_pca9685Addresses[q][1] != 1) {  //led boards are 1
        Serial.println("ERROR:addPCA9685PanelLed invalid board address... address already in use for servos");
        validLed = 1;       // DONT ADD SERVO
        existingBoard = 1;  //DONT ADD NEW BOARD
      }
    }
  }

  //Adding new board
  if (existingBoard < 1) {
    if (_pca9685AddressesCount < MAXPCA9685SERVOBOARDS) {
      _pca9685Addresses[_pca9685AddressesCount][0] = byte(_pca9685PanelLEDS[_pca9685PanelLEDCount][0]);  //store new board address in array
      _pca9685Addresses[_pca9685AddressesCount][1] = 1;
      //now set it up as a servo board
      if (_pca9685AddressesCount < 1) {
#if defined(CUSTOM_SDA)
        Wire.begin(CUSTOM_SDA, CUSTOM_SCL);  //custom pins
#else
        Wire.begin();  //standard pins
#endif
        delay(100);
      }
      nowRail::setupPCA9685Board(_pca9685Addresses[_pca9685AddressesCount][0], _pca9685Addresses[_pca9685AddressesCount][1]);  //0_9_2 mod
      _pca9685AddressesCount++;
    } else {  //0_9_0 mod...error message triggering at wrong time
      Serial.println("ERROR:addPCA9685PanelLed to many boards increase MAXPCA9685SERVOBOARDS in user_setup.h");
    }
  }

  if (validLed < 1) {
    _pca9685PanelLEDStates[_pca9685PanelLEDCount][0] = 255;  //set to an unset value so whatevr trigger it responds
    _pca9685PanelLEDStates[_pca9685PanelLEDCount][1] = 255;  //set to an unset value so whatevr trigger it responds
    _pca9685PanelLEDCount++;
  }
}

//adds led on pca 9685 to system 0_9_2 mod
void nowRail::addPCA9685Led(byte board, byte port, int accNum, int dirOn, int effect, int maxBright, int effectBright) {  //dir)n = 0 or 1 to turn on, effect 0 = on/0ff, 1 = fire flicker, 2 = gas light, 3 = arc welder
  //int _pca9685LEDS[MAXPCA9685SERVOBOARDS][7];//board, port, accNum,dirOn,effect Type, max brightness, effect brightness
  byte validLed = 0;
  byte existingBoard = 0;
  int q;
  //int _pca9685LEDS[MAXPCA9685SERVOBOARDS][7];//board, port, accNum,dirOn,effect Type, max brightness, effect brightness
  //int _pca9685LEDCount;
  _pca9685LEDS[_pca9685LEDCount][0] = board;
  _pca9685LEDS[_pca9685LEDCount][1] = port;
  _pca9685LEDS[_pca9685LEDCount][2] = accNum;
  _pca9685LEDS[_pca9685LEDCount][3] = dirOn;
  _pca9685LEDS[_pca9685LEDCount][4] = effect;
  _pca9685LEDS[_pca9685LEDCount][5] = maxBright;
  _pca9685LEDS[_pca9685LEDCount][6] = effectBright;

  //_pca9685LEDStates[_pca9685LEDCount][0] = 0;
  //nowRail::setPCA695Led(_pca9685LEDS[q][0], _pca9685LEDS[q][1], 0);

  if (port < 0 || port > 15) {
    Serial.println("ERROR:addPCA9685Led invalid port address");
    validLed = 1;
    existingBoard = 1;  //dont add new board
  }

  for (q = 0; q < _pca9685AddressesCount; q++) {
    if (_pca9685Addresses[q][0] == _pca9685LEDS[_pca9685LEDCount][0]) {  //0_9_2 mod
      existingBoard = 1;
      //now to check if this is a servo board 0_9_2 mod
      if (_pca9685Addresses[q][1] != 1) {  //led boards are 1
        Serial.println("ERROR:addPCA9685Led invalid board address... address already in use for servos");
        validLed = 1;       // DONT ADD SERVO
        existingBoard = 1;  //DONT ADD NEW BOARD
      }
    }
  }

  //Adding new board
  if (existingBoard < 1) {
    if (_pca9685AddressesCount < MAXPCA9685SERVOBOARDS) {
      _pca9685Addresses[_pca9685AddressesCount][0] = byte(_pca9685LEDS[_pca9685LEDCount][0]);  //store new board address in array
      _pca9685Addresses[_pca9685AddressesCount][1] = 1;
      //now set it up as a servo board
      if (_pca9685AddressesCount < 1) {
#if defined(CUSTOM_SDA)
        Wire.begin(CUSTOM_SDA, CUSTOM_SCL);  //custom pins
#else
        Wire.begin();  //standard pins
#endif
        delay(100);
      }
      nowRail::setupPCA9685Board(_pca9685Addresses[_pca9685AddressesCount][0], _pca9685Addresses[_pca9685AddressesCount][1]);  //0_9_2 mod
      _pca9685AddressesCount++;
    } else {  //0_9_0 mod...error message triggering at wrong time
      Serial.println("ERROR:addPCA9685Led to many boards increase MAXPCA9685SERVOBOARDS in user_setup.h");
    }
  }

  if (validLed < 1) {
    _pca9685LEDStates[_pca9685LEDCount][0] = 255;  //set to an unset value so whatevr trigger it responds
    _pca9685LEDStates[_pca9685LEDCount][1] = 255;  //set to an unset value so whatevr trigger it responds
                                                   // nowRail::setPCA695Led(_pca9685LEDS[_pca9685LEDCount][0], _pca9685LEDS[_pca9685LEDCount][1], 0);
    _pca9685LEDCount++;
  }
}


//adds servos into system
//mod 1.0.3 moveTime added
void nowRail::addPCA9685Servo(byte board, byte port, int accNum, int angle0, int angle1, int moveTime) {
  byte validServo = 0;
  byte existingBoard = 0;
  int q;
  //1.0.3 slow move mods
  int millisPerStep;
  int moveAngle;
  if (moveTime > 0) {
    if (angle0 > angle1) {
      moveAngle = angle0 - angle1;  //work out how may degrees of move
    } else {
      moveAngle = angle1 - angle0;  ////work out how may degrees of move
    }
    millisPerStep = moveTime / moveAngle;
  } else {
    millisPerStep = 0;  //if no speed it's an instant 0
  }
  //end 1.0.3 mods
  _pca9685Servos[_pca9685ServoCount][0] = board;
  _pca9685Servos[_pca9685ServoCount][1] = port;
  _pca9685Servos[_pca9685ServoCount][2] = accNum;
  _pca9685Servos[_pca9685ServoCount][3] = angle0;
  _pca9685Servos[_pca9685ServoCount][4] = angle1;
  _pca9685Servos[_pca9685ServoCount][5] = millisPerStep;  //millis per step... 1.0.3 mod
  _pca9685Servos[_pca9685ServoCount][6] = 90;             //preset the servo current angle...1.3.1 mod
  _pca9685Servos[_pca9685ServoCount][7] = 90;             //preset the servo move to angle...1.3.1 mod
  for (q = 0; q < 6; q++) {
    Serial.print("A: ");
    Serial.print(_pca9685Servos[_pca9685ServoCount][q]);
    Serial.print(",");
  }
  Serial.println(" ");
  if (port < 0 || port > 15) {
    Serial.println("ERROR:addPCA9685Servo invalid port address");
    validServo = 1;
    existingBoard = 1;  //dont add new board
  }
  for (q = 0; q < _pca9685AddressesCount; q++) {
    if (_pca9685Addresses[q][0] == _pca9685Servos[_pca9685ServoCount][0]) {  //0_9_2 mod
      existingBoard = 1;
      //now to check if this is a servo board 0_9_2 mod
      if (_pca9685Addresses[q][1] != 0) {  //servo boards are 0
        Serial.println("ERROR:addPCA9685Servo invalid board address... address already in use for LED's");
        validServo = 1;     // DONT ADD SERVO
        existingBoard = 1;  //DONT ADD NEW BOARD
      }
    }
  }
  //Not an existing board and max boards not added yet


  //Adding new board
  if (existingBoard < 1) {
    if (_pca9685AddressesCount < MAXPCA9685SERVOBOARDS) {
      _pca9685Addresses[_pca9685AddressesCount][0] = byte(_pca9685Servos[_pca9685ServoCount][0]);  //store new board address in array
      _pca9685Addresses[_pca9685AddressesCount][1] = 0;
      //now set it up as a servo board
      if (_pca9685AddressesCount < 1) {
#if defined(CUSTOM_SDA)
        Wire.begin(CUSTOM_SDA, CUSTOM_SCL);  //custom pins
#else
        Wire.begin();  //standard pins
#endif
        delay(100);
      }
      nowRail::setupPCA9685Board(_pca9685Addresses[_pca9685AddressesCount][0], _pca9685Addresses[_pca9685AddressesCount][1]);  //0_9_2 mod
      _pca9685AddressesCount++;
    } else {  //0_9_0 mod...error message triggering at wrong time
      Serial.println("ERROR:addPCA9685Servo to many boards increase MAXPCA9685SERVOBOARDS in user_setup.h");
    }
  }


  if (validServo < 1) {
    _pca9685ServoCount++;
  }
}

//1.0.3 mod for PCAp685 servo movement control...slow speed etc.. gets called from main loop
//works like the led timings

void nowRail::pca9685ServoControl() {
  int q;
  for (q = 0; q < _pca9685ServoCount; q++) {             //work through all the servos and see if any need to move
    if (_pca9685Servos[q][6] != _pca9685Servos[q][7]) {  //if target is not equal to the current position...needs to moveAngle
      //1.9.1 mod reset detach flag
      _pca9685Servos[q][8] = 0;  //0 means attached
      //is it time
      if (_pca9685Servos[q][5] > 0) {                                            //timer between steps
        if (_currentMillis - _pca9685ServosMillis[q] >= _pca9685Servos[q][5]) {  //if it's time
          _pca9685ServosMillis[q] = _currentMillis;                              //update timing
          //now sort the move
          if (_pca9685Servos[q][7] > _pca9685Servos[q][6]) {  //If the target is greater than current
            _pca9685Servos[q][6]++;                           //increase the current pos
          } else {
            _pca9685Servos[q][6]--;  //decrease the current pos
          }
          nowRail::setPCA695Servo(_pca9685Servos[q][0], _pca9685Servos[q][1], _pca9685Servos[q][6]);  //move to new current pos
        }

      } else {                                                                                      //move instantly
        nowRail::setPCA695Servo(_pca9685Servos[q][0], _pca9685Servos[q][1], _pca9685Servos[q][7]);  //set it for target angle
        _pca9685Servos[q][6] = _pca9685Servos[q][7];                                                //update the target angle so it doesn't keep trying to move it.
      }

    }
    //1.9.1 mod
    //method
    //if defined to detach
    //check flag...fast
    //check time... slightly slower
    //if time detach servo and set flag

#if defined(PCA9685SERVODETACH)                                                //if they have set system to detach servos
    else {                                                                     //if they are equal
      if (_pca9685Servos[q][8] < 1) {                                          //currently attached
        if (_currentMillis - _pca9685ServosMillis[q] >= PCA9685SERVODETACH) {  //is it time to detach
          detachPCA9685Servo(_pca9685Servos[q][0], _pca9685Servos[q][1]);      //detach the servo
          _pca9685Servos[q][8] = 1;                                            //set detached flag
        }
      }
    }
#endif
  }
}



//version 2 sends 4096...OFF OFF
void nowRail::detachPCA9685Servo(byte boardAddress, byte port) {
  if (port < 16) {
    uint16_t sendPulse = 4096;
    Wire.beginTransmission(boardAddress);  //Start transmissions to board
    Wire.write((port * 4) + 6);            //6 = port 0.............               sets the start transmission address
    Wire.write(0x00);                      //pulse start time..using fixed on time... writes date to addrsss and increments...start time low byte
    Wire.write(0x00);                      //pulse start time                         writes date to addrsss and increments.... high byte...both zero in this example
    Wire.write(byte(sendPulse));           //value between 4096..writes date to addrsss and increments...finish time low byte
    Wire.write(byte(sendPulse >> 8));      //high byte              writes date to addrsss and increments...finish time high byte byte
    Wire.endTransmission();                //end transmission, check if any errores occured with the writes...0 = good news.
  } else {
    Serial.println("invalid PCA9685 port Max = 15");
  }
}

void nowRail::setPCA695Servo(byte boardAddress, byte port, byte angle) {  //addr = PCA9685 address 0x40, byte port 0-15, int angle...angle to set servo to
  //int sendPulse;
  uint16_t sendPulse;  //pulse to send to board
  int error;           //stores return value if you want to test it.
  if (port < 16) {
    if (angle > 179) {  //don't allow out of range angles
      angle = 179;
    }
    sendPulse = map(angle, 0, 179, SERVOMIN, SERVOMAX);
    //testing
    // if(angle < 90){
    //   sendPulse = 800;
    // }else{
    //   sendPulse = 1500;
    // }
    //Serial.println(sendPulse);
    Wire.beginTransmission(boardAddress);  //Start transmissions to board
    Wire.write((port * 4) + 6);            //6 = port 0.............               sets the start transmission address
    Wire.write(0x00);                      //pulse start time..using fixed on time... writes date to addrsss and increments...start time low byte
    Wire.write(0x00);                      //pulse start time                         writes date to addrsss and increments.... high byte...both zero in this example
    Wire.write(byte(sendPulse));           //value between 0 and 4095..writes date to address and increments...finish time low byte
    Wire.write(byte(sendPulse >> 8));      //high byte              writes date to addrsss and increments...finish time high byte byte
    error = Wire.endTransmission();        //end transmission, check if any errores occured with the writes...0 = good news.
    //Serial.println(error);//print return value 0 = OK
  } else {
    Serial.println("invalid PCA9685 port Max = 15");
  }
}

//sets up the PCA9685 board frequency and control system
void nowRail::setupPCA9685Board(byte boardAddress, byte boardType) {  //board type 0 = servo, 1 = led
  int error;                                                          //use of you want to check status
  int pca9685ServoFreq = 50;                                          //50 hz
  int pca9685LedFreq = 1600;                                          //1600 hz
  long reqHZ;
  Serial.print("boardAddress: ");
  Serial.println(boardAddress, HEX);
  Serial.print("boardType: ");
  Serial.println(boardType);
  //set up stuff
  Wire.beginTransmission(boardAddress);  //Start transmissions to board
  Wire.write(0x00);                      //the adress on the board to write to...MODE Register 1
  Wire.write(0x21);                      //sets auto increment to next location...0x21 = 00100001  bit 5 set to 1 = auto increment bit 0 = 1 responds to all led call
  error = Wire.endTransmission();

  //not really needed as this just resets the default configuration for MODE 2...see data sheet
  Wire.beginTransmission(boardAddress);  //Start transmissions to board
  Wire.write(0x01);                      //the address on the board to write to.... MODE register 2
  Wire.write(0x04);                      //sets 2nd variable to write to...........0100  INVT value to use when no externa; driver used.
  error = Wire.endTransmission();
  if (boardType < 1) {         //servos
    reqHZ = pca9685ServoFreq;  //servo freq
  } else {
    reqHZ = pca9685LedFreq;  //led freq
  }
  //int preScale = 25000000 / (4096 * reqHZ) - 1;
  int preScale = 27000000 / (4096 * reqHZ) - 1;
  if (preScale < 3) {  //Min prescale value
    preScale = 3;
  }
  if (preScale > 255) {  //Max prescale value
    preScale = 255;
  }
  Serial.print("preScale: ");  //
  Serial.println(preScale);    //Should come out at about 121 for servos

  //Put board to sleep
  Wire.beginTransmission(boardAddress);  //Start transmissions to board
  Wire.write(0x00);                      //the adress on the board to write to...MODE Register 1
  Wire.write(0b01000000);                //sets auto increment to next location...0x21 = 00100001  bit 5 set to 1 = auto increment bit 0 = 1 responds to all led call
  error = Wire.endTransmission();
  Serial.print("sleep: ");
  Serial.println(error);

  //change preScale
  Wire.beginTransmission(boardAddress);  //Start transmissions to board
  Wire.write(0xFE);                      //the address on the board to write to...MODE Register 1
  Wire.write(preScale);                  //sets auto increment to next location...0x21 = 00100001  bit 5 set to 1 = auto increment bit 0 = 1 responds to all led call
  error = Wire.endTransmission();
  Serial.print("error State: ");
  Serial.println(error);
#if defined(PCA9685LEDOPENDRAIN)
  if (boardType > 0) {                     //leds... only do this for leds boards
    Wire.beginTransmission(boardAddress);  //Start transmissions to board
    //uint8_t oldmode = read8(PCA9685_MODE2);
    Wire.write(0x01);  //Mode 2 address
    //Wire.write(0x00);            //Open Drain and invert = 10000 0x10
    Wire.write(0x10);  //Drain plus inverts signal
    error = Wire.endTransmission();
  }
#endif
  //return board to awake and the state I want.
  Wire.beginTransmission(boardAddress);  //Start transmissions to board
  Wire.write(0x00);                      //the adress on the board to write to...MODE Register 1
  Wire.write(0x21);                      //sets auto increment to next location...0x21 = 00100001  bit 5 set to 1 = auto increment bit 0 = 1 responds to all led call
  error = Wire.endTransmission();
}
#endif


//Turns off accessory pulses
void nowRail::accPulseOFF() {
  int q;
  //standard pins
  for (q = 0; q < _stdPinAccCount; q++) {                          //work through the accs
    if (_stdPinAcc[q][3] > 0 && _stdPinAcc[q][5] > 0) {            //3 says it's a pulse acc... 5 says not cancelled yet
      if (_currentMillis - _stdPinAccMillis[q] >= TURNOUTPULSE) {  //if pulse time exceeded
        if (_stdPinAcc[q][4] > 0) {                                //set the pin to opposite state
          digitalWrite(_stdPinAcc[q][0], LOW);
        } else {
          digitalWrite(_stdPinAcc[q][0], HIGH);
        }
        //Now turn off pulse mode
        _stdPinAcc[q][5] = 0;  //No pulse so will now be ignored
      }
    }
  }
//74HC595N Shift registers
#if defined(NUM74HC595NCHIPS)
  byte myBitSet = 0;
  for (q = 0; q < _S74HC595NPinAccCount; q++) {
    if (_S74HC595NPins[q][4] > 0 && _S74HC595NPins[q][6] > 0) {
      if (_currentMillis - _S74HC595NPinAccMillis[q] >= TURNOUTPULSE) {
        if (_S74HC595NPins[q][5] > 0) {  //if pulse high set low                               //set the pin to opposite state
          bitClear(_S74HC595NbyteData[_S74HC595NPins[q][0]], _S74HC595NPins[q][1]);
        } else {
          bitSet(_S74HC595NbyteData[_S74HC595NPins[q][0]], _S74HC595NPins[q][1]);  //if pulse low set high
        }
        _S74HC595NPins[q][6] = 0;
        myBitSet = 1;
      }
    }
  }
  if (myBitSet > 0) {  //if I set somethiing update the shift registers
    nowRail::update74HC595N();
  }
#endif
}



//adds accessory driven outputs using std board pins
void nowRail::addStdPinAcc(int pin, int accNum, int dir, int pulse, int setpinState) {  //Adds std pin accessories to system
  if (_stdPinAccCount < 50) {
    _stdPinAcc[_stdPinAccCount][0] = pin;
    _stdPinAcc[_stdPinAccCount][1] = accNum;
    _stdPinAcc[_stdPinAccCount][2] = dir;
    _stdPinAcc[_stdPinAccCount][3] = pulse;
    _stdPinAcc[_stdPinAccCount][4] = setpinState;

    pinMode(pin, OUTPUT);                      //put all buttons as INPUT's
    if (_stdPinAcc[_stdPinAccCount][4] > 0) {  //Set the default pins state on start up
      digitalWrite(_stdPinAcc[_stdPinAccCount][0], LOW);
    } else {
      digitalWrite(_stdPinAcc[_stdPinAccCount][0], HIGH);
    }
    _stdPinAccCount++;
  } else {
    Serial.println("addStdPinAcc: Acc not added, too many in system. Pin:" + String(pin) + "AccNum: " + String(accNum) + "dir: " + String(dir));
  }
}

//1.7.0 adds toggle switch
void nowRail::addStdPinTSwitch(int pin, int accNum) {
  if (_stdPinTSwitchCount < 50) {
    _stdPinTSwitch[_stdPinTSwitchCount][0] = pin;
    _stdPinTSwitch[_stdPinTSwitchCount][1] = accNum;
    _stdPinTSwitch[_stdPinTSwitchCount][2] = 2;  //Default value
    _stdPinTSwitchCount++;
    pinMode(pin, INPUT);
  } else {
    Serial.println("addStdPinTSwitch: Toggle Switch not added, too many in system. Pin:" + String(pin) + "accNum: " + String(accNum));
  }
}

//74HC595N Shift register functions
#if defined(NUM74HC595NCHIPS)
// 1.4.0 mod allows user to set an individual output
void nowRail::set74HC595NPPinState(unsigned int chip, unsigned int pin, byte pinState) {
  int q = 0;
  byte myBitSet = 0;
  if (chip < NUM74HC595NCHIPS) {  //check valid chip ID
    if (pin < 8) {
      for (q = 0; q < _S74HC595NPanelLedPinCount; q++) {
        if (_S74HC595NPanelLedPins[q][0] == chip && _S74HC595NPanelLedPins[q][1] == pin) {
          if (pinState > 0) {
            //WORKING HERE...how does this array work
            bitSet(_S74HC595NbyteData[_S74HC595NPanelLedPins[q][0]], _S74HC595NPanelLedPins[q][1]);
            nowRail::update74HC595N();
          } else {
            bitClear(_S74HC595NbyteData[_S74HC595NPanelLedPins[q][0]], _S74HC595NPanelLedPins[q][1]);
            nowRail::update74HC595N();
          }
        }
      }
    } else {
      Serial.println("set74HC595NPinState error Invalid pin number, greater or equal to 8");
    }

  } else {
    Serial.println("set74HC595NPinState error Invalid Chip number, greater or equal to NUM74HC595NCHIPS");
  }
}


//Set up pins for 74HC595N control
void nowRail::setup74HC595N(int latchPin, int clockPin, int dataPin) {  // Sets up the pins
  _S74HC595NBoardPins[LATCHPIN] = latchPin;
  _S74HC595NBoardPins[CLOCKPIN] = clockPin;
  _S74HC595NBoardPins[DATAPIN] = dataPin;
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

void nowRail::update74HC595N() {
  digitalWrite(_S74HC595NBoardPins[LATCHPIN], LOW);
  for (int q = NUM74HC595NCHIPS; q > 0; q--) {  //cycle through the chips v0_3 fix was cycling in wrong order
    shiftOut(_S74HC595NBoardPins[DATAPIN], _S74HC595NBoardPins[CLOCKPIN], MSBFIRST, _S74HC595NbyteData[q - 1]);
  }
  digitalWrite(_S74HC595NBoardPins[LATCHPIN], HIGH);
}

//Set up the triggers for pins
void nowRail::add74HC595NPinAcc(int chip, int pin, int accNum, int dir, int pulse, int setpinState) {
  if (_S74HC595NPinAccCount < (NUM74HC595NCHIPS * 16)) {
    if (chip < NUM74HC595NCHIPS) {
      _S74HC595NPins[_S74HC595NPinAccCount][0] = chip;
      _S74HC595NPins[_S74HC595NPinAccCount][1] = pin;
      _S74HC595NPins[_S74HC595NPinAccCount][2] = accNum;
      _S74HC595NPins[_S74HC595NPinAccCount][3] = dir;
      _S74HC595NPins[_S74HC595NPinAccCount][4] = pulse;
      _S74HC595NPins[_S74HC595NPinAccCount][5] = setpinState;
      //set pin initial state
      if (_S74HC595NPins[_S74HC595NPinAccCount][5] > 0) {  //high when triggered set to low
        bitClear(_S74HC595NbyteData[_S74HC595NPins[_S74HC595NPinAccCount][0]], _S74HC595NPins[_S74HC595NPinAccCount][1]);
      } else {  //low when triggered set to high
        bitSet(_S74HC595NbyteData[_S74HC595NPins[_S74HC595NPinAccCount][0]], _S74HC595NPins[_S74HC595NPinAccCount][1]);
      }
      Serial.print("input");
      Serial.println(_S74HC595NbyteData[_S74HC595NPins[_S74HC595NPinAccCount][0]], BIN);
      nowRail::update74HC595N();

      _S74HC595NPinAccCount++;
    } else {
      Serial.println("add74HC595NPinAcc: item not added, INVALID Chip Number: " + String(chip) + " Increase NUM74HC595NCHIPS in nowrail_user_setup.h");
    }
  } else {
    Serial.println("add74HC595NPinAcc: item not added, too many in system 2 items per pin");
  }
}
#endif
//end 74HC595N Shift register functions

//Sensors
void nowRail::addStdPinSensor(int pin, int senNum) {
  if (_stdPinButtonsCount < 50) {
    _stdPinSensors[_stdPinSensorsCount][0] = pin;
    _stdPinSensors[_stdPinSensorsCount][1] = senNum;
    _stdPinSensors[_stdPinSensorsCount][2] = 2;  //Default value
    _stdPinSensorsCount++;
    pinMode(pin, INPUT);
    //setupDCCEXSensor(senNum);  //sends DCCEXSensor set up
  } else {
    Serial.println("addStdPinSensor: Sensor not added, too many in system. Pin:" + String(pin) + "senNum: " + String(senNum));
  }
}

//CD4012 Shift register functions
#if defined(NUMCD4021CHIPS)

//1.7.0 toggle switches
void nowRail::addCD4021PinTSwitch(int chip, int pin, int accNum) {  //ok
  if (_CD4021TSwitchPinCount < NUMCD4021CHIPSNUMTSWITCH) {
    if (chip < NUMCD4021CHIPS) {
      _CD4021PinTSwitch[_CD4021TSwitchPinCount][0] = chip;
      _CD4021PinTSwitch[_CD4021TSwitchPinCount][1] = pin;
      _CD4021PinTSwitch[_CD4021TSwitchPinCount][2] = accNum;
      _CD4021PinTSwitch[_CD4021TSwitchPinCount][3] = 2;  //default start value
      _CD4021TSwitchPinCount++;
    } else {
      Serial.println("addCD4021PinTSwitch: item not added, INVALID Chip Number: " + String(chip) + " Increase NUMCD4021CHIPS in nowrail_user_setup.h");
    }
  } else {
    Serial.println("addCD4021PinTSwitch: item not added, too many in system. Increase NUMCD4021CHIPSNUMTSWITCH in nowrail_user_setup.h");
  }
}

//sensors
void nowRail::addCD4021PinSensor(int chip, int pin, int senNum) {
  if (_CD4021SensorPinCount < NUMCD4021CHIPS * 8) {
    if (chip < NUMCD4021CHIPS) {
      _CD4021PinSensors[_CD4021SensorPinCount][0] = chip;
      _CD4021PinSensors[_CD4021SensorPinCount][1] = pin;
      _CD4021PinSensors[_CD4021SensorPinCount][2] = senNum;
      _CD4021PinSensors[_CD4021SensorPinCount][3] = 2;  //default start value
      _CD4021SensorPinCount++;
      //setupDCCEXSensor(senNum);  //sends DCCEXSensor set up

    } else {
      Serial.println("addCD4021PinButton: item not added, INVALID Chip Number: " + String(chip) + " Increase NUMCD4021CHIPS in nowrail_user_setup.h");
    }
  } else {
    Serial.println("addCD4021PinSensor: item not added, too many in system. Increase Add more NUMCD4021CHIPS in nowrail_user_setup.h");
  }
}

void nowRail::setupCD4021(int latchPin, int clockPin, int dataPin) {
  _CD4021BoardPins[LATCHPIN] = latchPin;
  _CD4021BoardPins[CLOCKPIN] = clockPin;
  _CD4021BoardPins[DATAPIN] = dataPin;
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, INPUT);
}

void nowRail::latch4021(int funcLatchPin) {
  digitalWrite(funcLatchPin, HIGH);
  delayMicroseconds(5);  //works... I want the smallest stable value possible
  digitalWrite(funcLatchPin, LOW);
}

byte nowRail::get4021Byte(int funcDataPin, int funcClockPin) {
  byte registerContent;
  int bitContent;
  for (int idx = 0; idx < 8; idx++) {
    bitContent = digitalRead(funcDataPin);
    if (bitContent == 1) {
      bitWrite(registerContent, idx, 1);
    } else {
      bitWrite(registerContent, idx, 0);
    }
    digitalWrite(funcClockPin, LOW);
    delayMicroseconds(5);  //works... I want the smallest stable value possible
    digitalWrite(funcClockPin, HIGH);
  }
  return registerContent;
}

//Adds buttons to system
void nowRail::addCD4021PinButton(int chip, int pin, int accNum, int press1, int press2) {  //CD4021 pin/button
  if (_CD4021PinCount < NUMCD4021CHIPSNUMBUTTONS) {
    if (chip < NUMCD4021CHIPS) {
      _CD4021PinButtons[_CD4021PinCount][0] = chip;
      _CD4021PinButtons[_CD4021PinCount][1] = pin;
      _CD4021PinButtons[_CD4021PinCount][2] = accNum;
      _CD4021PinButtons[_CD4021PinCount][3] = press1;
      _CD4021PinButtons[_CD4021PinCount][4] = press2;
      _CD4021PinCount++;
    } else {
      Serial.println("addCD4021PinButton: item not added, INVALID Chip Number: " + String(chip) + " Increase NUMCD4021CHIPS in nowrail_user_setup.h");
    }
  } else {
    Serial.println("addCD4021PinButton: item not added, too many in system. Increase NUMCD4021CHIPSNUMBUTTONS in nowrail_user_setup.h");
  }
}
#endif
//END CD4021 functions

// //adds buttons to system using board pins only
void nowRail::addStdPinButton(int pin, int accNum, int press1, int press2) {
  if (_stdPinButtonsCount < 50) {
    _stdPinButtons[_stdPinButtonsCount][0] = pin;
    _stdPinButtons[_stdPinButtonsCount][1] = accNum;
    _stdPinButtons[_stdPinButtonsCount][2] = press1;
    _stdPinButtons[_stdPinButtonsCount][3] = press2;
    _stdPinButtonsCount++;
    pinMode(pin, INPUT);  //put all buttons as INPUT's
  } else {
    Serial.println("addStdPinButton: Button not added, too many in system. Pin:" + String(pin) + "AccNum: " + String(accNum));
  }
}

//work through sensors looking for change
void nowRail::sensorEvents() {
  int q;
  int senRead;
  if (_currentMillis - _sensorDebounceMillis >= SENSORDEBOUNCE) {
    _sensorDebounceMillis = _currentMillis;
    for (q = 0; q < _stdPinSensorsCount; q++) {
      senRead = digitalRead(_stdPinSensors[q][0]);
      if (senRead != _stdPinSensors[q][2]) {  //check if reading has changed
        _stdPinSensors[q][2] = senRead;       //update the sensor status
        nowRail::sendSensorUpdate(_stdPinSensors[q][1], _stdPinSensors[q][2]);
      }
    }

    //1.7.0 std toggle switches added here uses sensor debounce
    for (q = 0; q < _stdPinTSwitchCount; q++) {
      senRead = digitalRead(_stdPinTSwitch[q][0]);
      if (senRead != _stdPinTSwitch[q][2]) {  //check if reading has changed
        _stdPinTSwitch[q][2] = senRead;       //update the sensor status
        nowRail::sendAccessoryCommand(_stdPinTSwitch[q][1], _stdPinTSwitch[q][2], MESSRESPREQ);
      }
    }


    //Now its the CD4021 registers
#if defined(NUMCD4021CHIPS)

    nowRail::latch4021(_CD4021BoardPins[LATCHPIN]);
    for (q = 0; q < NUMCD4021CHIPS; q++) {
      //v0_3 mods gone back to shiftIn()
      _CD4021byteData[q] = shiftIn(_CD4021BoardPins[DATAPIN], _CD4021BoardPins[CLOCKPIN], LSBFIRST);
      //Serial.println(_CD4021byteData[q],BIN);//check data ok
    }
    //all data should be stored now so process it

    for (q = 0; q < _CD4021SensorPinCount; q++) {  //work through the array of sensors
      senRead = bitRead(_CD4021byteData[_CD4021PinSensors[q][0]], _CD4021PinSensors[q][1]);

      if (senRead != _CD4021PinSensors[q][3]) {
        _CD4021PinSensors[q][3] = senRead;

        nowRail::sendSensorUpdate(_CD4021PinSensors[q][2], _CD4021PinSensors[q][3]);
      }
    }
    //1.7.0 toggle switches...added in sensors as uses the same code
    for (q = 0; q < _CD4021TSwitchPinCount; q++) {  //work through the array of sensors

      senRead = bitRead(_CD4021byteData[_CD4021PinTSwitch[q][0]], _CD4021PinTSwitch[q][1]);

      if (senRead != _CD4021PinTSwitch[q][3]) {
        _CD4021PinTSwitch[q][3] = senRead;
        nowRail::sendAccessoryCommand(_CD4021PinTSwitch[q][2], _CD4021PinTSwitch[q][3], MESSRESPREQ);
      }
    }

#endif
  }
}

// //This checks if it's time to read button states
void nowRail::buttonsPressed(void) {
  int q;
  byte senRead;
  if (_currentMillis - _buttonDebounceMillis >= BUTTONDEBOUNCE) {
    _buttonDebounceMillis = _currentMillis;
    //removed issue when function in main page removed.
    if (nowMomentButton)
      nowMomentButton();  //check momentary buttons

    //Now works through buttons added in addStdPinButton(int pin,int accNum,int press1,int press2)
    for (q = 0; q < _stdPinButtonsCount; q++) {     //work though the pins
      if (digitalRead(_stdPinButtons[q][0]) > 0) {  //if the pin is being pressed
        if (_stdPinButtons[q][4] > 2) {
          _stdPinButtons[q][4] = 2;  //set to target for this button press
        } else {
          _stdPinButtons[q][4] = 3;  //set to target for this button press
        }
        //Now process the button
        nowRail::sendAccessoryCommand(_stdPinButtons[q][1], _stdPinButtons[q][_stdPinButtons[q][4]], MESSRESPREQ);  //1 response required
      }
    }

//Now it the CD4021 registers
#if defined(NUMCD4021CHIPS)

    nowRail::latch4021(_CD4021BoardPins[LATCHPIN]);
    for (q = 0; q < NUMCD4021CHIPS; q++) {
      //v0_3 mods gone back to shiftIn()
      //_CD4021byteData[q] = get4021Byte(_CD4021BoardPins[DATAPIN], _CD4021BoardPins[CLOCKPIN]);
      _CD4021byteData[q] = shiftIn(_CD4021BoardPins[DATAPIN], _CD4021BoardPins[CLOCKPIN], LSBFIRST);
    }
    //all data should be stored now so process it
    //buttons first
    for (q = 0; q < _CD4021PinCount; q++) {  //work through the array

      if (bitRead(_CD4021byteData[_CD4021PinButtons[q][0]], _CD4021PinButtons[q][1]) == 1) {
        if (_CD4021PinButtons[q][5] > 3) {
          _CD4021PinButtons[q][5] = 3;  //set to target for this button press
        } else {
          _CD4021PinButtons[q][5] = 4;  //set to target for this button press
        }
        nowRail::sendAccessoryCommand(_CD4021PinButtons[q][2], _CD4021PinButtons[q][_CD4021PinButtons[q][5]], MESSRESPREQ);  //1 response required
      }
    }

    //1.7.0 toggle switches


#endif
  }
}

// //
void nowRail::sendAccessoryCommand(int accNum, byte accInst, byte respReq) {
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = respReq;                    //1 requires response, 0 not required...points confirmation received
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;                        //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = ACCESSORYCOMMAND;            //setting an acc decoder
  sendFifoBuffer[sendWriteFifoCounter][ACCADDRHIGH] = (accNum >> 8) & 0xFF;        //get High Byte
  sendFifoBuffer[sendWriteFifoCounter][ACCADDRLOW] = accNum & 0xFF;                //get low byte
  sendFifoBuffer[sendWriteFifoCounter][ACCINST] = accInst;
  sendFifoBuffer[sendWriteFifoCounter][19] = 0;  //non DCC received command
  incsendWriteFifoCounter();                     //new function with high byte update
}

void nowRail::sendPanelUpdate(int accNum, byte accInst) {
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;             //Doesn't require response, could go to multiple panels
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;                        //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = PANELUPDATE;                 //setting an acc decoder
  sendFifoBuffer[sendWriteFifoCounter][ACCADDRHIGH] = (accNum >> 8) & 0xFF;        //get High Byte
  sendFifoBuffer[sendWriteFifoCounter][ACCADDRLOW] = accNum & 0xFF;                //get low byte
  sendFifoBuffer[sendWriteFifoCounter][ACCINST] = accInst;
  incsendWriteFifoCounter();  //new function with high byte update
}

void nowRail::sendSensorUpdate(int senNum, byte senInst) {
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPREQ;                //v1.8.3 requires response, could go to multiple panels
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;                        //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = SENSORUPDATE;                //sending sensor update
  sendFifoBuffer[sendWriteFifoCounter][ACCADDRHIGH] = (senNum >> 8) & 0xFF;        //get High Byte
  sendFifoBuffer[sendWriteFifoCounter][ACCADDRLOW] = senNum & 0xFF;                //get low byte
  sendFifoBuffer[sendWriteFifoCounter][ACCINST] = senInst;
  incsendWriteFifoCounter();  //new function with high byte update
}

// //Time functions below here

//The following get various time functions
byte nowRail::rtcClockSpeed() {
  return _timeArray[TIMEARRAYCLOCKSPEED];
}

byte nowRail::rtcHours() {
  return _timeArray[TIMEARRAYCLOCKHOUR];
}

byte nowRail::rtcMinutes() {
  return _timeArray[TIMEARRAYCLOCKMIN];
}

byte nowRail::rtcSeconds() {
  return _timeArray[TIMEARRAYCLOCKSECONDS];
}

byte nowRail::rtcDays() {
  return _timeArray[TIMEARRAYCLOCKDAY];
}

void nowRail::accProcessed(byte processed) {
  _accMoved = processed;
}

//changes the fast clock speed
void nowRail::sendClockSpeedChange(byte clockSpeed) {
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;             //Doesn't require response, could go to multiple panels
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;                        //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = FASTCLOCKUPDATE;             //FASTCLOCKUPDATE
  sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKSPEED] = clockSpeed;            //new clock speed 0 - 255
  incsendWriteFifoCounter();                                                       //new function with high byte update
}

void nowRail::sendClockTimeChange(byte hour, byte mins, byte seconds, byte day) {
  if (hour > 23) {
    hour = 0;
  }
  if (mins > 59) {
    mins = 0;
  }
  if (seconds > 59) {
    seconds = 0;
  }
  if (day > 6) {
    day = 0;
  }
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;             //Doesn't require response, could go to multiple panels
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;                        //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = SETMASTERCLOCKTIME;          //FASTCLOCKUPDATE
  sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKHOUR] = hour;                   //hour
  sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKMIN] = mins;                    //min
  sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKSEC] = seconds;                 //seconds
  sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKDAY] = day;                     //day
  incsendWriteFifoCounter();
}


// //Deals with setting time and time broadcaster if MASTERCLOCK_ON
void nowRail::clockEvents(void) {
  int clockTimer;
  //   //_timeArray[TIMEARRAYCLOCKSPEED] = 1; //for testing
  if (_timeArray[TIMEARRAYCLOCKSPEED] > 0) {               //if the clock speed is not set to Zero
    _clockTimer = 1000 / _timeArray[TIMEARRAYCLOCKSPEED];  //this will be accurate to within a fraction of a second, enough to fool the eye

    if (_currentMillis - _clockMillis >= _clockTimer) {  //update the layout time for this board
      _clockMillis = _currentMillis;
      _timeArray[TIMEARRAYCLOCKSECONDS]++;
      if (_timeArray[TIMEARRAYCLOCKSECONDS] > 59) {
        _timeArray[TIMEARRAYCLOCKSECONDS] = 0;
        _timeArray[TIMEARRAYCLOCKMIN]++;
        if (_timeArray[TIMEARRAYCLOCKMIN] > 59) {
          _timeArray[TIMEARRAYCLOCKMIN] = 0;
          _timeArray[TIMEARRAYCLOCKHOUR]++;
          if (_timeArray[TIMEARRAYCLOCKHOUR] > 23) {
            _timeArray[TIMEARRAYCLOCKHOUR] = 0;
            _timeArray[TIMEARRAYCLOCKDAY]++;
            if (_timeArray[TIMEARRAYCLOCKDAY] > 6) {
              _timeArray[TIMEARRAYCLOCKDAY] = 0;
            }
          }
        }
      }
      //this function is called every layoutsecond update...could be 256 times per second
      if (nowTimeEvents)
        nowTimeEvents(_timeArray[TIMEARRAYCLOCKSPEED], _timeArray[TIMEARRAYCLOCKHOUR], _timeArray[TIMEARRAYCLOCKMIN], _timeArray[TIMEARRAYCLOCKSECONDS], _timeArray[TIMEARRAYCLOCKDAY]);
    }
  }

#if defined(MASTERCLOCK_ON)  //this board does the masterclock broadcast
  //This is seperate to the layout time update
  if (_currentMillis - _heartMillis >= HEARTBEAT) {  //This sends out the layout time heartbeat
    _heartMillis = _currentMillis;

    Serial.print("MASTERCLOCK>");
    Serial.print(_timeArray[TIMEARRAYCLOCKSPEED]);
    Serial.print("  ");
    Serial.print(_timeArray[TIMEARRAYCLOCKHOUR]);
    Serial.print(" : ");
    Serial.print(_timeArray[TIMEARRAYCLOCKMIN]);
    Serial.print(" : ");
    Serial.print(_timeArray[TIMEARRAYCLOCKSECONDS]);
    Serial.print(" D ");
    Serial.println(_timeArray[TIMEARRAYCLOCKDAY]);

    memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
    sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageLOW BYTE ID
    sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
    sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;
    sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;        //0 as new message
    sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = TIMEUPDATE;  //Time Update
    sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKSPEED] = _timeArray[TIMEARRAYCLOCKSPEED];
    sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKHOUR] = _timeArray[TIMEARRAYCLOCKHOUR];
    sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKMIN] = _timeArray[TIMEARRAYCLOCKMIN];
    sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKSEC] = _timeArray[TIMEARRAYCLOCKSECONDS];
    sendFifoBuffer[sendWriteFifoCounter][MESSAGECLOCKDAY] = _timeArray[TIMEARRAYCLOCKDAY];
    nowRail::incsendWriteFifoCounter();  //new function to add highbyte to messageID
  }
#endif
}

//sends a message response
void nowRail::sendMessResp(void) {
  recFifoBuffer[recReadFifoCounter][MESSRESPONSE] = MESSRESPRESP;  //Change message status to this is a response
  recFifoBuffer[recReadFifoCounter][MESSTRANSCOUNT] = 0;           //reset the transmission count
  //now send it back by putting the updated data in the sendFifoBuffer
  memcpy(sendFifoBuffer[sendWriteFifoCounter], recFifoBuffer[recReadFifoCounter], PACKETLENGTH);
  incsendWriteFifoCounter();  //new function with high byte update
}

// //Check to see if any incoming messages and process them
void nowRail::checkRecFifo(void) {
  int q;
  int accNum;
  byte accInst;
  int boardaddress;
  int boardindex;
  byte myBitSet;
  int senNum;
  byte senInst;
  String funcFirstString;
  int locoAddr;
  byte locoSpeed;
  byte locoDir;
  byte encKey1;
  byte encKey2;
  int32_t ansenValue;
  String messRespString;
  byte messageAddr;
  messageAddr = 1;
  uint16_t consistAddress;
  String DCCString;

  //
  if (recWriteFifoCounter != recReadFifoCounter) {  //check if there is a message to be processed
#if defined(ENCRYPT)                                //decrypt routing in here 1.4.0
    encKey1 = recFifoBuffer[recReadFifoCounter][48];
    encKey1 = encKey1 - recFifoBuffer[recReadFifoCounter][MESSAGETYPE];

    encKey2 = recFifoBuffer[recReadFifoCounter][49];
    encKey2 = encKey2 + recFifoBuffer[recReadFifoCounter][MESSAGELOWID];
    recFifoBuffer[recReadFifoCounter][0] = recFifoBuffer[recReadFifoCounter][0] + encKey1;
    recFifoBuffer[recReadFifoCounter][1] = recFifoBuffer[recReadFifoCounter][1] - encKey2;
    recFifoBuffer[recReadFifoCounter][2] = recFifoBuffer[recReadFifoCounter][2] - encKey1;
    recFifoBuffer[recReadFifoCounter][3] = recFifoBuffer[recReadFifoCounter][3] + encKey2;


#endif

    //Is this message for this system?...check layout prefix
    if (recFifoBuffer[recReadFifoCounter][0] == _transmissionPrefix[0] && recFifoBuffer[recReadFifoCounter][1] == _transmissionPrefix[1] && recFifoBuffer[recReadFifoCounter][2] == _transmissionPrefix[2] && recFifoBuffer[recReadFifoCounter][3] == _transmissionPrefix[3]) {

      //1.8.2 mod  to deal with messageResponse now needed for sensors and DCC EX stuff
      if (recFifoBuffer[recReadFifoCounter][MESSRESPONSE] == MESSRESPRESP) {  //deal with a response
        switch (recFifoBuffer[recReadFifoCounter][MESSAGETYPE]) {
          case ACCESSORYCOMMAND:
            messRespString = "DIAG> ACC COMMAND RESPONSE: " + String(recFifoBuffer[recReadFifoCounter][MESSRESPONSE]);
            break;  //Accessory Command such as turnpout/lights/animation
                    //PANELUPDATE 3          //When accessory has moved a PANELUPDATE is broadcast so any control panels can update
          case DCCLOCOSPEED:
            messRespString = "DIAG> DCCLOCOSPEED RESPONSE: " + String(recFifoBuffer[recReadFifoCounter][MESSRESPONSE]);
            break;  //DCC Loco Speed instruction
                    //DCCLOCOFUNCTION 5      //DCC Loco function instruction
                    //FASTCLOCKUPDATE 6      //New fast Clock Time
                    //LAYOUTPOWERCOMMANDS 7  //Turns Power on/off emergency stop
          case SENSORUPDATE:
            messRespString = "DIAG> SENSORUPDATE RESPONSE: " + String(recFifoBuffer[recReadFifoCounter][MESSRESPONSE]);
            break;  //Sensor Update
                    //LOCODATATX 9           //This is a full transmission 1 locos data including functions and name
                    //LOCOBULKDATATX 10      //As above but part of 200 loco transmissions
                    //WIFICHANNELCMD 11      //WIFI channel change command
                    //SETMASTERCLOCKTIME 12  //Command that will only be processed by MASTERCLOCK,,, sets time and day
          case RFIDDATA:
            messRespString = "DIAG> RFIDDATA RESPONSE: " + String(recFifoBuffer[recReadFifoCounter][MESSRESPONSE]);
            break;  //command that holds RFID data packet
          // case DCCEXCREATESENSOR:
          //   messRespString = "DIAG> DCCEXCREATESENSOR RESPONSE: " + String(recFifoBuffer[recReadFifoCounter][MESSRESPONSE]);
          //   break;  //command that holds sets up a virtual pin sensor
          case ANALOGUESENSORUPDATE:
            messRespString = "DIAG> ANALOGUESENSORUPDATE RESPONSE: " + String(recFifoBuffer[recReadFifoCounter][MESSRESPONSE]);
            break;  //allows transmission of sensor values
          case DCCEXCUSTOMCMD:
            messRespString = "DIAG> DCCEXCUSTOMCMD RESPONSE: " + String(recFifoBuffer[recReadFifoCounter][MESSRESPONSE]);
            break;  //allows 30 bytes of custom data
          default:
            break;
        }
        for (q = 4; q < 10; q++) {  //check the sender address
          if (recFifoBuffer[recReadFifoCounter][q] != _transmissionPrefix[q]) {
            messageAddr = 0;
          }
        }
        if (messageAddr != 0) {        //it's a response to a message this board sent
          for (q = 0; q < 256; q++) {  //work through the repeat send array
            if (repeatFifoBuffer[q][MESSAGELOWID] == recFifoBuffer[recReadFifoCounter][MESSAGELOWID] && repeatFifoBuffer[q][MESSAGEHIGHID] == recFifoBuffer[recReadFifoCounter][MESSAGEHIGHID]) {
              repeatFifoBuffer[q][MESSRESPONSE] = 0;    //stop it being repeated
              repeatFifoBuffer[q][MESSTRANSCOUNT] = 5;  //Set it to sent 5 times
            }
          }
        }
#if defined(DIAGNOSTICS_ON)
        Serial.println(messRespString);  //diagnostics print out
#endif
      } else {  //else it's command to process

        switch (recFifoBuffer[recReadFifoCounter][MESSAGETYPE]) {
          case TIMEUPDATE:

            _timeArray[TIMEARRAYCLOCKSPEED] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKSPEED];
            _timeArray[TIMEARRAYCLOCKHOUR] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKHOUR];
            _timeArray[TIMEARRAYCLOCKMIN] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKMIN];
            _timeArray[TIMEARRAYCLOCKSECONDS] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKSEC];
            _timeArray[TIMEARRAYCLOCKDAY] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKDAY];

#if defined(DIAGNOSTICS_ON)
#if defined(REDUCEMASTERCLOCKDIAG)  //1.8.3 diag message once per minute
            if (_timeArray[TIMEARRAYCLOCKSECONDS] == 0) {
#endif
              Serial.print("DIAG>TIMEUPDATE: CLCK ");
              Serial.print(_timeArray[TIMEARRAYCLOCKSPEED]);
              Serial.print(" TIME:");
              Serial.print(_timeArray[TIMEARRAYCLOCKHOUR]);
              Serial.print(":");
              Serial.print(_timeArray[TIMEARRAYCLOCKMIN]);
              Serial.print(":");
              Serial.print(_timeArray[TIMEARRAYCLOCKSECONDS]);
              Serial.print(" DAY");
              Serial.println(_timeArray[TIMEARRAYCLOCKDAY]);
#if defined(REDUCEMASTERCLOCKDIAG)
            }
#endif
#endif
            //1.4.2 lost wifi in here, looks for masterclock messages
#if defined(WIFIMASTERCLOCKCHANGE)
            _lastWIFIMillis = _currentMillis;  //just keeps track of when last command was received
#endif

            break;
          case ACCESSORYCOMMAND:
            accNum = (recFifoBuffer[recReadFifoCounter][ACCADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][ACCADDRLOW];
            accInst = recFifoBuffer[recReadFifoCounter][ACCINST];

#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> ACC COMMAND >accNum: ");
            Serial.print(accNum);
            Serial.print(" accInst: ");
            Serial.println(accInst);
#endif
            _accMoved = 0;  //set flag to know if panel response required

            //DCC COMMANDS MUST HAVE ADDRESS from 1-2000
            //DCC EX Commands...best done on ESP32 Dev Module
            if (accNum < 2001) {
//#if defined(ESP32)
#if defined(DCCEXSSERIAL2_ON)
              boardaddress = (accNum + 3) / 4;
              boardindex = (accNum - (boardaddress * 4)) + 3;
              //SEND OUT DCC Commands to DCC EX...Serial 2
              Serial2.println("<a " + String(boardaddress) + " " + String(boardindex) + " " + String(accInst) + " >");
              _accMoved = 1;  //send response
#endif
#if defined(DCCEXSERIAL_ON)  //SEND the same command over Serial
              Serial.println("<a " + String(boardaddress) + " " + String(boardindex) + " " + String(accInst) + " >");
#endif

#if defined(CAB_BUS_ADDRESS)
              nowRail::nceAccComProcess(accNum, accInst);
              _accMoved = 1;  //send response
#endif



              //#endif         // end if for #if defined(ESP32)
            }  //end of if acc below 2001
            //             //call the accessory command function
            //
            if (nowAccComRec)
              nowAccComRec(accNum, accInst);
            //             // Serial.print("accMoved: ");
            //             // Serial.println(accMoved);


            for (q = 0; q < _stdPinAccCount; q++) {                             //work through all built in accs
              if (_stdPinAcc[q][1] == accNum && _stdPinAcc[q][2] == accInst) {  //if this is the right acc and direction/instruction
                //Now process it
                digitalWrite(_stdPinAcc[q][0], _stdPinAcc[q][4]);
                _stdPinAcc[q][5] = 1;                  //set to started
                _stdPinAccMillis[q] = _currentMillis;  //set the timer...in case it's needed
                _accMoved = 1;                         //it's been processed
              }
            }
            //end stdPinAccs

#if defined(NUM74HC595NCHIPS)
            myBitSet = 0;
            for (q = 0; q < _S74HC595NPinAccCount; q++) {
              if (_S74HC595NPins[q][2] == accNum && _S74HC595NPins[q][3] == accInst) {

                //set the bit in the byte array
                if (_S74HC595NPins[q][5] == HIGH) {
                  bitSet(_S74HC595NbyteData[_S74HC595NPins[q][0]], _S74HC595NPins[q][1]);
                } else {
                  bitClear(_S74HC595NbyteData[_S74HC595NPins[q][0]], _S74HC595NPins[q][1]);
                }

                _S74HC595NPins[q][6] = 1;
                _S74HC595NPinAccMillis[q] = _currentMillis;

                myBitSet = 1;  //set a flag that change has been made
              }
            }
            //do this after cycling through all items
            if (myBitSet > 0) {  //if I set somethiing update the shift registers

              nowRail::update74HC595N();
            }

#endif
#if defined(MAXPCA9685SERVOBOARDS)  //now check any servos
#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> PCA9685 ACC COMMAND >accNum: ");
            Serial.print(accNum);
            Serial.print(" accInst: ");
            Serial.println(accInst);
#endif
            //servos
            //1.0.3 mods... old system used to set the servo, new version changes target and move takes place in loop
            //int _pca9685Servos[MAXPCA9685SERVOBOARDS * 16][8];  //board, port, accNum, angle0, angle1,millisperstep,currentAngle,targetAngle  (millisperstep,currentAngle,targetAngle...added 1.0.3 for slow motion)

            for (q = 0; q < _pca9685ServoCount; q++) {
              if (_pca9685Servos[q][2] == accNum) {
                if (accInst < 1) {
                  // nowRail::setPCA695Servo(byte(_pca9685Servos[q][0]), byte(_pca9685Servos[q][1]), byte(_pca9685Servos[q][3]));
                  _pca9685Servos[q][7] = _pca9685Servos[q][3];  //set the new target angle
                } else {
                  //  nowRail::setPCA695Servo(byte(_pca9685Servos[q][0]), byte(_pca9685Servos[q][1]), byte(_pca9685Servos[q][4]));
                  _pca9685Servos[q][7] = _pca9685Servos[q][4];  //set the new target angle
                }
                _accMoved = 1;  //was processed
              }
            }

            for (q = 0; q < _pca9685LEDCount; q++) {
              if (_pca9685LEDS[q][2] == accNum) {  //if it's the correct acc number
                //turn on/off is done in main loop as it may be an effect
                if (_pca9685LEDS[q][3] == accInst) {  //turn on
                  _pca9685LEDStates[q][0] = 1;        //turn on or start effect
                } else {                              //turn off
                  _pca9685LEDStates[q][0] = 0;        //turn off or stop effect
                }
                _accMoved = 1;  //was processed
              }
            }


#endif

#if defined(MP3BUSYPIN)
            for (q = 0; q < _mp3NumAccs; q++) {  //work through the accessories
              // Serial.print("TT Track: ");
              //     Serial.println(_mp3Accessories[q][2]);
              if (_mp3Accessories[q][0] == accNum) {
                if (_mp3Accessories[q][1] == accInst) {            //activate
                  if (_mp3Accessories[q][4] == 0) {                //single play..then play track right now
                                                                   // Serial.print("Track: ");
                                                                   // Serial.println(_mp3Accessories[q][2]);
                    nowRail::mp3PlayTrack(_mp3Accessories[q][2]);  //send play command
                  } else {
                    for (int w = 0; w < _mp3NumAccs; w++) {
                      _mp3Accessories[w][5] = 0;  //reset any other accessories that are running random list
                    }                             //random mode
                    _mp3Accessories[q][5] = 1;    //stop playing in random mode
                  }
                } else {                      //deactivate
                  _mp3Accessories[q][5] = 0;  //stop playing
                  //_mp3PlayState++;
                }
                _accMoved = 1;  //stop more transmissions
              }
            }

#endif
//Delayed accessory trigger system
#if defined(NUMDELAYEDACCS)
            //uint8_t _DelayAccs[NUMDELAYEDACCS][7];  //TriggerAccNum,TriggerAccDir,DelaAccNum,DelayAccDir,DelayTime,Triggerstate,TriggerMillis
            //int _DelayAccsCount;
            //void addDelayedAccTrigger(int triggerAccNum,byte triggerAccDir,int delayedAccNum,byte delayedAccDir,int delayTime);

            //The first part is to see if an accessory command coming through should trigger some timed events
            //Work through the array, check for matching triggerAccNum and TriggerAccDir
            //This then sets their start time for triggering later
            for (q = 0; q < _DelayAccsCount; q++) {
              //work through all the triggers
              //Serial.println(q);

              if (_DelayAccs[q][0] == accNum && _DelayAccs[q][1] == accInst) {
                _accMoved = 1;                      //if it is found set the acc response
                _DelayAccs[q][5] = 1;               //Set TriggerState
                _DelayAccs[q][6] = _currentMillis;  //TriggerMillis
              }
            }
#endif  //#if defined(NUMDELAYEDACCS)

            if (_accMoved > 0) {  //did I process this instruction? 1 = yes
              //Does it need a response?

              if (recFifoBuffer[recReadFifoCounter][MESSRESPONSE] == MESSRESPREQ) {
                sendMessResp();
                //nowRail::sendPanelUpdate(accNum, accInst);
              }
              nowRail::sendPanelUpdate(accNum, accInst);
            }
            //  }

            break;
          case PANELUPDATE:
            accNum = (recFifoBuffer[recReadFifoCounter][ACCADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][ACCADDRLOW];
            accInst = recFifoBuffer[recReadFifoCounter][ACCINST];
#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> PANEL UPDATE >accNum: ");
            Serial.print(accNum);
            Serial.print(" accInst: ");
            Serial.println(accInst);
#endif
            if (nowPanelUpdate)
              nowPanelUpdate(accNum, accInst);

            // _StdPanelLeds v0_3 working
            for (q = 0; q < _StdPanelLedCount; q++) {
              if (_StdPanelLed[q][1] == accNum) {
                if (_StdPanelLed[q][2] == accInst) {
                  digitalWrite(_StdPanelLed[q][0], HIGH);
                } else {
                  digitalWrite(_StdPanelLed[q][0], LOW);
                }
              }
            }

#if defined(MAXPCA9685SERVOBOARDS)

            //pca 9685 panel leds
            for (q = 0; q < _pca9685PanelLEDCount; q++) {
              if (_pca9685PanelLEDS[q][2] == accNum) {  //if it's the correct acc number
                //turn on/off is done in main loop as it may be an effect
                if (_pca9685PanelLEDS[q][3] == accInst) {  //turn on
                  _pca9685PanelLEDStates[q][0] = 1;        //turn on or start effect
                } else {                                   //turn off
                  _pca9685PanelLEDStates[q][0] = 0;        //turn off or stop effect
                }
              }
            }
#endif

#if defined(NUM74HC595NCHIPS)
            myBitSet = 0;
            for (q = 0; q < _S74HC595NPanelLedPinCount; q++) {
              if (_S74HC595NPanelLedPins[q][2] == accNum) {
                //set the bit in the byte array
                if (_S74HC595NPanelLedPins[q][3] == accInst) {
                  bitSet(_S74HC595NbyteData[_S74HC595NPanelLedPins[q][0]], _S74HC595NPanelLedPins[q][1]);
                  // Serial.print("bitSet: ");
                  // Serial.println(_S74HC595NbyteData[_S74HC595NPanelLedPins[q][0]],BIN);
                  //Serial.println(_S74HC595NbyteData[_S74HC595NPanelLedPins[q][0]], _S74HC595NPanelLedPins[q][1]);
                } else {
                  bitClear(_S74HC595NbyteData[_S74HC595NPanelLedPins[q][0]], _S74HC595NPanelLedPins[q][1]);
                  //Serial.print("bitClear: ");
                  //Serial.println(_S74HC595NbyteData[_S74HC595NPanelLedPins[q][0]],BIN);
                  //Serial.println(_S74HC595NbyteData[_S74HC595NPanelLedPins[q][0]], _S74HC595NPanelLedPins[q][1]);
                }
                myBitSet = 1;  //set a flag that change has been made
              }
            }
            //do this after cycling through all items
            if (myBitSet > 0) {  //if I set somethiing update the shift registers
              nowRail::update74HC595N();
            }
#endif
//2_1_1 mod..updates states on double press Std pins, cd4021 pins gtp11
#if defined(DOUBLEPRESSAUPDATE_ON)
  //std pins
  for(q=0;q<_stdPinButtonsCount;q++){
    //  Serial.print("original _stdPinButtons[q][4]: ");
    //  Serial.println(_stdPinButtons[q][4]);
    if(_stdPinButtons[q][2] == accNum){  //if it's the correct accessory
      if(accInst > 0){
         _stdPinButtons[q][4] = 3; //update the button state ready for next press
      }else{
         _stdPinButtons[q][4] = 2;
      }
      //  Serial.print("_stdPinButtons[q][4]: ");
      //  Serial.println(_stdPinButtons[q][4]);
    }
  }
  //4021 pins
  for(q=0;q<_CD4021PinCount;q++){
      // Serial.print("original _CD4021PinButtons[q][5]: ");
      // Serial.println(_CD4021PinButtons[q][5]);
    if(_CD4021PinButtons[q][2] == accNum){  //if it's the correct accessory
      if(accInst > 0){
         _CD4021PinButtons[q][5] = 4; //update the button state ready for next press
      }else{
         _CD4021PinButtons[q][5] = 3;
      }
        // Serial.print("_CD4021PinButtons[q][5]: ");
        // Serial.println(_CD4021PinButtons[q][5]);
    }
  }

 #if defined(GT911) //gt911 updates
  for(q=0;q<_GT911ButtonCount;q++){
    // Serial.print("original _GT911Buttons[q][5]: ");
    // Serial.println(_GT911Buttons[q][5]);
    if(_GT911Buttons[q][2] == accNum){  //if it's the correct accessory
      if(accInst > 0){
         _GT911Buttons[q][5] = 4; //update the button state ready for next press
      }else{
         _GT911Buttons[q][5] = 3;
      }
      // Serial.print("_GT911Buttons[q][5]: ");
      // Serial.println(_GT911Buttons[q][5]);
    }
  }
 #endif           
#endif

            break;
          case DCCLOCOSPEED:

            locoAddr = (recFifoBuffer[recReadFifoCounter][LOCOADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][LOCOADDRLOW];
            locoSpeed = recFifoBuffer[recReadFifoCounter][LOCOSPEED];
            locoDir = recFifoBuffer[recReadFifoCounter][LOCODIR];

            if (nowLocoFuncUpdate)  //external function
              nowLocoSpeedUpdate(locoAddr, locoSpeed, locoDir);

#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> DCCLOCOSPEED >locoAddr: ");
            Serial.print(locoAddr);
            Serial.print(" locoSpeed: ");
            Serial.print(locoSpeed);
            Serial.print(" locoDir: ");
            Serial.println(locoDir);
#endif
            //send DCC speed command
#if defined(DCCEXSSERIAL2_ON)
            Serial2.println("<t 1 " + String(locoAddr) + " " + String(locoSpeed) + " " + String(locoDir) + " >");
#endif
            //serial print DCC speed command
#if defined(DCCEXSERIAL_ON)  //SEND the same command over Serial
            Serial.println("<t 1 " + String(locoAddr) + " " + String(locoSpeed) + " " + String(locoDir) + " >");
#endif
//0.8 NCE mods
#if defined(CAB_BUS_ADDRESS)
            locoComToNCE(locoAddr, locoSpeed, locoDir);
#endif
            break;
          case DCCLOCOFUNCTION:
            byte funcNum;
            byte funcState;
            byte funcValue;
            byte secondFuncByte;
            byte nceFuncGroup;  //for byte 3
            byte nceFuncData;   //for byte 4
            int w;
            //0.7.2 mod
            byte locoFunctStates[64];  //added in v7_2 to prep for NCE
            byte funcCounter;
            //String funcFirstString;
            for (q = 20; q < 28; q++) {
              //Serial.println(recFifoBuffer[recReadFifoCounter][q],BIN);
              for (w = 0; w < 8; w++) {
                locoFunctStates[funcCounter] = bitRead(recFifoBuffer[recReadFifoCounter][q], w);
                funcCounter++;
              }
            }

            locoAddr = (recFifoBuffer[recReadFifoCounter][LOCOADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][LOCOADDRLOW];
            funcNum = recFifoBuffer[recReadFifoCounter][LOCOFUNC];
            funcState = recFifoBuffer[recReadFifoCounter][LOCOFUNCSTATE];
            // Serial.println("states");
            // for(q=0;q<64;q++){
            //   Serial.print(q);
            //   Serial.print(" : ");
            //   Serial.println(locoFunctStates[q]);
            // }

            switch (funcNum) {
              case 0 ... 4:
                funcValue = 128 + locoFunctStates[1] + (locoFunctStates[2] * 2) + (locoFunctStates[3] * 4) + (locoFunctStates[4] * 8) + (locoFunctStates[0] * 16);
                nceFuncData = locoFunctStates[1] + (locoFunctStates[2] * 2) + (locoFunctStates[3] * 4) + (locoFunctStates[4] * 8) + (locoFunctStates[0] * 16);
                nceFuncGroup = 0x07;
                break;
              case 5 ... 8:
                funcValue = 176 + locoFunctStates[5] + (locoFunctStates[6] * 2) + (locoFunctStates[7] * 4) + (locoFunctStates[8] * 8);
                nceFuncData = locoFunctStates[5] + (locoFunctStates[6] * 2) + (locoFunctStates[7] * 4) + (locoFunctStates[8] * 8);
                nceFuncGroup = 0x08;
                break;
              case 9 ... 12:
                funcValue = 160 + locoFunctStates[9] + (locoFunctStates[10] * 2) + (locoFunctStates[11] * 4) + (locoFunctStates[12] * 8);
                nceFuncData = locoFunctStates[9] + (locoFunctStates[10] * 2) + (locoFunctStates[11] * 4) + (locoFunctStates[12] * 8);
                nceFuncGroup = 0x09;
                break;
              case 13 ... 20:
                funcValue = locoFunctStates[13] + (locoFunctStates[14] * 2) + (locoFunctStates[15] * 4) + (locoFunctStates[16] * 8) + (locoFunctStates[17] * 16) + (locoFunctStates[18] * 32) + (locoFunctStates[19] * 64) + (locoFunctStates[20] * 128);
                secondFuncByte = 222;
                nceFuncData = funcValue;
                nceFuncGroup = 0x15;
                break;
              case 21 ... 28:
                funcValue = locoFunctStates[21] + (locoFunctStates[22] * 2) + (locoFunctStates[23] * 4) + (locoFunctStates[24] * 8) + (locoFunctStates[25] * 16) + (locoFunctStates[26] * 32) + (locoFunctStates[27] * 64) + (locoFunctStates[28] * 128);
                secondFuncByte = 223;
                nceFuncData = funcValue;
                nceFuncGroup = 0x16;
                break;

              default:
                break;
            }
            //end 0.7.2 mod



            if (funcNum < 13) {
              funcFirstString = "<f " + String(locoAddr) + ' ' + String(funcValue) + ">";
            } else {
              if (funcNum < 29) {
                funcFirstString = "<f " + String(locoAddr) + ' ' + String(secondFuncByte) + ' ' + String(funcValue) + ">";
              }
            }

            if (nowLocoFuncUpdate)  //external function
              nowLocoFuncUpdate(locoAddr, funcNum, funcState);

#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> DCCLOCOFUNCTION >locoAddr: ");
            Serial.print(locoAddr);
            Serial.print(" funcNum: ");
            Serial.print(funcNum);
            Serial.print(" funcState: ");
            Serial.println(funcState);
#endif
            //send DCC loco func command
#if defined(DCCEXSSERIAL2_ON)
            if (funcNum > 28) {
              Serial2.println("<F " + String(locoAddr) + " " + String(funcNum) + " " + String(funcState) + " >");
            } else {
              Serial2.print(funcFirstString);
            }
#endif
            //serial print DCC loco func command
#if defined(DCCEXSERIAL_ON)  //SEND the same command over Serial
            if (funcNum > 28) {
              Serial.println("<F " + String(locoAddr) + " " + String(funcNum) + " " + String(funcState) + " >");
            } else {
              Serial.println(funcFirstString);
            }
#endif

#if defined(CAB_BUS_ADDRESS)  //if CABBUS send NCE

            nowRail::locoFuncToNCE(locoAddr, nceFuncGroup, nceFuncData);
#endif
            break;
          case FASTCLOCKUPDATE:  //fast clock speed update send from controller
            _timeArray[TIMEARRAYCLOCKSPEED] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKSPEED];
            if (nowClockSpeedUpdate)
              nowClockSpeedUpdate();  //new function
#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> FASTCLOCKUPDATE >clockSpeed: ");
            Serial.println(_timeArray[TIMEARRAYCLOCKSPEED]);
#endif
            break;
          case LAYOUTPOWERCOMMANDS:  //Power commands to base stations
            //As LAYOUTPOWERCOMMANDS are Fail safe critical no response is sent
            //Therefore message will be processed 5 times.... and reponse sent 5 times
            //This is deliberate to meet Fail Safe criteria
            locoAddr = (recFifoBuffer[recReadFifoCounter][LOCOADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][LOCOADDRLOW];
            locoSpeed = recFifoBuffer[recReadFifoCounter][LOCOSPEED];
            locoDir = recFifoBuffer[recReadFifoCounter][LOCODIR];

            if (nowPowerCommand)  //custom function
              nowPowerCommand(recFifoBuffer[recReadFifoCounter][POWERCOMMAND]);

#if defined(DCCEXSSERIAL2_ON)
            _DCCEXCommand = "";
            switch (recFifoBuffer[recReadFifoCounter][POWERCOMMAND]) {
              case TURNPOWEROFF:
                _DCCEXCommand = "<0>";
                Serial2.println(_DCCEXCommand);
                nowRail::sendPowerCommand(DONEPOWEROFF, locoAddr, locoSpeed, locoDir);
                break;
              case TURNPOWERON:
                _DCCEXCommand = "<1>";
                Serial2.println(_DCCEXCommand);
                nowRail::sendPowerCommand(DONEPOWERON, locoAddr, locoSpeed, locoDir);
                break;
              case TURNEMERGENCYSTOP:
                _DCCEXCommand = "<!>";
                Serial2.println(_DCCEXCommand);
                nowRail::sendPowerCommand(DONEEMERGENCYSTOP, locoAddr, locoSpeed, locoDir);
                break;
              default:
                break;  //for returns
            }

#endif
#if defined(DCCEXSERIAL_ON)  //SEND the command over Serial
            Serial.println(_DCCEXCommand);
#endif

#if defined(CAB_BUS_ADDRESS)  //1.2.2 bug fix \
                              //loco data needed


            switch (recFifoBuffer[recReadFifoCounter][POWERCOMMAND]) {
              case TURNPOWEROFF:                                     //doesn't work well with cab bus
                nowRail::ncePowerOff(locoAddr, locoSpeed, locoDir);  //emergency stop
                nowRail::sendPowerCommand(DONEPOWEROFF, locoAddr, locoSpeed, locoDir);
                break;
              // case TURNPOWERON:... not used with Cab Bus
              //   _DCCEXCommand = "<1>";
              //   Serial2.println(_DCCEXCommand);
              //   nowRail::sendPowerCommand(DONEPOWERON);
              //   break;
              case TURNEMERGENCYSTOP:                                //doesn't work well with cabbus
                nowRail::ncePowerOff(locoAddr, locoSpeed, locoDir);  //emergency stop
                nowRail::sendPowerCommand(DONEEMERGENCYSTOP, locoAddr, locoSpeed, locoDir);
                break;
              default:
                break;  //for returns
            }
#endif

#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> POWERCOMMAND >Command: ");
            Serial.println(recFifoBuffer[recReadFifoCounter][POWERCOMMAND]);
#endif
            break;
          case SENSORUPDATE:  //Sensor transmission
            senNum = (recFifoBuffer[recReadFifoCounter][SENADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][SENADDRLOW];
            senInst = recFifoBuffer[recReadFifoCounter][SENINST];
            if (nowSensorUpdate)
              nowSensorUpdate(senNum, senInst);

#if defined(JMRICMRI)
            nowRail::jmriCmriUpdateSendBuffer(senNum, senInst);
            sendMessResp();
#endif
#if defined(DCCEXSSERIAL2_ON)
            if (senInst > 0) {
              _DCCEXCommand = "<z " + String(senNum) + ">";
            } else {
              _DCCEXCommand = "<z -" + String(senNum) + ">";
            }
            Serial2.println(_DCCEXCommand);
            sendMessResp();
#if defined(DCCEXSERIAL_ON)  //SEND the command over Serial
            Serial.println(_DCCEXCommand);
#endif
#endif
#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> SENSORUPDATE >  ");
            Serial.println(String(senNum) + " : " + String(senInst));
#endif
            break;
          case LOCODATATX:  //9 loco data set transmission
            //check if there is a loco to be updated
            if (_locoRXState[0] > 0 && _locoRXState[1] < 200) {  //make sure function can't overrun array
              for (q = 0; q < 32; q++) {
                _locoData[_locoRXState[1]][q] = recFifoBuffer[recReadFifoCounter][q + 16];
                _locoTXdataSet[q] = recFifoBuffer[recReadFifoCounter][q + 16];
              }
              if (nowLocoDataSetRX)  //if the custom function is out there
                nowLocoDataSetRX();
            }
#if defined(NOWDisc)
            locoEEPROMUpdate(_locoRXState[1]);
#endif
            _locoRXState[0] = 0;  //reset so doesn't get updated by next transmission


#if defined(DIAGNOSTICS_ON)
            Serial.println("DIAG> LOCODATATX: ");
#endif


            break;
          case LOCOBULKDATATX:  //10 transmission of all locos data

            if (_locoBulkDataRXFlag > 0 && _locoBulkDataRXPos < 200) {  //receive mode...valid slot and set to receive
              for (q = 0; q < 32; q++) {
                _locoData[_locoBulkDataRXPos][q] = recFifoBuffer[recReadFifoCounter][q + 16];
              }
#if defined(NOWDisc)
              locoEEPROMUpdate(_locoBulkDataRXPos);
#endif
              _locoBulkDataRXPos++;
              if (_locoBulkDataRXPos > 199) {  //if full set received
                if (nowLocoDataSetRX)          //if the custom function is out there
                  nowLocoBulkDataRX();
              }
            }


#if defined(DIAGNOSTICS_ON)
            Serial.println("DIAG> LOCOBULKDATATX: ");
#endif
            break;
          case WIFICHANNELCMD:  //11 transmission of all locos data
            byte newWifiChannel;

#if defined(WIFIMASTERCLOCKCHANGE)  //code only runs if enabled
            newWifiChannel = recFifoBuffer[recReadFifoCounter][NEWWIFICHANNEL];
            nowChannelUpdate(newWifiChannel, 0);  //2.0.1 update
#if defined(MASTERCLOCK_ON)                       //code only runs if enabled
            //newWifiChannel = recFifoBuffer[recReadFifoCounter][NEWWIFICHANNEL];
            WiFi.setChannel(newWifiChannel);
#endif
#endif
#if defined(DIAGNOSTICS_ON)
            Serial.println("DIAG>WIFICHANNELCMD>Channel: " + String(newWifiChannel));
#endif
            break;
          case SETMASTERCLOCKTIME:  //1.5.0 mod allows time to be set
#if defined(MASTERCLOCK_ON)         //code only runs if enabled
            _timeArray[TIMEARRAYCLOCKHOUR] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKHOUR];
            _timeArray[TIMEARRAYCLOCKMIN] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKMIN];
            _timeArray[TIMEARRAYCLOCKSECONDS] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKSEC];
            _timeArray[TIMEARRAYCLOCKDAY] = recFifoBuffer[recReadFifoCounter][MESSAGECLOCKDAY];


#endif
            Serial.print("DIAG> FASTCLOCKUPDATE > SETMASTERCLOCKTIME: ");
            break;
          case RFIDDATA:  //1.5.4 mod adds RFID data transmission
#if defined(DIAGNOSTICS_ON)
            Serial.println("DIAG>RFIDDATA");
            uint8_t sendRFIDDataBytes[30];
            if (nowRFIDDataRec)  //custom function

              memcpy(sendRFIDDataBytes, recFifoBuffer[recReadFifoCounter] + RFIDDATASTART, recFifoBuffer[recReadFifoCounter][RFIDNUMBYTES]);
            // nowRFIDDataRec( sendRFIDDataBytes,recFifoBuffer[recReadFifoCounter][RFIDNUMBYTES]);
            //nowRFIDDataRec(uint8_t *incomingData, int len)
            nowRFIDDataRec(sendRFIDDataBytes, recFifoBuffer[recReadFifoCounter][RFIDNUMBYTES]);
#endif
            break;

          case ANALOGUESENSORUPDATE:  //custom sensor data
            senNum = (recFifoBuffer[recReadFifoCounter][SENADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][SENADDRLOW];
            ansenValue = ((int32_t)recFifoBuffer[recReadFifoCounter][18] << 24) + ((int32_t)recFifoBuffer[recReadFifoCounter][19] << 16) + ((int32_t)recFifoBuffer[recReadFifoCounter][20] << 8) + (int32_t)recFifoBuffer[recReadFifoCounter][21];
            if (nowSensorUpdate)
              nowSensorUpdate(senNum, ansenValue);
              //1.8.3
              // if(_senProcessed > 0){  //if updated in nowSensorUpdate
              //  sendMessResp();
              // }
#if defined(DCCEXSSERIAL2_ON)
            _DCCEXCommand = "<z " + String(senNum) + " " + String(ansenValue) + " >";
            if (ansenValue > -1 && ansenValue < 32768) {  //make sure value within DCC EX limits
              Serial2.println(_DCCEXCommand);
            }
            sendMessResp();

#if defined(DCCEXSERIAL_ON)  //SEND the command over Serial
            if (ansenValue < 0 || ansenValue > 32767) {
              Serial.println("INVALID DCC EX value (too big/small) :");  //error warning
            }
            Serial.println(_DCCEXCommand);
#endif
#endif
#if defined(DIAGNOSTICS_ON)
            Serial.print("DIAG> ANALOGUESENSORUPDATE >");
            Serial.println(String(senNum) + " : " + String(ansenValue));
#endif
            break;
          case DCCEXCUSTOMCMD:
#if defined(DCCEXSSERIAL2_ON)
            _DCCEXCommand = "";
            for (q = 0; q < recFifoBuffer[recReadFifoCounter][16]; q++) {
              _DCCEXCommand = _DCCEXCommand + char(recFifoBuffer[recReadFifoCounter][17 + q]);
            }
            Serial2.println(_DCCEXCommand);
            sendMessResp();
#if defined(DCCEXSERIAL_ON)  //SEND the command over Serial
            Serial.println(_DCCEXCommand);
#endif
#endif
#if defined(DIAGNOSTICS_ON)
            Serial.println("DIAG> DCCEXCUSTOMCMD");  //not wasting time on showing message as probably not for this board
#endif
            break;
          //1_9_2 mods for EX RAIL and NCE CAB BUS consists
          case SETCONSIST:
            int16_t consistLocoArray[10];  //store all the loco details
            byte numCLocos;
            numCLocos = recFifoBuffer[recReadFifoCounter][CONSISTNUMLOCOS];
            Serial.print("numCLocos: ");
            Serial.println(numCLocos);
            consistAddress = (recFifoBuffer[recReadFifoCounter][CONSISTADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][CONSISTADDRLOW];
            Serial.print("M Con addr: ");
            Serial.println(consistAddress);
            for (q = 0; q < 10; q++) {
              consistLocoArray[q] = (recFifoBuffer[recReadFifoCounter][19 + (2 * q)] * 256) + recFifoBuffer[recReadFifoCounter][20 + (2 * q)];
              Serial.println(consistLocoArray[q]);
            }
            //all values correct
            //DCCString = "<^ " + String(consistAddress) + '>';
//DCC EX / EX RAIL consist
#if defined(DCCEXSSERIAL2_ON)
            DCCString = "<^";
            for (q = 0; q < numCLocos; q++) {
              DCCString = DCCString + ' ' + String(consistLocoArray[q]);
            }
            DCCString = DCCString + " >";
            Serial2.println(DCCString);
#endif
#if defined(DCCEXSERIAL_ON)  //SEND the command over Serial
            Serial.println(DCCString);
#endif
            //NCE CAB BUS consist NOT WORKING YET
            /* Method
A loco is set to the current loco in use and then added to the consist but each loco needs to be set to current loco
*/
            // #if defined(CAB_BUS_ADDRESS)

            // //set consist address as lead loco
            // _nceCommandBytes[_nceCommandBytesWrite][0] = consistLocoArray[0] >> 8;
            // _nceCommandBytes[_nceCommandBytesWrite][1] = consistLocoArray[0];//
            // _nceCommandBytes[_nceCommandBytesWrite][2] = 0x00;  //make current loco
            // _nceCommandBytes[_nceCommandBytesWrite][3] = 0x00; //make current loco
            // _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            // _nceCommandBytesWrite++;
            // //lead loco
            // _nceCommandBytes[_nceCommandBytesWrite][0] = consistLocoArray[0] >> 8;  //loco address high byte
            // _nceCommandBytes[_nceCommandBytesWrite][1] = consistLocoArray[0];       //loco address low byte
            // _nceCommandBytes[_nceCommandBytesWrite][2] = 0x0b;  //forward lead loco
            // _nceCommandBytes[_nceCommandBytesWrite][3] = consistAddress; //consist address
            // _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            // _nceCommandBytesWrite++;
            // //centre locos
            // int16_t consLoco;

            // for(q=1;q<(numCLocos - 1);q++){//last loco is the rear loco
            //   if(consistLocoArray[q] < 0){
            //     consLoco = consistLocoArray[q] * -1;//make positive
            //   }else{
            //     consLoco = consistLocoArray[q];
            //   }
            //   Serial.println(consistLocoArray[q]);
            //   Serial.println(consLoco);
            //   _nceCommandBytes[_nceCommandBytesWrite][0] = consLoco >> 8;
            //   _nceCommandBytes[_nceCommandBytesWrite][1] = consLoco;//
            //   _nceCommandBytes[_nceCommandBytesWrite][2] = 0x00;  //make current loco
            //   _nceCommandBytes[_nceCommandBytesWrite][3] = 0x00; //make current loco
            //   _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            //   _nceCommandBytesWrite++;

            //   _nceCommandBytes[_nceCommandBytesWrite][0] = consLoco >> 8;  //loco address high byte
            //   _nceCommandBytes[_nceCommandBytesWrite][1] = consLoco;       //loco address low byte
            //   if(consistLocoArray[q] < 0){
            //     _nceCommandBytes[_nceCommandBytesWrite][2] = 0x0e;  //reverse consist loco
            //   }else{
            //     _nceCommandBytes[_nceCommandBytesWrite][2] = 0x0f;  //forward consist loco;
            //   }
            //   _nceCommandBytes[_nceCommandBytesWrite][3] = consistAddress; //consist address
            //   _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            //   _nceCommandBytesWrite++;
            // }
            // //now for the rear loco
            // Serial.println("Last Loco");
            // if(consistLocoArray[numCLocos - 1] < 0){
            //     consLoco = consistLocoArray[numCLocos - 1] * -1;//make positive
            //   }else{
            //     consLoco = consistLocoArray[numCLocos - 1];
            //   }
            //   Serial.println(consistLocoArray[q]);
            //   Serial.println(consLoco);
            //   _nceCommandBytes[_nceCommandBytesWrite][0] = consLoco >> 8;
            //   _nceCommandBytes[_nceCommandBytesWrite][1] = consLoco;//
            //   _nceCommandBytes[_nceCommandBytesWrite][2] = 0x00;  //make current loco
            //   _nceCommandBytes[_nceCommandBytesWrite][3] = 0x00; //make current loco
            //   _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            //   _nceCommandBytesWrite++;

            //   _nceCommandBytes[_nceCommandBytesWrite][0] = consLoco >> 8;  //loco address high byte
            //   _nceCommandBytes[_nceCommandBytesWrite][1] = consLoco;       //loco address low byte
            //   if(consistLocoArray[numCLocos - 1] < 0){
            //     _nceCommandBytes[_nceCommandBytesWrite][2] = 0x0c;  //reverse rear consist loco
            //   }else{
            //     _nceCommandBytes[_nceCommandBytesWrite][2] = 0x0d;  //forward rear consist loco;
            //   }
            //   _nceCommandBytes[_nceCommandBytesWrite][3] = consistAddress; //consist address
            //   _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            //   _nceCommandBytesWrite++;

            // //make 1st loco current again
            // _nceCommandBytes[_nceCommandBytesWrite][0] = consistLocoArray[0] >> 8;
            // _nceCommandBytes[_nceCommandBytesWrite][1] = consistLocoArray[0];//
            // _nceCommandBytes[_nceCommandBytesWrite][2] = 0x00;  //make current loco
            // _nceCommandBytes[_nceCommandBytesWrite][3] = 0x00; //make current loco
            // _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            // _nceCommandBytesWrite++;

            // #endif
            break;
          case UNSETCONSIST:  //unset a consist either with EX RAIL or CAB BUS

            consistAddress = (recFifoBuffer[recReadFifoCounter][CONSISTADDRHIGH] * 256) + recFifoBuffer[recReadFifoCounter][CONSISTADDRLOW];
            // Serial.print("REC: ");
            // Serial.println(consistAddress);
            //All values correct
            //DCC EX string
            //Serial2.println("<a " + String(boardaddress) + " " + String(boardindex) + " " + String(accInst) + " >");
#if defined(DCCEXSSERIAL2_ON)
            DCCString = "<^ " + String(consistAddress) + '>';
            Serial2.println(DCCString);
#endif
#if defined(DCCEXSERIAL_ON)  //SEND the command over Serial
            Serial.println(DCCString);
#endif
            // #if defined(CAB_BUS_ADDRESS)
            //             //ste lead loco as current

            //             _nceCommandBytes[_nceCommandBytesWrite][0] = consistLocoArray[0] >> 8;
            //             _nceCommandBytes[_nceCommandBytesWrite][1] = consistLocoArray[0];//
            //             _nceCommandBytes[_nceCommandBytesWrite][2] = 0x00;  //make current loco
            //             _nceCommandBytes[_nceCommandBytesWrite][3] = 0x00; //make current loco
            //             _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            //             _nceCommandBytesWrite++;
            //             //kill consist
            //              _nceCommandBytes[_nceCommandBytesWrite][0] = consistLocoArray[0] >> 8;
            //             _nceCommandBytes[_nceCommandBytesWrite][1] = consistLocoArray[0];//
            //             _nceCommandBytes[_nceCommandBytesWrite][2] = 0x11;  //Kill consist
            //             _nceCommandBytes[_nceCommandBytesWrite][3] = consistAddress; //consist address
            //             _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
            //             _nceCommandBytesWrite++;



            // #endif
            break;

          default:
            Serial.print("Unknown MessageType: ");
            Serial.println(recFifoBuffer[recReadFifoCounter][MESSAGETYPE]);
            break;
        }
      }
    }
    recReadFifoCounter++;
  }
}

// //Processes incoming data from ESP-NOW into the recFifoBuffer
//ESP32 2.0.17
#if ESP_IDF_VERSION_MAJOR < 5                                                // IDF 4.xxx
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {  //problem line original
  memcpy(recFifoBuffer[recWriteFifoCounter], incomingData, PACKETLENGTH);    //copy data into recData array
  recWriteFifoCounter++;
}
#endif
#if ESP_IDF_VERSION_MAJOR > 4  // IDF 5.xxx
//New V3.0.0 esp32 LIBRARY
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(recFifoBuffer[recWriteFifoCounter], incomingData, PACKETLENGTH);  //copy data into recData array
  recWriteFifoCounter++;
}
#endif



// //deals with the messageID incrementation
void nowRail::incsendWriteFifoCounter() {
  sendWriteFifoCounter++;
  if (sendWriteFifoCounter == 0) {
    sendWriteHighFifoCounter++;
  }
}
//unsigned long sendFifoMillis;//1.4.0

//1.4.0 rewrite for encryption and 2ms slow down timer

void nowRail::checkSendFifo(void) {
  int q;
  byte encKey1;
  byte encKey2;
  //while (sendWriteFifoCounter != sendReadFifoCounter) {
  //1.4.0 adds a very slight slow down to transmissions, prevents buffer over run with bulk TX
  if (sendWriteFifoCounter != sendReadFifoCounter) {  //check if there is a message to be processed


    //Does this need to be responded to?
    if (sendFifoBuffer[sendReadFifoCounter][MESSRESPONSE] == MESSRESPREQ) {  //covers replies and requests
      //if response required copy message to repeatFif0Buffer                   //does this message require a response
      memcpy(repeatFifoBuffer[writeRepeatFifoCounter], sendFifoBuffer[sendReadFifoCounter], PACKETLENGTH);  //copy the data to repeat fifo

      //new feature 2.1
      _repeatFifoMillis[writeRepeatFifoCounter] = _currentMillis;
      //end feature
      writeRepeatFifoCounter++;
    }
    //Place 2 random values in the packet
    //Keys are added to all packets, encrypted or not
    //sendFifoBuffer[sendReadFifoCounter][48] = random(256);  //key1 1.4.0 mod
    //sendFifoBuffer[sendReadFifoCounter][49] = random(256);  //key2 1.4.0 mod

    //Encryption needs to go in here as refFifoBuffer should be full of encrypted messages
#if defined(ENCRYPT)  //encrypt routine in here 1.4.0

    //now choose which of the values to use

    encKey1 = sendFifoBuffer[sendReadFifoCounter][48];
    encKey1 = encKey1 - sendFifoBuffer[sendReadFifoCounter][MESSAGETYPE];

    encKey2 = sendFifoBuffer[sendReadFifoCounter][49];
    encKey2 = encKey2 + sendFifoBuffer[sendReadFifoCounter][MESSAGELOWID];


    sendFifoBuffer[sendReadFifoCounter][0] = sendFifoBuffer[sendReadFifoCounter][0] - encKey1;  //add the encryption key
    sendFifoBuffer[sendReadFifoCounter][1] = sendFifoBuffer[sendReadFifoCounter][1] + encKey2;  //add the encryption key
    sendFifoBuffer[sendReadFifoCounter][2] = sendFifoBuffer[sendReadFifoCounter][2] + encKey1;  //add the encryption key
    sendFifoBuffer[sendReadFifoCounter][3] = sendFifoBuffer[sendReadFifoCounter][3] - encKey2;  //add the encryption key

#endif  //end of 1.4.0 mods

    //New feature 08/03/24...copies message to Receive FIFO so that the board could process it's own commands...double ended analogue system
    memcpy(recFifoBuffer[recWriteFifoCounter], sendFifoBuffer[sendReadFifoCounter], PACKETLENGTH);
    recWriteFifoCounter++;
    //end feature
    //Now send
    //The data isn't copied into a structure, it's transmitted straight from the sendFifiBuffer

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sendFifoBuffer[sendReadFifoCounter], PACKETLENGTH);

    sendReadFifoCounter++;  //move to next item to be read...and transmitted
  }
  //NOw check if any repeated messages need to be loaded back into the send buffer
  //Checks individual message times to speed up system
  //no longer needs to work through whole buffer
  if (writeRepeatFifoCounter != readRepeatFifoCounter) {
    if (_currentMillis - _repeatFifoMillis[readRepeatFifoCounter] >= _repeatFIFOTimer) {
      if (repeatFifoBuffer[readRepeatFifoCounter][MESSRESPONSE] == MESSRESPREQ && repeatFifoBuffer[readRepeatFifoCounter][MESSTRANSCOUNT] < 5) {  //response required and not sent 5 times
        repeatFifoBuffer[readRepeatFifoCounter][MESSTRANSCOUNT]++;                                                                                //increment the transmission counter
        memcpy(sendFifoBuffer[sendWriteFifoCounter], repeatFifoBuffer[readRepeatFifoCounter], PACKETLENGTH);                                      //copy data
        repeatFifoBuffer[readRepeatFifoCounter][MESSRESPONSE] = 0;
#if defined(DIAGNOSTICS_ON)  //stop it being repeated again as the send system will re add record
        Serial.print("C: ");
        Serial.print(repeatFifoBuffer[readRepeatFifoCounter][MESSAGELOWID]);
        Serial.print(" TC: ");
        Serial.println(repeatFifoBuffer[readRepeatFifoCounter][MESSTRANSCOUNT]);
#endif
        sendWriteFifoCounter++;
      }
      readRepeatFifoCounter++;
    }
  }
  //end new section
}

// //process incoming and outgoing messages as well as clockevents
void nowRail::runLayout(void) {
  _currentMillis = millis();  //update timer
  nowRail::checkSendFifo();   //check for messages to be sent
  nowRail::checkRecFifo();
  nowRail::buttonsPressed();  //check for buttons being pressed
  nowRail::accPulseOFF();     //check for any accessories that need pulse turing off
  nowRail::clockEvents();
  nowRail::sensorEvents();  //check the sensors
#if defined(WIFIMASTERCLOCKCHANGE)
  if (_currentMillis - _lastWIFIMillis >= _lastWIFITimer) {
    //   byte _currentWIFIChannel = 1;
    // unsigned long _lastWIFIMillis;
    // unsigned long _lastWIFITimer = 5000;
    switch (_currentWIFIChannel) {
      case 1:
        _currentWIFIChannel = 6;
        break;
      case 6:
        _currentWIFIChannel = 11;
        break;
      case 11:
        _currentWIFIChannel = 1;
        break;
    }
    nowChannelUpdate(_currentWIFIChannel, 1);  //2.0.1 update
    Serial.println("Looking for MASTERCLOCK... Is it turned on?");
    Serial.println("Is layout ID correct?");
    Serial.println("Layout ID :" + getnowRailAddr() + " Trying WiFi Channel: " + String(_currentWIFIChannel));
    Serial.println("  ");
    WiFi.setChannel(_currentWIFIChannel);  //change to next channel
    _lastWIFIMillis = _currentMillis;      //reset to try again
  }
#endif
#if defined(NOWDisc)
  nowRail::timeUpdateEEPROM();  //updates the eeprom if there is one
#endif
#if defined(MAXPCA9685SERVOBOARDS)
  nowRail::pca9685LedControl();  //controls effects or turns on and off
  nowRail::pca9685ServoControl();
#endif
#if defined(DCCDECODERPIN)
  nowRail::processDCC();  //processes DCC if being used as DCC decoder
#endif
#if defined(GT911)
  nowRail::GT911ProcessButtons();
#endif
#if defined(JMRICMRI)
  nowRail::jmriCmri();
#endif
#if defined(CAB_BUS_ADDRESS)  //CAB BUS
  nowRail::nceProcess();
#endif
#if defined(MP3BUSYPIN)
  nowRail::mp3Control();
#endif
#if defined(NUMDELAYEDACCS)  //delayed tasks \
                             //Now run though the array to check for any items that have been set up to trigger
  for (int q = 0; q < _DelayAccsCount; q++) {
    if (_DelayAccs[q][5] > 0) {                                     //does this have a trigger set
      if (_currentMillis - _DelayAccs[q][6] >= _DelayAccs[q][4]) {  //if its waiting is it time yet
        //if it's time
        _DelayAccs[q][5] = 0;                                                            //Unset the trigger so it only fires once
        nowRail::sendAccessoryCommand(_DelayAccs[q][2], _DelayAccs[q][3], MESSRESPREQ);  //send out accessory command
      }
    }
  }
#endif
}

// //Initialisastion routine...sets up ESP-NOW and other initialisation items
void nowRail::init() {
  //start wifi and print mac address

  WiFi.mode(WIFI_STA);
  //1.4.1 changes channel if defined
#if defined(WIFICHANNEL)
  WiFi.setChannel(WIFICHANNEL);  //If not set default channel will be 1 or if using wifi the channel the board has connected to at router box
#endif
  /////////new code required for wifi sta to start up///////////
#if ESP_IDF_VERSION_MAJOR > 4  // IDF 5.xxx
  while (!WiFi.STA.started()) {
    delay(100);
  }
#endif
  /////////end new code


  Serial.print("Original MAC address: ");
  Serial.println(WiFi.macAddress());

  char str[17];
  String myString = WiFi.macAddress();

  strcpy(str, myString.c_str());
  uint8_t MAC[6];
  char *ptr;  //start and end pointer for strtol

  MAC[0] = strtol(str, &ptr, HEX);
  for (uint8_t i = 1; i < 6; i++) {
    MAC[i] = strtol(ptr + 1, &ptr, HEX);
  }

  _transmissionPrefix[4] = MAC[0];
  _transmissionPrefix[5] = MAC[1];
  _transmissionPrefix[6] = MAC[2];
  _transmissionPrefix[7] = MAC[3];
  _transmissionPrefix[8] = MAC[4];
  _transmissionPrefix[9] = MAC[5];

  int q;
  for (q = 0; q < 10; q++) {
    Serial.print(_transmissionPrefix[q], HEX);
    Serial.print(" : ");
  }
  Serial.println(" ");


  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  //register for incoming
  esp_now_register_recv_cb(OnDataRecv);
  // register peer...outgoing
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // register first peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }


//Now deal with eeprom
#if defined(NOWDisc)
  if (NOWDisc > 0) {
    nowRail::useEEPROM(NOWDisc);  //Starts the eeprom if required
  }
#endif

#if defined(DCCEXSSERIAL2_ON)
  Serial2.begin(115200, SERIAL_8N1, SERIALRX2, SERIALTX2);  //RX2 =  pin 16 TX2 = pin 17
  Serial.println("Serial2 started...");
#endif

#if defined(CAB_BUS_ADDRESS)
  Serial2.begin(9600, SERIAL_8N1, SERIALRX2, SERIALTX2);  //RX2 =  pin 16 TX2 = pin 17
  Serial.println("Serial2 started...");
#endif

#if defined(MP3BUSYPIN)                                   //MP3 player
  Serial2.begin(9600, SERIAL_8N1, SERIALRX2, SERIALTX2);  //RX2 =  pin 16 TX2 = pin 17
  Serial.println("Serial2 started...");
  pinMode(MP3BUSYPIN, INPUT);  //Set the busy pin up as an interrupt

  nowRail::mp3PlayVolume(MP3VOLUME);  //Sets up ther default volume at start up
#endif

#if defined(DCCDECODERPIN)
  pinMode(DCCDECODERPIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(DCCDECODERPIN), dccPinState, CHANGE);  //v0.5 mod

#endif

#if defined(CAB_BUS_ADDRESS)
  pinMode(RS485ENABLLEPIN, OUTPUT);  // set up RS485 ready for communication
  delay(10);
  digitalWrite(RS485ENABLLEPIN, LOW);

#endif
}

//Sets up the layout address and starts system
nowRail::nowRail(int addr1, int addr2, int addr3, int addr4) {
  _transmissionPrefix[0] = addr1;
  _transmissionPrefix[1] = addr2;
  _transmissionPrefix[2] = addr3;
  _transmissionPrefix[3] = addr4;
}

#if defined(DCCDECODERPIN)  //functions changed for v0.5

//0.5 additions
unsigned long fifoDCCBuffer[256];
byte writeDCCPosition;

void IRAM_ATTR dccPinState() {
  fifoDCCBuffer[writeDCCPosition] = micros();
  writeDCCPosition++;
}


//re written for 1.3.2 due to bug
void nowRail::processDCC(void) {
  unsigned long interruptTime;
  byte bitVal;
  if (_readDCCPosition != writeDCCPosition) {                         //work to be done... bit received
    if (_currentInterruptPhasePosition == _interruptPhasePosition) {  //am I on the correct rise or Fall
      _dccMicros[1] = _dccMicros[0];
      _dccMicros[0] = fifoDCCBuffer[_readDCCPosition];  //now I'm only stpring correct phase
      interruptTime = _dccMicros[0] - _dccMicros[1];
      //Serial.println(interruptTime);
      if (interruptTime > ONEBITTIME && interruptTime < 200) {  //error

        _badBits++;
        if (_badBits > 10) {
          _badBits = 0;
          Serial.print("Re Sync DCC Signal Phase: ");
          Serial.println(interruptTime);
          if (_currentInterruptPhasePosition > 0) {
            _currentInterruptPhasePosition = 0;
          } else {
            _currentInterruptPhasePosition = 1;
          }
        }
      } else {                             //good bit
        if (interruptTime < ONEBITTIME) {  //is it a zero or 1
          bitVal = 1;                      //it's a 1
        } else {
          bitVal = 0;  //Zero received
        }
        //Serial.println(bitVal);
        //now start to work with bitVal
        if (_Preamble < 1) {  //hasn't finished preamble
          if (bitVal > 0) {   //if it's a 1 it could be preamble
            _PreambleCounter++;
          } else {  //if it's a zero start looking again
            _PreambleCounter = 0;
          }
          if (_PreambleCounter > 9) {  //NMRA say a decoder must receive a minimum of 10 "1" bits before accepting a packet
            _Preamble = 1;             //pramble condition met
            _PacketStart = 0;
            _PreambleCounter = 0;
            // Serial.print("P");
          }
        } else {  //preamble complete...look for packet start
          if (_PacketStart < 1) {
            if (bitVal == 0) {  //find the fist 0 after preamble
              _PacketStart = 1;
              _bitCounter = 1;
            }
          } else {  //we have a preamble and a packet start so now get the rest of the data ready for processing
            _packetData[_bitCounter] = bitVal;
            _bitCounter++;
            if (_bitCounter > 27) {  //should now have the whole packet in array
              _Preamble = 0;
              _PreambleCounter = 0;
              nowRail::processPacket();
            }
          }
        }
      }  //end of good bit
    }
    //update the phase position to read next interrupt
    if (_interruptPhasePosition > 0) {
      _interruptPhasePosition = 0;
    } else {
      _interruptPhasePosition = 1;
    }
    _readDCCPosition++;  //increment whatever the phase
  }
}

void nowRail::processPacket() {
  byte AddressByte = 0;
  byte InstructionByte = 0;
  byte ErrorByte = 0;
  byte errorTest;
  for (int q = 0; q < 8; q++) {
    if (_packetData[1 + q] > 0) {
      bitSet(AddressByte, 7 - q);
    } else {
      bitClear(AddressByte, 7 - q);
    }

    if (_packetData[10 + q] > 0) {
      bitSet(InstructionByte, 7 - q);
    } else {
      bitClear(InstructionByte, 7 - q);
    }

    if (_packetData[19 + q] > 0) {
      bitSet(ErrorByte, 7 - q);
    } else {
      bitClear(ErrorByte, 7 - q);
    }
  }
  //certain Addressess are reserved for the system so not shown to reduce load
  if (AddressByte > 0 && AddressByte != 0b11111110 && AddressByte != 0b11111111 && bitRead(AddressByte, 7) == 1 && bitRead(AddressByte, 6) == 0) {
    errorTest = AddressByte ^ InstructionByte;  //error byte test
    if (errorTest == ErrorByte) {               //only send command if error byte checks out
      nowRail::accDecoder(AddressByte, InstructionByte, ErrorByte);
    }
  }
}

void nowRail::accDecoder(byte AddressByte, byte InstructionByte, byte ErrorByte) {
  byte index;
  byte dir;
  int AccAddr;
  int BoardAddr;

  if (_packetData[27] == 1) {  //basic packet
    dir = bitRead(InstructionByte, 0);
    index = bitRead(InstructionByte, 1);
    if (bitRead(InstructionByte, 2) > 0) {
      bitSet(index, 1);
    }

    BoardAddr = AddressByte - 0b10000000;
    //now get the weird address system from instruction byte
    if (bitRead(InstructionByte, 4) < 1) {
      bitSet(BoardAddr, 6);
    }
    if (bitRead(InstructionByte, 5) < 1) {
      bitSet(BoardAddr, 7);
    }
    if (bitRead(InstructionByte, 6) < 1) {
      bitSet(BoardAddr, 8);
    }
    AccAddr = ((BoardAddr - 1) * 4) + index + 1;

    //the if command stops the same instruction being sent multiple times
    //as DCC systems tend to send instructions about 5 times
    if (_lastDCCAccCommand[0] != AccAddr || _lastDCCAccCommand[1] != dir) {
      _lastDCCAccCommand[0] = AccAddr;
      _lastDCCAccCommand[1] = dir;

      // Serial.println(index);
      // Serial.println(dir);
      // Serial.println(AccAddr);
      // Serial.println(BoardAddr);

      //now add commands to sendfifo
      nowRail::sendAccessoryCommand(AccAddr, dir, MESSRESPREQ);  //1 response required
    }
  } else {
    Serial.print("Ext pckt format not supported yet");
  }
}
#endif  //end of DCCDECODERPIN

#if defined(GT911)

//Function to initialise the GT911 touch screen
void nowRail::addGT911Screen(int GT911ResetPin, int GT911IntPin) {
  unsigned char _GTP_CFG_DATA[] = {
    0x5A, 0x20, 0x03, 0xE0, 0x01, 0x05, 0x0D, 0x00,
    0x01, 0x08, 0x28, 0x08, 0x50, 0x32, 0x03, 0x05,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x88, 0x29, 0x0A, 0x35, 0x37,
    0xD3, 0x07, 0x00, 0x00, 0x01, 0x81, 0x02, 0x1D,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x64, 0x32,
    0x00, 0x00, 0x00, 0x28, 0x5A, 0x94, 0xC5, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x98, 0x2B, 0x00, 0x84,
    0x33, 0x00, 0x74, 0x3C, 0x00, 0x67, 0x46, 0x00,
    0x5C, 0x53, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10,
    0x12, 0x14, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x04, 0x06, 0x08, 0x0F, 0x10, 0x12, 0x16, 0x18,
    0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x25, 0x01
  };
  _GT911_RESET = GT911ResetPin;  //CTP RESET
  _GT911_INT = GT911IntPin;      //CTP  INT
  _addr = GT911;
  uint8_t re = 0;
#if defined(CUSTOM_SDA)
  Wire.begin(CUSTOM_SDA, CUSTOM_SCL);  //custom pins
#else
  Wire.begin();        //standard pins
#endif
  delay(100);
  pinMode(_GT911_RESET, OUTPUT);
  pinMode(_GT911_INT, OUTPUT);
  digitalWrite(_GT911_RESET, LOW);
  //delay(20);
  digitalWrite(_GT911_INT, LOW);
  delay(10);
  digitalWrite(_GT911_RESET, HIGH);
  delay(6);
  pinMode(_GT911_RESET, INPUT);
  delay(50);
  re = nowRail::GT911_Send_Cfg((uint8_t *)_GTP_CFG_DATA, sizeof(_GTP_CFG_DATA));
  pinMode(_GT911_RESET, OUTPUT);
  pinMode(_GT911_INT, OUTPUT);
  digitalWrite(_GT911_RESET, LOW);
  //delay(20);
  digitalWrite(_GT911_INT, LOW);
  delay(10);
  digitalWrite(_GT911_RESET, HIGH);
  delay(6);
  pinMode(_GT911_INT, INPUT);
  delay(50);
  re = nowRail::GT911_Send_Cfg((uint8_t *)_GTP_CFG_DATA, sizeof(_GTP_CFG_DATA));
  uint8_t bb[2];
  nowRail::readGT911TouchAddr(0x8047, bb, 2);
  while (bb[1] != 32) {
    Serial.println("Capacitive touch screen initialized failure");
    pinMode(_GT911_RESET, OUTPUT);
    pinMode(_GT911_INT, OUTPUT);
    digitalWrite(_GT911_RESET, LOW);
    //delay(20);
    digitalWrite(_GT911_INT, LOW);
    delay(10);
    digitalWrite(_GT911_RESET, HIGH);
    delay(6);
    pinMode(_GT911_INT, INPUT);
    delay(50);
    uint8_t re = nowRail::GT911_Send_Cfg((uint8_t *)_GTP_CFG_DATA, sizeof(_GTP_CFG_DATA));
  }
  Serial.println("Capacitive touch screen  initialized success");
}

uint8_t nowRail::GT911_Send_Cfg(uint8_t *buf, uint16_t cfg_len) {
  uint8_t retry = 0;
  for (retry = 0; retry < 5; retry++) {
    nowRail::writeGT911TouchRegister(0x8047, buf, cfg_len);
    //if(ret==0)break;
    delay(10);
  }
  return retry;
}

void nowRail::writeGT911TouchRegister(uint16_t regAddr, uint8_t *val, uint16_t cnt) {
  uint16_t i = 0;

  Wire.beginTransmission(_addr);
  Wire.write(regAddr >> 8);         // register 0
  Wire.write(regAddr);              // register 0
  for (i = 0; i < cnt; i++, val++)  //data
  {
    Wire.write(*val);  // value
  }
  uint8_t retVal = Wire.endTransmission();
}

//function that reads from the GT911...do not edit
uint8_t nowRail::readGT911TouchAddr(uint16_t regAddr, uint8_t *pBuf, uint8_t len) {
  uint8_t i;
  uint8_t returned;
  uint8_t retVal;
  Wire.beginTransmission(_addr);
  Wire.write(regAddr >> 8);  // register 0
  Wire.write(regAddr);       // register 0
  retVal = Wire.endTransmission();

  returned = Wire.requestFrom(_addr, len);  // request 1 bytes from slave device #2
  for (i = 0; (i < len) && Wire.available(); i++)

  {
    pBuf[i] = Wire.read();
  }
  return i;
}





////function that works out the touch coordinates for GT911...do not edit
uint8_t nowRail::readGT911TouchLocation(TouchLocation *pLoc, uint8_t num) {
  uint8_t retVal;
  uint8_t i;
  uint8_t k;
  uint8_t ss[1];
  do {
    if (!pLoc) break;  // must have a buffer
    if (!num) break;   // must be able to take at least one
    ss[0] = 0;

    nowRail::readGT911TouchAddr(0x814e, ss, 1);
    uint8_t status = ss[0];

    if ((status & 0x0f) == 0) break;  // no points detected
    uint8_t hitPoints = status & 0x0f;
    //Serial.print("number of hit points = ");
    //  Serial.println( hitPoints );

    uint8_t tbuf[40];  //changed to 40 as that is number called for in  readGT911TouchAddrTest( 0x8150, tbuf, 40);
    uint8_t tbuf1[8];
    nowRail::readGT911TouchAddr(0x8150, tbuf, 40);
    nowRail::readGT911TouchAddr(0x8150 + 32, tbuf1, 8);

    for (k = 0, i = 0; (i < 4 * 8) && (k < num); k++, i += 8) {
      pLoc[k].x = tbuf[i + 1] << 8 | tbuf[i + 0];
      pLoc[k].y = tbuf[i + 3] << 8 | tbuf[i + 2];
    }
    pLoc[k].x = tbuf1[1] << 8 | tbuf1[0];
    pLoc[k].y = tbuf1[3] << 8 | tbuf1[2];

    retVal = hitPoints;

  } while (0);

  ss[0] = 0;
  nowRail::writeGT911TouchRegister(0x814e, ss, 1);

  //delay(2);
  return retVal;
}

//Function that uses the functions above to give a simple X/Y position to be used in your code
//rerwritten for version 1.5.0 due to issues with ESP32 v3.2.0 or greater
void nowRail::checkfortouchscreen() {

  uint16_t newcaptouchx = 0;
  uint16_t newcaptouchy = 0;
  unsigned long capCurrentMillis = millis();
  _captouched = 0;
  pinMode(_GT911_INT, INPUT);
  uint8_t st = digitalRead(_GT911_INT);
  if (!st)  //Hardware touch interrupt
  {

    //Screen can deal with 5 touches at once
    uint8_t count = nowRail::readGT911TouchLocation(touchLocations, 5);
    if (count != 0) {
      // Serial.println(count);
      static TouchLocation caplastTouch = touchLocations[0];  // only take single touch, not dealing with multitouch
      caplastTouch = touchLocations[0];
      if (capCurrentMillis - _captouchbounce >= BUTTONDEBOUNCE) {  //cuts out multitouch
        _captouchbounce = capCurrentMillis;

        //only using first touch for now
        newcaptouchx = touchLocations[0].x;  //I only want 1 touch
        newcaptouchy = touchLocations[0].y;  //I only want 1 touch
        if (_captouchx != newcaptouchx || _captouchy != newcaptouchy) {
          _captouched = 1;
          _captouchx = newcaptouchx;
          _captouchy = newcaptouchy;
          nowGT911Touch(_captouchx, _captouchy);
        }
      }
    }
  }
}

void nowRail::addGT911Button(int xPos, int yPos, int accNum, int press1, int press2) {
  if (_CD4021PinCount < GT911TOUCHBUTTONS) {
    _GT911Buttons[_GT911ButtonCount][0] = xPos;
    _GT911Buttons[_GT911ButtonCount][1] = yPos;
    _GT911Buttons[_GT911ButtonCount][2] = accNum;
    _GT911Buttons[_GT911ButtonCount][3] = press1;
    _GT911Buttons[_GT911ButtonCount][4] = press2;
    _GT911ButtonCount++;
  } else {
    Serial.println("addGT911Button: item not added, too many in system. Increase GT911TOUCHBUTTONS in nowrail_user_setup.h");
  }
}

void nowRail::GT911ProcessButtons() {
  int q;
  checkfortouchscreen();  //keep looking for screen touches
  if (_captouched > 0) {
    //Serial.println(captouchx);
    //Serial.println(captouchy);
    for (q = 0; q < _GT911ButtonCount; q++) {
      if (_captouchx > (_GT911Buttons[q][0] - GT911TOUCHRADIUS) && _captouchx < (_GT911Buttons[q][0] + GT911TOUCHRADIUS) && _captouchy > (_GT911Buttons[q][1] - GT911TOUCHRADIUS) && _captouchy < (_GT911Buttons[q][1] + GT911TOUCHRADIUS)) {
        //Serial.print("GT: ");
        //Serial.println(q);
        if (_GT911Buttons[q][5] > 3) {
          _GT911Buttons[q][5] = 3;  //set to target for this button press
        } else {
          _GT911Buttons[q][5] = 4;  //set to target for this button press
        }
        nowRail::sendAccessoryCommand(_GT911Buttons[q][2], _GT911Buttons[q][_GT911Buttons[q][5]], MESSRESPREQ);  //1 response required
      }
    }
  }
}
#endif

//Loco functions ...gets and updates data from _locoData[200][32]
//gets DCC address
int nowRail::getLocoDCCAddress(byte locoID) {
  int dccAddress;
  if (locoID < 200) {                                                  //check within range
    dccAddress = (_locoData[locoID][2] * 256) + _locoData[locoID][3];  //loco HIGH byte and low byte combined
  }
  return dccAddress;
}
//gets a function state
byte nowRail::getLocoDCCFuncState(byte locoID, byte funcNum) {
  byte funcState;
  byte funcArrayByte;
  funcArrayByte = 4;                   //lowest fuinction byte is in array pos 4
  if (locoID < 200 && funcNum < 64) {  //check within range
    while (funcNum > 7) {
      funcNum = funcNum - 8;
      funcArrayByte++;
    }
    funcState = bitRead(_locoData[locoID][funcArrayByte], funcNum);
  }
  return funcState;
}

String nowRail::getLocoName(byte locoID) {
  int q;
  String locoString;
  if (locoID < 200) {  //check within range
    for (int q = 13; q < 32; q++) {
      locoString = locoString + char(_locoData[locoID][q]);
    }
  }
  locoString.trim();  //remove white space
  return locoString;
}

//sets an address within the _locodata array
void nowRail::setLocoDCCAddress(byte locoID, int dccAddress) {
  if (locoID < 200 && dccAddress < 10239 && dccAddress > 0) {  //check within range 10239 is current maximum address value
    _locoData[locoID][2] = (dccAddress >> 8) & 0xFF;
    _locoData[locoID][3] = dccAddress & 0xFF;
#if defined(NOWDisc)
    nowRail::locoEEPROMUpdate(locoID);
#endif
  }
}

void nowRail::setLocoName(byte locoID, String locoName) {
  int q;
  int nameLength;
  char myBuffer[20];
  locoName.trim();
  nameLength = locoName.length();
  //nameLength = nameLength - 1;
  if (nameLength > 20) {
    nameLength = 20;
  }
  if (locoID < 200) {  //check within range

    for (q = 0; q < nameLength; q++) {
      _locoData[locoID][q + 13] = locoName.charAt(q);
    }
    while (q < 20) {  //fill any excess with ' ' in case string is shorter than last name
      _locoData[locoID][q + 13] = ' ';
      q++;
    }
#if defined(NOWDisc)
    nowRail::locoEEPROMUpdate(locoID);
#endif
  }
}

void nowRail::setLocoDCCFuncState(byte locoID, byte funcNum, byte funcState) {
  byte funcArrayByte;
  int dccAddress;
  byte sendFuncNum;

  sendFuncNum = funcNum;

  funcArrayByte = 4;                                  //lowest function byte is in array pos 4
  if (locoID < 200 && funcNum < 64) {                 //check within range
    dccAddress = nowRail::getLocoDCCAddress(locoID);  //get DCC address to send out command
    if (funcState > 0) {
      funcState = 1;
    }
    //nowRail::sendDCCLocoFunc(locoID, dccAddress, funcNum, funcState);
    while (funcNum > 7) {
      funcNum = funcNum - 8;
      funcArrayByte++;
    }
    if (funcState == 0) {
      bitClear(_locoData[locoID][funcArrayByte], funcNum);
    } else {
      bitSet(_locoData[locoID][funcArrayByte], funcNum);
    }
    //   Serial.print("locoID ");
    // Serial.println(locoID);
    // Serial.print("funcNum ");
    // Serial.println(sendFuncNum);
    // Serial.print("funcState ");
    // Serial.println(funcState);
    nowRail::sendDCCLocoFunc(locoID, dccAddress, sendFuncNum, funcState);  //order changed for NCE system
#if defined(NOWDisc)
    nowRail::locoEEPROMUpdate(locoID);
#endif
  }
}

//nowRail 1.2.0
byte nowRail::getLocoTXdataSetByte(byte item) {
  byte returnValue;
  if (item < 32) {
    returnValue = _locoTXdataSet[item];
  }
  return returnValue;
}

byte _locoRXState[2];  //0 = 0 or 1 = waiting tom receive [1] = locoID to be updated

//used to set the loco slot for incoming data
void nowRail::setLocoRXState(byte state, byte updateLocoID) {
  _locoRXState[0] = state;
  _locoRXState[1] = updateLocoID;
}

//gets the locos current consist state
byte nowRail::locoGetConsistState(byte locoID) {
  byte consistState;
  consistState = _locoData[locoID][12];  //12 is the consist byte
  return consistState;
}

//sets the consist state
void nowRail::locoSetConsistState(byte locoID, byte consistState) {
  _locoData[locoID][12] = consistState;
  nowRail::locoEEPROMUpdate(locoID);  //update the eeprom
}

//sends the full data array for a single loco
void nowRail::locoTXLocoData(byte locoID) {
  int q;
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageLOW BYTE ID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;        //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = LOCODATATX;  //LOCODATATX 9
  for (q = 0; q < 32; q++) {
    // Serial.println(q);
    sendFifoBuffer[sendWriteFifoCounter][q + 16] = _locoData[locoID][q];
  }
  nowRail::incsendWriteFifoCounter();  //new function to add highbyte to messageID
}

//1.3.0 mod allows all locos data to be transmitted
void nowRail::locoTXAllLocoData() {
  int q;
  int w;
  for (w = 0; w < 200; w++) {
    memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);
    sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageLOW BYTE ID
    sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
    sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;
    sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;            //0 as new message
    sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = LOCOBULKDATATX;  //LOCOBULKDATATX 10
    for (q = 0; q < 32; q++) {
      // Serial.println(q);
      sendFifoBuffer[sendWriteFifoCounter][q + 16] = _locoData[w][q];
    }
    nowRail::incsendWriteFifoCounter();  //new function to add highbyte to messageID
  }
}

//end loco functions

//JMRI CMRI functions
#if defined(JMRICMRI)
void nowRail::jmriCmri() {  //Main processing loop
  byte incomingByte;
  //while (Serial.available() > 0) {
  if (Serial.available() > 0) {
    //serialBuffer = Serial.available();
    incomingByte = Serial.read();
    //Serial2.write(incomingByte);
    jmriCmriProcessByte(incomingByte);
  }
}

//build up packets and ecide if for this node
void nowRail::jmriCmriProcessByte(byte incomingByte) {
  switch (_jmriCmriPacketSync) {
    case 0 ... 1:  //check the first 2 255 bytes
      if (incomingByte == 255) {
        _jmriCmriPacketSync++;
      } else {
        _jmriCmriPacketSync = 0;  //rest of packet will be ignored as not for me
      }
      _jmriCmriByteCount = 0;   //reset the byteCount
      _jmriCmriPacketType = 0;  //reset for new transmission
      break;
    case 2:  //Is it STX... start byte
      if (incomingByte == 2) {
        _jmriCmriPacketSync++;
      } else {
        _jmriCmriPacketSync = 0;  //rest of packet will be ignored as not for me
      }
      break;
    case 3:  //Is it my node 65 + thisNode
      if (incomingByte == 65 + JMRICMRI) {
        _jmriCmriPacketSync++;
      } else {
        _jmriCmriPacketSync = 0;  //rest of packet will be ignored as not for me
      }
      break;
    case 4:  //Is it a Transmit 84 T Transmit data  I intyialisation P poll request R receive data
      switch (incomingByte) {
        case 84:  //T Transmision received
          _jmriCmriPacketType = incomingByte;
          _jmriCmriPacketSync++;
          break;
        case 80:  //P Polled
          //Serial.println("Poll");
          _jmriCmriPacketType = incomingByte;
          _jmriCmriPacketSync++;  //Stores contents so I know when end of packet is reached.
          break;
        default:
          _jmriCmriPacketSync = 0;
          break;
      }

      break;
    case 5:  //process the data
      _jmriCmriByteRecBuffer[_jmriCmriByteCount][0] = incomingByte;

      //new end of transmission
      if (_jmriCmriByteRecBuffer[_jmriCmriByteCount][0] == 3) {
        if (_jmriCmriPacketType == 80) {  //Poll
          jmriCmriProcessSendBuffer();
          _jmriCmriByteCount = 0;
          _jmriCmriPacketSync = 0;
        }
        if (_jmriCmriPacketType == 84) {  //Transmission
          if (_jmriCmriByteRecBuffer[_jmriCmriByteCount][0] == 3 && _jmriCmriByteRecBuffer[_jmriCmriByteCount - 1][0] != 16) {
            jmriCmriProcessRecBuffer();
            _jmriCmriByteCount = 0;
            _jmriCmriPacketSync = 0;
          }
        }
      }


      _jmriCmriByteCount++;
      break;
    default:
      break;
  }
}

//Process the received bytes
void nowRail::jmriCmriProcessRecBuffer() {
  int q;                                          //reads through the bytes
  int w;                                          //works through byte
  int a = 1;                                      //counts through all the bits
  int e;                                          //e reads through [1]..due to the ESC issue
  for (q = 0; q < _jmriCmriByteCount - 1; q++) {  //-1 because last byte is the end of transmission
    if (_jmriCmriByteRecBuffer[q][0] == 16) {     //esc
      if (_jmriCmriByteRecBuffer[q + 1][0] == 2 || _jmriCmriByteRecBuffer[q + 1][0] == 3 || _jmriCmriByteRecBuffer[q + 1][0] == 16) {
        q++;  //skip byte
      }
    }
    if (_jmriCmriByteRecBuffer[q][0] != _jmriCmriByteRecBuffer[e][1]) {
      for (w = 0; w < 8; w++) {
        if (bitRead(_jmriCmriByteRecBuffer[q][0], w) != bitRead(_jmriCmriByteRecBuffer[e][1], w)) {
          // Serial2.print("CM: ");
          // Serial2.print(a);//a is acc number
          // Serial2.print(": ");
          // Serial2.println(bitRead(_jmriCmriByteRecBuffer[q][0], w));//direction
          //Changed light/turnout address is now known...success...send via nowRail
          nowRail::sendAccessoryCommand(a, bitRead(_jmriCmriByteRecBuffer[q][0], w), MESSRESPREQ);  //1 response required
        }
        a++;
      }
      _jmriCmriByteRecBuffer[e][1] = _jmriCmriByteRecBuffer[q][0];  //set the old byte for next time
    } else {                                                        //skip the byte..no change
      a = a + 8;
    }
    e++;
  }
}

//process the send buffer
void nowRail::jmriCmriProcessSendBuffer() {
  int q;

  Serial.write(255);
  Serial.write(255);
  Serial.write(2);              //STX
  Serial.write(65 + JMRICMRI);  //address
  Serial.write(82);             //R = replay
  for (q = 0; q < JMRICMRINUMCARDS; q++) {
    if (_jmriCmriSendData[q] == 2 || _jmriCmriSendData[q] == 3 || _jmriCmriSendData[q] == 16) {
      Serial.write(16);  //send escape character
    }
    Serial.write(_jmriCmriSendData[q]);  //work through the data bytes
  }

  Serial.write(3);  //ETX end of message
  //Serial.flush();
}

void nowRail::jmriCmriUpdateSendBuffer(int senNum, byte senInst) {
  byte updateByte;
  while (senNum > 7) {
    updateByte++;
    senNum = senNum - 8;
  }
  bitWrite(_jmriCmriSendData[updateByte], senNum - 1, senInst);
}

#endif

//END JMRO CMRI functions

//NCA Cab BUS
#if defined(CAB_BUS_ADDRESS)

//Main process called on every loop
void nowRail::nceProcess() {
  byte readByte;
  byte polledCab;
  byte clockHour;
  byte clockMin;
  byte clockSpeedCal;
  static byte lastClockTime;
  byte setClockSpeed;
  const int transmitTimer = 780;  //may need to be tweaked
  int q;
  while (Serial2.available()) {
    readByte = Serial2.read();
    //Serial.println(readByte,BIN);
    if (bitRead(readByte, 7) == 1 && bitRead(readByte, 6) == 0) {
      //command poll
      polledCab = readByte;
      bitClear(polledCab, 7);              //Get the cab number polled
      if (polledCab == CAB_BUS_ADDRESS) {  //for this cab...send any outstanding bytes
        //Serial.println("poll");
        if (_currentMillis - _messageDelayMillis >= _messageDelayTimer) {  //1.9.2...timer to test consist issues
          _messageDelayMillis = _currentMillis;
          if (_messageDelayTimer > 0) {
            Serial.println(_currentMillis);
            Serial.println(_messageDelayMillis);
            Serial.println(_messageDelayTimer);
          }
          if (_nceCommandBytesRead != _nceCommandBytesWrite) {  //0.8.2
            delayMicroseconds(transmitTimer);
            digitalWrite(RS485ENABLLEPIN, HIGH);

            Serial2.write(_nceCommandBytes[_nceCommandBytesRead], 5);  //send the command
            Serial2.flush();
            digitalWrite(RS485ENABLLEPIN, LOW);

            if (_nceCommandBytes[_nceCommandBytesRead][2] > 0x09 && _nceCommandBytes[_nceCommandBytesRead][2] < 0x12) {
              _messageDelayTimer = 1000;
            } else {
              _messageDelayTimer = 0;
            }

#if defined(DIAGNOSTICS_ON)  //stop it being repeated again as the send system will re add record
            Serial.println("DIAG>NCE>Send");
            for (q = 0; q < 5; q++) {

              Serial.print(_nceCommandBytes[_nceCommandBytesRead][q], HEX);
              Serial.print(" , ");
            }
            Serial.println(" ");

#endif
            _nceCommandBytesRead++;  //move to next read pos 0.8.2
            //_sendNCEFlag = 0;  //reset for new instruction
          }
        }  //end millis
      }
    } else {  //not a cab poll
              //Serial.println(readByte,HEX);
#if defined(MASTERCLOCK_ON)
      if (_nceclockMode < 1) {
        if (readByte == 0xC1) {  //clock data to follow
          _nceclockMode = 1;
          _nceclockCount = 0;

          clockSpeedCal = nowRail::rtcMinutes();
          if (clockSpeedCal > lastClockTime + 2) {  //allows for some error
            Serial.print(clockSpeedCal);
            Serial.print(" : ");
            Serial.println(lastClockTime + rtcClockSpeed());
            // lastClockTime = clockSpeedCal;//reset to latest minutes
            setClockSpeed = rtcClockSpeed() + 1;
            if (setClockSpeed > 15) {
              setClockSpeed = 0;
            }
            nowRail::sendClockSpeedChange(setClockSpeed);
          }
          lastClockTime = clockSpeedCal;
        }
      } else {
        _nceclockData[_nceclockCount] = readByte;
        _nceclockCount++;
        if (_nceclockCount > 7) {
          _nceclockMode = 0;
          // Serial.println("clk");
          // for(q=0;q<8;q++){
          //  Serial.println(_nceclockData[q],HEX);
          // }
          //Serial.println("clk Conv");
          for (q = 0; q < 8; q++) {
            if (bitRead(_nceclockData[q], 5) > 0) {  //turn to text byte
              bitClear(_nceclockData[q], 6);
            }
            bitClear(_nceclockData[q], 7);
            //   Serial.print(char(_nceclockData[q]));
          }
          // Serial.println("clk byte");
          //  for(q=0;q<8;q++){

          //     Serial.println(_nceclockData[q]);
          //  }
          clockHour = _nceclockData[2] - 48;
          if (_nceclockData[1] - 48 > 0) {
            clockHour = clockHour + 10;
          }
          if (_nceclockData[6] == 80) {  //PM
            clockHour = clockHour + 12;
          }
          _timeArray[1] = clockHour;
          clockMin = ((_nceclockData[4] - 48) * 10) + (_nceclockData[5] - 48);
          _timeArray[2] = clockMin;
          _timeArray[3] = 0;
        }
      }
#endif
    }
  }
}
//byte _timeArray[5]; //Clockspeed, Hour, Min, Sec, Day of week

//takes nowRail command and changes to NCE...working
void nowRail::nceAccComProcess(int addr, byte dir) {
  _nceCommandBytes[_nceCommandBytesWrite][0] = 0x50 + (addr >> 7);
  _nceCommandBytes[_nceCommandBytesWrite][1] = addr & 0x7F;
  if (dir > 0) {
    _nceCommandBytes[_nceCommandBytesWrite][2] = 0x03;
  } else {
    _nceCommandBytes[_nceCommandBytesWrite][2] = 0x04;
  }
  _nceCommandBytes[_nceCommandBytesWrite][3] = 0x00;
  _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
  //_sendNCEFlag = 1;
  _nceCommandBytesWrite++;
}

//sends nowrail to NCE loco speed
void nowRail::locoComToNCE(int addr, int speed, int dir) {
  int q;
  if (addr < 0x80 && CAB_BUS_SHORTADDRON == 1) {  //less than 128 short address
    _nceCommandBytes[_nceCommandBytesWrite][0] = 0x4F;
    _nceCommandBytes[_nceCommandBytesWrite][1] = addr;
  } else {
    _nceCommandBytes[_nceCommandBytesWrite][0] = addr >> 7;
    _nceCommandBytes[_nceCommandBytesWrite][1] = addr & 0x7F;
  }
  if (dir > 0) {
    _nceCommandBytes[_nceCommandBytesWrite][2] = 0x04;  //forward
  } else {
    _nceCommandBytes[_nceCommandBytesWrite][2] = 0x03;  //reverse
  }
  _nceCommandBytes[_nceCommandBytesWrite][3] = speed;
  _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
  //mod for stopping...sends stop command twice 0_8_3 mod

  if (speed == 0) {
    for (q = 0; q < 5; q++) {  //make a 2nd copy of the command
      _nceCommandBytes[_nceCommandBytesWrite + 1][q] = _nceCommandBytes[_nceCommandBytesWrite][q];
    }
    _nceCommandBytesWrite++;
  }
  _nceCommandBytesWrite++;
}


void nowRail::locoFuncToNCE(int addr, byte funcGroup, byte funcData) {
  if (addr < 0x80 && CAB_BUS_SHORTADDRON == 1) {  //less than 128 short address and Short addresses wanted
    _nceCommandBytes[_nceCommandBytesWrite][0] = 0x4F;
    _nceCommandBytes[_nceCommandBytesWrite][1] = addr;
  } else {
    _nceCommandBytes[_nceCommandBytesWrite][0] = addr >> 7;
    _nceCommandBytes[_nceCommandBytesWrite][1] = addr & 0x7F;
  }
  _nceCommandBytes[_nceCommandBytesWrite][2] = funcGroup;

  _nceCommandBytes[_nceCommandBytesWrite][3] = funcData;
  _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
  //_sendNCEFlag = 1;
  _nceCommandBytesWrite++;
}

void nowRail::ncePowerOff(int addr, int speed, int dir) {  //turn power off for all locos E Stop...1.2.2 mod

  int q;
  if (addr < 0x80 && CAB_BUS_SHORTADDRON == 1) {  //less than 128 short address
    _nceCommandBytes[_nceCommandBytesWrite][0] = 0x4F;
    _nceCommandBytes[_nceCommandBytesWrite][1] = addr;
  } else {
    _nceCommandBytes[_nceCommandBytesWrite][0] = addr >> 7;
    _nceCommandBytes[_nceCommandBytesWrite][1] = addr & 0x7F;
  }
  if (dir > 0) {
    _nceCommandBytes[_nceCommandBytesWrite][2] = 0x06;  //forced to forwards its an emergency stop
  } else {
    _nceCommandBytes[_nceCommandBytesWrite][2] = 0x05;  //reverse
  }
  _nceCommandBytes[_nceCommandBytesWrite][3] = 0;
  _nceCommandBytes[_nceCommandBytesWrite][4] = _nceCommandBytes[_nceCommandBytesWrite][0] ^ _nceCommandBytes[_nceCommandBytesWrite][1] ^ _nceCommandBytes[_nceCommandBytesWrite][2] ^ _nceCommandBytes[_nceCommandBytesWrite][3];
  //mod for stopping...sends stop command 3 times for E Stop 1.2.2 mod
  _nceCommandBytesWrite++;
  if (speed == 0) {
    for (q = 0; q < 5; q++) {  //make a 2nd copy of the command
      _nceCommandBytes[_nceCommandBytesWrite][q] = _nceCommandBytes[_nceCommandBytesWrite - 1][q];
    }
    _nceCommandBytesWrite++;
    for (q = 0; q < 5; q++) {  //make a 3rd copy of the command
      _nceCommandBytes[_nceCommandBytesWrite][q] = _nceCommandBytes[_nceCommandBytesWrite - 1][q];
    }
    _nceCommandBytesWrite++;
  }
}

#endif

#if defined(MP3BUSYPIN)
void nowRail::mp3SendCommand() {
  int q;
#if defined(DIAGNOSTICS_ON)
  Serial.println("DIAG>");
#endif
  for (q = 0; q < _mp3CommandLength; q++) {
    Serial2.write(_mp3Command[q]);
#if defined(DIAGNOSTICS_ON)
    Serial.print(" ");
    Serial.print(_mp3Command[q], HEX);
#endif
  }
#if defined(DIAGNOSTICS_ON)
  Serial.println(" End");
#endif
}


void nowRail::mp3PlayVolume(int vol) {
  if (vol > 30) {  //check within limits
    vol = 30;
  }
  _mp3Command[0] = 0xAA;  //first byte says it's a command
  _mp3Command[1] = 0x13;
  _mp3Command[2] = 0x01;
  _mp3Command[3] = vol;  //volume
  _mp3CheckSum = 0;
  for (int q = 0; q < 4; q++) {
    _mp3CheckSum += _mp3Command[q];
  }
  _mp3Command[4] = (_mp3CheckSum & 0xFF);  //SM check bit... low bit of the sum of all previous values
  _mp3CommandLength = 5;
  mp3SendCommand();
}

void nowRail::mp3PlayTrack(int trackNum) {
  //select track
#if defined(DIAGNOSTICS_ON)
  Serial.print("DIAG>soundTrack: ");
  Serial.println(trackNum);
#endif
  _mp3Command[0] = 0xAA;  //first byte says it's a command
  _mp3Command[1] = 0x07;
  _mp3Command[2] = 0x02;
  _mp3Command[3] = (trackNum >> 8) & 0xFF;  //snh...track HIGH bit
  _mp3Command[4] = trackNum & 0xFF;         //SNL... track low bit
  _mp3CheckSum = 0;
  for (int q = 0; q < 5; q++) {
    _mp3CheckSum += _mp3Command[q];
  }
  _mp3Command[5] = (_mp3CheckSum & 0xFF);  //SM check bit... low bit of the sum of all previous values
  _mp3CommandLength = 6;
  mp3SendCommand();
}



void nowRail::mp3Control() {  //deals with multi track play etc
  int q;

  if (digitalRead(MP3BUSYPIN) > 0) {  //nothing playing
    _mp3BusyHigh++;
    //Serial.println(_mp3BusyHigh++);
    if (_mp3BusyHigh == 1) {
      _mp3Millis = _currentMillis;
      _mp3PlayNextTrack = 1;
    }
  } else {  //track playing
    _mp3BusyHigh = 0;
  }

  if (_currentMillis - _mp3Millis >= _mp3Timer && _mp3PlayNextTrack > 0) {
    for (q = 0; q < _mp3NumAccs; q++) {
      if (_mp3Accessories[q][5] == 1) {                                                   //if function has been triggered
        nowRail::mp3PlayTrack(random(_mp3Accessories[q][2], _mp3Accessories[q][3] + 1));  //play a random track
        _mp3PlayNextTrack = 0;
        _mp3Timer = random(MP3INTERVALMIN, MP3INTERVALMAX);
        _mp3Millis = _currentMillis;
      }
    }
  }
}
#endif

void nowRail::locoRXAllLocoData(int state) {
  if (state > 0) {
    _locoBulkDataRXPos = 0;   //set the postion
    _locoBulkDataRXFlag = 1;  //set receive flag
  } else {
    _locoBulkDataRXFlag = 0;  //turn off receive flag
  }
}
//byte _locoBulkDataRXFlag;//1 = waiting to receive data
//byte _locoBulkDataRXPos;

//RFID data
void nowRail::sendRFIDData(uint8_t rfidData[], uint8_t len) {                        //receives and transmits data
  if (len < 31) {                                                                    //max 30 byes
    memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
    sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
    sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
    //sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;             //Doesn't require response, could go to multiple panels
    sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPREQ;
    sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;      //0 as new message
    sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = RFIDDATA;  //FASTCLOCKUPDATE
    sendFifoBuffer[sendWriteFifoCounter][RFIDNUMBYTES] = len;      //send length of data
    memcpy(sendFifoBuffer[sendWriteFifoCounter] + RFIDDATASTART, rfidData, len);

    incsendWriteFifoCounter();

  } else {  //send error message and DO NOT transmit
    Serial.print("ERROR> sendRFIDData  too much data, max 30 bytes ");
    Serial.print(len);
    Serial.println(" sent");
  }
}

//analogue sensors
void nowRail::sensorCustomValue(uint16_t senNum, int32_t senValue) {
  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  //sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;             //Doesn't require response, could go to multiple panels
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPREQ;
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;                  //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = ANALOGUESENSORUPDATE;  //sending sensor update
  sendFifoBuffer[sendWriteFifoCounter][SENADDRHIGH] = (senNum >> 8) & 0xFF;  //get High Byte
  sendFifoBuffer[sendWriteFifoCounter][SENADDRLOW] = senNum & 0xFF;

  sendFifoBuffer[sendWriteFifoCounter][18] = (senValue >> 24) & 0xFF;
  sendFifoBuffer[sendWriteFifoCounter][19] = (senValue >> 16) & 0xFF;
  sendFifoBuffer[sendWriteFifoCounter][20] = (senValue >> 8) & 0xFF;  //get High Byte
  sendFifoBuffer[sendWriteFifoCounter][21] = senValue & 0xFF;         //get low byte
  incsendWriteFifoCounter();
}

//DCCEXCUSTOMCMD
void nowRail::sendDCCEXCustomCmd(String cmdString) {
  char stringArray[30];
  int q;
  int cmdLength;
  cmdLength = cmdString.length();  //get the length of the string
  if (cmdLength > 30) {
    cmdLength = 30;  //max transmission 30 bytes don;'t send half a transmission
    Serial.print("ERROR> sendDCCEXCustomCmd  String too long, max 30 chars ");
    Serial.print(cmdLength);
    Serial.println(" sent");
  } else {
    strcpy(stringArray, cmdString.c_str());                                          //copy string into array...now I know it fits
    memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
    sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
    sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
    //sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;             //Doesn't require response, could go to multiple panels
    sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPREQ;

    sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;            //0 as new message
    sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = DCCEXCUSTOMCMD;  //sending sensor update
    sendFifoBuffer[sendWriteFifoCounter][16] = cmdLength;                //let the other end know how lng the string is
    for (q = 0; q < cmdLength; q++) {                                    //put the command in the send fifo
      sendFifoBuffer[sendWriteFifoCounter][17 + q] = stringArray[q];
    }

    incsendWriteFifoCounter();
  }
}

//1.8.3 turns sensor repeats off
void nowRail::sensorProcessed() {
  sendMessResp();  //send message response
}

//1_9_2 consist functions
//this function ends the consist by transmitting a UNSETCONSIST command
// void nowRail::endConsist(uint16_t consistAddress) {
//   memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
//   sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
//   sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
//   sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;
//   sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;          //0 as new message
//   sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = UNSETCONSIST;  //UNset consist
//   sendFifoBuffer[sendWriteFifoCounter][CONSISTADDRHIGH] = consistAddress >> 8;
//   sendFifoBuffer[sendWriteFifoCounter][CONSISTADDRLOW] = consistAddress;
//   incsendWriteFifoCounter();
// }


//new version of function as cab bus requires list of locos
void nowRail::endConsist(byte numCLocos, int16_t consistAddress, int16_t LeadLocoAddress, int16_t Loco2Address, int16_t Loco3Address, int16_t Loco4Address, int16_t Loco5Address, int16_t Loco6Address, int16_t Loco7Address, int16_t Loco8Address, int16_t Loco9Address, int16_t Loco10Address) {

  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;          //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = UNSETCONSIST;  //UNset consist
  sendFifoBuffer[sendWriteFifoCounter][CONSISTNUMLOCOS] = numCLocos;
  sendFifoBuffer[sendWriteFifoCounter][CONSISTADDRHIGH] = consistAddress >> 8;
  sendFifoBuffer[sendWriteFifoCounter][CONSISTADDRLOW] = consistAddress;
  //now for all the other locos
  sendFifoBuffer[sendWriteFifoCounter][19] = LeadLocoAddress >> 8;
  sendFifoBuffer[sendWriteFifoCounter][20] = LeadLocoAddress;
  sendFifoBuffer[sendWriteFifoCounter][21] = Loco2Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][22] = Loco2Address;
  sendFifoBuffer[sendWriteFifoCounter][23] = Loco3Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][24] = Loco3Address;
  sendFifoBuffer[sendWriteFifoCounter][25] = Loco4Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][26] = Loco4Address;
  sendFifoBuffer[sendWriteFifoCounter][27] = Loco5Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][28] = Loco5Address;
  sendFifoBuffer[sendWriteFifoCounter][29] = Loco6Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][30] = Loco6Address;
  sendFifoBuffer[sendWriteFifoCounter][31] = Loco7Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][32] = Loco7Address;
  sendFifoBuffer[sendWriteFifoCounter][33] = Loco8Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][34] = Loco8Address;
  sendFifoBuffer[sendWriteFifoCounter][35] = Loco9Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][36] = Loco9Address;
  sendFifoBuffer[sendWriteFifoCounter][37] = Loco10Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][38] = Loco10Address;
  incsendWriteFifoCounter();
}

void nowRail::formConsist(byte numCLocos, int16_t consistAddress, int16_t LeadLocoAddress, int16_t Loco2Address, int16_t Loco3Address, int16_t Loco4Address, int16_t Loco5Address, int16_t Loco6Address, int16_t Loco7Address, int16_t Loco8Address, int16_t Loco9Address, int16_t Loco10Address) {

  memcpy(sendFifoBuffer[sendWriteFifoCounter], _transmissionPrefix, 10);           //set message prefix
  sendFifoBuffer[sendWriteFifoCounter][MESSAGELOWID] = sendWriteFifoCounter;       //MessageID
  sendFifoBuffer[sendWriteFifoCounter][MESSAGEHIGHID] = sendWriteHighFifoCounter;  //MessageID HIGH BYTE
  sendFifoBuffer[sendWriteFifoCounter][MESSRESPONSE] = MESSRESPNOTREQ;
  sendFifoBuffer[sendWriteFifoCounter][MESSTRANSCOUNT] = 0;        //0 as new message
  sendFifoBuffer[sendWriteFifoCounter][MESSAGETYPE] = SETCONSIST;  //UNset consist
  sendFifoBuffer[sendWriteFifoCounter][CONSISTNUMLOCOS] = numCLocos;
  sendFifoBuffer[sendWriteFifoCounter][CONSISTADDRHIGH] = consistAddress >> 8;
  sendFifoBuffer[sendWriteFifoCounter][CONSISTADDRLOW] = consistAddress;
  //now for all the other locos
  sendFifoBuffer[sendWriteFifoCounter][19] = LeadLocoAddress >> 8;
  sendFifoBuffer[sendWriteFifoCounter][20] = LeadLocoAddress;
  sendFifoBuffer[sendWriteFifoCounter][21] = Loco2Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][22] = Loco2Address;
  sendFifoBuffer[sendWriteFifoCounter][23] = Loco3Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][24] = Loco3Address;
  sendFifoBuffer[sendWriteFifoCounter][25] = Loco4Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][26] = Loco4Address;
  sendFifoBuffer[sendWriteFifoCounter][27] = Loco5Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][28] = Loco5Address;
  sendFifoBuffer[sendWriteFifoCounter][29] = Loco6Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][30] = Loco6Address;
  sendFifoBuffer[sendWriteFifoCounter][31] = Loco7Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][32] = Loco7Address;
  sendFifoBuffer[sendWriteFifoCounter][33] = Loco8Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][34] = Loco8Address;
  sendFifoBuffer[sendWriteFifoCounter][35] = Loco9Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][36] = Loco9Address;
  sendFifoBuffer[sendWriteFifoCounter][37] = Loco10Address >> 8;
  sendFifoBuffer[sendWriteFifoCounter][38] = Loco10Address;
  incsendWriteFifoCounter();
}
