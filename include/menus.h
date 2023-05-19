
#ifndef menus_h
#define menus_h

#include <arduino.h>
#include <MyDisplay.h>
#include <MyFreeFonts.h>
#include <MyTouchScreen.h>
#include <main.h>
#include "RTClib.h"

void updateKeypad(uint8_t);
void drawKeypad(uint8_t);
void drawMainMenu(uint8_t);
void saveIvSetup();
void saveIvSetupAndDrawMainMenu(uint8_t);
void saveIvSetupAndDrawIvMenu(uint8_t);
void saveIvSetupAndDrawIvAxisMenu(uint8_t);
void saveTempSetup();
void saveTempSetupAndDrawMainMenu(uint8_t);
void saveTempSetupAndDrawTempMenu(uint8_t);
void saveTempSetupAndDrawTempAxisMenu(uint8_t);
void saveAdSetup();
void saveAdSetupAndDrawMainMenu(uint8_t);
void saveAdSetupAndDrawAdMenu(uint8_t);
void saveAdSetupAndDrawAdAxisMenu(uint8_t);
void drawClockScreen(uint8_t);
void updateClock();
void updateClockAlarm();
void drawDoutMenu(uint8_t);
void draw110vMenu(uint8_t);
void drawIvAxisMenu(uint8_t);
void drawIvSetupMenu(uint8_t);
void drawIvMenu(uint8_t);
void drawTempSetupMenu(uint8_t);
void drawTempMenu(uint8_t);
void drawTempAxisMenu(uint8_t);
void drawAdAxisMenu(uint8_t);
void drawAdSetupMenu(uint8_t);
void drawAdMenu(uint8_t);


extern DateTime now;

extern MyTouchScreen * screenPtrs[];
extern MyTouchScreen * curScreenPtr;
extern MyTouchScreen * prevScreenPtr;
extern MyTouchScreen mainMenuScreen;
extern MyTouchScreen keypadScreen;
extern MyTouchScreen clockScreen;
extern MyTouchScreen graphScreen;
extern MyTouchScreen axisScreen;
extern MyTouchScreen screen110V;
extern MyTouchScreen setupScreen;
extern MyTouchScreen monitorScreen;
extern MyTouchScreen doutScreen;

extern uint8_t prevButtonNumber;

extern char curResType[];
extern char curStartResumeState[];

extern char dateString[][DATE_LEN];
extern char ivAlarmArmedS[];
extern char maxAlarmVS[];
extern char maxAlarmIS[];
extern char monitorIvDurationS[];
extern char monitorIvIntervalS[];

extern float curAxisMax;
extern float voltAxisMax;
extern float powerAxisMax;
extern float allIvAxisMin;

extern char current_mAS[];
extern char loadVoltageS[];
extern char power_mWS[];
extern char timeMonitoredS[];

extern char tempAlarmArmedS[];
extern char maxAlarmTempS[];
extern char maxAlarmHumidS[];
extern char monitorTempDurationS[];
extern char monitorTempIntervalS[];

extern float tempAxisMax;
extern float tempAxisMin;
extern float humidityAxisMax;
extern float humidityAxisMin;

extern char adAlarmArmedS[];
extern char maxDinCountLimitS[];
extern char maxAinVoltageLimitS[];
extern char monitorAdDurationS[];
extern char monitorAdIntervalS[];

extern char curYearS[];
extern char curMonthS[];
extern char curDayS[];
extern char curHourS[];
extern char curMinS[];
extern char curSecS[];

extern char curProbeTempS[];
extern char curModuleTempS[];
extern char curModuleHumidityS[];
extern char timeMonitoredS[];
extern char curAxisMaxS[];
extern char voltAxisMaxS[];
extern char powerAxisMaxS[];
extern char allIvAxisMinS[];
extern char tempAxisMaxS[];
extern char tempAxisMinS[];
extern char humidityAxisMaxS[];
extern char humidityAxisMinS[];

extern int maxDinCount;
extern float maxAinVoltage;
extern float allAdAxisMin;

extern char maxAinVoltageS[];
extern char maxDinCountS[];
extern char allAdAxisMinS[];
extern char dinLevelS[];
extern char dinCountS[];
extern char ainVoltageS[];

#endif