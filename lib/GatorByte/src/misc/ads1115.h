#ifndef GB_EADC_h
#define GB_EADC_h

#include "ADS1115-Driver.h"

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_EADC : public GB_DEVICE {
    public:
        GB_EADC(GB &gb);
        
        DEVICE device = {
            "eadc",
            "ADS1115 External ADC"
        };

        struct PINS {
            bool mux;
            int enable;
        } pins;

        GB_EADC& initialize();
        GB_EADC& configure(PINS);
        uint16_t readchannel(uint8_t);
        uint16_t getreading(uint8_t);
        float getdepth(uint8_t);

        GB_EADC& on();
        GB_EADC& off();
        bool ison();
        GB_EADC& persistent();
        
        bool testdevice();
        String status();

    private:
        GB *_gb;
        bool _ison = false;
        int _address = 0x48;
        // ADS1115 _eadc = ADS1115(ADS1115_I2C_ADDR_GND);
        ADS1115 _eadc = ADS1115(_address);
        bool _persistent = false;
        
        int VREF = 3300;
        float CURRENT_INIT = 4.0;
        int RANGE = 5000;
        int DENSITY_FLUID = 1;
        int PRINT_INTERVAL = 1000;
        float ADC_VDD_VALUE = 32767.0;
};

// Constructor 
GB_EADC::GB_EADC(GB &gb) {
    this->_gb = &gb;
    this->_gb->includelibrary(this->device.id, this->device.name); 
    this->_gb->devices.eadc = this;
}

// Configure device
GB_EADC& GB_EADC::configure(PINS pins) {

    // Set pins
    this->pins = pins;

    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    return *this;
}

// Test the device
bool GB_EADC::testdevice() { 
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_EADC::status() { 
    return this->device.detected ? String(this->readchannel(0)) + ":.:" + String(this->readchannel(1)) + ":.:" + String(this->readchannel(2)) + ":.:" + String(this->readchannel(3)) : ("not-detected" + String(":") + device.id);
}

// Initialize the module
GB_EADC& GB_EADC::initialize() { 
    _gb->init();

    _gb->log("Initializing " + this->device.name, false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    // Check if the device is connected and responding
    this->on();
    Wire.begin();
    Wire.beginTransmission(this->_address);
    bool success = Wire.endTransmission() == 0;

    // If no error encountered
    if (success) {
        this->device.detected = true;

        if (_gb->globals.GDC_CONNECTED) {
            _gb->arrow().log("Done", true);
            this->off();
            return *this;
        }

        // Read channels
        _gb->space(1).log("(" + String(this->readchannel(0)) + ", " + String(this->readchannel(1)) + ", " + String(this->readchannel(2)) + ", " + String(this->readchannel(3)) + ")", false);
        _gb->arrow().log("Done", true);
        
        if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("--..").wait(500).play("...");
    }
    else {
        this->device.detected = false;
        _gb->arrow().log("Not detected", true);

        if (_gb->globals.GDC_CONNECTED) {
            this->off();
            return *this;
        }
        if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("--..").wait(500).play("---");
    }

    return *this;
}



// Turn on the module
GB_EADC&  GB_EADC::on() { 
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    this->_ison = true;
    delay(100);
    return *this;
}

// Turn off the module
GB_EADC&  GB_EADC::off() { 
    if (this->_persistent) return *this;
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    this->_ison = false;
    return *this;
}

// Checks if the device is on or off
bool GB_EADC::ison() { 
    return this->_ison;
}

GB_EADC&  GB_EADC::persistent() {
    this->_persistent = true;
    return *this;
}

/*
    ! Read channel
    Reads the ADC value from the provided channel number. The 
    channel number can be any number between 0 and 3.
*/
uint16_t GB_EADC::readchannel(uint8_t input) {

    // Return if input channel number is out of range (0 - 3)
    if (input < 0 || input > 3) return -1;

    // Convert to real channel number
    input += 4;

    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);

    // _gb->log(".");

    // Set the channel
    this->_eadc.setMultiplexer(input);

    // Get reading
    this->_eadc.startSingleConvertion();

    // The ADS1115 needs to wake up from sleep mode
    delay(1); 

    // Wait while until reading is ready
    while (!this->_eadc.getOperationalStatus());

    // Get the reading
    uint16_t value = this->_eadc.readConvertedValue();

    // uint16_t value = this->_eadc.readADC(input);

    // _gb->log("..");
    return value;
}

uint16_t GB_EADC::getreading(uint8_t channel) {

    // Read ADC channel; channel is a integer {0, 1, 2, 3}
    int16_t value = this->readchannel(channel);
    return value;
}

float GB_EADC::getdepth(uint8_t channel) {

    // _gb->log("Reading depth (" + String(channel) + ")", false);

    // Read ADC channel; channel is a integer {0, 1, 2, 3}
    int16_t value = this->readchannel(channel);

    // Convert the ADC reading to voltage (mV)
    float voltage = (value / ADC_VDD_VALUE) * VREF;

    // Convert the voltage to current (mA) using a sense resistor (120 ohm)
    float current = voltage / 120.0;

    // Caluculate depth in mm
    float depth = (current - CURRENT_INIT) * (RANGE / DENSITY_FLUID / 16.0);

    // Convert mm to inches
    depth /= 25.4;

    // Ignore invalid readings
    if (depth < 0) depth = 0.0;
    
    // _gb->arrow().log("" + String(depth));

    return depth;
}

#endif