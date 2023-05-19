
#ifndef keypad_h
#define keypad_h

#include <arduino.h>
#include <MyDisplay.h>
#include <MyFreeFonts.h>
#include <MyTouchScreen.h>
#include <main.h>

void updateKeypad(uint8_t);

extern MyTouchScreen * curScreenPtr;
extern MyTouchScreen * prevScreenPtr;
extern uint8_t prevButtonNumber;
extern char  keypadStackArr[];
extern uint8_t keypadStackIdx;
extern TFT_eSPI tft;

#endif