
#include <callbacks.h>

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

