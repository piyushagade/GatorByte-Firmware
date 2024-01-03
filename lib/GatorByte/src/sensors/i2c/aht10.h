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
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    delay(50);
    return *this;
}

// Turn off the module
GB_AHT10& GB_AHT10::off() {
    
    if (this->_persistent) return *this;
    
    delay(50);
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
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
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_AHT10::status() { 
    return this->device.detected ? String(this->temperature()) + ":.:" + String(this->humidity()) : "not-detected" + String(":") + device.id;
}

GB_AHT10& GB_AHT10::initialize() { initialize(true); }
GB_AHT10& GB_AHT10::initialize(bool testdevice) { 
    _gb->init();
    
    _gb->log("Initializing AHT10 sensor", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    this->on();
    if (testdevice) {
        bool error = !_aht.begin();
        int counter = 5;
        while (error && counter-- > 0) { delay(200); error = !_aht.begin();}
        this->device.detected = !error;

        if (!error) {
            
            _gb->log(" -> Done", false);

            if (_gb->globals.GDC_CONNECTED) {
                _gb->log();
                this->off();
                return *this;
            }

            _gb->log(" -> " + String(this->temperature()) + " Celcius and " + String(this->humidity()) + " % R.H."); 
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("---").wait(250).play("...");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("green").wait(250).revert(); 
        }
        else {
            _gb->log(" -> Not detected"); 
            
            if (_gb->globals.GDC_CONNECTED) {
                this->off();
                return *this;
            }
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("---").wait(250).play("---");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red").wait(250).revert(); 
        }
    }
    else {
        this->device.detected = true;
        _gb->log(" -> Done");
    }
    this->off();
    return *this;    
}

float GB_AHT10::temperature() {
    this->on();

    // Enable watchdog
    _gb->getmcu().watchdog("enable", 8000);

    // Get the latest values
    float value = _aht.readTemperature();
    int count = 0;
    while (value == 255 && count++ <= 3) {
        _aht.readTemperature(); delay(250);
    }

    // Disable watchdog
    _gb->getmcu().watchdog("disable");
    
    this->off();

    return value;
}

float GB_AHT10::humidity() {
    this->on();
    
    // Enable watchdog
    _gb->getmcu().watchdog("enable", 8000);

    // Get the latest values
    float value = _aht.readHumidity();
    int count = 0;
    while (value == 255 && count++ <= 1) {
        _aht.softReset(); delay(150);
        _aht.readHumidity(); delay(150);
    }
    
    // Disable watchdog
    _gb->getmcu().watchdog("disable");
    
    this->off();

    return value;
}

#endif