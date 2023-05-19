
#ifndef callbacks_h
#define callbacks_h

#include <arduino.h>
#include <MyDisplay.h>
#include <MyFreeFonts.h>
#include <MyTouchScreen.h>
#include <main.h>
#include "RTClib.h"

void nop(uint8_t);
void setClockTime(uint8_t);
void setAlarmTime(uint8_t);
void toggleIvAlarm(uint8_t);
void toggleTempAlarm(uint8_t);
void toggleClockAlarm(uint8_t);
void toggleAdAlarm(uint8_t);
void clearCount(uint8_t);
void cycleAdAinMax(uint8_t);
void updateDoutPwmDutyCycle();
void cycleDoutOutput(uint8_t);
void cycleDoutPwmFrequency(uint8_t);
void cycleDoutPwmFollows(uint8_t);
void cycleDoutActionOnAlarm(uint8_t);
void cycle110vActionOnAlarm(uint8_t);
void cycle110vActionOnClock(uint8_t);
void manual110vAction(uint8_t);

extern TFT_eSprite statusSprite;

extern char dateStringFormat[DATE_LEN];
extern char dateString[][DATE_LEN];
extern DateTime now;
extern RTC_DS1307 RTC;

extern char adAlarmArmedS[];
extern boolean clockAlarmTripped;
extern boolean alarmTripped;

extern MyTouchScreen * curScreenPtr;
extern uint8_t curButtonPressed;
extern uint8_t prevButtonNumber;

extern float dinCount;
extern char  dinCountS[];

extern float ainVoltage;
extern float current_mA;
extern float maxAinVoltage;
extern char  maxAinVoltageS[];

extern uint8_t pwmChannel;
extern uint8_t pwmResolution;
extern int   pwmFrequency;
extern char  doutPwmFrequencyS[];
extern int   doutPwmDutyCycle;
extern char  doutPwmDutyCycleS[];
extern char  doutOutputS[];
extern char  doutPwmFollowsS[];
extern char  doutActionOnAlarmS[];

extern char  action110vOnAlarmS[];
extern char  action110vOnClockS[];
extern char  manual110vActionS[];

extern float curModuleTemp;
extern float curModuleHumidity;

extern char  ivAlarmArmedS[];
extern char  tempAlarmArmedS[];
extern char  clockAlarmArmedS[];

#endif