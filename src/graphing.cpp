//#########################################################################################
//#########################################################################################
// Graphing screens
//#########################################################################################
//#########################################################################################

#include <graphing.h>

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