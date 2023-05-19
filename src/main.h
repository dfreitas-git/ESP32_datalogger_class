
#ifndef main_h
#define main_h

#include <Arduino.h>
#include <MyTouchScreen.h>

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

#define SET_CLOCK_FROM_COMPILE false   // Set this to true to have the RTC reset in the setup() loop

// Data wire for DS18S20 temp probe using GPIO pin 16
#define ONE_WIRE_BUS_TEMP_PROBE 16

// For Analog-in/Digital-in/Digital-out
#define AINPIN 34
#define DINPIN 2
#define DOUTPIN 27

// For external 110v relay control
#define EXT_POWER_RELAY 4

// For temp/humidity module
#define DHTPIN 17

#define DATE_LEN 25 // date string length

// Number of screens we will use for displays
const uint8_t MAX_SCREEN_NUM = 9;

//###################################
// Prototypes
//###################################

MyTouchScreen * getScreenPtr(const char * );

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
void  drawTempResults();
void toggleTempAlarm(uint8_t);

void drawAdSetupMenu(uint8_t);
void toggleAdAlarm(uint8_t);
void drawAdMenu(uint8_t);
void  drawAdResults();
void drawAdGraph(uint8_t);
void drawAdAxisMenu(uint8_t);

void nop(uint8_t);
void clearCount(uint8_t);
void touchCalibrate();
void monitorResults(uint8_t);
void writeResultsToFile(boolean, const char *, int, float *, float *);

#endif