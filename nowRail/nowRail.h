/*nowRailV2_1_0
04/06/2026
  */

#ifndef nowRail_h  //header guard to stop it being imported twice
#define nowRail_h

#include "Arduino.h"
#include "nowrail_user_setup.h"  //configuration file

//system definitions
#define PACKETLENGTH 50  //number of bytes in packet transmission
#define HEARTBEAT 1000   //sends out the clock update once per second...could be increased for very busy layouts

//_timeArray[]
#define TIMEARRAYCLOCKSPEED 0
#define TIMEARRAYCLOCKHOUR 1
#define TIMEARRAYCLOCKMIN 2
#define TIMEARRAYCLOCKSECONDS 3
#define TIMEARRAYCLOCKDAY 4

//EEPROM
#define EEPROMVERSION 1                //1 is time plus loco array 0 time only... found in byte 10
#define EEPROMLOCODATASTART 1000       //1000 first pos of loco data
#define EEPROMLOCORECALLDATASTART 100  //This array is the same size as the main loco array...200 bytes by default

//Message Transmission...first 10 bytes are layout and board prefix
#define MESSAGELOWID 10    //Message ID  LOW byte keeps track so that messages can be repesent if no response
#define MESSAGEHIGHID 11   //MessagHIGHID creates a high byte type situation to give greater than 256 message ID's
#define MESSRESPONSE 12    //0 no response required  1 respond required 2 this is the response
#define MESSTRANSCOUNT 13  //number of times message transmitted
#define MESSAGETYPE 14     //0 TIMEUPDATE 1 ACCESSORYCOMMAND \
                           //15 is spare

//MESSRESPONSE Options
#define MESSRESPNOTREQ 0  //Message response not required
#define MESSRESPREQ 1     //Message response required
#define MESSRESPRESP 2    //Message response this is the response

//MESSAGETYPE Options
#define TIMEUPDATE 1           //This is the 5 second heart beat
#define ACCESSORYCOMMAND 2     //Accessory Command such as turnpout/lights/animation
#define PANELUPDATE 3          //When accessory has moved a PANELUPDATE is broadcast so any control panels can update
#define DCCLOCOSPEED 4         //DCC Loco Speed instruction
#define DCCLOCOFUNCTION 5      //DCC Loco function instruction
#define FASTCLOCKUPDATE 6      //New fast Clock Time
#define LAYOUTPOWERCOMMANDS 7  //Turns Power on/off emergency stop
#define SENSORUPDATE 8         //Sensor Update
#define LOCODATATX 9           //This is a full transmission 1 locos data including functions and name
#define LOCOBULKDATATX 10      //As above but part of 200 loco transmissions
#define WIFICHANNELCMD 11      //WIFI channel change command
#define SETMASTERCLOCKTIME 12  //Command that will only be processed by MASTERCLOCK,,, sets time and day
#define RFIDDATA 13  //command that holds RFID data packet 
//#define DCCEXCREATESENSOR 14  //command that holds sets up a virtual pin sensor
#define ANALOGUESENSORUPDATE 14  //allows transmission of sensor values
#define DCCEXCUSTOMCMD 15  //allows 30 bytes of custom data
#define SETCONSIST 16  //send consist creation data to ex rail /nce
#define UNSETCONSIST 17 //send command to break consist

//#define SETCONSIST 16
#define CONSISTNUMLOCOS 16
#define CONSISTADDRHIGH 17  //address high byte...consist address
#define CONSISTADDRLOW 18   //address low byte...consist address
// 19-20 loco1 ,...-address means loco reversed
// 21-22 loco2  
// 23-24 loco3
// 25-26 loco4
// 27-28 loco5 
// 29-30 loco6
// 31-32 loco7  
// 33-34 loco8  
// 35-36 loco9  
//37-38  loco 10
//void formConsist(byte numLocos,uint16_t consistAddress, uint16_t LeadLocoAddress,int16_t Loco2Address,int16_t Loco3Address,int16_t Loco4Address,int16_t Loco5Address,int16_t Loco6Address,int16_t Loco7Address,int16_t Loco8Address,int16_t Loco9Address,int16_t Loco10Address);

//#define UNSETCONSIST 17... already defined above
//#define CONSISTADDRHIGH 17  //address high byte
//#define CONSISTADDRLOW 18   //address low byte

//RFID data format byte 17 data length, bytes 18-47 carries data
#define RFIDNUMBYTES 16
#define RFIDDATASTART 17 //postion for start of RFID data
//TIMEUPDATE Options
#define MESSAGECLOCKSPEED 16
#define MESSAGECLOCKHOUR 17
#define MESSAGECLOCKMIN 18
#define MESSAGECLOCKSEC 19
#define MESSAGECLOCKDAY 20

//ACCESSORYCOMMAND
#define ACCADDRHIGH 16  //address high bytee
#define ACCADDRLOW 17   //address low byte       //get low byte
#define ACCINST 18      //address instruction...usually 1 or 0

//DCCLOCOSPEED
#define LOCOSPEED 16     //address high bytee
#define LOCODIR 17       //address high bytee
#define LOCOADDRHIGH 18  //address high byte
#define LOCOADDRLOW 19   //address low byte



//DCCLOCOFUNCTION
#define LOCOFUNC 16       //
#define LOCOFUNCSTATE 17  //
#define LOCOADDRHIGH 18   //address high byte
#define LOCOADDRLOW 19    //address low byte

//FASTCLOCKUPDATE
#define MESSAGECLOCKSPEED 16

//LAYOUTPOWERCOMMANDS
#define POWERCOMMAND 20  //changed from 16  1.2.2 for NCE power cab issues
//values for POWERCOMMAND
#define TURNPOWEROFF 0
#define TURNPOWERON 1
#define TURNEMERGENCYSTOP 2
#define DONEPOWEROFF 3
#define DONEPOWERON 4
#define DONEEMERGENCYSTOP 5

//SENSORUPDATE
#define SENADDRHIGH 16  //address high bytee
#define SENADDRLOW 17   //address low byte       //get low byte
#define SENINST 18      //address instruction...usually 1 or 0

//ANALOGUESENSORUPDATE
//#define SENADDRHIGH 16  //address high bytee As obove
//#define SENADDRLOW 17   //address low byte  
//data from 18 - 21  HIGH to LOW     

//1.4.2
//WIFICHANNEL
#define NEWWIFICHANNEL 16
//SHIFT REGISTERS
#define LATCHPIN 0
#define CLOCKPIN 1
#define DATAPIN 2


#include "WiFi.h"
#include "esp_now.h"


class nowRail {
public:

  nowRail(int addr1, int addr2, int addr3, int addr4);
  String getnowRailAddr();                           //function returns the current nowRail address of the board
  void setnowRailAddr(byte element, int changeVal);  //update the elemnt of address (0-3), by the amount changeVal
  void init();                                       //initialisation routine
  void runLayout(void);                              //main control function, needs to be in main loop
  //Masterclock
  byte rtcClockSpeed(void);
  byte rtcHours(void);
  byte rtcMinutes(void);
  byte rtcSeconds(void);
  byte rtcDays(void);
  //1.5.4
  void sendRFIDData(uint8_t rfidData[],uint8_t len);//Takes a data structure and the structures size.. max data 30 bytes
  void sendClockSpeedChange(byte clockSpeed);  //changes MASTERCLOCK speed
  //1.5.0
  void sendClockTimeChange(byte hour, byte mins, byte seconds, byte day);  //updates MASTERCLOCK to specific time
  //loco stuff
  int getLocoDCCAddress(byte locoID);                   //gets DCC address from _locoData[200][32]
  byte getLocoDCCFuncState(byte locoID, byte funcNum);  //gets function state _locoData[200][32]
  String getLocoName(byte locoID);                      //returns string of address _locoData[200][32]
  void setLocoDCCAddress(byte locoID, int dccAddress);
  void setLocoName(byte locoID, String locoName);
  void setLocoDCCFuncState(byte locoID, byte funcNum, byte funcState);
  byte getLocoTXdataSetByte(byte item);

  void locoReCallDataUpdate(byte theLoco, byte theLocoSpeed);  //0.9 mod function that updates _locoReCallData[32] array
  byte locoReCallGetLocoID(byte ReCallPos);                    //0.9 gets locoID
  byte locoReCallGetLocoSpeed(byte ReCallPos);                 //0.9 gets locoSpeed
  //nowRail 1.9.2
  byte locoGetConsistState(byte locoID);                     //L means last loco in consist results 0...F, 1...R, 2 LF, 3 LR, 10 F in consist, 11 R in consist,12 LF in consist,13 LR  in consist
  void locoSetConsistState(byte locoID, byte consistState);  //L means last loco in consist results 0...F, 1...R, 2 LF, 3 LR, 10 F in consist, 11 R in consist,12 LF in consist,13 LR  in consist
  //void endConsist(uint16_t consistAddress); //will send command to DCC EX or NCE CAb bus to remove consist
  void endConsist(byte numLocos,int16_t consistAddress, int16_t LeadLocoAddress,int16_t Loco2Address,int16_t Loco3Address,int16_t Loco4Address,int16_t Loco5Address,int16_t Loco6Address,int16_t Loco7Address,int16_t Loco8Address,int16_t Loco9Address,int16_t Loco10Address);//skip any locos not needed
  void formConsist(byte numLocos,int16_t consistAddress, int16_t LeadLocoAddress,int16_t Loco2Address,int16_t Loco3Address,int16_t Loco4Address,int16_t Loco5Address,int16_t Loco6Address,int16_t Loco7Address,int16_t Loco8Address,int16_t Loco9Address,int16_t Loco10Address);//skip any locos not needed
  
  void locoTXLocoData(byte locoID);                          //sends a broadcast of all loco data to be picked up by another controller
  void setLocoRXState(byte state, byte updateLocoID);        //used to set the loco slot for incoming data
  //nowrail 1.3.0
  void locoTXAllLocoData(void);       //transmits all 200 locos data
  void locoRXAllLocoData(int state);  //0 = 0ff, > 0 set to receive
  //end locostuff
  //1.4.2... wifi stuff
  void changeWifiChannel(byte newChannel);  //will only accept values 1,6 and 11 Sends out a command to change the wifi channel
  //end 1.4.2
#if defined(GT911)


  struct TouchLocation {
    uint16_t x;
    uint16_t y;
  };
  TouchLocation touchLocations[5];
  void addGT911Screen(int GT911ResetPin, int GT911IntPin);
  void addGT911Button(int xPos, int yPos, int accNum, int press1, int press2);  //Adds std pin accessories to system
#endif
  void sendPowerCommand(byte Command, int dccAddr, byte dccSpeed, byte dccDir);  //sends commands to base stations/controllers  POWEROFF 0 POWERON 1 EMERGENCYSTOP 2
  void addStdPanelLed(int pin, int accNum, int dir);                             //Adds a panel LED connected directly to a board pin...pin goes HIGH when triggered

  void accProcessed(byte processed);
  void sendDCCLocoFunc(byte locoID, int dccAddr, byte funcNum, byte funcState);
  void sendDCCLocoSpeed(int dccAddr, byte dccSpeed, byte dccDir);               //send DCC speed command
  void sendAccessoryCommand(int accNum, byte accInst, byte respReq);            //sends an accessory command
  void addStdPinButton(int pin, int accNum, int press1, int press2);            //adds standard pin buttons to system
  void addStdPinAcc(int pin, int accNum, int dir, int pulse, int setpinState);  //Adds std pin accessories to system
  
  void sendSensorUpdate(int senNum, byte senInst);  //Sends a sensor update
  void addStdPinSensor(int pin, int senNum);
  //1.7.0
  void addStdPinTSwitch(int pin, int accNum);
   //1.8.1
 // void setupDCCEXSensor(uint16_t senNum); //function to set up a new virtual sensor in DCCEX...will send when any sensor set up
  void sensorCustomValue(uint16_t senNum, int32_t senValue); //Allows user to send custom value to a sensor
  void sendDCCEXCustomCmd(String cmdString);//will allow up to 30 bytes per command
  void sensorProcessed(void);//1.8.3

#if defined(NUMCD4021CHIPS)
  void setupCD4021(int latchPin, int clockPin, int dataPin);                       // Sets up the pins
  void addCD4021PinButton(int chip, int pin, int accNum, int press1, int press2);  //Adds std pin accessories to system
  void addCD4021PinSensor(int chip, int pin, int senNum);
  void addCD4021PinTSwitch(int chip, int pin, int accNum);  //1.7.0 adds toggle switch pin
#endif
#if defined(NUM74HC595NCHIPS)
  void setup74HC595N(int latchPin, int clockPin, int dataPin);                                 // Sets up the pins
  void add74HC595NPinAcc(int chip, int pin, int accNum, int dir, int pulse, int setpinState);  //Adds std pin accessories to system
  void add74HC595NPPanelLed(int chip, int pin, int accNum, int dir);                           //Adds a panel LED connected to 74HC595N
  //1.4.0 Mod
  void set74HC595NPPinState(unsigned int chip, unsigned int pin, byte pinState);
#endif
#if defined(MAXPCA9685SERVOBOARDS)
  void addPCA9685Servo(byte board, byte port, int accNum, int angle0, int angle1, int moveTime);  //time added in 1.0.3
  //1_9_0 LED mods
  void addPCA9685Led(byte board, byte port, int accNum, int dirOn, int effect, int maxBright, int effectBright);  //dir)n = 0 or 1 to turn on, effect 0 = on/0ff, 1 = fire flicker, 2 = gas light, 3 = arc welder, 4 = flash
  //int _pca9685LEDS[MAXPCA9685SERVOBOARDS][7];//board, port, accNum,dirOn,effect Type, max brightness, effect brightness
  //panel leds
  void addPCA9685PanelLed(byte board, byte port, int accNum, int dirOn, int effect, int maxBright);  //panel led... efect on/off or flashing
  //1.9.3
  //void setPCA9685LEDOpenDrain(byte boardAddress);//sets board to open drain for leds
  void setPCA695Servo(byte boardAddress, byte port, byte angle);
#endif

#if defined(MP3BUSYPIN)
  //void addMP3PlayTrack(int accNum,int dirOn, int trackMin, int trackMax, int playType, int playInterval);//AccNumber, dir, track min, track max, (single, random, order), time interval bwteen plays, state
  void addMP3PlayTrack(int accNum, int dirOn, int trackNum, int maxTrackNum, int mode);  //mode 0 = play now, 1 = random//state is current play state 1 = play
  void mp3PlayVolume(int vol);                                                           //sets the volume from default
#endif
#if defined(NUMDELAYEDACCS)
  //uint8_t _DelayAccs[NUMDELAYEDACCS][7];  //TriggerAccNum,TriggerAccDir,DelaAccNum,DelayAccDir,DelayTime,Triggerstate,TriggerMillis
  //int _DelayAccsCount;  
  void addDelayedAccTrigger(int triggerAccNum,byte triggerAccDir,int delayedAccNum,byte delayedAccDir,int delayTime);
#endif

private:
  byte _transmissionPrefix[10];
  unsigned long _currentMillis;
  byte _timeArray[5];  //Clockspeed, Hour, Min, Sec, Day of week

  //Time
  void clockEvents(void);
  unsigned long _clockMillis;  // real time clock time
  int _clockTimer;             //stores the interval that clock should update...based on _timeArray[TIMEARRAYCLOCKSPEED]
  unsigned long _heartMillis;  //stores the layout heart beat time

  //Accessories
  byte _accMoved;                                  //has the item been actioned by this board
  void sendPanelUpdate(int accNum, byte accInst);  //sends out a panel update
  int _stdPinAcc[50][6];                           //max 50 (allows for multiple triggers per pin)items> pin,accid,dir, pulse,setpinState,currentPinState
  byte _stdPinAccCount;                            //how many buttons have been added to system
  unsigned long _stdPinAccMillis[50];              //stores the time pulse started
  void accPulseOFF(void);                          //function that stops pulses to accs

  //Panel Buttons
  unsigned long _buttonDebounceMillis;  // button press debounce
  void buttonsPressed(void);            //Sets the debounce to check for button presses
  int _stdPinButtons[50][5];            //max 50 (allows for multiple triggers per pin)items> pin,accid,press 1, press 2,buttonstate
  byte _stdPinButtonsCount;             //how many buttons have been added to system

  //Sensors
  //byte _senProcessed; 
  unsigned long _sensorDebounceMillis;
  void sensorEvents(void);          //go through sensors
  int _stdPinSensors[50][3];        //pin, senNum, LastState
  byte _stdPinSensorsCount;         //how mant Std pin sensors on board

  //ToggleSwitches 1.7.0
  void tswitchEvents(void);          //go through sensors
  int _stdPinTSwitch[50][3];        //pin, senNum, LastState
  byte _stdPinTSwitchCount;         //how mant Std pin sensors on board

#if defined(WIFIMASTERCLOCKCHANGE)  //auto wifi channel change
  byte _currentWIFIChannel = 1;
  unsigned long _lastWIFIMillis;
  unsigned long _lastWIFITimer = 5000;


#endif


//CD4021 Shift Registers for buttons/sensors...system to work like _stdPinButtons
#if defined(NUMCD4021CHIPS)
  int _CD4021PinButtons[NUMCD4021CHIPSNUMBUTTONS][6];  //allows for multiple triggers per pin)items> chip,pin,accid,press 1, press 2,buttonstate
  int _CD4021PinCount;                                 //how many buttons have been added to system...needs to be int
  byte _CD4021byteData[NUMCD4021CHIPS];
  int _CD4021BoardPins[3];                              //latch pin, clockpin, datapin
  void latch4021(int funcLatchPin);                     //latch pin function
  byte get4021Byte(int funcDataPin, int funcClockPin);  //gets the byte of data
  //CD4021 sensors
  int _CD4021PinSensors[NUMCD4021CHIPS * 8][4];  //chip, pin, senNum, LastState
  int _CD4021SensorPinCount;                     //sensor pins added
  //Toggle switches 1.7.0
  int _CD4021PinTSwitch[NUMCD4021CHIPSNUMTSWITCH][4]; 
  //int _CD4021PinTSwitch[NUMCD4021CHIPS * 8][4];  //chip, pin, AccNum, LastState
  int _CD4021TSwitchPinCount;                    //Toggle switch pins added

#endif
#if defined(NUM74HC595NCHIPS)
  void update74HC595N(void);
  byte _S74HC595NbyteData[NUM74HC595NCHIPS];  //creates array, 1 byte per board
  int _S74HC595NBoardPins[3];
  //wasn't sure if this would work...allowsd for 2 inputs per pin...one to turn on another to turn off
  int _S74HC595NPins[NUM74HC595NCHIPS * 16][7];  //chip, pin,accid,dir, pulse,setpinState,currentPinState
  int _S74HC595NPinAccCount;
  unsigned long _S74HC595NPinAccMillis[NUM74HC595NCHIPS * 16];  //stores the time pulse started...if required

  //panel pins
  int _S74HC595NPanelLedPins[NUM74HC595NCHIPS * 8][4];  //chip, pin,accNUm,dir
  int _S74HC595NPanelLedPinCount;                       //keeps track of panel pins added
  //add74HC595NPPanelLed(int chip,int pin,int accNum, int dir);
#endif
//PCA9685Servos
#if defined(MAXPCA9685SERVOBOARDS)
  byte _pca9685Addresses[MAXPCA9685SERVOBOARDS][2];  //0_9_2 mod 0 = address 1 = servo/0 led 1
  byte _pca9685AddressesCount;
  //1.9.1 mod
  int _pca9685Servos[MAXPCA9685SERVOBOARDS * 16][9];               //board, port, accNum, angle0, angle1,millisperstep,currentAngle,targetAngle  (millisperstep,currentAngle,targetAngle...added 1.0.3 for slow motion),detachState (0 = attached)
  //int _pca9685Servos[MAXPCA9685SERVOBOARDS * 16][8];               //board, port, accNum, angle0, angle1,millisperstep,currentAngle,targetAngle  (millisperstep,currentAngle,targetAngle...added 1.0.3 for slow motion)
  unsigned long _pca9685ServosMillis[MAXPCA9685SERVOBOARDS * 16];  //stores last move time in milliseconds 1.0.3
  int _pca9685ServoCount;
  void pca9685ServoControl();                                     //deals with servo movement in main loop
  void setupPCA9685Board(byte boardAddress, byte boardType);      //0_9_2 mods sets up servo boards, type 0 = servo, type 1 = led
  //void setPCA695Servo(byte boardAddress, byte port, byte angle);  //move servo to an angle
  //1.9.1 mod
  void detachPCA9685Servo(byte boardAddress, byte port);
  //0_9_2 LED mods
  unsigned long _pca9685LEDTimers[MAXPCA9685SERVOBOARDS * 16][2];  //used for led flicker timings 0 = flicker time, 1 = wait interval
  byte _pca9685LEDStates[MAXPCA9685SERVOBOARDS * 16][3];           //current state, requested state values are 0 = OFF, 1 full brightness or  effect brightness, 2 effects state
  int _pca9685LEDCount;                                            //keeps track of number of leds
  void pca9685LedControl(void);                                    //controls all leds effects..turns on off
  void setPCA695Led(byte boardAddress, byte port, int brightness);
  int _pca9685LEDS[MAXPCA9685SERVOBOARDS * 16][7];  //board, port, accNum, dirOn, effect, maxBright, effect bright ?? effects 0 = ON/Off, 1 = fire, 2 = gas, 3 = arc
  //panel leds
  int _pca9685PanelLEDS[MAXPCA9685SERVOBOARDS * 16][6];  //board, port, accNum, dirOn, effect, maxBright
  int _pca9685PanelLEDCount;
  byte _pca9685PanelLEDStates[MAXPCA9685SERVOBOARDS * 16][3];
  unsigned long _pca9685PanelLEDTimers[MAXPCA9685SERVOBOARDS * 16];
  
#endif
#if defined(NOWDisc)
  //EEPROM
  void timeUpdateEEPROM();           //updates the EEPROM
  void useEEPROM(byte discAddress);  //sets up system to use EEPROM for time storage
  int EEPROMRead(int disk, int startdatareadaddress, int numbytes);
  void EEPROMWrite(int disk, int eepromaddress, byte eepromdata);
  unsigned long _EEPROMMillis;
  int _EEPROMTimer = 120000;  //every 2 minutes...greater than 32000 because 32 bit processor
#endif
  void locoEEPROMUpdate(byte locoID);  //if eeprom is running will update the locoEEPROM date from locoData
  //Loco data system
  byte _locoData[200][32];      //stores the loco data from eeprom
  byte _locoReCallData[32][2];  //0.9 modded     Stores the order of last 32 used locos, ID and speed
  byte _locoReCallDataFlag;
  byte _locoReCallPos;  //works out position of next loco in recall array
  unsigned long _locoReCallMillis;
  byte _locoTXdataSet[32];  //stores data when received for loco data change
  byte _locoRXState[2];     //0 = 0 or 1 = waiting tom receive [1] = locoID to be updated

  //bulk loco data transmission receive flags 1.3.0
  byte _locoBulkDataRXFlag;  //1 = waiting to receive data
  byte _locoBulkDataRXPos;   //stores the array position 0-199

  //Fifo buffers
  void checkRecFifo(void);
  byte recReadFifoCounter;
  // //Holds command to be sent out
  void checkSendFifo(void);
  void sendMessResp(void);
  void incsendWriteFifoCounter(void);
  byte sendFifoBuffer[256][PACKETLENGTH];  //Way more than needed but better safe than sorry
  byte sendWriteFifoCounter;               //keeps track of current write position
  byte sendWriteHighFifoCounter;           //increments every loop to add a HIGH byte
  byte sendReadFifoCounter;
  //Repeat fifo
  int _repeatFIFOTimer = 150;  // set the repeat timer to 0.15 seconds
  unsigned long _repeatFIFOMillis;
  unsigned long _repeatFifoMillis[256];
  byte repeatFifoBuffer[256][PACKETLENGTH];  //Way more than needed but better safe than sorry
  byte writeRepeatFifoCounter;
  byte readRepeatFifoCounter;

  //DCC decoder
#if defined(DCCDECODERPIN)
  void processDCC(void);  //main process in loop
  void processPacket();
  void accDecoder(byte AddressByte, byte InstructionByte, byte ErrorByte);
  unsigned long _isrLastRiseTime;
  //byte _timeRiseFall = 0;  //0 = Rising 1 = Falling
  //byte _timingSync = 0;    //1 means in sync
  //int _timingSyncCounter;  //Counter that works through a number of interrupts to check they are correct
  byte _Preamble = 0;
  byte _PreambleCounter = 0;
  byte _PacketStart = 0;
  //byte _bitcounter = 0;
  byte _packetData[28];       //array to get all the data ready for processing
  int _lastDCCAccCommand[2];  //stores the last instruction recieved
  //0.5
  byte _badBits;  //counts bad bit timings and changes interrupt direction if needed
  byte _readDCCPosition;



  byte _bitPartPos;
  int _bitParts[2];
  byte _bitReadPos;
  byte _bitPrintCounter;
  byte _bitCounter;

  byte _interruptPhasePosition;         //1.3.2
  byte _currentInterruptPhasePosition;  //1.3.2
  unsigned long _dccMicros[2];          //1.3.2

#endif

  //Panel std buttons addStdPanelLed(int accNum, int dir)
  int _StdPanelLed[20][3];  //only 20 allowed...pin accNUm Dir
  int _StdPanelLedCount;    //keeps track of how many stdPanelLed's have been added

  //power commands
  byte _powerComStatus;

  //DCCEX commands
  String _DCCEXCommand;

  //JMRI CMRI functions
#if defined(JMRICMRI)


  //receive
  byte _jmriCmriPacketSync;
  int _jmriCmriByteCount;
  byte _jmriCmriPacketType;
  byte _jmriCmriSerialBuffer;
  byte _jmriCmriByteRecBuffer[JMRICMRINUMCARDS * 2][2];  //max number of bytes...new state [0] and old state [1]
  //send

  byte _jmriCmriSendData[JMRICMRINUMCARDS];
  void jmriCmriUpdateSendBuffer(int senNum, byte senInst);
  void jmriCmri(void);                          //reads incoming serial packets
  void jmriCmriProcessByte(byte incomingByte);  //decides what to do with packet
  void jmriCmriProcessRecBuffer(void);
  void jmriCmriProcessSendBuffer(void);
  //test variables
  byte _jmriCmrisendFlag;
  byte _jmriCmritestFlip;
#endif
//END JMRICMRI functions

//NCE CAB BUS
#if defined(CAB_BUS_ADDRESS)
  byte _nceCommandBytes[256][5];  //Holds the bytes to be transmitted...mod 0.8.2
  byte _nceCommandBytesRead;      //0.8.2
  byte _nceCommandBytesWrite;     //0.8.2
  byte _sendNCEFlag;              //Flag set to 1 when bytes to be transmitted

  void nceProcess(void);                                        //main function, waits to be polled and then sends transmission
  void nceAccComProcess(int addr, byte dir);                    //turns nowRail acc commands into nce
  void locoComToNCE(int addr, int speed, int dir);              //loco speed control
  void locoFuncToNCE(int addr, byte funcGroup, byte funcData);  //send functions
  void ncePowerOff(int addr, int speed, int dir);               //tries to turns power off for all locos..E Stop
  byte _nceclockData[8];
  byte _nceclockCount;
  byte _nceclockMode;
  unsigned long _messageDelayMillis;
  unsigned long _messageDelayTimer = 0;
#endif

#if defined(MP3BUSYPIN)
  void mp3SendCommand(void);
  void mp3PlayTrack(int trackNum);
  void mp3Control(void);  //deals with mult track play etc


  int _mp3Accessories[MP3NUMTRACKS][6];  //AccNumber, dir, track num, mode = 0 single play, 1 part of random play
  int _mp3NumAccs;                       //stores number of MP3 accessories set up

  int16_t _mp3CheckSum = 0;
  byte _mp3Command[6];
  byte _mp3CommandLength;

  int _mp3PlayState;
  unsigned long _mp3Millis;
  int _mp3Timer;
  int _mp3PlayNextTrack;
  int _mp3BusyHigh;
#endif
#if defined(GT911)

  
  int _GT911Buttons[GT911TOUCHBUTTONS][6];  //xpos xpos, accnum, press 1, press 2, buttonstate
  int _GT911ButtonCount;

  uint8_t _addr;  //CTP IIC ADDRESS
  //Pins
  int _GT911_RESET;  //CTP RESET
  int _GT911_INT;    //CTP  INT

  unsigned long _captouchbounce = 0;  //like button bounce for touch
  int _captouched = 0;                //1 means there has been a touch

  int _captouchx = 0;
  int _captouchy = 0;

  uint8_t _buf[80];

  void GT911ProcessButtons(void);
  void checkfortouchscreen();
  void inttostr(uint16_t value, uint8_t *str);
  uint8_t GT911_Send_Cfg(uint8_t *buf, uint16_t cfg_len);
  void writeGT911TouchRegister(uint16_t regAddr, uint8_t *val, uint16_t cnt);
  uint8_t readGT911TouchAddr(uint16_t regAddr, uint8_t *pBuf, uint8_t len);
  uint8_t readGT911TouchLocation(TouchLocation *pLoc, uint8_t num);
#endif
#if defined(NUMDELAYEDACCS)
  uint32_t _DelayAccs[NUMDELAYEDACCS][7];  //TriggerAccNum,TriggerAccDir,DelaAccNum,DelayAccDir,DelayTime,Triggerstate,TriggerMillis
  int _DelayAccsCount;  

#endif
};



#if defined(DCCDECODERPIN)
void dccPinState(void);
#endif


#if ESP_IDF_VERSION_MAJOR < 5  // IDF 4.xxx
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
#endif
#if ESP_IDF_VERSION_MAJOR > 4  // IDF 5.xxx
                               // //New V3.0.0 esp32 LIBRARY
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);

#endif


#if defined(__cplusplus)
extern "C" {
#endif

  //These are user functions that they may or may not use
  extern void nowTimeEvents(byte clockSpeed, byte clockHour, byte clockMinute, byte clockSecond, byte clockDay) __attribute__((weak));
  extern void nowAccComRec(int accNum, byte accInst) __attribute__((weak));
  extern void nowPanelUpdate(int accNum, byte accInst) __attribute__((weak));
  extern void nowMomentButton(void) __attribute__((weak));                                                 //Function for reading momentary button presses
  extern void nowPowerCommand(byte Command) __attribute__((weak));                                         //Function controlling base station power
  extern void nowGT911Touch(int xPos, int yPos) __attribute__((weak));                                     //reports GT911 touches
  extern void nowSensorUpdate(int senNum, int32_t senInst) __attribute__((weak));                             //sensor updates
  extern void nowClockSpeedUpdate(void) __attribute__((weak));                                             //clock speed updates
  extern void nowLocoFuncUpdate(int nowLocoID, byte nowFuncNum, byte nowFuncState) __attribute__((weak));  //receives loco function updates..for updating controllers
  extern void nowLocoSpeedUpdate(int locoAddr, byte locoSpeed, byte locoDir) __attribute__((weak));        //receives loco function updates..for updating controllers
  extern void nowLocoDataSetRX(void) __attribute__((weak));                                                                      //calls custom function when loco data transmitted
  extern void nowLocoBulkDataRX(void) __attribute__((weak));                                                                     //called when a bulk data upload has finished
 // extern void nowRFIDDataRec(uint8_t incomingData[], int len) __attribute__((weak));  
  extern void nowRFIDDataRec(uint8_t *incomingData, int len) __attribute__((weak)); 
  extern void nowChannelUpdate(uint8_t channelNum, uint8_t channelState) __attribute__((weak)); 
#if defined(__cplusplus)
}
#endif

#endif