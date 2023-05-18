#include <Arduino.h>

/*###########################################################################################################################################
  Code for my ESP32 based data logger.   The data logger has various inputs/outputs to interface to different sensors including:

  DS18S20 Temperature sensor and DHT22 temperature/humitity module
  V+/V-/Gnd to interface to a INA219 current/voltage measurment module
  One raw AnalogIn line (selectable 0-3, 0-9, 0-24v range), a digital GPIO input (can be used as an interrupt input to 
  build a counter, etc.).
  A set of I2C lines (SCL/SDA/+5/Gnd) to talk to various I2C modules
  One PWM digital output for controlling LEDs, fans, etc.
  A 110v power input/output cord with a relay switching the hot.  This can be used to control the power
  to a device where, if an alarm situation is encountered, the power can be switched off.

  There is a touch sensitive LED display (480x320) for the user to interact with the various menus.

  An RTC module is included to log date/time events.

  A SD card module is included to write data to.  The SD card is formatted as FAT32 for plugging into a PC to retreive the datalog results.

  The first version will not use the ESP32's wifi capability, but future versions may use that to transmit data logged values in real time
  or control the datalogger remotely..

  The datalogger will be powered from a 12v supply that is regulated down to 5v for the ESP32 internal 3.3v regulator input.  
  The raw 12v is there in case other motors/pump controllers, fans, actuators need to be powered.  Optionally, three 18650 Li-ion batteries 
  may be used for short duration operation (2200mAh should power the box for about 8-10 hours with no sleep modes used).

  Data/menu structure:

  The data logger has a touch-screen interface.  Each menu is made up of a set of screen elements:  Buttons, text fields and data fields.
  Data fields are implemented as "sprites" (small pixel memory that can be quickly mapped onto the main tft frame buffer for quick, 
  flicker-free updates).  Each screen is in instantiation of a screen class.  The class contains the data structures used for holding the   
  various coordinates, strings, visible/invisible attributes, etc. for the screen elements.

  The screens have a fixed number of button and text fields that may or may not be visible on that screen (controlled by buttonVisible,
  textVisible, and spriteVisible variables.  This way each menu screen object can hold customized controls and data for it's purpose.  
  Each button in the menu screen has a callback function assigned to it.  No screen stack is necessary as every screen has a "back" button 
  whos callback points to the parent screen.

  Any menu controlled options are displayed as buttons where the button text is the current option value.  The user clicks on the button
  and a option setting screen is displayed or the button state is simply toggled in the case of on/off selections.
 
  The touch panel must me calibrated once.  The calibration constants are stored in a SPIFFS file on the ESP32.  To call the calibration 
  routine, set the REPEAT_CAL define to true and reboot the ESP32. 

  dlf 
  V1.0  3/29/2023 
  v1.1  5/17/2023 - Moved to PlatformIO
########################################################################################################################################### */

//##################
// Libraries
//##################

// For INA219 current/voltage measuring module
#include "Wire.h"
#include "Adafruit_INA219.h"

// For temperature probe
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire for DS18S20 temp probe using GPIO pin 16
#define ONE_WIRE_BUS_TEMP_PROBE 16

// For temp/humidity module
#include <dhtnew.h>
#define DHTPIN 17

// For Analog-in/Digital-in/Digital-out
#define AINPIN 34
#define DINPIN 2
#define DOUTPIN 27

// For external 110v relay control
#define EXT_POWER_RELAY 4

// For real time clock
#include "RTClib.h"
#define SET_CLOCK_FROM_COMPILE false   // Set this to true to have the RTC reset in the setup() loop

// For sd card
#include <FS.h>
#include <SPI.h>
#include <SD.h>

// For  TFT display
#include <TFT_eSPI.h>     // Bodmer Graphics lib
#include <MyFreeFonts.h>  // Copied from the TFT_eSPI/examples/480x320/Free_Font_Demo directory
#include <MyDisplay.h>    // Display coords, label-font aliases, custom colors, etc.

// My libraries for creating/managing screens/menus
#include <MyTouchScreen.h>

//##########################
// Instantiation of Objects
//##########################

// Touch screen
TFT_eSPI tft = TFT_eSPI();

// sprite for adding text over the buttons.  We don't use the button's built in text so we can change it on the 
// fly for parameter buttons like a menu's option buttons (Alarm enable/disable, Min/Max conditions, etc.)
TFT_eSprite btnTextSprite = TFT_eSprite(&tft);  

// sprite for displaying results text (along the right side of the screen)
TFT_eSprite textSprite = TFT_eSprite(&tft);

// sprite for displaying status/error messages in a pop-up window
TFT_eSprite statusSprite = TFT_eSprite(&tft);

// sprite for rotating text in the Y axis for the graphs
TFT_eSprite yAxisSprite = TFT_eSprite(&tft);

// sprite for displaying the clock string
TFT_eSprite clockSprite = TFT_eSprite(&tft);

// Current/voltage measuring module
Adafruit_INA219 ivModule;

// OneWire object for DS18S20 temp probe
OneWire tempProbe(ONE_WIRE_BUS_TEMP_PROBE);

// DallasTemperature object (need to pass the oneWire instance reference to it)
// The DallasTemperature object is a wrapper that provides a nice API to control/read the temperature sensor
DallasTemperature tempSensor(&tempProbe);

// Temperature/Humidity DHT22 module
DHTNEW dht(DHTPIN);

// SD card and RTC settings
File myFile; // don't change given name for the SD card file 
RTC_DS1307 RTC;  // rtc object

#define MAIN_MENU "Main Menu"
#define KEYPAD "Keypad"
#define CLOCK_MENU "Clock Menu"
#define GRAPH "Graph"
#define AXIS_MENU "Graph Axis"
#define MENU_110V "110v Outlet Control"
#define SETUP_MENU "Setup Menu"
#define MONITOR_MENU "Monitor Results"
#define IV_MENU "IV Results"
#define TEMP_MENU "Temperature Results"
#define DOUT_MENU "D-Out"

// Create screen objects for each of data logger menus
// Note: each screen object uses ~8K bytes of memory.  I ran out of memory after creating 13 screens...
// So, I am now sharing multiple menus on some screens (the ones that were laid out identically but just 
// had different field values (e.g. IV_SETUP_MENU, TEMP_SETUP_MENU, etc.).  It means you need to save off 
// and restore the menu fields to the screen object each time you leave/enter a screen.
const uint8_t MAX_SCREEN_NUM = 9;
MyTouchScreen * screenPtrs[MAX_SCREEN_NUM];
MyTouchScreen mainMenuScreen(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, MAIN_MENU,1);
MyTouchScreen keypad(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, KEYPAD,1);    // last field is the "title-visible" flag
MyTouchScreen clockScreen(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, CLOCK_MENU,1);
MyTouchScreen graphScreen(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, GRAPH,0);
MyTouchScreen axisScreen(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, AXIS_MENU,1);
MyTouchScreen screen110v(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, MENU_110V,1);
MyTouchScreen setupScreen(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, SETUP_MENU,1);
MyTouchScreen monitorScreen(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, MONITOR_MENU,1);
MyTouchScreen doutScreen(&tft, &btnTextSprite, &textSprite, &statusSprite, &yAxisSprite, &clockSprite, DOUT_MENU,1);

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The SPIFFS file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData2"

// Set REPEAT_CAL to true instead of false to run calibration again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

// How many points we keep in the result arrays before writing them out to the SDCard
const int MAX_RESULT_POINTS = 25;


//###################################
// Prototypes
//###################################

void drawKeypad(uint8_t);
void updateKeypad(uint8_t);
void updateResults(const char *, const char *, const char *, const char *, const char *, const char *, const char *, float *, float *, float *, void *); 
void drawMainMenu(uint8_t);

void saveIvSetup();
void saveIvSetupAndDrawMainMenu(uint8_t);
void saveIvSetupAndDrawIvMenu(uint8_t);
void saveIvSetupAndDrawIvAxisMenu(uint8_t);
void drawIvGraph(uint8_t);
void drawIvSetupMenu(uint8_t);
void drawIvMenu(uint8_t);
void drawIvAxisMenu(uint8_t);
void drawIvResults();
void toggleIvAlarm(uint8_t);

void drawClockScreen(uint8_t);
void updateClock();
void updateClockAlarm();
void setClockTime(uint8_t);
void setAlarmTime(uint8_t);
void toggleClockAlarm(uint8_t);

void draw110vMenu(uint8_t);
void cycle110vActionOnAlarm(uint8_t);
void cycle110vActionOnClock(uint8_t);
void manual110vAction(uint8_t);

void cycleAdAinMax(uint8_t);

void cycleDoutOutput(uint8_t);
void cycleDoutPwmFrequency(uint8_t);
void updateDoutPwmDutyCycle();
void cycleDoutPwmFollows(uint8_t);
void cycleDoutActionOnAlarm(uint8_t);
void drawDoutMenu(uint8_t);

void saveTempSetup();
void saveTempSetupAndDrawMainMenu(uint8_t);
void saveTempSetupAndDrawTempMenu(uint8_t);
void saveTempSetupAndDrawTempAxisMenu(uint8_t);
void drawTempGraph(uint8_t);
void drawTempSetupMenu(uint8_t);
void drawTempMenu(uint8_t);
void drawTempAxisMenu(uint8_t);
void drawTempResults();
void toggleTempAlarm(uint8_t);

void drawAdSetupMenu(uint8_t);
void toggleAdAlarm(uint8_t);
void drawAdMenu(uint8_t);
void drawAdGraph(uint8_t);
void drawAdAxisMenu(uint8_t);

void nop(uint8_t);
void clearCount(uint8_t);
void touchCalibrate();
void monitorResults(uint8_t);
void writeResultsToFile(boolean, const char *, int, float *, float *);

MyTouchScreen * getScreenPtr(const char * );

//###################################
// Globals
//###################################


// Pointer to the currently display-screen/button-push (and previous screen so we can get back...)
MyTouchScreen * curScreenPtr;
MyTouchScreen * prevScreenPtr;
uint8_t curButtonPressed;
uint8_t prevButtonNumber;

// Timers used to read sensors at a specified interval
unsigned long lastClockReadTime = millis(); 
unsigned long lastIvReadTime = millis(); 
unsigned long lastTempReadTime = millis(); 
unsigned long lastAdReadTime = millis(); 

// RTC variables
DateTime now;
char dateStringFormat[25];
char dateString[2][25];

// Result log files.  We can monitor three streams of results at a time.
char resF0[TEXT_LEN+25];
char resF1[TEXT_LEN+25];
char resF2[TEXT_LEN+25];
char resMenu[TEXT_LEN];

// Global alarm flag. Set if alarm condition exists on any enabled alarms
boolean alarmTripped = false;

// Clock alarm flag. Set if the clock reaches the alarm set time when the clock alarm is enabled.
boolean clockAlarmTripped = false;

// 110v output control fields with text sprites
char  action110vOnAlarmS[TITLE_LEN] = {"None"};
char  action110vOnClockS[TITLE_LEN] = {"None"};
char  manual110vActionS[TITLE_LEN] = {"Off"};

// Clock variables to use with screen sprites when updating result fields.
char  curYearS[INT_STRING_WIDTH] = {""};
char  curMonthS[INT_STRING_WIDTH] = {""};
char  curDayS[INT_STRING_WIDTH] = {""};
char  curHourS[INT_STRING_WIDTH] = {""};
char  curMinS[INT_STRING_WIDTH] = {""};
char  curSecS[INT_STRING_WIDTH] = {""};
char  clockAlarmArmedS[TITLE_LEN] = {"AlarmOff"};

// Current/Voltage variables. 
char  ivAlarmArmedS[TITLE_LEN] = {"Disabled"};
char  maxAlarmVS[FLOAT_STRING_WIDTH] = {"10.0"};  // Voltage above this will trigger an alarm
char  maxAlarmIS[FLOAT_STRING_WIDTH] = {"10.0"};  // Current above this will trigger an alarm
char  monitorIvDurationS[FLOAT_STRING_WIDTH] = {"1"};
char  monitorIvIntervalS[FLOAT_STRING_WIDTH] = {".01"};
float curAxisMax = 10.0;        // Min/Max Y-axis range to plot
float voltAxisMax = 10.0;
float powerAxisMax = 40.0;
float allIvAxisMin = 0.0;
char  curAxisMaxS[FLOAT_STRING_WIDTH] = {"10.0"};    // String values used for labeling the graph axis
char  voltAxisMaxS[FLOAT_STRING_WIDTH] = {"10.0"};
char  powerAxisMaxS[FLOAT_STRING_WIDTH] = {"40.0"};
char  allIvAxisMinS[FLOAT_STRING_WIDTH] = {"0.0"};

float current_mA = 0.0;
char  current_mAS[FLOAT_STRING_WIDTH];
float loadVoltage = 0.0;
char  loadVoltageS[FLOAT_STRING_WIDTH];
float power_mW = 0.0;
char  power_mWS[FLOAT_STRING_WIDTH];

// Temperature variables.  Create string vars also to use with screen sprites when updating result fields.
char  tempAlarmArmedS[TITLE_LEN] = {"Disabled"};
char  maxAlarmHumidS[FLOAT_STRING_WIDTH] = {"50.0"};
char  maxAlarmTempS[FLOAT_STRING_WIDTH] = {"100.0"};
char  monitorTempDurationS[FLOAT_STRING_WIDTH] = {"1"};
char  monitorTempIntervalS[FLOAT_STRING_WIDTH] = {".01"};
   
float curProbeTemp = 0.0;
char  curProbeTempS[FLOAT_STRING_WIDTH];
float curModuleTemp = 0.0;
char  curModuleTempS[FLOAT_STRING_WIDTH];
float curModuleHumidity = 0.0;
char  curModuleHumidityS[FLOAT_STRING_WIDTH];
float tempAxisMax = 100.0;
float tempAxisMin = 0.0;
float humidityAxisMax = 100.0;
float humidityAxisMin = 0.0;
char  tempAxisMaxS[FLOAT_STRING_WIDTH] = {"100.0"};
char  tempAxisMinS[FLOAT_STRING_WIDTH] = {"0.0"};
char  humidityAxisMaxS[FLOAT_STRING_WIDTH] = {"100.0"};
char  humidityAxisMinS[FLOAT_STRING_WIDTH] = {"0.0"};

// Analog/Digital in variables.  Create string vars also to use with screen sprites when updating result fields.
boolean lastDinLevel = 1;  // Starts off with a 1 as we're using an internal pullup input
boolean dinLevel = 1;
char  dinLevelS[INT_STRING_WIDTH] = {"0"};
float dinCount = 0.0;
char  dinCountS[INT_STRING_WIDTH] = {"0"};
float ainVoltage = 0.0;
char  ainVoltageS[FLOAT_STRING_WIDTH] = {"0.0"};
char  adAlarmArmedS[TITLE_LEN] = {"Disabled"};
int   maxDinCount = 20;
char  maxDinCountS[INT_STRING_WIDTH] = {"20"};
char  maxDinCountLimitS[INT_STRING_WIDTH] = {"20"};
float maxAinVoltage = 9.0;
char  maxAinVoltageS[FLOAT_STRING_WIDTH] = {"9.0"};
char  maxAinVoltageLimitS[FLOAT_STRING_WIDTH] = {"9.0"};
float allAdAxisMin = 0.0;
char  allAdAxisMinS[FLOAT_STRING_WIDTH] = {"0.0"};
char  monitorAdDurationS[FLOAT_STRING_WIDTH] = {"1"};
char  monitorAdIntervalS[FLOAT_STRING_WIDTH] = {".01"};

// Digital out variables. 
uint8_t pwmChannel = 0;
uint8_t pwmResolution = 10;  // 10-bit so 1024 steps (can be 1 to 16 bit)
int   pwmFrequency = 4000;   // Depends on the resolution.  Generally 1-10KHz.
char  doutPwmFrequencyS[TITLE_LEN] = {"4 KHz"};
int   doutPwmDutyCycle = 50;
char  doutPwmDutyCycleS[INT_STRING_WIDTH] = {"50"};
char  doutOutputS[TITLE_LEN] = {"Low"};
char  doutPwmFollowsS[TITLE_LEN] = {"Fixed"};
char  doutActionOnAlarmS[TITLE_LEN] = {"None"};

// Variables used by multiple menus
unsigned long lastResultsLoggedTime;  // The last millis time we logged results to the results arrays
unsigned long monitoringStartTime = millis();    // The millis time when we started monitoring results
float curMonitorTime = 0.0;
float timeMonitored = 0.0;  // How many minutes we've been monitoring results (and writing them to the results file)
char timeMonitoredS[FLOAT_STRING_WIDTH]; // String to print on the screen
boolean monitoringResults = false;
float monitoredResultsYAxis0[MAX_RESULT_POINTS];  // Arrays to keep monitored Y-Axis results.  Read these to graph or write to results file
float monitoredResultsYAxis1[MAX_RESULT_POINTS];
float monitoredResultsYAxis2[MAX_RESULT_POINTS];
float monitoredResultsXAxis0[MAX_RESULT_POINTS];  
float monitoredResultsXAxis1[MAX_RESULT_POINTS];  
float monitoredResultsXAxis2[MAX_RESULT_POINTS];  
char curResType[5] = {""};  // Which type of results are currently being displayed
char currentlyGraphing[TEXT_PLUS_DATE_LEN]; // String that indicates what parameter is being displayed in the graph window
boolean resultArraysFilled = false;  //Flag that is set once we fill the results array and begin writing to the SD-Card
int resArrIdx;
char  keypadStackArr[TITLE_LEN]; // Keep the keypad results in a simple stack LIFO
uint8_t keypadStackIdx = 0;  // Stack pointer
char curStartResumeState[TITLE_LEN];  //Need to keep track of the button label when leaving/returning to a monitor-results screen


//##################################################################
//##################################################################
// Functions
//##################################################################
//##################################################################


//#################################################################################################
// Keypad handling.  Enter the key that was pushed into a character stack.  Once the "enter" key
// is pressed, store the string in the button label variable.  Later, when we need to use the value,
// we then convert the string to an int or float as required.
//#################################################################################################
void updateKeypad(uint8_t buttonNumber) {
   boolean enter = false;
   boolean cancel = false;

   if(keypadStackIdx < TEXT_LEN) {
      switch(buttonNumber) {

         case 4:  keypadStackArr[keypadStackIdx++] = '7';keypadStackArr[keypadStackIdx] = '\0'; break;
         case 5:  keypadStackArr[keypadStackIdx++] = '8'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 6:  keypadStackArr[keypadStackIdx++] = '9'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 7:  enter = true; break;   
         case 8:  keypadStackArr[keypadStackIdx++] = '4'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 9:  keypadStackArr[keypadStackIdx++] = '5'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 10: keypadStackArr[keypadStackIdx++] = '6'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 12: keypadStackArr[keypadStackIdx++] = '1'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 13: keypadStackArr[keypadStackIdx++] = '2'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 14: keypadStackArr[keypadStackIdx++] = '3'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 16: keypadStackArr[keypadStackIdx++] = '0'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 17: keypadStackArr[keypadStackIdx++] = '.'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 18: if(keypadStackIdx != 0) { keypadStackIdx--; } keypadStackArr[keypadStackIdx] = '\0'; break; // backspace key
         case 19: keypadStackIdx=0;strcpy(keypadStackArr,""); break;                                          // clear key
         case 23: cancel = true; break;   
      }

      // Clear the screen field and update with the latest string chars
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(TEXT_DATUM);         
      tft.setFreeFont(TEXT_FONT);
      tft.fillRect(KEYPAD_RESULT_X, TEXT_LINE0, SCREEN_WIDTH-KEYPAD_RESULT_X,25,TFT_BLACK);
      tft.drawString(keypadStackArr, KEYPAD_RESULT_X, TEXT_LINE0);

      // The user is done entering numbers.  Go back to the screen that called the keyboard
      if(enter || cancel) {
         curScreenPtr = prevScreenPtr;
         if(!cancel) {
            curScreenPtr->updateButtonLabel(prevButtonNumber,keypadStackArr);
         }
         keypadStackIdx=0;
         keypadStackArr[0] = '\0';
         curScreenPtr->drawScreen();
      }
   }
}


//########################################################################
//########################################################################
// Draw the various menus 
//########################################################################
//########################################################################

// Return the screen pointer for the given screen name
MyTouchScreen * getScreenPtr(const char * screenName) {
   for(uint8_t i=0; i<MAX_SCREEN_NUM; i++) {
      if(!strcmp(screenPtrs[i]->getScreenTitle(), screenName)) {
         return(screenPtrs[i]);
      }
   }
   return(screenPtrs[0]);  // Just a default that we should never get to in order to quiet the compiler warning...)
}

// Keypad where user can enter numbers for setting parameters
void drawKeypad(uint8_t buttonNumber) {

   // Keep track of where we came from so we can update after the value is entered on the keypad
   prevScreenPtr = curScreenPtr;
   prevButtonNumber = buttonNumber;
   curScreenPtr =  getScreenPtr(KEYPAD);
   curScreenPtr->drawScreen();
}

// The top level menu presented when the datalogger is first booted up
void drawMainMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(MAIN_MENU);
   curScreenPtr->drawScreen();
}

// Need to save off the iv setup buttons when leaving the setup menu
void saveIvSetup() {
   strcpy(ivAlarmArmedS , curScreenPtr->getButtonLabel(3));
   strcpy(maxAlarmIS , curScreenPtr->getButtonLabel(7));
   strcpy(maxAlarmVS , curScreenPtr->getButtonLabel(11));
   strcpy(monitorIvDurationS , curScreenPtr->getButtonLabel(15));
   strcpy(monitorIvIntervalS , curScreenPtr->getButtonLabel(19));
}
void saveIvSetupAndDrawMainMenu(uint8_t buttonNumber) {
   saveIvSetup();
   drawMainMenu(buttonNumber);
}
void saveIvSetupAndDrawIvMenu(uint8_t buttonNumber) {
   saveIvSetup();
   drawIvMenu(buttonNumber);
}
void saveIvSetupAndDrawIvAxisMenu(uint8_t buttonNumber) {
   saveIvSetup();
   drawIvAxisMenu(buttonNumber);
}

// Need to save off the temp setup buttons when leaving the setup menu
void saveTempSetup() {
   strcpy(tempAlarmArmedS , curScreenPtr->getButtonLabel(3));
   strcpy(maxAlarmTempS , curScreenPtr->getButtonLabel(7));
   strcpy(maxAlarmHumidS , curScreenPtr->getButtonLabel(11));
   strcpy(monitorTempDurationS , curScreenPtr->getButtonLabel(15));
   strcpy(monitorTempIntervalS , curScreenPtr->getButtonLabel(19));
}
void saveTempSetupAndDrawMainMenu(uint8_t buttonNumber) {
   saveTempSetup();
   drawMainMenu(buttonNumber);
}
void saveTempSetupAndDrawTempMenu(uint8_t buttonNumber) {
   saveTempSetup();
   drawTempMenu(buttonNumber);
}
void saveTempSetupAndDrawTempAxisMenu(uint8_t buttonNumber) {
   saveTempSetup();
   drawTempAxisMenu(buttonNumber);
}

// Need to save off the Ain/Din setup buttons when leaving the setup menu
void saveAdSetup() {
   strcpy(adAlarmArmedS , curScreenPtr->getButtonLabel(3));
   strcpy(maxDinCountLimitS , curScreenPtr->getButtonLabel(7));
   strcpy(maxAinVoltageLimitS , curScreenPtr->getButtonLabel(11));
   strcpy(monitorAdDurationS , curScreenPtr->getButtonLabel(15));
   strcpy(monitorAdIntervalS , curScreenPtr->getButtonLabel(19));
}
void saveAdSetupAndDrawMainMenu(uint8_t buttonNumber) {
   saveAdSetup();
   drawMainMenu(buttonNumber);
}
void saveAdSetupAndDrawAdMenu(uint8_t buttonNumber) {
   saveAdSetup();
   drawAdMenu(buttonNumber);
}
void saveAdSetupAndDrawAdAxisMenu(uint8_t buttonNumber) {
   saveAdSetup();
   drawAdAxisMenu(buttonNumber);
}

// Clock Menus
void drawClockScreen(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(CLOCK_MENU);

   // Preset the change time buttons with the current date/time
   strcpy(curYearS , "YYYY"); now.toString(curYearS);
   strcpy(curMonthS, "MM"); now.toString(curMonthS);
   strcpy(curDayS, "DD"); now.toString(curDayS);
   strcpy(curHourS, "hh"); now.toString(curHourS);
   strcpy(curMinS, "mm"); now.toString(curMinS);
   strcpy(curSecS, "ss"); now.toString(curSecS);

   curScreenPtr->updateButtonLabel(9,curYearS);
   curScreenPtr->updateButtonLabel(10,curMonthS);
   curScreenPtr->updateButtonLabel(11,curDayS);
   curScreenPtr->updateButtonLabel(13,curHourS);
   curScreenPtr->updateButtonLabel(14,curMinS);
   curScreenPtr->updateButtonLabel(15,curSecS);

   curScreenPtr->drawScreen();
   updateClock();
   updateClockAlarm();
}

// This is where the user can update the clock manually (rather than at compile time)
void updateClock() {
   curScreenPtr->updateClockSprite(0,dateString[0]);
   curScreenPtr->drawClockSprite();  
}

void updateClockAlarm() {
   curScreenPtr->updateClockSprite(1,dateString[1]);
   curScreenPtr->drawClockSprite();  
}

// The digital-out control menu
void drawDoutMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(DOUT_MENU);
   curScreenPtr->drawScreen();
}

// We can control an external 110v relay-box.  Power can be manually turned on/off or
// the power may be controlled via a measurment alarm or clock alarm.
void draw110vMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(MENU_110V);
   curScreenPtr->drawScreen();
}

// All the Current/Voltage measurment screens
void drawIvAxisMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   axisScreen.init(&axisScreen);
   curScreenPtr =  getScreenPtr(AXIS_MENU);

   // IV Graph Axis Settings
   dtostrf(curAxisMax,3,1,curAxisMaxS);
   dtostrf(voltAxisMax,3,1,voltAxisMaxS);
   dtostrf(powerAxisMax,3,1,powerAxisMaxS);
   dtostrf(allIvAxisMin,3,1,allIvAxisMinS);

   // (button number, button label,  button callback)
   axisScreen.enableButton(3,  curAxisMaxS,    drawKeypad);
   axisScreen.enableButton(7,  voltAxisMaxS,   drawKeypad);
   axisScreen.enableButton(11, powerAxisMaxS,  drawKeypad);
   axisScreen.enableButton(15, allIvAxisMinS,    drawKeypad);
   axisScreen.enableButton(23, "Back",         drawIvSetupMenu);

   axisScreen.enableTextField(0, "Max Current",      TEXT_LEFT, TEXT_LINE0);
   axisScreen.enableTextField(1, "Max Voltage",      TEXT_LEFT, TEXT_LINE1);
   axisScreen.enableTextField(2, "Max Power",        TEXT_LEFT, TEXT_LINE2);
   axisScreen.enableTextField(3, "Min For All",      TEXT_LEFT, TEXT_LINE3);

   curScreenPtr =  getScreenPtr(AXIS_MENU);
   curScreenPtr->drawScreen();
}

void drawIvSetupMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(SETUP_MENU);

   // If coming back from the axis setup menu, save off all the buttons in case any were changed
   if(!strcmp(prevScreenPtr->getScreenTitle(), AXIS_MENU)) {
      curAxisMax = atof(prevScreenPtr->getButtonLabel(3));
      voltAxisMax = atof(prevScreenPtr->getButtonLabel(7));
      powerAxisMax = atof(prevScreenPtr->getButtonLabel(11));
      allIvAxisMin = atof(prevScreenPtr->getButtonLabel(15));
   }

   // load iv setup screen parameters
   setupScreen.init(&setupScreen);
   // (button number, button label,  button callback)
   setupScreen.enableButton(3,  ivAlarmArmedS,        toggleIvAlarm);
   setupScreen.enableButton(7,  maxAlarmIS,           drawKeypad);
   setupScreen.enableButton(11, maxAlarmVS,           drawKeypad);
   setupScreen.enableButton(15, monitorIvDurationS,   drawKeypad);
   setupScreen.enableButton(19, monitorIvIntervalS,   drawKeypad);
   setupScreen.enableButton(20, "SetAxis",            saveIvSetupAndDrawIvAxisMenu);
   setupScreen.enableButton(21, "Monitor",            saveIvSetupAndDrawIvMenu);
   setupScreen.enableButton(23, "Back",               saveIvSetupAndDrawMainMenu);

   setupScreen.enableTextField(0, "Alarm",                  TEXT_LEFT, TEXT_LINE0);
   setupScreen.enableTextField(1, "Max Current Limit (mA)", TEXT_LEFT, TEXT_LINE1);
   setupScreen.enableTextField(2, "Max Voltage Limit (V)",  TEXT_LEFT, TEXT_LINE2);
   setupScreen.enableTextField(3, "Monitor Duration",       TEXT_LEFT, TEXT_LINE3);
   setupScreen.enableTextField(4, "Monitor Interval",       TEXT_LEFT, TEXT_LINE4);

   curScreenPtr->drawScreen();
}

// The IV result monitoring screen where we see measurements in real time or may 
// choose to start logging to the SD-Card and view the graph.
void drawIvMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(MONITOR_MENU);
   strcpy(curResType,"IV");
   curScreenPtr->setScreenType(curResType);

   // Load the iv monitor screen variables into the monitor screen
   monitorScreen.init(&monitorScreen);
   // (button number, button label,  button callback)
   monitorScreen.enableButton(20, "ViewGraph", drawIvGraph);
   monitorScreen.enableButton(21, curStartResumeState, monitorResults);
   monitorScreen.enableButton(22, "StopLog",  monitorResults);
   monitorScreen.enableButton(23, "Back",  drawIvSetupMenu);

   // (Text field, Text string,  text placement X,Y)
   monitorScreen.enableTextField(0, "Load Current (mA)",    TEXT_LEFT, TEXT_LINE0);
   monitorScreen.enableTextField(1, "Load Voltage (V)",     TEXT_LEFT, TEXT_LINE1);
   monitorScreen.enableTextField(2, "Load Power (mW)",      TEXT_LEFT, TEXT_LINE2);
   monitorScreen.enableTextField(3, "Time Monitored (Min)", TEXT_LEFT, TEXT_LINE3);

   // (Text Sprite field, Text Sprite string,  sprite X,Y placement coords)
   monitorScreen.enableTextSprite(0, current_mAS,         TEXT_SP_LEFT, TEXT_SP_LINE0);
   monitorScreen.enableTextSprite(1, loadVoltageS,        TEXT_SP_LEFT, TEXT_SP_LINE1);
   monitorScreen.enableTextSprite(2, power_mWS,           TEXT_SP_LEFT, TEXT_SP_LINE2);
   monitorScreen.enableTextSprite(3, timeMonitoredS,      TEXT_SP_LEFT, TEXT_SP_LINE3);

   // Set up graph pointers
   dtostrf(curAxisMax,3,1,curAxisMaxS);
   dtostrf(voltAxisMax,3,1,voltAxisMaxS);
   dtostrf(powerAxisMax,3,1,powerAxisMaxS);
   dtostrf(allIvAxisMin,3,1,allIvAxisMinS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(3,curAxisMaxS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(7,voltAxisMaxS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(11,powerAxisMaxS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(15,allIvAxisMinS);

   graphScreen.init(&graphScreen);
   graphScreen.enableButton(20, "Current",  drawIvGraph);
   graphScreen.enableButton(21, "Voltage",  drawIvGraph);
   graphScreen.enableButton(22, "Power",    drawIvGraph);
   graphScreen.enableButton(23, "Back",     drawIvMenu);

   curScreenPtr->drawScreen();
}

// Probe and a built in temp/humidity module mounted on the data-logger.
void drawTempSetupMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(SETUP_MENU);

   // If coming back from the axis setup menu, save off all the buttons in case any were changed
   if(!strcmp(prevScreenPtr->getScreenTitle(), AXIS_MENU)) {
      tempAxisMax = atof(prevScreenPtr->getButtonLabel(3));
      tempAxisMin = atof(prevScreenPtr->getButtonLabel(7));
      humidityAxisMax = atof(prevScreenPtr->getButtonLabel(11));
      humidityAxisMin = atof(prevScreenPtr->getButtonLabel(15));
   }

   // Load the temperature setup screen parameters
   setupScreen.init(&setupScreen);
   // (button number, button label,  button callback)
   setupScreen.enableButton(3,  tempAlarmArmedS,      toggleTempAlarm);
   setupScreen.enableButton(7,  maxAlarmTempS,        drawKeypad);
   setupScreen.enableButton(11, maxAlarmHumidS,       drawKeypad);
   setupScreen.enableButton(15, monitorTempDurationS, drawKeypad);
   setupScreen.enableButton(19, monitorTempIntervalS, drawKeypad);
   setupScreen.enableButton(20, "SetAxis",            saveTempSetupAndDrawTempAxisMenu);
   setupScreen.enableButton(21, "Monitor",            saveTempSetupAndDrawTempMenu);
   setupScreen.enableButton(23, "Back",               saveTempSetupAndDrawMainMenu);

   setupScreen.enableTextField(0, "Alarm",                  TEXT_LEFT, TEXT_LINE0);
   setupScreen.enableTextField(1, "Max Temperature Limit",  TEXT_LEFT, TEXT_LINE1);
   setupScreen.enableTextField(2, "Max Humidity Limit",     TEXT_LEFT, TEXT_LINE2);
   setupScreen.enableTextField(3, "Monitor Duration",       TEXT_LEFT, TEXT_LINE3);
   setupScreen.enableTextField(4, "Monitor Interval",       TEXT_LEFT, TEXT_LINE4);

   curScreenPtr->drawScreen();
}

// The temp/humidity result monitoring screen.
void drawTempMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(MONITOR_MENU);
   strcpy(curResType,"TEMP");
   curScreenPtr->setScreenType(curResType);

   // Load the temp menu settings into the monitorResults screen
   monitorScreen.init(&monitorScreen);
   // (button number, button label,  button callback)
   monitorScreen.enableButton(20, "ViewGraph", drawTempGraph);
   monitorScreen.enableButton(21, curStartResumeState, monitorResults);
   monitorScreen.enableButton(22, "StopLog",  monitorResults);
   monitorScreen.enableButton(23, "Back",  drawTempSetupMenu);

   // (Text field, Text string,  text placement X,Y)
   monitorScreen.enableTextField(0, "Probe Temp (F)",       TEXT_LEFT, TEXT_LINE0);
   monitorScreen.enableTextField(1, "Module Temp (F)",      TEXT_LEFT, TEXT_LINE1);
   monitorScreen.enableTextField(2, "Module Humidity (%)",  TEXT_LEFT, TEXT_LINE2);
   monitorScreen.enableTextField(3, "Time Monitored (Min)", TEXT_LEFT, TEXT_LINE3);

   // (Text Sprite field, Text Sprite string,  sprite placement X,Y)
   monitorScreen.enableTextSprite(0, curProbeTempS,         TEXT_SP_LEFT, TEXT_SP_LINE0);
   monitorScreen.enableTextSprite(1, curModuleTempS,        TEXT_SP_LEFT, TEXT_SP_LINE1);
   monitorScreen.enableTextSprite(2, curModuleHumidityS,    TEXT_SP_LEFT, TEXT_SP_LINE2);
   monitorScreen.enableTextSprite(3, timeMonitoredS,        TEXT_SP_LEFT, TEXT_SP_LINE3);

   // Set up graph pointers
   dtostrf(tempAxisMax,3,1,tempAxisMaxS);
   dtostrf(tempAxisMin,3,1,tempAxisMinS);
   dtostrf(humidityAxisMax,3,1,humidityAxisMaxS);
   dtostrf(humidityAxisMin,3,1,humidityAxisMinS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(3,tempAxisMaxS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(7,tempAxisMinS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(11,humidityAxisMaxS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(15,humidityAxisMinS);

   graphScreen.init(&graphScreen);
   graphScreen.enableButton(20, "Probe-T",   drawTempGraph);
   graphScreen.enableButton(21, "Module-T",  drawTempGraph);
   graphScreen.enableButton(22, "Humidity",  drawTempGraph);
   graphScreen.enableButton(23, "Back",      drawTempMenu);

   curScreenPtr->drawScreen();
}

void drawTempAxisMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   axisScreen.init(&axisScreen);
   curScreenPtr =  getScreenPtr(AXIS_MENU);

   // Temperature Graph Axis Settings
   dtostrf(tempAxisMax,3,1,tempAxisMaxS);
   dtostrf(tempAxisMin,3,1,tempAxisMinS);
   dtostrf(humidityAxisMax,3,1,humidityAxisMaxS);
   dtostrf(humidityAxisMin,3,1,humidityAxisMinS);

   // (button number, button label,  button callback)
   axisScreen.enableButton(3,  tempAxisMaxS,      drawKeypad);
   axisScreen.enableButton(7,  tempAxisMinS,      drawKeypad);
   axisScreen.enableButton(11, humidityAxisMaxS,  drawKeypad);
   axisScreen.enableButton(15, humidityAxisMinS,  drawKeypad);
   axisScreen.enableButton(23, "Back",         drawTempSetupMenu);

   axisScreen.enableTextField(0, "Max Temperature",      TEXT_LEFT, TEXT_LINE0);
   axisScreen.enableTextField(1, "Min Temperature",      TEXT_LEFT, TEXT_LINE1);
   axisScreen.enableTextField(2, "Max Humidity",         TEXT_LEFT, TEXT_LINE2);
   axisScreen.enableTextField(3, "Min Humidity",         TEXT_LEFT, TEXT_LINE3);

   curScreenPtr->drawScreen();
}


// The Analog-in/Digital-in screens.
void drawAdAxisMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   axisScreen.init(&axisScreen);
   curScreenPtr =  getScreenPtr(AXIS_MENU);

   // Ain/Din graph Axis Settings
   dtostrf(maxDinCount,3,1,maxDinCountS);
   dtostrf(maxAinVoltage,3,1,maxAinVoltageS);
   dtostrf(allAdAxisMin,3,1,allAdAxisMinS);

   // (button number, button label,  button callback)
   axisScreen.enableButton(3,  maxDinCountS,     drawKeypad);
   axisScreen.enableButton(7,  maxAinVoltageS,   cycleAdAinMax);
   axisScreen.enableButton(11, allAdAxisMinS,    drawKeypad);
   axisScreen.enableButton(23, "Back",           drawAdSetupMenu);

   axisScreen.enableTextField(0, "Max Din Count",     TEXT_LEFT, TEXT_LINE0);
   axisScreen.enableTextField(1, "Max Ain Voltage",   TEXT_LEFT, TEXT_LINE1);
   axisScreen.enableTextField(2, "Min For All",       TEXT_LEFT, TEXT_LINE2);

   curScreenPtr =  getScreenPtr(AXIS_MENU);
   curScreenPtr->drawScreen();
}

void drawAdSetupMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(SETUP_MENU);

   // Load the AD setup screen parameters
   setupScreen.init(&setupScreen);
   // (button number, button label,  button callback)
   setupScreen.enableButton(3,  adAlarmArmedS,        toggleAdAlarm);
   setupScreen.enableButton(7,  maxDinCountLimitS,    drawKeypad);
   setupScreen.enableButton(11, maxAinVoltageLimitS,       drawKeypad);
   setupScreen.enableButton(15, monitorAdDurationS,   drawKeypad);
   setupScreen.enableButton(19, monitorAdIntervalS,   drawKeypad);
   setupScreen.enableButton(20, "SetAxis",            saveAdSetupAndDrawAdAxisMenu);
   setupScreen.enableButton(21, "Monitor",            saveAdSetupAndDrawAdMenu);
   setupScreen.enableButton(23, "Back",               saveAdSetupAndDrawMainMenu);

   setupScreen.enableTextField(0, "Alarm",                  TEXT_LEFT, TEXT_LINE0);
   setupScreen.enableTextField(1, "Max Din Count Limit",    TEXT_LEFT, TEXT_LINE1);
   setupScreen.enableTextField(2, "Max Ain Volt Limit",     TEXT_LEFT, TEXT_LINE2);
   setupScreen.enableTextField(3, "Monitor Duration",       TEXT_LEFT, TEXT_LINE3);
   setupScreen.enableTextField(4, "Monitor Interval",       TEXT_LEFT, TEXT_LINE4);

   // If coming back from the axis setup menu, save off all the buttons in case any were changed
   if(!strcmp(prevScreenPtr->getScreenTitle(), AXIS_MENU)) {
      maxDinCount = atof(prevScreenPtr->getButtonLabel(3));
      maxAinVoltage = atof(prevScreenPtr->getButtonLabel(7));
      allAdAxisMin = atof(prevScreenPtr->getButtonLabel(11));
   }
   curScreenPtr->drawScreen();
}

// The Ain/Din monitoring screen.  We will see real time results here or can start logging/graphing.
void drawAdMenu(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr =  getScreenPtr(MONITOR_MENU);
   strcpy(curResType,"AD");
   curScreenPtr->setScreenType(curResType);

   // Load AD menu settings into monitor screen
   monitorScreen.init(&monitorScreen);
   // (button number, button label,  button callback)
   monitorScreen.enableButton(16, "Clr-Count", clearCount);
   monitorScreen.enableButton(20, "ViewGraph", drawAdGraph);
   monitorScreen.enableButton(21, curStartResumeState, monitorResults);
   monitorScreen.enableButton(22, "StopLog",  monitorResults);
   monitorScreen.enableButton(23, "Back",  drawAdSetupMenu);

   // (Text field, Text string,  text placement X,Y)
   monitorScreen.enableTextField(0, "D-in Level  (Int-pullup)", TEXT_LEFT, TEXT_LINE0);
   monitorScreen.enableTextField(1, "D-in Count",               TEXT_LEFT, TEXT_LINE1);
   monitorScreen.enableTextField(2, "A-in Voltage",             TEXT_LEFT, TEXT_LINE2);
   monitorScreen.enableTextField(3, "Time Monitored (Min)",     TEXT_LEFT, TEXT_LINE3);

   // (Text Sprite field, Text Sprite string,  sprite placement X,Y)
   monitorScreen.enableTextSprite(0, dinLevelS,        TEXT_SP_LEFT, TEXT_SP_LINE0);
   monitorScreen.enableTextSprite(1, dinCountS,        TEXT_SP_LEFT, TEXT_SP_LINE1);
   monitorScreen.enableTextSprite(2, ainVoltageS,      TEXT_SP_LEFT, TEXT_SP_LINE2);
   monitorScreen.enableTextSprite(3, timeMonitoredS,   TEXT_SP_LEFT, TEXT_SP_LINE3);

   // Set up graph pointers
   dtostrf(maxDinCount,3,1,maxDinCountS);
   dtostrf(maxAinVoltage,3,1,maxAinVoltageS);
   dtostrf(allAdAxisMin,3,1,allAdAxisMinS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(3,maxDinCountS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(7,maxAinVoltageS);
   getScreenPtr(AXIS_MENU)->updateButtonLabel(11,allAdAxisMinS);

   graphScreen.init(&graphScreen);
   graphScreen.enableButton(20, "DinCount",  drawAdGraph);
   graphScreen.enableButton(21, "AinVolt",   drawAdGraph);
   graphScreen.enableButton(23, "Back",      drawAdMenu);

   curScreenPtr->drawScreen();
}


//########################################################################################
//########################################################################################
// Callbacks - These functions are associated with keys on the menus.  See the "Setup" 
//             section for which button is assigned which callback.
//########################################################################################
//########################################################################################

//##############################
// Ain/Din Callbacks
//##############################
void toggleAdAlarm(uint8_t buttonNumber) {
   if(!strcmp(adAlarmArmedS,"Enabled")) {
      strcpy(adAlarmArmedS,"Disabled");
      clockAlarmTripped = false;
      alarmTripped = false;
   } else {
      strcpy(adAlarmArmedS,"Enabled");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,adAlarmArmedS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

void clearCount(uint8_t buttonNumber) {
   dinCount = 0.0;
   dtostrf(dinCount,3,0,dinCountS); 
}

void cycleAdAinMax(uint8_t buttonNumber) {
   if(!strcmp(maxAinVoltageS,"3.0")) {
      strcpy(maxAinVoltageS,"9.0");
   } else if(!strcmp(maxAinVoltageS,"9.0")) {
      strcpy(maxAinVoltageS,"24.0");
   } else if(!strcmp(maxAinVoltageS,"24.0")) {
      strcpy(maxAinVoltageS,"3.0");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,maxAinVoltageS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons

   // NOTE: User MUST match the Ain range setting (knob on side of box) to the maxAinVoltage selected
   char txt[TEXT_LEN] = "Pls Set Range Knob To ";
   strcat(txt,maxAinVoltageS);
   statusSprite.setTextColor(STATUS_COLOR, STATUS_BACKGROUND);
   statusSprite.setTextDatum(STATUS_DATUM);
   statusSprite.setFreeFont(STATUS_TEXT_FONT);
   statusSprite.fillSprite(STATUS_BACKGROUND);
   statusSprite.drawString(txt, STATUS_WIDTH/2,STATUS_HEIGHT/2,GFXFF);
   statusSprite.pushSprite(STATUS_X, STATUS_Y);
   delay(2000);
   curScreenPtr->drawScreen();
}

//##############################
// Dout Control Callbacks
//##############################
void updateDoutPwmDutyCycle() {

   ledcAttachPin(DOUTPIN, pwmChannel);  
   ledcSetup(pwmChannel, pwmFrequency, pwmResolution); 

   int mapLow = 0;
   int mapHigh = 100;

   // If running in PWM-Inv mode, then flip the dutyCycle hi/low (e.g. 10% dutyCycle pulse would be high 90%, low 10%)
   if(!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(3), "PWM-Inv")) {
      mapLow = 100;
      mapHigh = 0;
   }

   // PWM Fixed so just grab the pwm duty cycle setting directly
   if(!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(15), "Fixed")) {
      doutPwmDutyCycle = atoi(getScreenPtr(DOUT_MENU)->getButtonLabel(11));
      ledcWrite(pwmChannel, constrain(map(doutPwmDutyCycle,mapLow,mapHigh,0,1023),0,1023));  // 10-bit pwm gives 0-1023 range for pwm

   // PWM can follow Ain/Temperature/Humidity/Current so get the measured value and map it to the pwm range
   } else if(!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(15), "Ain")) {     
      ledcWrite(pwmChannel, constrain(map((ainVoltage/atof(getScreenPtr(SETUP_MENU)->getButtonLabel(11)))*100,mapLow,mapHigh,0,1023),0,1023));  

   } else if(!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(15), "Temp")) {     
      ledcWrite(pwmChannel, constrain(map((curModuleTemp/atof(getScreenPtr(SETUP_MENU)->getButtonLabel(7)))*100,mapLow,mapHigh,0,1023),0,1023)); 
      
   } else if(!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(15), "Humidity")) {   
      ledcWrite(pwmChannel, constrain(map((curModuleHumidity/atof(getScreenPtr(SETUP_MENU)->getButtonLabel(11)))*100,mapLow,mapHigh,0,1023),0,1023)); 

   } else if(!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(15), "Current")) {   
      ledcWrite(pwmChannel, constrain(map((current_mA/atof(getScreenPtr(SETUP_MENU)->getButtonLabel(7)))*100,mapLow,mapHigh,0,1023),0,1023));
   }
}

void cycleDoutOutput(uint8_t buttonNumber) {
   if(!strcmp(doutOutputS,"Low")) {
      strcpy(doutOutputS,"High");
      ledcDetachPin(DOUTPIN);  
      digitalWrite(DOUTPIN,1);
   } else if(!strcmp(doutOutputS,"High")) {
      strcpy(doutOutputS,"PWM");
      // Set up and turn on PWM
      doutPwmDutyCycle = atoi(getScreenPtr(DOUT_MENU)->getButtonLabel(11));
      ledcAttachPin(DOUTPIN, pwmChannel);  
      ledcSetup(pwmChannel, pwmFrequency, pwmResolution); 
      ledcWrite(pwmChannel, constrain(map(doutPwmDutyCycle,0,100,0,1023),0,1023));  // 10-bit pwm gives 0-1023 range for pwm
   } else if(!strcmp(doutOutputS,"PWM")) {
      strcpy(doutOutputS,"PWM-Inv");  // Inverted PWM  (Means 10% dutyCycle is 90% high, 10% low)
      ledcAttachPin(DOUTPIN, pwmChannel);  
      ledcSetup(pwmChannel, pwmFrequency, pwmResolution); 
      ledcWrite(pwmChannel, constrain(map(doutPwmDutyCycle,100,0,0,1023),0,1023)); 
   } else if(!strcmp(doutOutputS,"PWM-Inv")) {
      strcpy(doutOutputS,"Low");
      ledcDetachPin(DOUTPIN);  
      digitalWrite(DOUTPIN,0);
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,doutOutputS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

void cycleDoutPwmFrequency(uint8_t buttonNumber) {
   if(!strcmp(doutPwmFrequencyS,"1 KHz")) {
      strcpy(doutPwmFrequencyS,"2 KHz");
      pwmFrequency=2000;
   } else if(!strcmp(doutPwmFrequencyS,"2 KHz")) {
      strcpy(doutPwmFrequencyS,"4 KHz");
      pwmFrequency=4000;
   } else if(!strcmp(doutPwmFrequencyS,"4 KHz")) {
      strcpy(doutPwmFrequencyS,"8 KHz");
      pwmFrequency=8000;
   } else if(!strcmp(doutPwmFrequencyS,"8 KHz")) {
      strcpy(doutPwmFrequencyS,"1 KHz");
      pwmFrequency=1000;
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,doutPwmFrequencyS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
   ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
   ledcWrite(pwmChannel, constrain(map(doutPwmDutyCycle,0,100,0,1023),0,1023));  // 10-bit pwm gives 0-1023 range for pwm
}

void cycleDoutPwmFollows(uint8_t buttonNumber) {
   if(!strcmp(doutPwmFollowsS,"Ain")) {
      strcpy(doutPwmFollowsS,"Temp");
   } else if(!strcmp(doutPwmFollowsS,"Temp")) {
      strcpy(doutPwmFollowsS,"Humidity");
   } else if(!strcmp(doutPwmFollowsS,"Humidity")) {
      strcpy(doutPwmFollowsS,"Current");
   } else if(!strcmp(doutPwmFollowsS,"Current")) {
      strcpy(doutPwmFollowsS,"Fixed");
   } else if(!strcmp(doutPwmFollowsS,"Fixed")) {
      strcpy(doutPwmFollowsS,"Ain");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,doutPwmFollowsS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

void cycleDoutActionOnAlarm(uint8_t buttonNumber) {
   if(!strcmp(doutActionOnAlarmS,"None")) {
      strcpy(doutActionOnAlarmS,"Low");
   } else if(!strcmp(doutActionOnAlarmS,"Low")) {
      strcpy(doutActionOnAlarmS,"High");
   } else if(!strcmp(doutActionOnAlarmS,"High")) {
      strcpy(doutActionOnAlarmS,"PWM");
   } else if(!strcmp(doutActionOnAlarmS,"PWM")) {
      strcpy(doutActionOnAlarmS,"PWM-Inv");
   } else if(!strcmp(doutActionOnAlarmS,"PWM-Inv")) {
      strcpy(doutActionOnAlarmS,"None");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,doutActionOnAlarmS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

//##############################
// 110v Control Callbacks
//##############################
void cycle110vActionOnAlarm(uint8_t buttonNumber) {
   if(!strcmp(action110vOnAlarmS,"Turn On")) {
      strcpy(action110vOnAlarmS,"Turn Off");
   } else if(!strcmp(action110vOnAlarmS,"Turn Off")) {
      strcpy(action110vOnAlarmS,"None");
   } else if(!strcmp(action110vOnAlarmS,"None")) {
      strcpy(action110vOnAlarmS,"Turn On");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,action110vOnAlarmS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

void cycle110vActionOnClock(uint8_t buttonNumber) {
   if(!strcmp(action110vOnClockS,"Turn On")) {
      strcpy(action110vOnClockS,"Turn Off");
   } else if(!strcmp(action110vOnClockS,"Turn Off")) {
      strcpy(action110vOnClockS,"None");
   } else if(!strcmp(action110vOnClockS,"None")) {
      strcpy(action110vOnClockS,"Turn On");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,action110vOnClockS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

void manual110vAction(uint8_t buttonNumber) {
   if(!strcmp(manual110vActionS,"On")) {
      strcpy(manual110vActionS,"Off");
      digitalWrite(EXT_POWER_RELAY, LOW);
   } else {
      strcpy(manual110vActionS,"On");
      digitalWrite(EXT_POWER_RELAY, HIGH);
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,manual110vActionS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

//##############################
// IV Setup Callbacks
//##############################
void toggleIvAlarm(uint8_t buttonNumber) {
   if(!strcmp(ivAlarmArmedS,"Enabled")) {
      strcpy(ivAlarmArmedS,"Disabled");
      clockAlarmTripped = false;
      alarmTripped = false;
   } else {
      strcpy(ivAlarmArmedS,"Enabled");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,ivAlarmArmedS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

//##############################
// Temperature Setup Callbacks
//##############################
void toggleTempAlarm(uint8_t buttonNumber) {
   if(!strcmp(tempAlarmArmedS,"Enabled")) {
      strcpy(tempAlarmArmedS,"Disabled");
      clockAlarmTripped = false;
      alarmTripped = false;
   } else {
      strcpy(tempAlarmArmedS,"Enabled");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,tempAlarmArmedS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

// #########################
// Clock screen callbacks
// #########################
// User can change the data/time for the clock or alarm here
void setClockTime(uint8_t) {

   // Reset the RTC to the settings loaded into the clock menu Y/M/D  H/M/S buttons
   RTC.adjust(DateTime(atoi(curScreenPtr->getButtonLabel(9)),
                       atoi(curScreenPtr->getButtonLabel(10)),
                       atoi(curScreenPtr->getButtonLabel(11)),
                       atoi(curScreenPtr->getButtonLabel(13)),
                       atoi(curScreenPtr->getButtonLabel(14)),
                       atoi(curScreenPtr->getButtonLabel(15))));
   now = RTC.now();
   strcpy(dateStringFormat, "YYYY-MM-DD hh:mm:ss");
   strcpy(dateString[0],now.toString(dateStringFormat));
   updateClock();
}

// There is a user settable alarm that can be used to trigger time-based events (like turning on the external 110v power module)
void setAlarmTime(uint8_t) {
   strcpy(dateString[1] , "");
   strcat(dateString[1], curScreenPtr->getButtonLabel(9));
   strcat(dateString[1], "-");
   strcat(dateString[1], curScreenPtr->getButtonLabel(10));
   strcat(dateString[1], "-");
   strcat(dateString[1], curScreenPtr->getButtonLabel(11));
   strcat(dateString[1], " ");
   strcat(dateString[1], curScreenPtr->getButtonLabel(13));
   strcat(dateString[1], ":");
   strcat(dateString[1], curScreenPtr->getButtonLabel(14));
   strcat(dateString[1], ":");
   strcat(dateString[1], curScreenPtr->getButtonLabel(15));
   updateClockAlarm();
}

// Turn on/off the alarm
void toggleClockAlarm(uint8_t buttonNumber) {
   if(!strcmp(clockAlarmArmedS,"AlarmOn")) {
      strcpy(clockAlarmArmedS,"AlarmOff");
   } else {
      strcpy(clockAlarmArmedS,"AlarmOn");
   }
   curScreenPtr->updateButtonLabel(curButtonPressed,clockAlarmArmedS);
   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}

//#################################################
// Placeholder for buttons with no callback defined 
//#################################################
void nop(uint8_t buttonNumber) {
   char txt[] = "Not Yet Implemented";
   statusSprite.setTextColor(STATUS_COLOR, STATUS_BACKGROUND);
   statusSprite.setTextDatum(STATUS_DATUM);
   statusSprite.setFreeFont(STATUS_TEXT_FONT);
   statusSprite.fillSprite(STATUS_BACKGROUND);
   statusSprite.drawString(txt, STATUS_WIDTH/2,STATUS_HEIGHT/2,GFXFF);
   statusSprite.pushSprite(STATUS_X, STATUS_Y);
   delay(1500);
   curScreenPtr->drawScreen();
}

//#########################################################################################
//#########################################################################################
// Graphing screens
//#########################################################################################
//#########################################################################################

//#################################
// AD (Analog-in/Digital-in) graphs
//#################################
void drawAdGraph(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr = getScreenPtr(GRAPH);
   strcpy(curResType,"AD");
   curScreenPtr->setScreenType(curResType);

   // Fix the number of intervals at 10 as that's about the most we can fit grid labels for
   // Use the "Monitor-Duration" button as the X-Axis maximum
   curScreenPtr->setXAxis(0, atof(getScreenPtr(SETUP_MENU)->getButtonLabel(15)), 10, "Time (Min)");

   // The "graph" button will initially display the current-ma graph
   if(!strcmp(prevScreenPtr->getScreenTitle(), MONITOR_MENU) || buttonNumber == 20) {
      strcpy(currentlyGraphing,resF0);
      curScreenPtr->setYAxis(atof(getScreenPtr(AXIS_MENU)->getButtonLabel(11)), 
                             atof(getScreenPtr(AXIS_MENU)->getButtonLabel(3)), 10, "Din Count");
      curScreenPtr->drawGraph(resultArraysFilled,resF0,resArrIdx,monitoredResultsXAxis0,monitoredResultsYAxis0);
   } else if(buttonNumber == 21) {
      strcpy(currentlyGraphing,resF1);
      curScreenPtr->setYAxis(atof(getScreenPtr(AXIS_MENU)->getButtonLabel(11)), 
                             atof(getScreenPtr(AXIS_MENU)->getButtonLabel(7)), 10, "Ain Voltage");
      curScreenPtr->drawGraph(resultArraysFilled,resF1,resArrIdx,monitoredResultsXAxis1,monitoredResultsYAxis1);
   } 
}

// IV (current/voltage) graphs
void drawIvGraph(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr = getScreenPtr(GRAPH);
   strcpy(curResType,"IV");
   curScreenPtr->setScreenType(curResType);

   // Fix the number of intervals at 10 as that's about the most we can fit grid labels for
   // Use the "Monitor-Duration" button as the X-Axis maximum
   curScreenPtr->setXAxis(0, atof(getScreenPtr(SETUP_MENU)->getButtonLabel(15)), 10, "Time (Min)");

   // The "graph" button will initially display the current-ma graph
   if(!strcmp(prevScreenPtr->getScreenTitle(), MONITOR_MENU) || buttonNumber == 20) {
      strcpy(currentlyGraphing,resF0);
      curScreenPtr->setYAxis(atof(getScreenPtr(AXIS_MENU)->getButtonLabel(15)), 
                             atof(getScreenPtr(AXIS_MENU)->getButtonLabel(3)), 10, "Current (mA)");
      curScreenPtr->drawGraph(resultArraysFilled,resF0,resArrIdx,monitoredResultsXAxis0,monitoredResultsYAxis0);
   } else if(buttonNumber == 21) {
      strcpy(currentlyGraphing,resF1);
      curScreenPtr->setYAxis(atof(getScreenPtr(AXIS_MENU)->getButtonLabel(15)), 
                             atof(getScreenPtr(AXIS_MENU)->getButtonLabel(7)), 10, "Voltage (V)");
      curScreenPtr->drawGraph(resultArraysFilled,resF1,resArrIdx,monitoredResultsXAxis1,monitoredResultsYAxis1);
   } else if(buttonNumber == 22) {
      strcpy(currentlyGraphing,resF2);
      curScreenPtr->setYAxis(atof(getScreenPtr(AXIS_MENU)->getButtonLabel(15)), 
                             atof(getScreenPtr(AXIS_MENU)->getButtonLabel(11)), 10, "Power (mW)");
      curScreenPtr->drawGraph(resultArraysFilled,resF2,resArrIdx,monitoredResultsXAxis2,monitoredResultsYAxis2);
   }
}

//###############################
// Temperature/Humidity graphs
//###############################
void drawTempGraph(uint8_t buttonNumber) {
   prevScreenPtr = curScreenPtr;
   curScreenPtr = getScreenPtr(GRAPH);
   strcpy(curResType,"TEMP");
   curScreenPtr->setScreenType(curResType);

   // Fix the number of intervals at 10 as that's about the most we can fit grid labels for
   curScreenPtr->setXAxis(0, atof(getScreenPtr(SETUP_MENU)->getButtonLabel(15)), 10, "Time (Min)");

   // The "graph" button will initially display the probe-temp graph
   if(!strcmp(prevScreenPtr->getScreenTitle(), MONITOR_MENU) || buttonNumber == 20) {
      strcpy(currentlyGraphing,resF0);
      curScreenPtr->setYAxis(atof(getScreenPtr(AXIS_MENU)->getButtonLabel(7)), 
                             atof(getScreenPtr(AXIS_MENU)->getButtonLabel(3)), 10, "Probe Temp (F)");
      curScreenPtr->drawGraph(resultArraysFilled,resF0,resArrIdx,monitoredResultsXAxis0,monitoredResultsYAxis0);
   } else if(buttonNumber == 21) {
      strcpy(currentlyGraphing,resF1);
      curScreenPtr->setYAxis(atof(getScreenPtr(AXIS_MENU)->getButtonLabel(7)), 
                             atof(getScreenPtr(AXIS_MENU)->getButtonLabel(3)), 10, "Module Temp (F)");
      curScreenPtr->drawGraph(resultArraysFilled,resF1,resArrIdx,monitoredResultsXAxis1,monitoredResultsYAxis1);
   } else if(buttonNumber == 22) {
      strcpy(currentlyGraphing,resF2);
      curScreenPtr->setYAxis(atof(getScreenPtr(AXIS_MENU)->getButtonLabel(15)), 
                             atof(getScreenPtr(AXIS_MENU)->getButtonLabel(11)), 10, "Humidity(%)");
      curScreenPtr->drawGraph(resultArraysFilled,resF2,resArrIdx,monitoredResultsXAxis2,monitoredResultsYAxis2);
   }
}


//################################################################################33
//################################################################################33
// Results monitoring/updating
//################################################################################33
//################################################################################33

//##############################################################################################################
// Update result screen and graphs
// NOTE:  This assumes we use the same buttons in all the setup screens (i.e. 19 is the monitor-interval, etc.)
//        We can change this if we start changing layouts by passing the button numbers in the parameter list
//##############################################################################################################
void updateResults(const char * resultMenu, const char * graphMenu, const char * resType, const char * setupMenu, 
                   const char * res0File,   const char * res1File,  const char * res2File,
                   float * res0ptr, float * res1ptr, float * res2ptr, void (*funcPtr)()) {

   if(!strcmp(curScreenPtr->getScreenType(), resType) && (!strcmp(curScreenPtr->getScreenTitle(), resultMenu) || !strcmp(curScreenPtr->getScreenTitle(), graphMenu))) {
       (*funcPtr)();  // Read the sensor results and update the text sprite fields

      // Need to store the current measured results and draw the monitored results on the graph if graphing
      if(monitoringResults) {
         timeMonitored =  (millis() - monitoringStartTime)/1000.0/60.0;  // in minutes
         if(timeMonitored < 0) { 
            timeMonitored = 0.0; 
         }

         // Initialize the 0th result if this is the first time we're drawing the graphs so we have an initial time-0 measurment..
         if(!resultArraysFilled && resArrIdx == 0) {
            monitoredResultsXAxis0[resArrIdx] = 0.0; 
            monitoredResultsXAxis1[resArrIdx] = 0.0; 
            monitoredResultsXAxis2[resArrIdx] = 0.0; 
            monitoredResultsYAxis0[resArrIdx] = *res0ptr; 
            monitoredResultsYAxis1[resArrIdx] = *res1ptr; 
            monitoredResultsYAxis2[resArrIdx] = *res2ptr; 
            resArrIdx++;
         }
         // Only log to the array in time increments set by the "Monitor Interval" (Button (19) in the setup menu)
         curMonitorTime = (millis() - lastResultsLoggedTime)/1000.0/60.0;
         if(curMonitorTime >= atof(getScreenPtr(setupMenu)->getButtonLabel(19))){
            monitoredResultsXAxis0[resArrIdx] = timeMonitored; 
            monitoredResultsXAxis1[resArrIdx] = timeMonitored; 
            monitoredResultsXAxis2[resArrIdx] = timeMonitored; 

            monitoredResultsYAxis0[resArrIdx] = *res0ptr; 
            monitoredResultsYAxis1[resArrIdx] = *res1ptr; 
            monitoredResultsYAxis2[resArrIdx] = *res2ptr; 

            lastResultsLoggedTime = millis();

            // Displaying the graph so go plot the latest data
            if(!strcmp(curScreenPtr->getScreenTitle(), graphMenu)) {
               if(!strcmp(currentlyGraphing,res0File)) {
                  curScreenPtr->addGraphData(resArrIdx, monitoredResultsXAxis0, monitoredResultsYAxis0);
               } else if(!strcmp(currentlyGraphing,res1File)) {
                  curScreenPtr->addGraphData(resArrIdx, monitoredResultsXAxis1, monitoredResultsYAxis1);
               } else if(!strcmp(currentlyGraphing,res2File)) {
                  curScreenPtr->addGraphData(resArrIdx, monitoredResultsXAxis2, monitoredResultsYAxis2);
               }
               // Need to delay after drawing the updates before trying to update again else the graph lines get chopped up.
               // Looks like a minimum time requrement for drawing.  More investigation needed...
               delay(10);

            } 
            resArrIdx++;
         }
         // If we've filled the result arrays, write them to the SD card and reset to the start of the array.
         // We move the last entry in the array buffer to the 0th entry and reset the index to the 1st entry.
         // This way there is a "prevData" point for the next call to addGraphData.
         if(resArrIdx == MAX_RESULT_POINTS ) { 

           // NOTE:  Be sure to include "/" in front of the file name (starts at root directory) or the file opening will fail...
            writeResultsToFile(0,res0File,resArrIdx,monitoredResultsXAxis0,monitoredResultsYAxis0);
            writeResultsToFile(0,res1File,resArrIdx,monitoredResultsXAxis1,monitoredResultsYAxis1);
            writeResultsToFile(0,res2File,resArrIdx,monitoredResultsXAxis2,monitoredResultsYAxis2);
            resArrIdx = 1;  // Reset to "1" not "0" as writeResultsToFile(0) moves the upper result back to entry 0
            resultArraysFilled = true;
         }
         // setup-menu button-15 is monitor duration.
         if(timeMonitored > atof(getScreenPtr(setupMenu)->getButtonLabel(15))) { 
            monitorResults(22);   // Virtually "press" the "Stop" monitoring button (button-22)
         }
      }
   }
}

//#######################################################################
// Update the Ain/DinD screen result fields with the measured results
//#######################################################################
void drawAdResults() {

   // Create strings from the measured values for printing
   dtostrf(dinLevel,3,1,dinLevelS); 
   dtostrf(dinCount,3,1,dinCountS); 
   dtostrf(ainVoltage,3,1,ainVoltageS); 
   dtostrf(timeMonitored,3,1,timeMonitoredS);
   curScreenPtr->updateTextSprite(0,dinLevelS);
   curScreenPtr->updateTextSprite(1,dinCountS);
   curScreenPtr->updateTextSprite(2,ainVoltageS);
   curScreenPtr->updateTextSprite(3,timeMonitoredS);

   curScreenPtr->drawTextSprite();  // Update the text result fields
}


//#######################################################################
// Update the IV screen result fields with the measured results
//#######################################################################
void drawIvResults() {

   // Create strings from the measured values for printing
   dtostrf(current_mA,3,1,current_mAS); 
   dtostrf(loadVoltage,3,1,loadVoltageS); 
   dtostrf(power_mW,3,1,power_mWS);
   dtostrf(timeMonitored,3,1,timeMonitoredS);
   curScreenPtr->updateTextSprite(0,current_mAS);
   curScreenPtr->updateTextSprite(1,loadVoltageS);
   curScreenPtr->updateTextSprite(2,power_mWS);
   curScreenPtr->updateTextSprite(3,timeMonitoredS);

   curScreenPtr->drawTextSprite();  // Update the text result fields
}

//#######################################################################
// Update the temperature screen result fields with the measured results
//#######################################################################
void drawTempResults() {

   // Create strings from the measured values for printing
   dtostrf(curProbeTemp,3,1,curProbeTempS); 
   dtostrf(curModuleTemp,3,1,curModuleTempS); 
   dtostrf(curModuleHumidity,3,1,curModuleHumidityS);
   dtostrf(timeMonitored,3,1,timeMonitoredS);
   curScreenPtr->updateTextSprite(0,curProbeTempS);
   curScreenPtr->updateTextSprite(1,curModuleTempS);
   curScreenPtr->updateTextSprite(2,curModuleHumidityS);
   curScreenPtr->updateTextSprite(3,timeMonitoredS);

   curScreenPtr->drawTextSprite();  // Update the text result fields
}

//######################################################
// Results monitoring.  Start timer.  Write results file
//######################################################
void monitorResults(uint8_t buttonNumber) {

   // Get the current date/time from the real-time-clock module to append to the results file name
   now = RTC.now();
   strcpy(dateStringFormat, "YYYY-MM-DD_hh-mm-ss");
   strcpy(dateString[0],now.toString(dateStringFormat));

   if(!strcmp(curScreenPtr->getScreenTitle(), MONITOR_MENU) || !strcmp(curResType, "IV") || !strcmp(curResType, "TEMP") || !strcmp(curResType, "AD")) {
      strcpy(resMenu , MONITOR_MENU);
   }

   // StartLog button
   if(buttonNumber == 21) { 
      strcpy(curStartResumeState, "Restart"); 
      getScreenPtr(resMenu)->updateButtonLabel(21,curStartResumeState);

      // Start tracking the monitoring time
      monitoringResults = true;
      dinCount = 0.0;
      timeMonitored = 0.0;
      lastResultsLoggedTime = millis();
      monitoringStartTime=millis();
      resArrIdx=0;
      resultArraysFilled = false;

      // NOTE:  Be sure to include "/" in front of the file name (starts at root directory) or the file opening will fail...
      if(!strcmp(curScreenPtr->getScreenTitle(), MONITOR_MENU) || !strcmp(curResType, "IV")) {
         strcpy(resF0 , "/ivCurrent_"); strcat(resF0 , dateString[0]); strcat(resF0, ".csv");
         strcpy(resF1 , "/ivVoltage_"); strcat(resF1 , dateString[0]); strcat(resF1, ".csv");
         strcpy(resF2 , "/ivPower_"); strcat(resF2 , dateString[0]); strcat(resF2, ".csv");
      } else if(!strcmp(curScreenPtr->getScreenTitle(), MONITOR_MENU) || !strcmp(curResType, "TEMP")) {
         strcpy(resF0 , "/probeTemp_"); strcat(resF0 , dateString[0]); strcat(resF0, ".csv");
         strcpy(resF1 , "/moduleTemp_"); strcat(resF1 , dateString[0]); strcat(resF1, ".csv");
         strcpy(resF2 , "/moduleHumidity_"); strcat(resF2 , dateString[0]); strcat(resF2, ".csv");
      } else if(!strcmp(curScreenPtr->getScreenTitle(), MONITOR_MENU) || !strcmp(curResType, "AD")) {
         strcpy(resF0 , "/dinCount_"); strcat(resF0 , dateString[0]); strcat(resF0, ".csv");
         strcpy(resF1 , "/ainVoltage_"); strcat(resF1 , dateString[0]); strcat(resF1, ".csv");
      }

   // Stop button
   } else if(buttonNumber == 22) {
      monitoringResults = false;
      strcpy(curStartResumeState, "StartLog");
      getScreenPtr(resMenu)->updateButtonLabel(21,curStartResumeState);

      writeResultsToFile(1, resF0,resArrIdx,monitoredResultsXAxis0,monitoredResultsYAxis0);
      writeResultsToFile(1, resF1,resArrIdx,monitoredResultsXAxis1,monitoredResultsYAxis1);
      writeResultsToFile(1, resF2,resArrIdx,monitoredResultsXAxis2,monitoredResultsYAxis2);
      resArrIdx = 0;
      resultArraysFilled = true;
   }

   curScreenPtr->drawButtonTextSprite();  // Add the labels to the buttons
}


//###########################################################
// Write results array to the SD card file 
// We actually move the last entry in the array buffer 
// to the 0th entry and reset the index to the 1st entry.
//###########################################################
void writeResultsToFile(boolean writeAllEntries, const char * filename, int arrIndex, float * xAxisArrPtr, float * yAxisArrPtr) {

   myFile = SD.open(filename, FILE_APPEND);
   uint8_t indexToStopAt;

   // If we are finished logging, writeAllEntries will be true and we'll write out all the entries in the array
   if(writeAllEntries) {
      indexToStopAt = arrIndex;
   } else {
      indexToStopAt = arrIndex-1;
   }

   // if the file opened okay, write to it:
   if (myFile) {
      //Serial.print(F("Writing to logFile: ")); Serial.println(filename);
      for(uint8_t i=0; i<indexToStopAt; i++) {
         myFile.print(*(xAxisArrPtr+i));
         myFile.print(",");
         myFile.println(*(yAxisArrPtr+i));
      }
      myFile.close();

      if(!writeAllEntries) {
         // If we're not flushing the buffer, move the last array entry to the 0th entry.  That leaves 
         // a lower entry so the next call to "addGraphData" will have a "prevData" result to plot from.
         *xAxisArrPtr = *(xAxisArrPtr+arrIndex-1);
         *yAxisArrPtr = *(yAxisArrPtr+arrIndex-1);
      }
   } else {
      // if the file didn't open, print an error:
      Serial.print(F("Error opening ")); Serial.print(filename);
   }
}

//###########################################################################
//###########################################################################
// Misc Functions
//###########################################################################
//###########################################################################

//###########################################################################
// Touch screen calibration.  Need to only do this once with a new 
// touch panel.  The calibration constants are stored in the ESP32 
// SPIFFS file system for future use.
//###########################################################################
void touchCalibrate() {
   uint16_t calData[5];
   uint8_t calDataOK = 0;

   // Check file system exists
   if (!SPIFFS.begin()) {
      Serial.println(F("Formating file system"));
      SPIFFS.format();
      SPIFFS.begin();
   }

   // Check if calibration file exists and size is correct
   if (SPIFFS.exists(CALIBRATION_FILE)) {
      if (REPEAT_CAL)
      {
         // Delete if we want to re-calibrate
         SPIFFS.remove(CALIBRATION_FILE);
      }
      else
      {
         File f = SPIFFS.open(CALIBRATION_FILE, "r");
         if (f) {
            if (f.readBytes((char *)calData, 14) == 14)
               calDataOK = 1;
            f.close();
         }
      }
   }

   if (calDataOK && !REPEAT_CAL) {
      // Calibration data valid
      tft.setTouch(calData);
   } else {
      // Data not valid so recalibrate
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(20, 0);
      tft.setTextFont(2);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);

      tft.println(F("Touch corners as indicated"));

      tft.setTextFont(1);
      tft.println();

      if (REPEAT_CAL) {
         tft.setTextColor(TFT_RED, TFT_BLACK);
         tft.println(F("Set REPEAT_CAL to false to stop this running again!"));
      }

      tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.println(F("Calibration complete!"));

      // store data
      File f = SPIFFS.open(CALIBRATION_FILE, "w");
      if (f) {
         f.write((const unsigned char *)calData, 14);
         f.close();
      }
   }
}
//###################################################
// Check the state of a switch after debouncing it
//###################################################
boolean checkSwitch(int pin) {
    boolean state;
    boolean prevState;
    boolean bouncing = true;
    prevState = digitalRead(pin);
    while(bouncing) {
        delay(10);
        state = digitalRead(pin);
        if(state != prevState) {
            prevState=state;
            continue;
        }
        bouncing = false;
    }
    // At this point the switch state is stable
    if(state == HIGH) {
        return true;
    } else {
        return false;
    }
}


//##################################################################
//##################################################################
// Setup section
//##################################################################
//##################################################################
void setup() {

   Serial.begin(115200);
   while(!Serial) { }
   delay(1000);

   uint32_t Freq ;

   // Display interesting chip info
   Freq = getCpuFrequencyMhz();
   Serial.print(F("CPU Freq = "));
   Serial.print(Freq);
   Serial.println(F(" MHz"));
   Freq = getXtalFrequencyMhz();
   Serial.print(F("XTAL Freq = "));
   Serial.print(Freq);
   Serial.println(F(" MHz"));
   Freq = getApbFrequency();
   Serial.print(F("APB Freq = "));
   Serial.print(Freq);
   Serial.println(F(" Hz"));

   // Set up the tft
   tft.init();
   tft.setRotation(1);  // Landscape

   // Using sprites for the text that changes values (temperature results, etc.) and status messages.
   // Also using one big cover sprite for all the buttons.  This is because we modify the button labels 
   // for buttons that are used to set options (e.g. temperature limit min/max, alarm on/off, etc.).
   //
   // Sprites eliminate flicker that would occur if we had to blank and refresh the values in real time.
   // The pixels for the sprite are computed first, then the sprite is transferred to display memory in one pass.

   btnTextSprite.createSprite(BUTTON_TEXT_SP_WIDTH,BUTTON_TEXT_SP_HEIGHT);   // Each button text has a small sprite for updating it
   btnTextSprite.setColorDepth(8); // Using color in the buttons so need at least 8-bit color depth

   textSprite.createSprite(TEXT_SP_WIDTH,TEXT_SP_HEIGHT);   // Size is X by Y pixels
   textSprite.setColorDepth(1); // Save memory since we are just printing text

   statusSprite.createSprite(STATUS_WIDTH, STATUS_HEIGHT);  // Used for displaying pop-up messages
   statusSprite.setColorDepth(1); 

   // Separate sprite to deal with the long clock strings and real time diaplaying of the time
   clockSprite.createSprite(CLOCK_WIDTH, CLOCK_HEIGHT);
   clockSprite.setColorDepth(1); 

   // Sprite to handle rotating the Y-Axis graph labels.  
   yAxisSprite.createSprite(SCREEN_HEIGHT/2, 25);  
   yAxisSprite.setColorDepth(1); 
   yAxisSprite.setPivot(0, GRAPH_LABEL_SP_H/2);      // Set pivot around the midpoint/left side of the sprite
   yAxisSprite.fillSprite(TFT_BLACK); // Fill the Sprite with black

   // Calibrate the touch screen and retrieve the scaling factors
   touchCalibrate();

   // Set up the current/voltage measuring module
   if(! ivModule.begin()) {
      Serial.println(F("Failed to initialize INA219 module"));
      while(1) {
      }
   }

   // Set up the real time clock
   if (! RTC.begin()) {
      Serial.println(F("Couldn't find RTC"));
      while (1);
   }

   if (! RTC.isrunning()) {
      Serial.println(F("RTC is NOT running!"));
   }

   //__________________ set rtc time here ________________________________________  
   //Sets the time in the RTC by getting the time from the laptop
   if(SET_CLOCK_FROM_COMPILE) {
      RTC.adjust(DateTime(__DATE__, __TIME__));
   }

   // sd card        
   Serial.println(F("Initializing SD card"));

   // pin gpio5 is the default CS pin...
   if (!SD.begin()) {
      Serial.println(F("SDcard initialization failed!"));
      return;
   }

   uint8_t cardType = SD.cardType();
   if(cardType == CARD_NONE) {
      Serial.println(F("No SD card attached"));
      return;
   }
   Serial.print(F("SD Card Type: "));
   if(cardType == CARD_MMC){
      Serial.println(F("MMC"));
   } else if(cardType == CARD_SD){
      Serial.println(F("SDSC"));
   } else if(cardType == CARD_SDHC){
      Serial.println(F("SDHC"));
   } else {
      Serial.println(F("UNKNOWN"));
   }

   // Setup the GPIOs
   pinMode(EXT_POWER_RELAY, OUTPUT);
   pinMode(DINPIN, INPUT_PULLUP);
   pinMode(AINPIN, INPUT);
   pinMode(DOUTPIN, OUTPUT);

   // Initiate the temp libs
   Serial.println(F("Initialize Temp Probe and Module"));
   tempSensor.begin();  // The temperature probe
   tempSensor.setWaitForConversion(false);  // Don't want to block for 750ms when initiating a temp read...
   dht.read();

   // Start a sampling so we can be ready to read it in the main loop
   tempSensor.requestTemperatures();
   lastTempReadTime = millis();
   lastIvReadTime = millis();
   delay(1000);

   // Splash Screen
   tft.fillScreen(TFT_BLACK);
   tft.setTextColor(TITLE_COLOR, TFT_BLACK);
   tft.setTextDatum(TITLE_DATUM);
   tft.setFreeFont(TITLE_FONT);
   tft.drawString("Data Logger V1.0",SCREEN_WIDTH/2,SCREEN_HEIGHT/4,GFXFF);
   tft.drawString("by",SCREEN_WIDTH/2,SCREEN_HEIGHT/4+50,GFXFF);
   tft.drawString("Hobby Hacker Labs",SCREEN_WIDTH/2,SCREEN_HEIGHT/4+95,GFXFF);
   delay(2000);


   //#############################################################################################################
   // Initialize all the screens and buttons (by button number: 0 at the top left, count across each row)
   // 0 1 2 3
   // 4 5 6 7 ...,
   // Also text for each of the menus, and callbacks (nop used for callbacks that are not implemented).
   //#############################################################################################################

   // Set up the screen Pointers array with all the defined screens.  Later we use this to get the 
   // screen pointer by looping through the array and matching against the screenPtr->getScreenTitle
   screenPtrs[0] = &mainMenuScreen;
   screenPtrs[1] = &keypad;
   screenPtrs[2] = &clockScreen;
   screenPtrs[3] = &screen110v;
   screenPtrs[4] = &axisScreen;
   screenPtrs[5] = &graphScreen;
   screenPtrs[6] = &setupScreen;
   screenPtrs[7] = &monitorScreen;
   screenPtrs[8] = &doutScreen;

   //#########################################################################################
   // Main menu buttons
   //#########################################################################################
   mainMenuScreen.init(&mainMenuScreen);
   mainMenuScreen.enableButton(5,  "I/V",   drawIvSetupMenu);
   mainMenuScreen.enableButton(6,  "Temp",  drawTempSetupMenu);
   mainMenuScreen.enableButton(9,  "A-In/D-In",  drawAdSetupMenu);
   mainMenuScreen.enableButton(10, "D-Out",      drawDoutMenu);
   mainMenuScreen.enableButton(13, "Clock", drawClockScreen);
   mainMenuScreen.enableButton(14, "110v",  draw110vMenu);

   //####################
   // Shared graph screen
   //####################
   graphScreen.init(&graphScreen);

   //###############
   // Clock menu
   //###############
   clockScreen.init(&clockScreen);
   // (button number, button label,  button callback)
   clockScreen.enableButton(9, curYearS, drawKeypad);
   clockScreen.enableButton(10, curMonthS, drawKeypad);
   clockScreen.enableButton(11, curDayS, drawKeypad);
   clockScreen.enableButton(13, curHourS, drawKeypad);
   clockScreen.enableButton(14, curMinS, drawKeypad);
   clockScreen.enableButton(15, curSecS, drawKeypad);
   clockScreen.enableButton(20, clockAlarmArmedS,toggleClockAlarm);
   clockScreen.enableButton(21, "SetAlrm", setAlarmTime);
   clockScreen.enableButton(22, "SetClk",  setClockTime);
   clockScreen.enableButton(23, "Back", drawMainMenu);

   // (Text field, Text string,  text placement X,Y)
   clockScreen.enableTextField(0, "Current Time:", TEXT_LEFT, TEXT_LINE0);
   clockScreen.enableTextField(1, "Alarm Time:",   TEXT_LEFT, TEXT_LINE1);
   clockScreen.enableTextField(2, "Y/M/D",    TEXT_LEFT, TEXT_LINE2);
   clockScreen.enableTextField(3, "H/M/S",    TEXT_LEFT, TEXT_LINE3);

   // (Text Sprite field, Text Sprite string,  text placement within sprite X,Y)
   clockScreen.enableClockSprite(0,dateString[0], CLOCK_SP_X, CLOCK_SP_Y);
   clockScreen.enableClockSprite(1,dateString[1], CLOCK_SP_X, ALARM_SP_Y);

   //####################
   // 110v control menu
   //####################
   screen110v.init(&screen110v);
   // (button number, button label,  button callback)
   screen110v.enableButton(7,  action110vOnAlarmS,      cycle110vActionOnAlarm);
   screen110v.enableButton(11, action110vOnClockS,      cycle110vActionOnClock);
   screen110v.enableButton(15, manual110vActionS,       manual110vAction);
   screen110v.enableButton(23, "Back",                  drawMainMenu);

   screen110v.enableTextField(1, "Action On Alarm",        TEXT_LEFT, TEXT_LINE1);
   screen110v.enableTextField(2, "Action On Clk-Alarm",    TEXT_LEFT, TEXT_LINE2);
   screen110v.enableTextField(3, "Manual On/Off",          TEXT_LEFT, TEXT_LINE3);

   //#########################################################################################
   // Current/Voltage menus
   //#########################################################################################

   strcpy(curStartResumeState, "StartLog");  // Initialize the monitor button (StartLog/Restart)

   // IV Graph Axis Settings
   dtostrf(curAxisMax,3,1,curAxisMaxS);
   dtostrf(voltAxisMax,3,1,voltAxisMaxS);
   dtostrf(powerAxisMax,3,1,powerAxisMaxS);
   dtostrf(allIvAxisMin,3,1,allIvAxisMinS);

   // IV screen
   dtostrf(current_mA,3,1,current_mAS);
   dtostrf(loadVoltage,3,1,loadVoltageS);
   dtostrf(power_mW,3,1,power_mWS);
   dtostrf(timeMonitored,3,1,timeMonitoredS);

   //#########################################################################################
   // Temperature menus
   //#########################################################################################

   // Temperature Graph Axis Settings
   dtostrf(tempAxisMax,3,1,tempAxisMaxS);
   dtostrf(tempAxisMin,3,1,tempAxisMinS);
   dtostrf(humidityAxisMax,3,1,humidityAxisMaxS);
   dtostrf(humidityAxisMin,3,1,humidityAxisMinS);

   //###################
   // Temperature screen
   //###################
   dtostrf(curProbeTemp,3,1,curProbeTempS);
   dtostrf(curModuleTemp,3,1,curModuleTempS);
   dtostrf(curModuleHumidity,3,1,curModuleHumidityS);
   dtostrf(timeMonitored,3,1,timeMonitoredS);

   //#########################################################################################
   // Analog-in/Digital-in menu
   //#########################################################################################

   //###################
   // AD results screen
   //###################
   itoa(dinLevel,dinLevelS,10);
   dtostrf(dinCount,3,1,dinCountS);
   dtostrf(ainVoltage,3,1,ainVoltageS);
   dtostrf(timeMonitored,3,1,timeMonitoredS);


   //#########################################################################################
   // Digital-out menu
   //#########################################################################################

   dtostrf(doutPwmDutyCycle,3,1,doutPwmDutyCycleS);
   doutScreen.init(&doutScreen);

   // (button number, button label,  button callback)
   doutScreen.enableButton(3,  doutOutputS,          cycleDoutOutput);
   doutScreen.enableButton(7,  doutPwmFrequencyS,    cycleDoutPwmFrequency);
   doutScreen.enableButton(11, doutPwmDutyCycleS,    drawKeypad);
   doutScreen.enableButton(15, doutPwmFollowsS,      cycleDoutPwmFollows);
   doutScreen.enableButton(19, doutActionOnAlarmS,   cycleDoutActionOnAlarm);
   doutScreen.enableButton(23, "Back",               drawMainMenu);

   doutScreen.enableTextField(0, "Dout Output",             TEXT_LEFT, TEXT_LINE0);
   doutScreen.enableTextField(1, "PWM Frequency",           TEXT_LEFT, TEXT_LINE1);
   doutScreen.enableTextField(2, "PWM Duty Cycle",          TEXT_LEFT, TEXT_LINE2);
   doutScreen.enableTextField(3, "PWM Duty Cycle Follows",  TEXT_LEFT, TEXT_LINE3);
   doutScreen.enableTextField(4, "Dout Action On Alarm",    TEXT_LEFT, TEXT_LINE4);



   //#########################################################################################
   // Keypad Screen - Used for entering numbers for the options buttons
   //#########################################################################################
   // Initialize the strings used in the keypad screen
   strcpy(keypadStackArr,"");
   keypad.init(&keypad);

   // (button number, button label,  button callback)
   keypad.enableButton(4,  "7",      updateKeypad);
   keypad.enableButton(5,  "8",      updateKeypad);
   keypad.enableButton(6,  "9",      updateKeypad);
   keypad.enableButton(7,  "Enter",  updateKeypad);
   keypad.enableButton(8,  "4",      updateKeypad);
   keypad.enableButton(9,  "5",      updateKeypad);
   keypad.enableButton(10, "6",      updateKeypad);
   keypad.enableButton(12, "1",      updateKeypad);
   keypad.enableButton(13, "2",      updateKeypad);
   keypad.enableButton(14, "3",      updateKeypad);
   keypad.enableButton(16, "0",      updateKeypad);
   keypad.enableButton(17, ".",      updateKeypad);
   keypad.enableButton(18, "<--",    updateKeypad);
   keypad.enableButton(19, "Clear",  updateKeypad);
   keypad.enableButton(23, "Cancel", updateKeypad);

   keypad.enableTextField(0, keypadStackArr, KEYPAD_RESULT_X, TEXT_LINE0);

   Serial.print(F("Free Heap Memory Left: "));Serial.println(esp_get_free_heap_size());
   drawMainMenu(0);
   Serial.print(F("Min Free Heap Memory: "));Serial.println(esp_get_minimum_free_heap_size());
}


//##################################################################
//##################################################################
// Main loop
//##################################################################
//##################################################################
void loop() {

   // Always check the din pin to see if it toggled (only count high to low transitions)
   // Keeping it simple and just polling each time around the loop as we will aren't intending 
   // on monitoring high frequency events.  Could always change this to interrupt based if
   // that assumption changes.
   dinLevel = checkSwitch(DINPIN);
   if(lastDinLevel == 1 && dinLevel == 0) {
      dinCount+=1;
      lastDinLevel = 0;
      dtostrf(dinLevel,3,0,dinLevelS); 
      dtostrf(dinCount,3,0,dinCountS); 
   }
   if(lastDinLevel == 0 && dinLevel == 1) {
      lastDinLevel = 1;
      dtostrf(dinLevel,3,0,dinLevelS); 
   }

   // Update the clock readout every second
   if(millis() - lastClockReadTime >= 1000) {
      now = RTC.now();
      strcpy(dateStringFormat, "YYYY-MM-DD hh:mm:ss");
      strcpy(dateString[0],now.toString(dateStringFormat));
      if(!strcmp(curScreenPtr->getScreenTitle(), CLOCK_MENU)) {
         updateClock();
      }
      lastClockReadTime = millis();
   }

   // Periodically check the current/voltage sensors
   unsigned long measureIvInterval = atof(getScreenPtr(SETUP_MENU)->getButtonLabel(19))*60*1000;  //button-19 is monitor-interval 
   if(millis() - lastIvReadTime >= measureIvInterval) {
      float shuntVoltage = 0.0;
      float busVoltage = 0.0;
      shuntVoltage = ivModule.getShuntVoltage_mV();
      busVoltage = ivModule.getBusVoltage_V();
      current_mA = ivModule.getCurrent_mA();
      if(current_mA < 0) {
         current_mA = 0.00;
      }
      power_mW = ivModule.getPower_mW();
      loadVoltage = busVoltage + (shuntVoltage / 1000);
      lastIvReadTime = millis();
   }
   // Periodically check the Analog-in voltage
   unsigned long measureAdInterval = atof(getScreenPtr(SETUP_MENU)->getButtonLabel(19))*60*1000;  //button-19 is monitor-interval 
   float ainRangeSelect = atof(getScreenPtr(SETUP_MENU)->getButtonLabel(11));
   if(millis() - lastAdReadTime >= measureAdInterval) {

      // ESP32 uses 12bit dac so 4096 counts
      ainVoltage = (analogRead(AINPIN)*1.0/4095.0) * ainRangeSelect;
      if(ainVoltage < 0) {
         ainVoltage = 0.00;
      }
      lastAdReadTime = millis();
   }

   // Periodically check the temperature and humidity from the probe and module
   // Note:  The sensor needs at least 750ms between samples.
   unsigned long measureTempInterval = atof(getScreenPtr(SETUP_MENU)->getButtonLabel(19))*60*1000;  //button-19 is monitor-interval 
   if(measureTempInterval < 750) { measureTempInterval = 750; }
   if(millis() - lastTempReadTime >= measureTempInterval) { 
      curProbeTemp = tempSensor.getTempFByIndex(0);
      curModuleTemp = (dht.getTemperature() * 1.8) + 32.0;  // Converted to Fahrenheit
      curModuleHumidity = dht.getHumidity();

      // Start another sampling for the next read
      dht.read();
      tempSensor.requestTemperatures();
      lastTempReadTime = millis();
   }

   // Check the touch panel.  See if a button was pushed.
   uint16_t touchX=0, touchY=0;

   // Will be true if there is a valid touch on the screen
   bool touchPressed = tft.getTouch(&touchX,&touchY);

   // Loop through all the buttons (for the currently displayed menu) to see 
   // if the touch coord is over one.  If so, execute that button's callback
   for(uint8_t bIndex=0; bIndex < NUM_BUTTONS; bIndex++) {
      if(curScreenPtr->isButtonVisible(bIndex)) {    // Only check buttons that are visible
         if(touchPressed && curScreenPtr->isPressCoordOverButton(bIndex,touchX,touchY)) {
            curScreenPtr->setButtonPressed(bIndex, true);
         } else {
            curScreenPtr->setButtonPressed(bIndex, false);
         }
      }
   }
   for(uint8_t bIndex=0; bIndex < NUM_BUTTONS; bIndex++) {
      if(curScreenPtr->isButtonVisible(bIndex)) {    // Only check buttons that are visible
         if(curScreenPtr->wasButtonJustReleased(bIndex)) {
            curScreenPtr->drawButton(bIndex, false);          // Back to normal to show it was released

            // Need to redraw the text as we stored a blank "" in the button object itself (we are adding our own labels over the top of the buttons)
            curScreenPtr->drawButtonTextSprite();
         }
         if(curScreenPtr->wasButtonJustPressed(bIndex)) {
            curScreenPtr->drawButton(bIndex, true);          // Hilite button to show it was pressed

            // Execute the callback for the button that was pressed 
            curButtonPressed = bIndex;
            curScreenPtr->executeButtonCallBack(bIndex);
            break;
         }
      }
   }

   // See if the Dout PWM duty-cycle needs updating (i.e. user changed the % in the Dout setup menu)
   if((!clockAlarmTripped && !alarmTripped) && 
      (!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(3), "PWM") || !strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(3), "PWM-Inv"))) {   
         updateDoutPwmDutyCycle();
   } else {
      if(!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(3), "Low")) {   //button-19 is "Dout Action On Alarm" button
         ledcDetachPin(DOUTPIN);  
         digitalWrite(DOUTPIN, 0);
      } else if (!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(3), "High")) {
         ledcDetachPin(DOUTPIN);  
         digitalWrite(DOUTPIN, 1);
      }
   }

   // Update any results that we're monitoring
   updateResults(MONITOR_MENU, GRAPH, "AD", SETUP_MENU, resF0, resF1, resF2, &dinCount, &ainVoltage, &ainVoltage, &drawAdResults);
   updateResults(MONITOR_MENU, GRAPH, "IV", SETUP_MENU, resF0, resF1, resF2, &current_mA, &loadVoltage, &power_mW, &drawIvResults);
   updateResults(MONITOR_MENU, GRAPH, "TEMP", SETUP_MENU, resF0, resF1, resF2, &curProbeTemp, &curModuleTemp, &curModuleHumidity, &drawTempResults);


   // Check if alarms are being monitored.  If so, see if any alarm conditions exist
   // Alarms are triggered if a monitored input alarm is enabled and it exceeds a user defined value.
   if(!strcmp(curScreenPtr->getScreenType(), "AD") && !strcmp(getScreenPtr(SETUP_MENU)->getButtonLabel(3), "Enabled")) {   //button-3 is Alarm Enable/Disable 
      if(dinCount > atof(getScreenPtr(SETUP_MENU)->getButtonLabel(7))) {
         alarmTripped = true;
      } else if(ainVoltage > atof(getScreenPtr(SETUP_MENU)->getButtonLabel(11))) { //Maximum analog in level
         alarmTripped = true;
      } else {
         alarmTripped = false;
      }
   }
   if(!strcmp(curScreenPtr->getScreenType(), "IV") && !strcmp(getScreenPtr(SETUP_MENU)->getButtonLabel(3), "Enabled")) {   //button-3 is Alarm Enable/Disable 
      if(current_mA > atof(getScreenPtr(SETUP_MENU)->getButtonLabel(7))) {  //Max Current Limit
         alarmTripped = true;
      } else if(loadVoltage > atof(getScreenPtr(SETUP_MENU)->getButtonLabel(11))) {  //Max Voltage Limit
         alarmTripped = true;
      } else {
         alarmTripped = false;
      }
   }
   if(!strcmp(curScreenPtr->getScreenType(), "TEMP") && !strcmp(getScreenPtr(SETUP_MENU)->getButtonLabel(3), "Enabled")) {   //button-3 is Alarm Enable/Disable 
      if(curModuleTemp > atof(getScreenPtr(SETUP_MENU)->getButtonLabel(7)) ||
         curProbeTemp > atof(getScreenPtr(SETUP_MENU)->getButtonLabel(7))) {  //Max Temperature Limit
         alarmTripped = true;
      } else if(curModuleHumidity > atof(getScreenPtr(SETUP_MENU)->getButtonLabel(11))) {  //Max Humidity Limit
         alarmTripped = true;
      } else {
         alarmTripped = false;
      }
   }
   if(!clockAlarmTripped && !strcmp(getScreenPtr(CLOCK_MENU)->getButtonLabel(20), "AlarmOn")) {   //clock menu ->button-20 is alarm enable/disable button
      if(!strcmp(dateString[0], dateString[1])) {
         clockAlarmTripped = true;
      }
   }
   // Turn the general alarm off only if the clock alarm was on and just got turned off
   if(clockAlarmTripped && !strcmp(getScreenPtr(CLOCK_MENU)->getButtonLabel(20), "AlarmOff")) {
      clockAlarmTripped = false;
   }

   // The external 110v power controller can be turned on via alarm triggers.  Check if any triggers exist or
   // have been cleared and turn on/off the 110v power box accordingly.
   if(alarmTripped && !strcmp(getScreenPtr(MENU_110V)->getButtonLabel(7), "Turn On")) {   //button-7 is "Action On Alarm" button
      digitalWrite(EXT_POWER_RELAY, HIGH);
   }
   if(alarmTripped && !strcmp(getScreenPtr(MENU_110V)->getButtonLabel(7), "Turn Off")) {   //button-7 is "Action On Alarm" button
      digitalWrite(EXT_POWER_RELAY, LOW);
   }
   if(clockAlarmTripped && !strcmp(getScreenPtr(MENU_110V)->getButtonLabel(11), "Turn On")) {   //button-11 is "Action On Clk-Alarm" button
      digitalWrite(EXT_POWER_RELAY, HIGH);
   }
   if(clockAlarmTripped && !strcmp(getScreenPtr(MENU_110V)->getButtonLabel(11), "Turn Off")) {   //button-11 is "Action On Clk-Alarm" button
      digitalWrite(EXT_POWER_RELAY, LOW);
   }

   // Reset the 110v box to the manual-control value once the alarm condition has passed
   if(!clockAlarmTripped && !alarmTripped && !strcmp(getScreenPtr(MENU_110V)->getButtonLabel(15), "Off")) {   //button-15 is manual On/Off button
      digitalWrite(EXT_POWER_RELAY, LOW);
   }
   if(!clockAlarmTripped && !alarmTripped && !strcmp(getScreenPtr(MENU_110V)->getButtonLabel(15), "On")) {   //button-15 is manual On/Off button
      digitalWrite(EXT_POWER_RELAY, HIGH);
   }


   //  Dout can be triggered/changed by an alarm if enabled
   if((clockAlarmTripped || alarmTripped) && (strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(19), "None") != 0)) {
      if(!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(19), "Low")) {   //button-19 is "Dout Action On Alarm" button
         ledcDetachPin(DOUTPIN);  
         digitalWrite(DOUTPIN, 0);
      } else if (!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(19), "High")) {
         ledcDetachPin(DOUTPIN);  
         digitalWrite(DOUTPIN, 1);
      } else if (!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(19), "PWM")) {
         ledcAttachPin(DOUTPIN, pwmChannel);  
         ledcSetup(pwmChannel, pwmFrequency, pwmResolution); 
         doutPwmDutyCycle = atoi(getScreenPtr(DOUT_MENU)->getButtonLabel(11));
         ledcWrite(pwmChannel, constrain(map(doutPwmDutyCycle,0,100,0,1023),0,1023));  // 10-bit pwm gives 0-1023 range for pwm
      } else if (!strcmp(getScreenPtr(DOUT_MENU)->getButtonLabel(19), "PWM-Inv")) {
         ledcAttachPin(DOUTPIN, pwmChannel);  
         ledcSetup(pwmChannel, pwmFrequency, pwmResolution); 
         doutPwmDutyCycle = atoi(getScreenPtr(DOUT_MENU)->getButtonLabel(11));
         ledcWrite(pwmChannel, constrain(map(doutPwmDutyCycle,100,0,0,1023),0,1023));  // 10-bit pwm gives 0-1023 range for pwm
      }
   }
}

