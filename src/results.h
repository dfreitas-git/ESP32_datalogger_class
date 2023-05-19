
#ifndef results_h
#define results_h

#include <arduino.h>
#include <MyDisplay.h>
#include <MyFreeFonts.h>
#include <MyTouchScreen.h>
#include <main.h>
#include "RTClib.h"

void updateResults(const char *, const char *, const char *, const char *, const char *,   const char *,  const char *,
                   float *, float *, float *, void (*)());

void drawAdResults();
void drawIvResults();
void drawTempResults();
void monitorResults(uint8_t);
void writeResultsToFile(boolean, const char *, int, float *, float *);

extern DateTime now;
extern RTC_DS1307 RTC;
extern char dateStringFormat[DATE_LEN];
extern char dateString[][DATE_LEN];

extern char resF0[];
extern char resF1[];
extern char resF2[];
extern char resMenu[];

extern File myFile; 

extern const int MAX_RESULT_POINTS;
extern MyTouchScreen * curScreenPtr;
extern MyTouchScreen * prevScreenPtr;

extern boolean monitoringResults;
extern unsigned long lastResultsLoggedTime;
extern unsigned long monitoringStartTime;
extern float curMonitorTime;
extern float timeMonitored;
extern char timeMonitoredS[];

extern float monitoredResultsYAxis0[];
extern float monitoredResultsYAxis1[];
extern float monitoredResultsYAxis2[];
extern float monitoredResultsXAxis0[];  
extern float monitoredResultsXAxis1[];  
extern float monitoredResultsXAxis2[];  
extern char curResType[];
extern char currentlyGraphing[];
extern boolean resultArraysFilled;
extern int resArrIdx;
extern char  keypadStackArr[];
extern uint8_t keypadStackIdx;
extern char curStartResumeState[];

extern boolean dinLevel;
extern float dinCount;
extern float ainVoltage;
extern int   maxDinCount;
extern float maxAinVoltage;
extern float allAdAxisMin;

extern char  dinLevelS[];
extern char  dinCountS[];
extern char  ainVoltageS[];
extern char  adAlarmArmedS[];
extern char  maxDinCountS[];
extern char  maxDinCountLimitS[];
extern char  maxAinVoltageS[];
extern char  maxAinVoltageLimitS[];
extern char  allAdAxisMinS[];
extern char  monitorAdDurationS[];
extern char  monitorAdIntervalS[];

extern float current_mA;
extern float loadVoltage;
extern float power_mW;
extern char current_mAS[];
extern char loadVoltageS[];
extern char power_mWS[];

extern float curProbeTemp;
extern float curModuleTemp;
extern float curModuleHumidity;
extern char curModuleTempS[];
extern char curProbeTempS[];
extern char curModuleHumidityS[];

#endif