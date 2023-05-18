

// Class to manage a TFT touch screen and the menus/buttons/text that is displayed.
// This uses the Bodmer graphics lib (TFT_eSPI.h) for writing to the screen.
//
// dlf 3/29/2023

#include "MyTouchScreen.h"

//####################################################################
// Constructor.  Pass all the screen pointers to the screen object.
//####################################################################
MyTouchScreen::MyTouchScreen(TFT_eSPI *tftPtr,TFT_eSprite *btnTextSpritePtr, TFT_eSprite *textSpritePtr, TFT_eSprite *statusSpritePtr, 
                             TFT_eSprite *yAxisSpritePtr, TFT_eSprite *clockSpritePtr, const char *title, boolean titleVisible){
   _tftPtr = tftPtr;
   _clockSpritePtr = clockSpritePtr;
   _btnTextSpritePtr = btnTextSpritePtr;
   _textSpritePtr = textSpritePtr;
   _statusSpritePtr = statusSpritePtr;
   _yAxisSpritePtr = yAxisSpritePtr;
   _title = title;
   _type = "";
   _titleVisible = titleVisible;
}


//#######################################
// Methods for the class
//#######################################


// Initialize the screen parameters.  Set all buttons to invisible with blank label text (nopS = "").
// Set all text fields to nopS as well to make invisible.  In the main setup code, each screen object
// will be loaded with its required screen settings.
void MyTouchScreen::init(MyTouchScreen * screenPtr){

   // Create the button objects and default to invisible, nop callbacks, and blank text (_nopS)
   strcpy(_nopS,"");
   _tftPtr->setFreeFont(LABEL0B_FONT);
   for(uint8_t row=0; row<BUTTON_ROWS; row++) {
      for(uint8_t col=0; col<BUTTON_COLUMNS;col++) {
         uint8_t bIndex = col + row * BUTTON_COLUMNS;


            // x, y, w, h, outline, fill, button text, textsize
            _buttonsPtr[bIndex].initButton(_tftPtr, KEY_X + col * (KEY_W + KEY_SPACING_X) , KEY_Y + row * ( KEY_H + KEY_SPACING_Y),
                                                    KEY_W, KEY_H, TFT_WHITE, TFT_BLUE, TFT_WHITE,_nopS, KEY_TEXTSIZE);
            strcpy(_buttonLabels+(sizeof(char)*TITLE_LEN+1)*bIndex, _nopS); // clear all button labels
            *(_buttonVisible+bIndex) = false;    // start with the buttons turned off
            _buttonsPtr[bIndex].press(false);  // clear any previous button press state
      }
   }
   strcpy(_textSpriteNopS,"");
   for(uint8_t row=0; row<TEXT_ROWS; row++) {
      _textVisible[row] = false;
      strcpy(_textFields+((sizeof(char)*TEXT_LEN)+1)*row, _nopS); // clear all the text fields
      _textSpriteVisible[row] = false;
      strcpy(_textSpriteFields+((sizeof(char)*TEXT_LEN)+1)*row, _textSpriteNopS); // clear all the text sprint fields
   }
   strcpy(_clockSpriteFields,  _textSpriteNopS);      // clear the clock sprint field
   strcpy(_clockSpriteFields+(sizeof(char)*TEXT_LEN+1),  _textSpriteNopS);      // clear the clock sprint field
}

//#######################################
// Screen drawing methods
//#######################################
// Manage the screen title
const char * MyTouchScreen::getScreenTitle() {
   return(_title);
}
const char * MyTouchScreen::getScreenType() {
   return(_type);
}
void MyTouchScreen::setScreenType(const char * type) {
   _type= type;
}

// Draw the text fields onto the screen
void MyTouchScreen::drawScreenText(){

   _tftPtr->setTextColor(TFT_WHITE, TFT_BLACK);
   _tftPtr->setTextDatum(TEXT_DATUM);
   _tftPtr->setFreeFont(TEXT_FONT);
   for(uint8_t row=0; row<TEXT_ROWS;row++) {
      if(_textVisible[row]) {
         //_tftPtr->drawString(_textFields[row],_textCoords[row][0],_textCoords[row][1],GFXFF);
         _tftPtr->drawString(_textFields+((sizeof(char)*TEXT_LEN)+1)*row,_textCoords[row][0],_textCoords[row][1],GFXFF);
      }
   }
}

// Draw the screen title and for all visible buttons, draw the button and add the text overlay to it
void MyTouchScreen::drawScreen(){

   _tftPtr->fillScreen(TFT_BLACK);
   _tftPtr->setTextColor(TITLE_COLOR, TFT_BLACK);
   _tftPtr->setTextDatum(TITLE_DATUM);
   _tftPtr->setFreeFont(TITLE_FONT);
   if(_titleVisible) {
      _tftPtr->drawString(_title,TITLE_X,TITLE_Y,GFXFF);
   }

   for(uint8_t row=0; row<BUTTON_ROWS; row++) {
      for(uint8_t col=0; col<BUTTON_COLUMNS;col++) {
         uint8_t bIndex = col + row * BUTTON_COLUMNS;
                                            
         if(*(_buttonVisible+bIndex)) {
            // Draw the button
            _buttonsPtr[bIndex].setLabelDatum(0,0,MC_DATUM);
            _buttonsPtr[bIndex].drawButton();
         }
      }
   }

   // Fill in the text overlays on the visible buttons
   MyTouchScreen::drawButtonTextSprite();

   // Now add any text lines outside the buttons
   MyTouchScreen::drawScreenText();
}

// For drawing the clock text fields.  Separate method as the clock field is very wide compared to the usual result fields.
// We still use the _textSpriteFields array to hold the string but use a wider _clockSprite for drawing the sprite
void MyTouchScreen::drawClockSprite() {
   _clockSpritePtr->setFreeFont(TEXT_FONT);
   _clockSpritePtr->setTextColor(TFT_WHITE, TFT_BLACK);
   _clockSpritePtr->setTextDatum(TEXT_DATUM);

   // Current time line
   _clockSpritePtr->fillSprite(TFT_BLACK);
   //_clockSpritePtr->drawString(_clockSpriteFields[0],0,0,GFXFF);
   _clockSpritePtr->drawString(_clockSpriteFields,0,0,GFXFF);
   _clockSpritePtr->pushSprite(_clockSpriteCoords[0][0], _clockSpriteCoords[0][1]);

   // Alarm line
   _clockSpritePtr->fillSprite(TFT_BLACK);
   //_clockSpritePtr->drawString(_clockSpriteFields[1],0,0,GFXFF);
   _clockSpritePtr->drawString(_clockSpriteFields+(sizeof(char)*TEXT_LEN+1),0,0,GFXFF);
   _clockSpritePtr->pushSprite(_clockSpriteCoords[1][0], _clockSpriteCoords[1][1]);
}

// For drawing the text fields where we store dynamically changing text fields like sensor results, etc.
void MyTouchScreen::drawTextSprite() {
   _textSpritePtr->setFreeFont(TEXT_FONT);
   _textSpritePtr->setTextColor(TFT_WHITE, TFT_BLACK);
   _textSpritePtr->setTextDatum(TEXT_DATUM);

   for(uint8_t row=0; row<TEXT_ROWS; row++) {
      if(_textSpriteVisible[row]) {

         // Clear the sprite text
         _textSpritePtr->fillSprite(TFT_BLACK);
         //_textSpritePtr->drawString(_textSpriteFields[row],0,0,GFXFF);
         _textSpritePtr->drawString(_textSpriteFields+((sizeof(char)*TEXT_LEN)+1)*row,0,0,GFXFF);
         _textSpritePtr->pushSprite(_textSpriteCoords[row][0], _textSpriteCoords[row][1]);
      }
   }
}

// For drawing the button text overlay.  We will call only this function for cases where we
// are using the button label to display user-modifiable options (alarm on/off, limits , etc.)
void MyTouchScreen::drawButtonTextSprite() {
   for(uint8_t row=0; row<BUTTON_ROWS; row++) {
      for(uint8_t col=0; col<BUTTON_COLUMNS;col++) {
         uint8_t bIndex = col + row * BUTTON_COLUMNS;

         if(*(_buttonVisible+bIndex)) {

            // Clear the sprite text that we overlay on the buttons
            _btnTextSpritePtr->setFreeFont(LABEL0B_FONT);
            _btnTextSpritePtr->setTextColor(TFT_WHITE, TFT_BLUE);
            _btnTextSpritePtr->setTextDatum(BUTTON_TEXT_DATUM);

            _btnTextSpritePtr->fillSprite(TFT_BLUE);
            //_btnTextSpritePtr->drawString(_buttonLabels[bIndex],(BUTTON_TEXT_SP_WIDTH/2),(BUTTON_TEXT_SP_HEIGHT/2-2),GFXFF);
            _btnTextSpritePtr->drawString(_buttonLabels+(sizeof(char)*TITLE_LEN+1)*bIndex,(BUTTON_TEXT_SP_WIDTH/2),(BUTTON_TEXT_SP_HEIGHT/2-2),GFXFF);
            _btnTextSpritePtr->pushSprite(KEY_X + col * (KEY_W + KEY_SPACING_X)  - BUTTON_TEXT_SP_WIDTH/2 , 
                                    KEY_Y + row * ( KEY_H + KEY_SPACING_Y) - BUTTON_TEXT_SP_HEIGHT/2 - 2);  // Scooch it up 2 pixels for better centering
         }
      }
   }
}


//#########################################
// Graphing methods
//#########################################
// Set up the graph axis
void MyTouchScreen::setXAxis(float min, float max, float numberOfIntervals, const char * label){
   _xAxisMin = min;
   _xAxisMax = max;
   _xAxisIntervals = numberOfIntervals;
   _xAxisLabel = label;
}
void MyTouchScreen::setYAxis(float min, float max, float numberOfIntervals, const char * label){
   _yAxisMin = min;
   _yAxisMax = max;
   _yAxisIntervals = numberOfIntervals;
   _yAxisLabel = label;
}


// ###########################
// Draw the graphing screen
// ###########################
void MyTouchScreen::drawGraph(boolean resultArraysFilled, const char * resultFile,
                              int resultIndex, float * resultsArrXAxisPtr, float * resultsArrYAxisPtr){

   float prevDataX = 0.0;
   float prevDataY = 0.0;
   float dataX = 0.0;
   float dataY = 0.0;
   char buff[FLOAT_STRING_WIDTH]; // For converting itoa for the graph labels
   _tftPtr->fillScreen(TFT_BLACK);
   _tftPtr->setTextColor(TITLE_COLOR, TFT_BLACK);
   _tftPtr->setTextDatum(TITLE_DATUM);
   _tftPtr->setFreeFont(TITLE_FONT);
   if(_titleVisible) {
      _tftPtr->drawString(_title,TITLE_X,TITLE_Y,GFXFF);
   }

   // Draw any buttons used by the graphing screen
   for(uint8_t row=0; row<BUTTON_ROWS; row++) {
      for(uint8_t col=0; col<BUTTON_COLUMNS;col++) {
         uint8_t bIndex = col + row * BUTTON_COLUMNS;
                                            
         if(*(_buttonVisible+bIndex)) {
            // Draw the button
            _buttonsPtr[bIndex].setLabelDatum(0,0,MC_DATUM);
            _buttonsPtr[bIndex].drawButton();
         }
      }
   }
   // Fill in the text overlays on the visible buttons
   MyTouchScreen::drawButtonTextSprite();

   // Draw the graph outer borders
   _tftPtr->drawLine(GRAPH_X_ORIGIN, GRAPH_Y_ORIGIN, GRAPH_X_RIGHT, GRAPH_Y_ORIGIN, TFT_YELLOW);
   _tftPtr->drawLine(GRAPH_X_ORIGIN, GRAPH_Y_ORIGIN, GRAPH_X_ORIGIN, GRAPH_Y_TOP, TFT_YELLOW);
   _tftPtr->drawLine(GRAPH_X_ORIGIN, GRAPH_Y_TOP, GRAPH_X_RIGHT, GRAPH_Y_TOP, TFT_YELLOW);
   _tftPtr->drawLine(GRAPH_X_RIGHT, GRAPH_Y_ORIGIN, GRAPH_X_RIGHT, GRAPH_Y_TOP, TFT_YELLOW);

   // Add the internal grid lines and labels.
   _tftPtr->setTextColor(TFT_WHITE, TFT_BLACK);
   _tftPtr->setFreeFont(LABEL0_FONT);

   // xstep/ystep are the delta-value per grid line (e.g. 100/0 degree delta over 10 grids is 10 degrees per step)
   float xstep = (_xAxisMax - _xAxisMin)/_xAxisIntervals;
   float ystep = (_yAxisMax - _yAxisMin)/_yAxisIntervals;


   // Draw vertical internal grid lines
   int graphWidth = GRAPH_X_RIGHT - GRAPH_X_ORIGIN;
   for(uint8_t i=1; i<_xAxisIntervals; i++) {
      _tftPtr->drawLine(GRAPH_X_ORIGIN + (i * (graphWidth/_xAxisIntervals)), GRAPH_Y_ORIGIN, 
                        GRAPH_X_ORIGIN + (i * (graphWidth/_xAxisIntervals)), GRAPH_Y_TOP, TFT_BLUE);
   }

   // We use a sprite to draw the Y-Axis label as we need to rotate the text to run vertically parallel to the axis
   _tftPtr->setPivot(GRAPH_SP_X_PIVOT, GRAPH_SP_Y_PIVOT);  // The point where the yAxis label sprite will pivot
   _yAxisSpritePtr->setTextDatum(TEXT_DATUM);
   _yAxisSpritePtr->setFreeFont(LABEL0_FONT);
   _yAxisSpritePtr->fillSprite(TFT_BLACK);
   _yAxisSpritePtr->drawString(_yAxisLabel,0,0);
   _yAxisSpritePtr->pushRotated(-90);

   // Draw the vertical increment numbers
   int graphHeight = GRAPH_Y_ORIGIN - GRAPH_Y_TOP;
   for(int i=0; i<=_yAxisIntervals; i++) {
      if(_yAxisMax - _yAxisMin >= 100) {
         itoa(i*ystep+_yAxisMin,buff,10);
      } else {
         dtostrf(i*ystep+_yAxisMin,3,1,buff);
      }
      _tftPtr->drawString(buff,GRAPH_X_ORIGIN-20,GRAPH_Y_ORIGIN-(i * (graphHeight/_yAxisIntervals))-10 ,GFXFF);
   }

   // Draw horizontal internal graph lines
   for(uint8_t i=1; i<_yAxisIntervals; i++) {
      _tftPtr->drawLine(GRAPH_X_ORIGIN,  GRAPH_Y_ORIGIN - (i * (graphHeight/_yAxisIntervals)), 
                        GRAPH_X_RIGHT,  GRAPH_Y_ORIGIN - (i * (graphHeight/_yAxisIntervals)), TFT_BLUE);
   }

   // Draw the horizontal increment numbers
   for(int i=0; i<=_xAxisIntervals; i++) {
      // no decimal if we have long durations so labels have more space
      if(_xAxisMax - _xAxisMin >= 100) {
         itoa(i*xstep+_xAxisMin,buff,10);
      } else {
         dtostrf(i*xstep+_xAxisMin,3,1,buff);
      }
      _tftPtr->drawString(buff,GRAPH_X_ORIGIN + (i * (graphWidth/_xAxisIntervals)),GRAPH_Y_ORIGIN+2 ,GFXFF);
   }
   _tftPtr->drawString(_xAxisLabel,GRAPH_X_LABELX,GRAPH_X_LABELY,GFXFF);

   // Fill in any data already recorded (i.e. we may be re-drawing the graph that is already in progress
   // We fill result arrays first.  Once the arrays fill, we write the array results to "disk" and 
   // start filling the result arrays again.  That means we need to read any results written to "disk" first,
   // then plot the data in the currently filling array buffer to bring the plot up to the latest data point.
   if(resultArraysFilled) {
      File resFH;
      char fieldX[FLOAT_STRING_WIDTH];
      char fieldY[FLOAT_STRING_WIDTH];
      int8_t idx;
      resFH = SD.open(resultFile,FILE_READ);
      if(!resFH){
        Serial.print("Failed to open "); Serial.print(resultFile); Serial.println(" for reading");
        return;
      }
      //Serial.print("Reading from logFile: "); Serial.println(resultFile);
      // data in the file is one pair of floats per line:  dataX,dataYCrLf
      while(resFH.available()){
         boolean foundCR = false;
         boolean foundComma = false;
         uint8_t c;

         // Read in the X data field
         idx=0;
         while(!foundComma) {
            c = resFH.read();
            if(c == 44) {  // comma found, we're at the end of the first float
               fieldX[idx]='\0';  // null pad the string
               break;
            }else{
               fieldX[idx++]=c; 
            }
         }
         idx=0;
         while(!foundCR) {  // Now read in the second float (Y data) (delimited by cr lf)
            c = resFH.read();
            if(c == 13) {  // cr
               c = resFH.read();  //flush out the newline
               fieldY[idx]='\0';  // null pad the string
               break;
            }else{
               fieldY[idx++]=c; 
            }
         }
         // At this point we have read the two data fields (dataX and dataY).  Go plot them
         dataX = atof(fieldX);  
         dataY = atof(fieldY);  
         //Serial.print("data: ");Serial.print(dataX); Serial.print(" "); Serial.println(dataY);
         _tftPtr->drawLine(int(((prevDataX-_xAxisMin)/(_xAxisMax-_xAxisMin))*graphWidth + GRAPH_X_ORIGIN), 
                             int(GRAPH_Y_ORIGIN - ((prevDataY-_yAxisMin)/(_yAxisMax-_yAxisMin))*graphHeight),
                            int(((dataX-_xAxisMin)/(_xAxisMax-_xAxisMin))*graphWidth + GRAPH_X_ORIGIN), 
                             int(GRAPH_Y_ORIGIN - ((dataY-_yAxisMin)/(_yAxisMax-_yAxisMin))*graphHeight), TFT_WHITE);
         prevDataX=dataX;
         prevDataY=dataY;
      }
      resFH.close();
   }

   // Now plot the data in the results Arrays (the data that hasn't yet been written back to the file)
   for(int i=1; i<resultIndex; i++) {
      prevDataX = *(resultsArrXAxisPtr + i - 1);
      prevDataY = *(resultsArrYAxisPtr + i - 1);
      dataX = *(resultsArrXAxisPtr + i);
      dataY = *(resultsArrYAxisPtr + i);

      // Plot only if the data is within the graph limits
      if(dataX >= _xAxisMin && dataX <= _xAxisMax && dataY >= _yAxisMin && dataY <= _yAxisMax) {
         _tftPtr->drawLine(int(((prevDataX-_xAxisMin)/(_xAxisMax-_xAxisMin))*graphWidth + GRAPH_X_ORIGIN), 
                             int(GRAPH_Y_ORIGIN - ((prevDataY-_yAxisMin)/(_yAxisMax-_yAxisMin))*graphHeight),
                            int(((dataX-_xAxisMin)/(_xAxisMax-_xAxisMin))*graphWidth + GRAPH_X_ORIGIN), 
                             int(GRAPH_Y_ORIGIN - ((dataY-_yAxisMin)/(_yAxisMax-_yAxisMin))*graphHeight), TFT_WHITE);
      }
   }
}

// ##################################
// Add array datapoints to the graph
// ##################################
void MyTouchScreen::addGraphData(int resultIndex, float * resultsArrXAxisPtr, float * resultsArrYAxisPtr){
   int graphWidth = GRAPH_X_RIGHT - GRAPH_X_ORIGIN;
   int graphHeight = GRAPH_Y_ORIGIN - GRAPH_Y_TOP;

   float prevDataX = *(resultsArrXAxisPtr + resultIndex - 1);
   float prevDataY = *(resultsArrYAxisPtr + resultIndex - 1);
   float dataX = *(resultsArrXAxisPtr + resultIndex);
   float dataY = *(resultsArrYAxisPtr + resultIndex);

   if(dataX >= _xAxisMin && dataX <= _xAxisMax && dataY >= _yAxisMin && dataY <= _yAxisMax) {
      _tftPtr->drawLine(int(((prevDataX-_xAxisMin)/(_xAxisMax-_xAxisMin))*graphWidth + GRAPH_X_ORIGIN), 
                          int(GRAPH_Y_ORIGIN - ((prevDataY-_yAxisMin)/(_yAxisMax-_yAxisMin))*graphHeight),
                         int(((dataX-_xAxisMin)/(_xAxisMax-_xAxisMin))*graphWidth + GRAPH_X_ORIGIN), 
                          int(GRAPH_Y_ORIGIN - ((dataY-_yAxisMin)/(_yAxisMax-_yAxisMin))*graphHeight), TFT_WHITE);
   }
}

//###############################
// Button methods
// ##############################
// To enable the button to make it visible on the screen 
void MyTouchScreen::enableButton(uint8_t buttonNumber, const char *label, callBackPtr btnCallBackPtr) {
   *(_buttonVisible+buttonNumber) = true;
   strcpy(_buttonLabels+(sizeof(char)*TITLE_LEN+1)*buttonNumber,label);
   _buttonCallBackPtr[buttonNumber] = btnCallBackPtr;
}

// Get the button label to use the value in the calling code
const char *  MyTouchScreen::getButtonLabel(uint8_t buttonNumber) {
   //return(_buttonLabels[buttonNumber]);
   return(_buttonLabels+(sizeof(char)*TITLE_LEN+1)*buttonNumber);
}

// To update the label on a button 
void MyTouchScreen::updateButtonLabel(uint8_t buttonNumber, const char *label) {
   strcpy(_buttonLabels+(sizeof(char)*TITLE_LEN+1)*buttonNumber,label);
}

// To disable the button to make it invisible (and unselectable) on the screen
void MyTouchScreen::disableButton(uint8_t buttonNumber) {
   *(_buttonVisible+buttonNumber) = false;
}


// Check if button is visible
boolean MyTouchScreen::isButtonVisible(uint8_t buttonNumber) {
   if(*(_buttonVisible+buttonNumber)) {
      return(true);
   } else {
      return(false);
   }
}

// Check if touch coord is over a button (i.e. a button is being pressed)
boolean MyTouchScreen::isPressCoordOverButton(uint8_t buttonNumber, uint16_t X, uint16_t Y) {
   if(_buttonsPtr[buttonNumber].contains(X,Y)) {
      return(true);
   } else {
      return(false);
   }
}

// Set the buttons "pressed" attribute true or false
void MyTouchScreen::setButtonPressed(uint8_t buttonNumber, boolean state) {
   _buttonsPtr[buttonNumber].press(state);
}

// Check if button was just Pressed
boolean MyTouchScreen::wasButtonJustPressed(uint8_t buttonNumber) {
   return(_buttonsPtr[buttonNumber].justPressed());
}

// Check if button was just Released
boolean MyTouchScreen::wasButtonJustReleased(uint8_t buttonNumber) {
   return(_buttonsPtr[buttonNumber].justReleased());
}

// Draw the indexed button onto the screen (plain or inverted background)
void MyTouchScreen::drawButton(uint8_t buttonNumber, boolean inverted) {
   _buttonsPtr[buttonNumber].drawButton(inverted);
}

// Execute the button callback code
void MyTouchScreen::executeButtonCallBack(uint8_t buttonNumber) {
   _buttonCallBackPtr[buttonNumber](buttonNumber);
}

//###############################
// text field methods
// ##############################
// To enable a text field to make it visible on the screen
void MyTouchScreen::enableTextField(uint8_t textFieldNumber, const char *label, int X, int Y) {
   _textVisible[textFieldNumber] = true;
   strcpy(_textFields+((sizeof(char)*TEXT_LEN)+1)*textFieldNumber,label);
   _textCoords[textFieldNumber][0] = X;
   _textCoords[textFieldNumber][1] = Y;
}

//###############################
// text sprite  methods
// ##############################
// To enable a text sprite field to make it visible on the screen
void MyTouchScreen::enableTextSprite(uint8_t fieldNumber, const char *label, int X, int Y) {
   _textSpriteVisible[fieldNumber] = true;
   strcpy(_textSpriteFields+((sizeof(char)*TEXT_LEN)+1)*fieldNumber,label);
   _textSpriteCoords[fieldNumber][0] = X;
   _textSpriteCoords[fieldNumber][1] = Y;
}

// To update the text for the given field's sprite
void MyTouchScreen::updateTextSprite(uint8_t fieldNumber, const char *label) {
   strcpy(_textSpriteFields+((sizeof(char)*TEXT_LEN)+1)*fieldNumber,label);
}

// To enable a clock text sprite field to make it visible on the screen
void MyTouchScreen::enableClockSprite(uint8_t fieldNumber, const char *label, int X, int Y) {
   strcpy(_clockSpriteFields+(sizeof(char)*TEXT_LEN+1)*fieldNumber,label);
   _clockSpriteCoords[fieldNumber][0] = X;
   _clockSpriteCoords[fieldNumber][1] = Y;
}

// To update the text for the clock field's sprite
void MyTouchScreen::updateClockSprite(uint8_t fieldNumber, const char *label) {
   strcpy(_clockSpriteFields+(sizeof(char)*TEXT_LEN+1)*fieldNumber,label);
}

