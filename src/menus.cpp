
#include <menus.h>

//########################################################################
//########################################################################
// Draw the various menus 
//########################################################################
//########################################################################

// Return the screen pointer for the given screen name
MyTouchScreen * getScreenPtr(const char * screenName) {
   for(uint8_t i=0; i<9; i++) {
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