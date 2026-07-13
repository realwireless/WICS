/*nowRailV2_1_2
18/06/2026
*/

#pragma once

//WIFI added in Version 1.4.4

// Version information
#define NOWRAIL_VERSION "2.1.2"   // nowRail
#define WICS_VERSION "1.1.0"      // WICS addOn Gateway_EX_S3

//-----------------------------------------------------------------
//All boards should be set to the same channel. Only use 1,6 and 11 for technical reasons
//See https://www.digitaltown.co.uk/nowRail.php#WIFI 
#define WIFICHANNEL 1 //Defaults for ESP-NOW is 1 or the Access point address if using wifi with a router

//Uncomment this line to encrypt data packets. All boards on a layout must have the same setting
//For legacy systems comment this line out
//See https://www.digitaltown.co.uk/nowRail.php#WIFI 
#define ENCRYPT  //uncomment to run encryption on broadcasts

//Uncomnenting this line will mean the board will automatically look for the MASTERCLOCK after missing 3 broadcasts.
//This allows layouts to change WIFI channel from a single command
//Layout MUST have a MASTERCLOCK running on it
//See https://www.digitaltown.co.uk/nowRail.php#WIFI 
#define WIFIMASTERCLOCKCHANGE //this allows the boards wifi channel to be changed while running

//Diagnostics----------------------------------------------------------------------
//Diagnostics shows the nowRail commands in the Serial monitor
//WARNING:DO NOT USE WHEN CONNECTED TO JMRICMRI...diagnostics will be sent to JMRI :-(
#define DIAGNOSTICS_ON    //Will display all incoming messages in Serial with DIAG> Prefix 

//Delayed Accessories
//Delayed accessories allow for accessories to be triggered after a time period
#define NUMDELAYEDACCS 20 //If defined Sets the number of delayed accessories that can be set up.  

//MASTERCLOCK
//https://www.digitaltown.co.uk/nowRail.php#MASTERCLOCK
//Only one board should be MASTERCLOCK_ON... This really needs to be a board that is ALWAYS turned on.
//A MASTERCLOCK board is required for if WIFIMASTERCLOCKCHANGE is being used
#define MASTERCLOCK_ON    //this board is the master clock and will send out a broadcast to sync all other boards
//1.8.3 https://www.digitaltown.co.uk/nowRail.php#MASTERCLOCK covered in video https://www.youtube.com/watch?v=Eih6xguq3gM at time point 20:20
#define REDUCEMASTERCLOCKDIAG //This reduces Masterclock messages to once per minute... reduced clutter when doing diagnostics

//SERIAL Devices.........................................
//nowRail use Serial2.... WARNING  some boards DO NOT have Serial 2
//Tested on ESP32 Dev Module... check other boards have Serial2 before using
//#define SERIALRX2 01 // (Espressif ESP32-C6)
//#define SERIALTX2 00 // (Espressif ESP32-C6)
#define SERIALRX2 05 // (Espressif ESP32-S3)
#define SERIALTX2 04 // (Espressif ESP32-S3)


//MP3 Player DY-SV5W, DY-HV8F, DY_HV20T, JQ8900............................
//See https://youtu.be/91_2KRJqaWs
//This sets up Serial 2 using the pins defined in SERIALRX2 and SERIALTX2
// #define MP3BUSYPIN 15  //15 is the interrupt pin that is connected to the MP3 Busy pin
// #define MP3NUMTRACKS 20 //defaults to 20 commands, increase number if more commands (accessories) are required
// #define MP3INTERVALMIN 250  //random time gaps min...WARNING times below 250 may lead to odd results as MP3 player may not react quick enough
// #define MP3INTERVALMAX 10000 //random time gaps max
// #define MP3VOLUME 27  //default volume (volumes are 0-30)

//Verion 2_1_2 mods to prevent loop backs
//DCC Systems
//Loop back prevention see See https://www.digitaltown.co.uk/nowRail.php#loopback
#define MINSENDACCADDRESS 1
#define MAXSENDACCADDRESS 2000

//DCC EX Serial communication.............................................
//WARNING Make sure SERIALRX2 and SERIALTX2 are uncommented in this file just below DIAGNOSTICS_ON 
//Accessory command range MINSENDACCADDRESS to MAXSENDACCADDRESS
#define DCCEXSSERIAL2_ON //best done on ESP32 Dev Module Writes DCC EX instructions to Serial 2...for Serial2 connection to DCC EX
#define DCCEXSERIAL_ON  //best done on ESP32 Dev Module Writes DCC EX instruction to Serial ... for debugging

//NCE Cab bus..........................................................
//See https://www.digitaltown.co.uk/nowRail.php#NCE
//Accessory command range MINSENDACCADDRESS to MAXSENDACCADDRESS
// #define CAB_BUS_ADDRESS 4 // This is the address on CAB BUS for the unit There are a limited number of addresses with the PowerCab
// #define CAB_BUS_SHORTADDRON 0 //Loco Addresses 1-128 will be treated as Short Addresses, set to 0 for all LONG addresses
// #define RS485ENABLLEPIN 4 //RS485 modules requires RX/TX at 9600 Baud and a transmit pin defined in this line 

//JMRI.....................................................................
//See https://youtu.be/5YLXue_GavU
//JMRI needs to connect to Serial so DIAGNOSTICS cannot be used at the same time.
//Serial needs to be configured in setup() Comment out standard setup and uncomment the JMRI serial configuration.
//#define JMRICMRI  1  //JMRICMRI connection, this uses Serial at 115200 baud rate, VALUE 1 is CMRI node number (0-127)
//#define JMRICMRINUMCARDS 24 //Number of Cards set up 24 x 8 gives 192 sensors, increase if needed

//Other DCC systems.................................................................
//See https://youtu.be/6ZjmP6hau1c
//DCC Decoder mode... reads accessory commands from any DCC system and sends them over nowRail
//Settings to allow the address range received to be limited to stop loop backs
//see See https://www.digitaltown.co.uk/nowRail.php#loopback
#define MINRECACCADDRESS 1
#define MAXRECACCADDRESS 2100
#define DCCDECODERPIN 6 //If board is being used as a DCC decoder this is the interrupt pin. 6=ESP32 C6/S3
#define ONEBITTIME 130  // If your DCC system does not work you can change this value. NCE systems and DCCEX seem to work between 130 - 180

//All I2C devices need the Wire.h uncommented
#include "Wire.h"
//If you wish to set custom SDA and SDA pins as required on 
//ESP32 C3 and S3 boards uncomment the lines below and set pin numbers
//Commenting out will use the default pins as shown on the ESP32 Dev Module wiring diagrams 
//#define CUSTOM_SDA 11 //SDA cutom pin number  (Espressif ESP32-C6)
//#define CUSTOM_SCL 10 //SCL custom pin number (Espressif ESP32-C6)
#define CUSTOM_SDA 10 //SDA cutom pin number (Espressif ESP32-S3)
#define CUSTOM_SCL 9 //SCL custom pin number (Espressif ESP32-S3)


//EEPROM...........................................................
//See https://www.digitaltown.co.uk/nowRail.php#EEPROM
//NowRail uses EEPROM to store MASTERCLOCK time if you want to restart at the same time when restarting layout
//EEPROM is also used in the loco controller to store the loco data
//just put in the default eeprom address
//#define NOWDisc 0x50    //Default Address of 24LC256 eeprom chip in Board
//END EEPROM

//PCA9685..............................................
//For servos see https://youtu.be/khPnUbnIN88
//For LEDs see https://youtu.be/xKgqPWWVrK8
//used for Servos and LED's NOTE: LED's and servos CANNOT be on the same board address
//#define MAXPCA9685SERVOBOARDS 5 //default is 1, increase if required
//#define SERVOMIN 450    //servo min value
//#define SERVOMAX 2000   //servo max value
//see https://www.digitaltown.co.uk/nowRail.php#PCA9685Servo for more information
//#define PCA9685SERVODETACH 600 //if uncommented will detach a PCA9685 servo 600 ms (0.6 seconds) after it's last movement. Change value to suit
//See https://www.digitaltown.co.uk/nowRail.php#PCA9685Led for more information
//#define PCA9685FLASHTIMER 1000 //any flashing PCS 9685 panel LED or accessory Leds flash timing in milliseconds
//#define PCA9685LEDOPENDRAIN 1 //will set boards driving oleds to open drain mode

//GT911 Touch Screen.....................................................
//See https://www.youtube.com/watch?v=0s4-Wp4rPuw
// #define GT911 0x5D  //Uncomment for gt911 touch screen
// #define GT911TOUCHBUTTONS 30  //sets the touch point buttons, increase for more buttons.
// #define GT911TOUCHRADIUS 35   //35 is approx 5mm either side of the centre touch point

//Other components........................................................

//CD4021 Shift registers.... to allow extra inputs, could be for control panel or sensors.
//See https://www.digitaltown.co.uk/nowRail.php#CD4021
#define NUMCD4021CHIPS 1 //Number of chips daisy chained...system will auto build the array
#define NUMCD4021CHIPSNUMBUTTONS 8 //Although only 8 pins per chip allow some space in case you want a pin to call multiple accs
//1.7.0 ToggleSwitches
#define NUMCD4021CHIPSNUMTSWITCH 8 // Toggle switches Although only 8 pins per chip allow some space in case you want a pin to call multiple accs

//74HC595N SHIFT REGISTERS....to allow more outputs
//See https://www.digitaltown.co.uk/nowRail.php#74HC595N
#define NUM74HC595NCHIPS 1

//Configuration settings
//See https://www.digitaltown.co.uk/nowRail.php#Debounce
#define SENSORDEBOUNCE 100  //Sensors checked every 0.1 seconds 
#define BUTTONDEBOUNCE 250  //Set the button debounce speed to 0.25 seconds...increasing value reduces double presses but too high and system is sluggish

//See https://www.digitaltown.co.uk/nowRail.php#TURNOUTPULSE
#define TURNOUTPULSE 500  //0.5 second pin HIGH to switch point/turnout