
// Class library to create and manage my TFT touch screens, buttons, text
// dlf  3/29/2023

#ifndef MyTouchScreen_h
#define MyTouchScreen_h

#include "Arduino.h"
#include "MyDisplay.h"
#include "MyFreeFonts.h"

// For sd card
#include <FS.h>
#include <SPI.h>
#include <SD.h>

// Uses the Bodmer TFT_eSPI graphics display library
#include <TFT_eSPI.h>

class MyTouchScreen  {

   public:
      //#######################################################################
      // Constructor 
      // Need to supply the screen handle, the sprite handles and the screen 
      // title and the max number of buttons a screen can define
      //#######################################################################
      MyTouchScreen(TFT_eSPI *,TFT_eSprite *,TFT_eSprite *, TFT_eSprite *, TFT_eSprite *, TFT_eSprite *,  const char *, boolean);
  
      // This is a pointer to a function that takes 1 arg and returns void
      typedef void (*callBackPtr)(uint8_t);

      //#######################################
      // Methods
      //#######################################
      // Initialize the screen with buttons (default to non-visible), blank labels, nop callbacks
      // Adds an entry to the screen vector arrays
      void init(MyTouchScreen *); 

      // Get the pointer to a given screen
      MyTouchScreen * getScreenPtr(const char *);

      const char * getScreenTitle();

      void setScreenType(const char *);  // Used with screens that are shared (like setup, results, graph, etc.)
      const char * getScreenType();

      // Draw the field text sprite area (the non-button fields where we display 
      // changing text like sensor measurements)
      void drawTextSprite(); 

      // Draw the clock sprite area to display the current date/time 
      void drawClockSprite(); 

      // Draw the button text overlay sprite to add labels to the buttons
      void drawButtonTextSprite(); 

      // Fill the screen with buttons that are enabled, associated button labls and any text lines defined
      void drawScreen();

      // Draw the text fields onto the screen.
      void drawScreenText();

      // There is a screen dedicated to plotting sensor data.  This sets up the graphing screen and adds the axis.
      void drawGraph(boolean, const char *, int, float *, float *);

      // This adds data points to the graph.  One X Y data point per call of this function..
      // The X Y data are in the graph units (degree, mA, seconds, etc.).  This function translates to pixel coords.
      void addGraphData(int, float *, float *);

      // Graph variable control
      //             min, max, intervals, label
      void setXAxis(float, float, float, const char *);
      void setYAxis(float, float, float, const char *);

      // Leave the axis in place but clear the data points
      void clearGraphData();
      
      // Set the button visible and clickable
      void enableButton(uint8_t, const char *, callBackPtr);    

      // Update the button label
      void updateButtonLabel(uint8_t, const char *);    

      // The button label is where we store option values.  Read it so we can use the value in the main code.
      const char * getButtonLabel(uint8_t);    

      // Set the individual button invisible
      void disableButton(uint8_t);   

      // Button state reflecting if the button was pressed or not
      void setButtonPressed(uint8_t, boolean);   

      // Check if button was just Pressed
      boolean wasButtonJustPressed(uint8_t);   

      // Check if button was just Released
      boolean wasButtonJustReleased(uint8_t);   

      // Check if button is visible
      boolean isButtonVisible(uint8_t);   

      // Draw the button on the screen
      void drawButton(uint8_t, boolean);   

      // Draw the button on the screen
      void executeButtonCallBack(uint8_t);   

      // Check if the current touch coords land over a button
      boolean isPressCoordOverButton(uint8_t, uint16_t, uint16_t);   

      // Set the text field row, the text string and the placement X/Ycoords
      void enableTextField(uint8_t, const char *, int, int);    

      // Set the text sprite field row, the text sprite string and the placement X/Ycoords
      void enableTextSprite(uint8_t, const char *, int, int);    

      //  Update the sprite text 
      void updateTextSprite(uint8_t, const char *);    

      // Set the clock sprite field row, the text sprite string and the placement X/Ycoords
      void enableClockSprite(uint8_t, const char *, int, int);    

      //  Update the clock sprite text 
      void updateClockSprite(uint8_t, const char *);    


   private:
      TFT_eSPI *_tftPtr;
      TFT_eSprite *_clockSpritePtr;
      TFT_eSprite *_btnTextSpritePtr;
      TFT_eSprite *_textSpritePtr;
      TFT_eSprite *_statusSpritePtr;
      TFT_eSprite *_yAxisSpritePtr;

      // The screen title at the top/center of the display
      const char * _title;
      const char * _type;  // Used where we share one screen for different resuls (iv, temp, AD)
      boolean _titleVisible;
      char _nopS[TITLE_LEN];

      // Using Malloc to move storage to Heap to make more room for .bss 
      char * _textSpriteNopS = (char *) malloc(sizeof(char) * TEXT_LEN + 1);

      // An array of pointers to callback functions.  Use this to load callbacks for the buttons
      // When a button is pushed, the associated callback function is executed.
      callBackPtr _buttonCallBackPtr[NUM_BUTTONS];

      // Each screen has an array of four columns and six rows of buttons.  Each button may be set visible or 
      // invisible (not used) so that we can customize each menu as the requirements dictate.
      TFT_eSPI_Button _buttonsPtr[NUM_BUTTONS];        // Array of button objects
      char * _buttonLabels = (char *) malloc(sizeof(char) * (TITLE_LEN + 1) * NUM_BUTTONS);
      boolean * _buttonVisible = (boolean *) malloc((sizeof *_buttonVisible) * NUM_BUTTONS);
                                                       
      char * _textFields = (char *) malloc((sizeof(char) * TEXT_LEN + 1) * TEXT_ROWS);
      int _textCoords[TEXT_ROWS][2];            // Array of text X,Y placement coords
      boolean _textVisible[TEXT_ROWS];          // Array of text visibility to display/hide a given field
   
      // Sprites are small text areas where dynamic results are displayed (sensor results, etc.)
      // When a value changes, just the small screen area under the sprite is updated (not the entire display)
      // so we avoid any text flickering issues.
      char * _textSpriteFields = (char *) malloc((sizeof(char) * TEXT_LEN + 1) * TEXT_ROWS);
      int _textSpriteCoords[TEXT_ROWS][2];          // Array of sprite text X,Y placement coords
      boolean _textSpriteVisible[TEXT_ROWS];        // Array of sprite text visibility to display/hide a given field
                                                      
      // Special clock sprite for the wide date/time field
      char * _clockSpriteFields = (char *) malloc((sizeof(char) * TEXT_LEN + 1) * CLOCK_ROWS);
      int _clockSpriteCoords[CLOCK_ROWS][2];                    // Sprite text X,Y placement coords
                                                      
      // Graphing variables
      float _xAxisMin;
      float _xAxisMax;
      float _xAxisIntervals;
      float _yAxisMin;
      float _yAxisMax;
      float _yAxisIntervals;
      const char * _xAxisLabel;
      const char * _yAxisLabel;
                                                
};
#endif
