#ifndef GB_MB7374_h
#define GB_MB7374_h

#ifndef GB_h
    #include "../../GB.h"
#endif

class GB_MB7374 {
    public:
        GB_MB7374(GB&);

        DEVICE device = {
            "mb7374",
            "MB7374 water level sensor"
        };
        
        struct PINS {
            int enable;
            int data;
            bool mux;
        } pins;

        GB_MB7374& configure(PINS);
        GB_MB7374& on();
        GB_MB7374& off();
        float read();

    private:
        GB *_gb;

        int _rain_pulse_state;
        unsigned long _debounce_delay = 1500;
        bool _last_rain_pulse_state = LOW;
        bool _last_debounce_at = 0;
        int _rain_pulse_count = 0;
};

GB_MB7374::GB_MB7374(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
}

GB_MB7374& GB_MB7374::configure(PINS pins) { 
    this->pins = pins;

    // Set pin modes
    pinMode(this->pins.data, INPUT);
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);
    return *this;
}

GB_MB7374& GB_MB7374::on() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH); 
    return *this;
}

GB_MB7374& GB_MB7374::off() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, HIGH);
    return *this;
}

float GB_MB7374::read() { 
    this->on();
    delay(100);
    float mm = pulseIn(this->pins.data, HIGH);
    float inch = mm * 0.0393701;
    this->off();
}

#endif