
#ifndef graphing_h
#define graphing_h

#include <main.h>
#include <MyTouchScreen.h>

MyTouchScreen * getScreenPtr(const char * );
void drawAdGraph(uint8_t);
void drawIvGraph(uint8_t);
void drawTempGraph(uint8_t);

extern MyTouchScreen * prevScreenPtr;
extern MyTouchScreen * curScreenPtr;
extern char currentlyGraphing[];
extern char curResType[];
extern char resF0[];
extern char resF1[];
extern char resF2[];
extern int resArrIdx;
extern boolean resultArraysFilled;
extern float  monitoredResultsYAxis0[];
extern float  monitoredResultsYAxis1[];
extern float  monitoredResultsYAxis2[];
extern float  monitoredResultsXAxis0[];
extern float  monitoredResultsXAxis1[];
extern float  monitoredResultsXAxis2[];

#endif