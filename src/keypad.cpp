
#include <keypad.h>

//#################################################################################################
// Keypad handling.  Enter the key that was pushed into a character stack.  Once the "enter" key
// is pressed, store the string in the button label variable.  Later, when we need to use the value,
// we then convert the string to an int or float as required.
//#################################################################################################
void updateKeypad(uint8_t buttonNumber) {
   boolean enter = false;
   boolean cancel = false;

   if(keypadStackIdx < TEXT_LEN) {
      switch(buttonNumber) {

         case 4:  keypadStackArr[keypadStackIdx++] = '7';keypadStackArr[keypadStackIdx] = '\0'; break;
         case 5:  keypadStackArr[keypadStackIdx++] = '8'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 6:  keypadStackArr[keypadStackIdx++] = '9'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 7:  enter = true; break;   
         case 8:  keypadStackArr[keypadStackIdx++] = '4'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 9:  keypadStackArr[keypadStackIdx++] = '5'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 10: keypadStackArr[keypadStackIdx++] = '6'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 12: keypadStackArr[keypadStackIdx++] = '1'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 13: keypadStackArr[keypadStackIdx++] = '2'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 14: keypadStackArr[keypadStackIdx++] = '3'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 16: keypadStackArr[keypadStackIdx++] = '0'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 17: keypadStackArr[keypadStackIdx++] = '.'; keypadStackArr[keypadStackIdx] = '\0';break;
         case 18: if(keypadStackIdx != 0) { keypadStackIdx--; } keypadStackArr[keypadStackIdx] = '\0'; break; // backspace key
         case 19: keypadStackIdx=0;strcpy(keypadStackArr,""); break;                                          // clear key
         case 23: cancel = true; break;   
      }

      // Clear the screen field and update with the latest string chars
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(TEXT_DATUM);         
      tft.setFreeFont(TEXT_FONT);
      tft.fillRect(KEYPAD_RESULT_X, TEXT_LINE0, SCREEN_WIDTH-KEYPAD_RESULT_X,25,TFT_BLACK);
      tft.drawString(keypadStackArr, KEYPAD_RESULT_X, TEXT_LINE0);

      // The user is done entering numbers.  Go back to the screen that called the keyboard
      if(enter || cancel) {
         curScreenPtr = prevScreenPtr;
         if(!cancel) {
            curScreenPtr->updateButtonLabel(prevButtonNumber,keypadStackArr);
         }
         keypadStackIdx=0;
         keypadStackArr[0] = '\0';
         curScreenPtr->drawScreen();
      }
   }
}
