#ifndef GB_TPBCK_h
#define GB_TPBCK_h

#ifndef GB_h
    #include "../../GB.h"
#endif

class GB_TPBCK : public GB_DEVICE {
    public:
        GB_TPBCK(GB&);

        DEVICE device = {
            "tpbck",
            "Tipping bucket rain sensor"
        };
        
        struct PINS {
            bool mux;
            int data;
        } pins;

        GB_TPBCK& configure(PINS);
        GB_TPBCK& initialize();
        GB_TPBCK& on();
        GB_TPBCK& off();

        int ADCVALUE = -1;
        int ADCLOWERTHRESHOLD = 15500;
        int ADCUPPERTHRESHOLD = 25500;
        
        // Type definitions
        typedef void (*callback_t)();
        bool listener(callback_t callback);
        bool listener();

    private:
        GB *_gb;

        int _rain_pulse_state;
        unsigned long _debounce_delay = 500;
        bool _last_rain_pulse_state = LOW;
        bool _last_debounce_at = 0;
        int _rain_pulse_count = 0;
};

GB_TPBCK::GB_TPBCK(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.tpbck = this;
}

GB_TPBCK& GB_TPBCK::configure(PINS pins) {
    this->pins = pins;
    
    // Set pin modes
    pinMode(this->pins.data, INPUT);
    
    return *this;
}

GB_TPBCK& GB_TPBCK::initialize() { 
    _gb->init();
    
    _gb->log("Initializing Tipping Bucket sensor", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    this->on();
    _gb->log(" -> Done");
    this->off();
    return *this;    
}

GB_TPBCK& GB_TPBCK::on() { 
    return *this;
}

GB_TPBCK& GB_TPBCK::off() { 
    return *this;
}

// Listener for rain events
bool GB_TPBCK::listener(callback_t callback) {

    // Read the data pin
    int reading = digitalRead(this->pins.data);

    // Debounce
    if (reading != this->_last_rain_pulse_state) this->_last_debounce_at = millis();
    if ((millis() - this->_last_debounce_at) > this->_debounce_delay){
        if (reading != this->_rain_pulse_state){
            this->_rain_pulse_state = reading;
            if (this->_rain_pulse_state == HIGH){

                this->_rain_pulse_count++;
                callback();
            }
        }
    }
    this->_last_rain_pulse_state = reading;
    return true;
}

bool GB_TPBCK::listener() {

    //// Read channel 3 from EADC
    // int value = _gb->getdevice("eadc").getreading(3);
    // ADCVALUE = value;
    // int reading = ADCLOWERTHRESHOLD <= value && value < ADCUPPERTHRESHOLD ? HIGH : LOW;

    // Read the data pin
    int value = analogRead(this->pins.data);
    ADCVALUE = value;
    int reading = 1000 <= value ? HIGH : LOW;

    // _gb->log("Rain value: " + String(value));

    // Debounce
    bool pulsedetected = false;
    if (reading != this->_last_rain_pulse_state) this->_last_debounce_at = millis();
    if ((millis() - this->_last_debounce_at) > this->_debounce_delay) {
        if (reading != this->_rain_pulse_state) {
            this->_rain_pulse_state = reading;
            if (this->_rain_pulse_state == HIGH) {
                this->_rain_pulse_count++;
                pulsedetected = true;

                if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("white");
                if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("...");
                if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").revert();
            }
        }
    }
    this->_last_rain_pulse_state = reading;
    return pulsedetected;
}

#endif