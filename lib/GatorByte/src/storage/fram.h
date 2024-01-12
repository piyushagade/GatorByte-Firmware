#ifndef GB_FRAM_h
#define GB_FRAM_h

#ifndef GB_h
    #include "../GB.h"
#endif

#ifndef CSVary_h
    #include "CSVary.h"
#endif

#ifndef GB_RGB_h
    #include "../misc/rgb.h"
#endif

#ifndef SPIMEMORY_H
    #include "SPIMemory.h"
#endif

struct LOCATION {
    int init_flag = 0;
    int rwtest = 1;
    int devicetype = 2;
} flashlocations;

class GB_FRAM : public GB_DEVICE {
    public:
        GB_FRAM(GB &gb);

        DEVICE device = {
            "fram",
            "Flash storage breakout"
        };

        struct PINS {
            bool mux;
            int cd;
            int ss;
            int enable;
        } pins;

        bool testdevice();
        bool framtest();
        GB_FRAM& configure(PINS);
        GB_FRAM& initialize();
        GB_FRAM& initialize(String speed);
        bool initialized();
        GB_FRAM& on();
        GB_FRAM& off();

        bool write(uint32_t address, String data);
        
        String read(uint32_t address);
        
        bool rwtest();
        
    private:
        bool _use_mux = false;

        SPIFlash *_flash;
        bool _fram_present = false;
        GB *_gb;
        int _sck_speed = SPI_HALF_SPEED;
        bool _initialized = false;
        
        typedef void (*callback_t_on_control)(JSONary data);
        callback_t_on_control _on_control_update;

        uint8_t _page_buffer[256];
        uint32_t addr;
        uint8_t dataByte;
        uint16_t dataInt;
};

GB_FRAM::GB_FRAM(GB &gb) {
    this->_gb = &gb;
    this->_gb->includelibrary(this->device.id, this->device.name);     
    this->_gb->devices.fram = this;
}

bool GB_FRAM::testdevice() { 
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}

bool GB_FRAM::framtest() { 
    _gb->log("Testing: " + this->device.name);
    return true;
}


/*
    R/W test for FRAM
*/
bool GB_FRAM::rwtest() {

    bool success = true;

    int address = flashlocations.rwtest;
    String prompt = "teststring";
    int counter = 0, timer = millis(), readtimer = 0, writetimer = 0;

    // Clear data at the address
    counter = 0;
    while (!_flash->eraseSection(address, 4 * KiB) && counter++ <= 500) delay(10);

    // Write string
    success = this->write(address, prompt);
    writetimer = millis() - timer;
    timer = millis();
    
    // Read string
    String readstring = this->read(address);
    readtimer = millis() - timer;
    success = success && (prompt == readstring);

    // Delete string
    counter = 0;
    while (!_flash->eraseSection(address, 4 * KiB) && counter++ <= 500) delay(10);
    
    if (prompt != readstring) _gb->log(" -> R/W code mismatch");
    else {
        if (!success) _gb->log(" -> Failed");
        else {
            _gb->log(" -> Read: " + String(readtimer) + " ms, Write: " + String(writetimer) + " ms.");
        }
    }

    return false;
}

// Write string
bool GB_FRAM::write(uint32_t address, String data) {
    return _flash->writeStr(address, data);
}

// Read string
String GB_FRAM::read(uint32_t address) {
    String readstring = "";
    _flash->readStr(address, readstring, true);

    return readstring;
}

GB_FRAM& GB_FRAM::configure(PINS pins) {
    this->pins = pins;

    _flash = new SPIFlash(pins.ss, &SPI);

    // Set pin modes of the device
    if(!this->pins.mux) {
        digitalWrite(this->pins.enable, OUTPUT);
        digitalWrite(this->pins.cd, INPUT);
    }

    return *this;
}

GB_FRAM& GB_FRAM::on() {

    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);

    return *this;
}

GB_FRAM& GB_FRAM::off() {
    
    // Safety delay
    delay(250);

    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);

    return *this;
}

// Initialize SD card
GB_FRAM& GB_FRAM::initialize() { this->initialize("quarter"); }
GB_FRAM& GB_FRAM::initialize(String speed) { 
    this->on();
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    /*
        ! Set SPI speed
    */

    // For AVR
    if (speed.indexOf("eight") > -1) this->_sck_speed = SPI_EIGHTH_SPEED;
    else if (speed.indexOf("quarter") > -1) this->_sck_speed = SPI_QUARTER_SPEED;
    else if (speed.indexOf("half") > -1) this->_sck_speed = SPI_HALF_SPEED;
    else if (speed.indexOf("full") > -1) this->_sck_speed = SPI_FULL_SPEED;

    // For Due
    else if (speed.indexOf("third") > -1) this->_sck_speed = SPI_DIV3_SPEED;
    else if (speed.indexOf("sixth") > -1) this->_sck_speed = SPI_DIV6_SPEED;

    // Default speed value
    else this->_sck_speed = SPI_HALF_SPEED;

    _gb->log("Initializing FRAM module", false);
    if(_gb->globals.WRITE_DATA_TO_SD){

        bool success = this->_flash->begin();

        if(!success) {
            _gb->log(" -> Not detected", true);
            this->device.detected = false;

            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("-").wait(250).play("---");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red").wait(250).on("red").wait(250).revert();
        }
        else {
            _fram_present = true;

            // Get FRAM capacity
            float capacity = _flash->getCapacity() / 1024 / 1024;
            
            _gb->log(" -> Done (" + String(capacity) + " MB)", false);

            // Perform R/W test
            bool result = this->rwtest();
            this->_initialized = result;
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("-").wait(250).play("...");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("blue").wait(250).revert();
        }
    }
    else _gb->log(" -> Disabled", true);
    this->off();
    return *this;
}

bool GB_FRAM::initialized() { 
    return this->_initialized;
}

#endif