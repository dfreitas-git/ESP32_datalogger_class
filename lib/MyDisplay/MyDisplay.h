

// This file contains the display coordinates, fonts, etc. for the screen and any sprites we're using.

// Overall screen size
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

#define BUTTON_ROWS  6     // Max number of button rows
#define BUTTON_COLUMNS  4  // Max number of button columns
#define NUM_BUTTONS (BUTTON_ROWS * BUTTON_COLUMNS)

#define TEXT_LEN 27            // Maximum length of any text field
#define TEXT_PLUS_DATE_LEN 55  // Field used to concatenate the results file name with the date 
#define TEXT_ROWS 5            // Number of rows of text we allow on any screen

#define FLOAT_STRING_WIDTH 7  // The width of the string fields used to print data results
#define INT_STRING_WIDTH 5    // The width of the string fields used for integer results

#define TITLE_LEN 10       // This is the max length (9 plus \0) that is hardwired in TFT_eSPI's Button.cpp
#define TFT_GREY 0x5AEB  // New color

// Status line for messages
#define STATUS_WIDTH 250
#define STATUS_HEIGHT 30
#define STATUS_X (SCREEN_WIDTH - STATUS_WIDTH)/2  // URX of the status sprite
#define STATUS_Y SCREEN_HEIGHT/3                  // URY of the status sprite
#define STATUS_COLOR TFT_BLACK
#define STATUS_BACKGROUND TFT_WHITE
#define STATUS_DATUM MC_DATUM
#define STATUS_TEXT_FONT &FreeSans9pt7b

// Clock text sprite
#define CLOCK_ROWS 2
#define CLOCK_WIDTH  240
#define CLOCK_HEIGHT 25
#define CLOCK_SP_X   225
#define CLOCK_SP_Y   50
#define ALARM_SP_Y   95

// Keypad start position, key sizes and spacing
#define KEY_X 60    // Center of key
#define KEY_Y 60
#define KEY_W 100   // Width and height
#define KEY_H 30

#define KEY_SPACING_X 20             // X and Y gap
#define KEY_SPACING_Y 15

#define KEY_TEXTSIZE 1        // Font size multiplier for regular keys

// Menu and text lines
#define TITLE_X 240
#define TITLE_Y 5
#define TITLE_FONT &FreeSansBold12pt7b
#define TITLE_COLOR TFT_WHITE
#define TITLE_DATUM TC_DATUM

#define TEXT_DATUM TL_DATUM
#define TEXT_FONT &FreeSansBold12pt7b

#define BUTTON_TEXT_DATUM MC_DATUM

#define KEYPAD_RESULT_X 210

// Button fonts
#define LABEL0_FONT  &FreeSans9pt7b         // Smaller, non-bold
#define LABEL0B_FONT &FreeSansBold9pt7b    // Smaller, Bold
#define LABEL1_FONT &FreeSansOblique12pt7b // Medium, Italic
#define LABEL2_FONT &FreeSansBold12pt7b    // Medium, Bold
#define LABEL4_FONT &FreeSans18pt7b        // Bigger

// The text coords relative to screen
#define TEXT_TOP  45          // Where to start the Top of the text field
#define TEXT_BOTTOM 250       // Where to end the Bottom of the text field
#define TEXT_CENTER_X SCREEN_WIDTH/2   // For horizontally centered text field
#define TEXT_LEFT 20          // Where to start the Left side Text fields
#define TEXT_RIGHT 400        // Where to start the Right side Text fields

// Screen's text coords are relative to the screen rectangle
#define TEXT_LINE0 50        // Y coord of the Text line
#define TEXT_LINE1 95   
#define TEXT_LINE2 140
#define TEXT_LINE3 185
#define TEXT_LINE4 230

// The button text sprites
#define BUTTON_TEXT_SP_WIDTH  94  // Width of individual button text sprite
#define BUTTON_TEXT_SP_HEIGHT 20  // Height of individual button text sprite

// The text sprite dimensions.
#define TEXT_SP_WIDTH  86     // Width of individual button text sprite
#define TEXT_SP_HEIGHT 25     // Height of individual button text sprite
#define TEXT_SP_LEFT 400      // Where to place the Left side of all Text sprites 
                           
// Sprite's text coords relative to the sprite rectangle
#define TEXT_SP_LINE0 50        // Y coord of the Text line
#define TEXT_SP_LINE1 95   
#define TEXT_SP_LINE2 140
#define TEXT_SP_LINE3 185
#define TEXT_SP_LINE4 230

// Graphing coords
#define GRAPH_X_ORIGIN  70
#define GRAPH_Y_ORIGIN  SCREEN_HEIGHT - 100

#define GRAPH_X_RIGHT   SCREEN_WIDTH - 30
#define GRAPH_X_LABELX  SCREEN_WIDTH/2
#define GRAPH_X_LABELY  SCREEN_HEIGHT - 75
#define GRAPH_X_INCY    SCREEN_HEIGHT - 100

#define GRAPH_Y_TOP     35
#define GRAPH_Y_INCX    25

// Sprite used to rotate text for the yAxisLabel
#define GRAPH_SP_X_PIVOT   20
#define GRAPH_SP_Y_PIVOT   SCREEN_HEIGHT/2 + 30
#define GRAPH_LABEL_SP_W   SCREEN_HEIGHT/2  
#define GRAPH_LABEL_SP_H   25  

