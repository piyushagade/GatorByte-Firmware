#ifndef GB_AHT10_h
#define GB_AHT10_h

#ifndef GB_h
    #include "../../GB.h"
#endif

#ifndef AHT10_h
    #include "../../lib/AHT10/src/AHT10.h"
#endif

class GB_AHT10 : public GB_DEVICE {
    public:
        GB_AHT10(GB&);

        DEVICE device = {
            "aht",
            "AHT10 sensor"
        };

        struct PINS {
            bool mux;
            int enable;
        } pins;

        GB_AHT10& configure(PINS);
        GB_AHT10& initialize();
        GB_AHT10& initialize(bool testdevice);
        GB_AHT10& on();
        GB_AHT10& off();
        GB_AHT10& persistent();
        
        bool testdevice();
        String status();

        float temperature();
        float humidity();

    private:

        AHT10 _aht = AHT10(0x38);
        GB *_gb;
        GB_AHT10& _initialize(bool testdevice);
        bool _persistent = false;
};

GB_AHT10::GB_AHT10(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.aht = this;
}

GB_AHT10& GB_AHT10::configure(PINS pins) { 
    this->pins = pins;

    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);
    
    return *this;
}

// Turn on the module
GB_AHT10& GB_AHT10::on() {
    delay(10);
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    return *this;
}

// Turn off the module
GB_AHT10& GB_AHT10::off() {
    
    if (this->_persistent) return *this;
    
    delay(50);
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    delay(10);
    return *this;
}

GB_AHT10& GB_AHT10::persistent() {
    
    this->_persistent = true;
    return *this;
}

// Test the device
bool GB_AHT10::testdevice() { 

    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(true);
    
    return this->device.detected;
}
String GB_AHT10::status() { 

    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(true);
    
    return this->device.detected ? String(this->temperature()) + ":.:" + String(this->humidity()) : "not-detected" + String(":") + device.id;
}

GB_AHT10& GB_AHT10::initialize() { return initialize(true); }
GB_AHT10& GB_AHT10::initialize(bool testdevice) { 
    if (_gb->globals.GDC_CONNECTED) {
        this->off();
        return *this;
    }
    return this->_initialize(testdevice);
}

GB_AHT10& GB_AHT10::_initialize(bool testdevice) {
    _gb->init();
    
    _gb->log("Initializing " + this->device.name, false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    this->on();
    if (testdevice) {
        bool error = !_aht.begin();
        int counter = 5;
        while (error && counter-- > 0) { delay(200); error = !_aht.begin();}
        this->device.detected = !error;

        if (!error) {
            
            _gb->arrow().log("Done", false);

            if (_gb->globals.GDC_CONNECTED) {
                this->off();
                return *this;
            }

            delay(2000); _gb->arrow().log(String(this->temperature()) + " Celcius and " + String(this->humidity()) + " % R.H."); 
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("---").wait(250).play("...");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on(2).wait(250).revert(); 
        }
        else {
            _gb->arrow().log("Not detected"); 
            _gb->globals.INIT_REPORT += this->device.id;
            
            if (_gb->globals.GDC_CONNECTED) {
                this->off();
                return *this;
            }
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("---").wait(250).play("---");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on(1).wait(250).revert(); 
        }
    }
    else {
        this->device.detected = true;
        _gb->arrow().log("Done");
    }
    this->off();
    return *this;    
}

float GB_AHT10::temperature() {

    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(false);

    this->on();

    // Get the latest values
    float value = _aht.readTemperature();
    int count = 0;
    bool invalid = value <= -20 || value >= 200 || floor(value) == 255;
    if (invalid) delay(250);
    while (invalid && count++ <= 5) {
        value = _aht.readTemperature();
        invalid = value <= -20 || value >= 200 || floor(value) == 255;
        if (invalid && (count + 1) % 2 == 0) { _aht.softReset(); delay(50); }
        delay(40 * count);
    }

    if (invalid) value = 255;
    
    this->off();

    return value;
}

float GB_AHT10::humidity() {

    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(false);
    
    this->on();
    
    // Get the latest values
    float value = _aht.readHumidity();
    bool invalid = value <= 0 || value >= 100 || value == 255;
    int count = 0;
    while (invalid && count++ <= 5) {
        value = _aht.readHumidity();
        invalid = value <= 0 || value >= 100 || value == 255;
        if (invalid && count % 2 == 0) { _aht.softReset(); delay(50); }
        delay(40 * count);
    }
    
    if (invalid) value = 255;
    
    this->off();

    return value;
}

#endif