#ifndef GB_RG11_h
#define GB_RG11_h

#ifndef GB_h
    #include "../../GB.h"
#endif

class GB_RG11 {
    public:
        GB_RG11(GB&);

        DEVICE device = {
            "rg11",
            "RG11 rain sensor"
        };
        
        struct PINS {
            int enable;
            int data;
            bool mux;
        } pins;

        GB_RG11& configure(PINS);
        GB_RG11& on();
        GB_RG11& off();
        
        // Type definitions
        typedef void (*callback_t)();
        bool listener(callback_t callback);

    private:
        bool _USE_MUX = false;
        GB *_gb;
        int _input_pin;
        int _enable_pin;

        int _rain_pulse_state;
        unsigned long _debounce_delay = 1500;
        bool _last_rain_pulse_state = LOW;
        bool _last_debounce_at = 0;
        int _rain_pulse_count = 0;
};

GB_RG11::GB_RG11(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);

    // _gb->devices.rg11 = this;
}

GB_RG11& GB_RG11::configure(PINS pins) {
    this->pins = pins;
    
    // Set pin modes
    pinMode(this->pins.data, INPUT);
    
    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    return *this;
}

GB_RG11& GB_RG11::on() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    return *this;
}

GB_RG11& GB_RG11::off() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    return *this;
}

bool GB_RG11::listener(callback_t callback) {
    int reading = digitalRead(_input_pin);

    // Debounce
    if (reading != _last_rain_pulse_state) _last_debounce_at = millis();
    if ((millis() - _last_debounce_at) > _debounce_delay){
        if (reading != _rain_pulse_state){
            _rain_pulse_state = reading;
            if (_rain_pulse_state == HIGH){
                _rain_pulse_count++;
                callback();
            }
        }
    }
    _last_rain_pulse_state = reading;
    return true;
}

#endif