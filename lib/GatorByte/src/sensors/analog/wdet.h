#ifndef GB_NCLS_h
#define GB_NCLS_h

#ifndef GB_h
    #include "../../GB.h"
#endif

class GB_NCLS {
    public:
        GB_NCLS(GB&);

        // Type definitions
        typedef void (*callback_t)();
        
        DEVICE device = {
            "liquid_sensor",
            "Non-contact liquid sensor"
        };
        
        struct PINS {
            int enable;
            int data;
            bool mux;
        } pins;

        GB_NCLS& configure(PINS);
        GB_NCLS& on();
        GB_NCLS& off();

        bool read();
        bool listener(callback_t callback);

    private:
        GB *_gb;

        int _wdet_pulse_state;
        unsigned long _debounce_delay = 1500;
        bool _last_wdet_pulse_state = LOW;
        bool _last_debounce_at = 0;
        int _wdet_pulse_count = 0;
};

GB_NCLS::GB_NCLS(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
}

GB_NCLS& GB_NCLS::configure(PINS pins) {
    this->pins = pins;
    
    // Set pin modes
    pinMode(this->pins.data, INPUT);
    
    if(this->pins.mux) _ioe.pinMode(this->pins.enable, OUTPUT);
    else pinMode(this->pins.enable, OUTPUT);
    return *this;
}

GB_NCLS& GB_NCLS::on() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH); 
    return *this;
}

GB_NCLS& GB_NCLS::off() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    return *this;
}

bool GB_NCLS::read() { 
    this->on();
    int wdet_raw_data = analogRead(this->pins.data);
    bool water_detected = wdet_raw_data > 2800;
    return water_detected;
    this->off();
}

bool GB_NCLS::listener(callback_t callback) {
    int reading = digitalRead(this->pins.data);

    // Debounce
    if (reading != _last_wdet_pulse_state) _last_debounce_at = millis();
    if ((millis() - _last_debounce_at) > _debounce_delay){
        if (reading != _wdet_pulse_state){
            _wdet_pulse_state = reading;
            if (_wdet_pulse_state == HIGH){
                _wdet_pulse_count++;
                callback();
            }
        }
    }
    _last_wdet_pulse_state = reading;
    return true;
}

#endif