#ifndef GB_74HC595_h
#define GB_74HC595_h

// Define pins
#define SR0 0
#define SR1 1
#define SR2 2
#define SR3 3
#define SR4 4
#define SR5 5
#define SR6 6
#define SR7 7
#define SR8 8
#define SR9 9
#define SR10 10
#define SR11 11
#define SR12 12
#define SR13 13
#define SR14 14
#define SR15 15

/* Include the device library */
#include "HC595.h"

/* Import the core GB library */
#ifndef GB_h
    #include "../GB.h"
#endif

class GB_74HC595 : public GB_DEVICE {
    public:
        struct PINS {
            int serialdata;
            int clock;
            int latch;
        } pins;
        
        GB_74HC595(GB &gb);

        DEVICE device = {
            "ioe",
            "74HC595 I/O expander"
        };

        void writepin(int pin, int state);
        int readpin(int pin);
        GB_74HC595& initialize();
        GB_74HC595& initialize(int state);
        GB_74HC595& configure(PINS pins);
        GB_74HC595& configure(PINS pins, int size);

    private:
        GB *_gb;
        const int _number_of_shift_registers = 2;
        HC595 _sr;
};

// Constructor
GB_74HC595::GB_74HC595(GB &gb) { 
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.ioe = this;
}

GB_74HC595& GB_74HC595::configure(PINS pins) { this->configure(pins, 1); }
GB_74HC595& GB_74HC595::configure(PINS pins, int size) { 

    this->pins = pins;
    this->_sr.begin(size, this->pins.serialdata, this->pins.clock, this->pins.latch);

    return *this;
}

GB_74HC595& GB_74HC595::initialize() { this->initialize(LOW); }
GB_74HC595& GB_74HC595::initialize(int state) { 
    _gb->init();
    
    _gb->log("Initializing " + this->device.name, false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    // // Set all pins to low
    // state == LOW ? this->_sr.setAllLow() : this->_sr.setAllHigh();
    // _gb->log(" -> All peripherals set to " + String(state == LOW ? "low" : "high"));
    
    _gb->log(" -> Done");
    
    return *this;
}

void GB_74HC595::writepin(int pin, int state) { 
    this->_sr.set(pin, state);
    delay(100); 
}

int GB_74HC595::readpin(int pin) { 
    int reading = this->_sr.get(pin);
    return reading;
}

#endif