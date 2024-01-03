/*
  HC595.h - Library for simplified control of 74HC595 shift registers.
  Developed and maintained by Timo Denk and contributers, since Nov 2014.
  Additional information is available at https://timodenk.com/blog/shift-register-arduino-library/
  Released into the public domain.
*/

#pragma once

#include <Arduino.h>

class HC595 
{
public:
    HC595();
    
    void begin(uint8_t size, uint8_t serialDataPin, uint8_t clockPin, uint8_t latchPin);
    void setAll(const uint8_t * digitalValues);
#ifdef __AVR__
    void setAll_P(const uint8_t * digitalValuesProgmem); // Experimental, PROGMEM data
#endif
    uint8_t * getAll(); 
    void set(const uint8_t pin, const uint8_t value);
    void setNoUpdate(const uint8_t pin, uint8_t value);
    void updateRegisters();
    void setAllLow();
    void setAllHigh(); 
    uint8_t get(const uint8_t pin);

private:
    uint8_t _size = 1;
    uint8_t _clockPin;
    uint8_t _serialDataPin;
    uint8_t _latchPin;

    uint8_t _digitalValues[1];
};

#include "HC595.hpp"
