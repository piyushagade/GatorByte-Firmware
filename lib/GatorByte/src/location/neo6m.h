#ifndef GB_NEO_6M_h
#define GB_NEO_6M_h

#include "TinyGPS++.h"

#ifndef GB_h
    #include "../GB.h"
#endif

struct GPS_DATA {
    bool has_fix;
    bool valid;
    bool updated;
    float lat;
    float lng;
    uint8_t satellites;
    float hdop;
    uint8_t date;
    uint8_t time;
    uint8_t age;
    uint8_t id;
    uint8_t last_fix_attempts;
};

class GB_NEO_6M : public GB_DEVICE {
    public:
        GB_NEO_6M(GB &gb);
        
        struct PINS {
            bool mux;
            int enable;
            int comm;
        } pins;
        
        DEVICE device = {
            "gps",
            "Neo-6m GPS module"
        };
        
        GPS_DATA data = {false, false, false, 0, 0, 0, 0, 0, 0, 0, 0, 99};

        GB_NEO_6M& initialize();
        GB_NEO_6M& configure(PINS);
        GB_NEO_6M& on();
        GB_NEO_6M& on(String);
        GB_NEO_6M& off();
        GB_NEO_6M& off(String);
        GB_NEO_6M& reset();
        GB_NEO_6M& persistent();
        GPS_DATA read();
        GPS_DATA read(bool dummy);
        bool ison();
        bool fix();
        
        bool testdevice();
        String status();

    private:
        GB *_gb;
        TinyGPSPlus _neo;

        int _baud = 9600;
        bool _persistent = false;
        void _read_nmea();
        bool _update();

        long _last_fix_timestamp = 0;
        long _last_update_timestamp = 0;
        long _last_update_expiration_duration = 30 * 1000;
        uint8_t MAX_ATTEMPTS = 20;

        String _manufacturer = "";
    
};

// Constructor
GB_NEO_6M::GB_NEO_6M(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.gps = this;
};

// Configure device and pins
GB_NEO_6M& GB_NEO_6M::configure(PINS pins) {
    this->pins = pins;

    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);
    
    return *this;
}

// Test the device
bool GB_NEO_6M::testdevice() { 
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_NEO_6M::status() { 

    // Update manufacturer info
    if (this->device.detected) {
        this->on();
        String manufacturer = "";
        for (unsigned long start = millis(); millis() - start < 10000;) { 
            while (_gb->serial.hardware->available()) {
                String c = _gb->serial.hardware->readStringUntil('\n');
                this->device.detected = c.indexOf("u-blox") > -1 || c.indexOf("$GP") > -1 || c.indexOf(",,,") > -1 || c.indexOf(",,") > -1;
                if (c.indexOf("u-blox") > -1) {
                    manufacturer = "u-Blox";
                }
                break;
            }
            if (manufacturer.length() > 0) {
                this->_manufacturer = manufacturer;
                break;
            }
        }
        this->off();
    }

    return this->device.detected ? this->_manufacturer : "not-detected" + String(":") + device.id;
}

// Initialize the module and communication
GB_NEO_6M& GB_NEO_6M::initialize() {
    _gb->init();
    
    this->on();
    _gb->log("Initializing GPS module", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    _gb->serial.hardware->begin(this->_baud);

    this->reset();
    this->device.detected = false;

    for (unsigned long start = millis(); millis() - start < 3000;){ 
        while (_gb->serial.hardware->available() && !this->device.detected) {
            String c = _gb->serial.hardware->readStringUntil('\n');
            
            this->device.detected = c.indexOf("u-blox") > -1 || 
                                    c.indexOf("$GP") > -1 || 
                                    c.indexOf(",,") > -1;
            
            if (this->device.detected) {
                if (c.indexOf("u-blox") > -1) {
                    _gb->log(" -> Manufacturer: u-Blox", false);
                    this->_manufacturer = "u-Blox";
                    break;
                }

            }
        }
        if (this->device.detected && this->_manufacturer.length() > 0) break;
    }

    if (this->device.detected) _gb->log(" -> Done");
    else _gb->log(" -> Not detected");

    this->off();
    return *this;
}

bool GB_NEO_6M::ison() {
    if(this->pins.mux) return _gb->getdevice("ioe").readpin(this->pins.enable) == HIGH;
    else return digitalRead(this->pins.enable) == HIGH;
}

// Turn on the module
GB_NEO_6M& GB_NEO_6M::on(String type) {
    if(this->pins.mux) {
        if (type == "power") _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
        if (type == "comm") _gb->getdevice("ioe").writepin(this->pins.comm, HIGH);
    }
    else {
        if (type == "power") digitalWrite(this->pins.enable, HIGH);
        if (type == "comm") digitalWrite(this->pins.comm, HIGH);
    }
}
GB_NEO_6M& GB_NEO_6M::on() {
    this->on("comm");
    this->on("power");
    return *this;
}

// Turn off the module
GB_NEO_6M& GB_NEO_6M::off(String type) {
    if (this->_persistent) return *this;
    if(this->pins.mux) {
        if (type == "power") _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
        if (type == "comm") _gb->getdevice("ioe").writepin(this->pins.comm, LOW);
    }
    else {
        if (type == "power") digitalWrite(this->pins.enable, LOW);
        if (type == "comm") digitalWrite(this->pins.comm, LOW);
    }

    // Clear the serial buffer
    for (unsigned long start = millis(); millis() - start < 1000;) while (_gb->serial.hardware->available()) _gb->serial.hardware->read();
    
    return *this;
}
GB_NEO_6M& GB_NEO_6M::off() {
    this->off("comm");
    this->off("power");
    return *this;
}

// Reset the gps data
GB_NEO_6M& GB_NEO_6M::reset() {
    this->data = {false, false, false, 0, 0, 0, 0, 0, 0, 0, 0, 99};
    return *this;
}

/*
    Keeps device persistently on
*/
GB_NEO_6M& GB_NEO_6M::persistent() {
    this->_persistent = true;
    return *this;
}

// Read dummy data
GPS_DATA GB_NEO_6M::read(bool dummy) {

    if (dummy) {
        // Return a dummy value if "dummy" mode is on
        this->data = {true, false, false, 12.34, -56.78, 4, 2, 0, 0, 0, 0, 5};
        _gb->getdevice("gdc").send("gdc-db", "gps=true,12.34, -56.78,99");
        return this->data;
    }
    else return this->read();
}

// Update and read the gps data
GPS_DATA GB_NEO_6M::read() {

    this->_gb->log("Reading " + this->device.name, false);

    // Return if the devices file on SD does not contain gps
    if (_gb->hasdevice("sd") && _gb->globals.DEVICES_LIST.indexOf("gps") == -1) {
        _gb->log(_gb->globals.DEVICES_LIST);
        this->_gb->log(" -> Not found in devices.txt");
        return this->data; 
    };

    // Return a dummy value if "dummy" mode is on
    if (this->_gb->globals.MODE == "dummy") {
        this->_gb->log(" -> Sending dummy data");
        this->data = {true, false, false, 12.34, -56.78, 4, 2, 0, 0, 0, 0, 5};
        _gb->getdevice("gdc").send("gdc-db", "gps=true,12.34, -56.78,99");
        return this->data;
    }

    // Return if GPS not connected/not communicating
    if (!this->device.detected) return this->data;

    // Update GPS data if '_last_update_expiration_duration' has passed since last update
    if (
        _last_update_timestamp == 0 ||
        millis() - _last_update_timestamp > _last_update_expiration_duration
    ) {
        _last_update_timestamp = millis();
        this->_update();
    }
    else {
        _gb->log("Last GPS fix was less than 30 seconds ago. Returning stale coordinates.");
    }

    // Indicate on the RGB led
    if (this->_gb->hasdevice("rgb") && this->fix()) this->_gb->getdevice("rgb").on("blue"); else this->_gb->getdevice("rgb").on("red");

    // Return latest data
    _gb->getdevice("gdc").send("gdc-db", "gps=" + String(this->data.has_fix) + "," + String(this->data.lat) + "," + String(this->data.lng) + "," + String(this->data.last_fix_attempts));
    return this->data;
}

// Return if a fix was achieved
bool GB_NEO_6M::fix() {
    return this->data.has_fix;
}

// Read the GPS data
void GB_NEO_6M::_read_nmea() {

    // Clear the serial buffer
    for (unsigned long start = millis(); millis() - start < 500;) while (_gb->serial.hardware->available()) char c = _gb->serial.hardware->read();

    // Read data from GPS over serial
    for (unsigned long start = millis(); millis() - start < 2000;) {
        while (_gb->serial.hardware->available()) {
            char c = _gb->serial.hardware->read();
            _gb->log(c, false);
            // _gb->log("|*gps*|" + String(c), false);
            _neo.encode(c);
        }
    }

    delay(100);
}

// Update the gps data
bool GB_NEO_6M::_update() {

    if (!this->device.detected) return false;

    this->on();

    // Set the baud rate
    _gb->serial.hardware->begin(this->_baud);
    
    // Start watchdog timer
    this->_gb->getmcu().watchdog("enable");

    // Read the serial data
    this->_read_nmea();

    // Allow more attempts if the device just started collecting data.
    if (this->_gb->globals.ITERATION < 5) this->MAX_ATTEMPTS = 40;
    else this->MAX_ATTEMPTS = 20;

    int counter = 0;
    bool ledstate = false;
    uint8_t lasttoggle = millis();
    while (
            // Minimum 5 updates if the GPS has a lock, but the data is not the latest
            (
                counter < 5 &&
                _neo.location.isValid() &&
                !_neo.location.isUpdated()
            ) ||

            // If no lock yet and the counter hasn't expired
            (
                counter < this->MAX_ATTEMPTS &&
                    (
                        !_neo.location.isValid() ||
                        !_neo.location.isUpdated()
                    )
            )

    ) {
        
        // Reset watchdog timer
        this->_gb->getmcu().watchdog("reset");
        
        this->_gb->log("  Attempt: " + String(counter + 1) + ", Valid: " + String(_neo.location.isValid() ? "Yes" : "No") + ", Updated: " + String(_neo.location.isUpdated() ? "Yes" : "No"));
        
        // Log to SD card
        if (this->_gb->hasdevice("sd")) this->_gb->getdevice("sd").debug("operations", "  Attempt: " + String(counter + 1) + ", Valid: " + String(_neo.location.isValid() ? "Yes" : "No") + ", Updated: " + String(_neo.location.isUpdated() ? "Yes" : "No"));

        // Read the serial data
        this->_read_nmea();

        // Toggle led
        if (lasttoggle == 0 || millis() - lasttoggle > 1000) {
            if (this->_gb->hasdevice("rgb") && ledstate) this->_gb->getdevice("rgb").on("yellow");
            else this->_gb->getdevice("rgb").off();
            ledstate = !ledstate;
        }
        
        // // Loop mqtt
        // if (this->_gb->hasdevice("mqtt")) this->_gb->getdevice("mqtt").update();
        
        // GB breathe
        this->_gb->breathe("bl");

        // Log to SD card
        // if (this->_gb->hasdevice("sd")) this->_gb->getdevice("sd").debug("operations", ". ", false);

        // Increment the counter
        counter++;
    }

    // Log to console
    this->_gb->log("Looped out, Valid: " + String(_neo.location.isValid() ? "Yes" : "No") + ", Updated: " + String(_neo.location.isUpdated() ? "Yes" : "No"));

    // Log to SD card
    if (this->_gb->hasdevice("sd")) this->_gb->getdevice("sd").debug("operations", "Looped out, Valid: " + String(_neo.location.isValid() ? "Yes" : "No") + ", Updated: " + String(_neo.location.isUpdated() ? "Yes" : "No"));

    // If we have a valid GPS location set data
    if (_neo.location.isValid() && _neo.location.isUpdated()){
        
        _last_fix_timestamp = millis();
        this->data.has_fix = true;
        this->data.lat = _neo.location.lat();
        this->data.lng = _neo.location.lng();
        this->data.age = _neo.location.age();
        this->data.valid = _neo.location.isValid();
        this->data.updated = _neo.location.isUpdated();

        this->data.satellites = (_neo.satellites.isValid() ? _neo.satellites.value() : 0);
        this->data.hdop = (_neo.hdop.isValid() ? _neo.hdop.value() : 0);
        this->data.last_fix_attempts = counter;
    }
    else this->reset();
    
    // Disable watchdog timer
    this->_gb->getmcu().watchdog("disable");
    
    this->off();
    return this->data.has_fix;
}

#endif