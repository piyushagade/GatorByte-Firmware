#ifndef GB_RELAY_h
#define GB_RELAY_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_RELAY : public GB_DEVICE {
    public:
        GB_RELAY(GB &gb);
        
        DEVICE device = {
            "relay",
            "SPST relay"
        };
        
        struct PINS {
            bool mux; 
            int trigger;
        } pins;

        GB_RELAY& initialize();
        GB_RELAY& configure(PINS);
        GB_RELAY& on();
        GB_RELAY& off();
        GB_RELAY& trigger();
        GB_RELAY& trigger(int duration);
        bool state();
        
        bool testdevice();
        String status();

    private:
        GB *_gb;
};

GB_RELAY::GB_RELAY(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.relay = this;
}

// Test the device
bool GB_RELAY::testdevice() { 
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));

    // Trigger the autosampler
    this->trigger(750);

    return this->device.detected;
}
String GB_RELAY::status() { 
    return this->device.detected ? "detected" : "not-detected" + String(":") + device.id;;
}

GB_RELAY& GB_RELAY::initialize() { 
    _gb->init();
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    this->device.detected = true;
    return *this;
}

/*
    Configure relay
*/
GB_RELAY& GB_RELAY::configure(PINS pins) { 
    this->pins = pins;
    
    // Set pin modes
    if(!this->pins.mux) pinMode(this->pins.trigger, OUTPUT);
    return *this;
}

GB_RELAY& GB_RELAY::on() { 
    return *this;
}

GB_RELAY& GB_RELAY::off() { 
    return *this;
}

/*
    Trigger the relay
*/
GB_RELAY& GB_RELAY::trigger() {
    return this->trigger(100);
}
GB_RELAY& GB_RELAY::trigger(int onduration) {

    if(this->pins.mux) {
        _gb->getdevice("ioe").writepin(this->pins.trigger, HIGH);
        delay(onduration);
        _gb->getdevice("ioe").writepin(this->pins.trigger, LOW);
    }
    else {
        digitalWrite(this->pins.trigger, HIGH);
        digitalWrite(this->pins.trigger, LOW);
    }
    return *this;
}

/*
    Check if the relay is triggered (hot)
*/
bool GB_RELAY::state(){
    return digitalRead(this->pins.trigger);
}

#endif