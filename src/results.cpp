
#include <results.h>

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