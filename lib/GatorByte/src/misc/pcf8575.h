#ifndef GB_PCF8575_h
#define GB_PCF8575_h

/* Include the device library */
#include "../lib/pcf8575/PCF8575.h"

#ifndef Wire_h
    #include "Wire.h"
#endif

class GB_PCF8575 {
    public:
        struct PINS {
            int enable;
        } pins;
        
        GB_PCF8575();
        
        DEVICE device = {
            "booster",
            "5V booster"
        };

        String MODULE_NAME = "PCF8575";
        String MODULE_DESC = "I/O expander";
        String MODULE_BRAND = "Generic";

        GB_PCF8575& mode(int pin, int mode);
        GB_PCF8575& write(int pin, int state);
        int read(int pin);
        GB_PCF8575& configure(PINS);
        GB_PCF8575& on();
        GB_PCF8575& off();

    private:
        int _ADDRESS = 0x20;
        PCF8575 _pcf = PCF8575(_ADDRESS);
};

// Constructor
GB_PCF8575::GB_PCF8575() {
    // Wire.begin();
    _pcf.begin();
}

GB_PCF8575& GB_PCF8575::configure(PINS pins) { 
    this->on();

    this->pins = pins;
    pinMode(this->pins.enable, OUTPUT);
    
    this->off();
    return *this;
}

GB_PCF8575& GB_PCF8575::mode(int pin, int mode) { 
    this->on();

    _pcf.pinMode(pin, mode);

    this->off();
    return *this;
}

GB_PCF8575& GB_PCF8575::write(int pin, int state) { 
    this->on();

    _pcf.digitalWrite(pin, state);

    this->off();
    return *this;
}

int GB_PCF8575::read(int pin) { 
    this->on();

    Serial.println("Reading PCF Pin: " + String(pin));
    int reading = _pcf.digitalRead(pin);
    
    this->off();
    return reading;
}

GB_PCF8575& GB_PCF8575::on() { 
    digitalWrite(this->pins.enable, HIGH);
    return *this;
}

GB_PCF8575& GB_PCF8575::off() { 
    digitalWrite(this->pins.enable, LOW);
    return *this;
}

#endif