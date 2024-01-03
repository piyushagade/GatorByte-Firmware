/*
  HC595.hpp - Library for simplified control of 74HC595 shift registers.
  Developed and maintained by Timo Denk and contributers, since Nov 2014.
  Additional information is available at https://timodenk.com/blog/shift-register-arduino-library/
  Released into the public domain.
*/

// HC595 constructor
// _size is the number of shiftregisters stacked in serial
HC595::HC595()
{ 

}

void HC595::begin(uint8_t size, const uint8_t serialDataPin, const uint8_t clockPin, const uint8_t latchPin)
{
    // set attributes
    this->_size = size;
    this->_clockPin = clockPin;
    this->_serialDataPin = serialDataPin;
    this->_latchPin = latchPin;
    this->_digitalValues[size];
    
    // define pins as outputs
    pinMode(clockPin, OUTPUT);
    pinMode(serialDataPin, OUTPUT);
    pinMode(latchPin, OUTPUT);

    // set pins low
    digitalWrite(clockPin, LOW);
    digitalWrite(serialDataPin, LOW);
    digitalWrite(latchPin, LOW);

    // allocates the specified number of bytes and initializes them to zero
    memset(this->_digitalValues, 0, this->_size * sizeof(uint8_t));

    updateRegisters();       // reset shift register
}

// Set all pins of the shift registers at once.
// digitalVAlues is a uint8_t array where the length is equal to the number of shift registers.
void HC595::setAll(const uint8_t * digitalValues)
{
    memcpy( _digitalValues, digitalValues, _size);   // dest, src, size
    updateRegisters();
}

// Experimental
// The same as setAll, but the data is located in PROGMEM
// For example with:
//     const uint8_t myFlashData[] PROGMEM = { 0x0F, 0x81 };
#ifdef __AVR__
void HC595::setAll_P(const uint8_t * digitalValuesProgmem)
{
    PGM_VOID_P p = reinterpret_cast<PGM_VOID_P>(digitalValuesProgmem);
    memcpy_P( _digitalValues, p, _size);
    updateRegisters();
}
#endif

// Retrieve all states of the shift registers' output pins.
// The returned array's length is equal to the number of shift registers.
uint8_t * HC595::getAll()
{
    return _digitalValues; 
}

// Set a specific pin to either HIGH (1) or LOW (0).
// The pin parameter is a positive, zero-based integer, indicating which pin to set.
void HC595::set(const uint8_t pin, const uint8_t value)
{
    setNoUpdate(pin, value);
    updateRegisters();
}

// Updates the shift register pins to the stored output values.
// This is the function that actually writes data into the shift registers of the 74HC595.
void HC595::updateRegisters()
{
    for (int i = _size - 1; i >= 0; i--) {
        shiftOut(_serialDataPin, _clockPin, MSBFIRST, _digitalValues[i]);
    }
    
    digitalWrite(_latchPin, HIGH); 
    digitalWrite(_latchPin, LOW); 
}

// Equivalent to set(int pin, uint8_t value), except the physical shift register is not updated.
// Should be used in combination with updateRegisters().
void HC595::setNoUpdate(const uint8_t pin, const uint8_t value)
{
    (value) ? bitSet(_digitalValues[pin / 8], pin % 8) : bitClear(_digitalValues[pin / 8], pin % 8);
}

// Returns the state of the given pin.
// Either HIGH (1) or LOW (0)
uint8_t HC595::get(const uint8_t pin)
{
    return (_digitalValues[pin / 8] >> (pin % 8)) & 1;
}

// Sets all pins of all shift registers to HIGH (1).
void HC595::setAllHigh()
{
    for (int i = 0; i < _size; i++) {
        _digitalValues[i] = 255;
    }
    updateRegisters();
}

// Sets all pins of all shift registers to LOW (0).
void HC595::setAllLow()
{
    for (int i = 0; i < _size; i++) {
        _digitalValues[i] = 0;
    }
    updateRegisters();
}
