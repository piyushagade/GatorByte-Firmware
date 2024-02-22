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
    String accuracy;
    uint8_t date;
    uint8_t time;
    uint8_t age;
    uint8_t id;
    uint8_t attempts;
    float speed;
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
        
        GPS_DATA data = {false, false, false, 0, 0, 0, 0, "unknown", 0, 0, 0, 0, 99, 0};

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
        float speed();
        
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

        const float MOVEMENT_THRESHOLD = 0.1; // Set your threshold distance in degrees
        const unsigned long MOVEMENT_CHECK_INTERVAL = 300000; // 5 minutes in milliseconds
        float _prev_lat = 0.0;
        float _prev_lng = 0.0;
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

    for (unsigned long start = millis(); millis() - start < 6000;){ 
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
    this->data = {false, false, false, 0, 0, 0, 0, "unknown", 0, 0, 0, 0, 99, 0};
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
    _gb->log("Reading " + this->device.name, false);

    // Dummy data requested?
    dummy = dummy || _gb->globals.MODE == "dummy";

    if (dummy) {
        _gb->arrow().log("Sending dummy data");
        this->data = {true, false, false, 12.34, -56.78, 4, 2, "unknown", 0, 0, 0, 0, 5, 3};
        _gb->getdevice("gdc").send("gdc-db", "gps=true,12.34, -56.78,99");
        return this->data;
    }
    else {

        // Return if the devices file on SD does not contain gps
        if (_gb->hasdevice("sd") && _gb->globals.DEVICES_LIST.indexOf("gps") == -1) {
            _gb->arrow().log("Not found in config.");
            return this->data; 
        };

        // Return if GPS not connected/not communicating
        if (!this->device.detected) {
            _gb->arrow().log("Skipped. Not detected."); 
            return this->data;
        }

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

        // Indicate on the RGB led if or not GPS fix was achieved
        if (_gb->hasdevice("rgb") && this->fix()) _gb->getdevice("rgb").on("blue"); else _gb->getdevice("rgb").on("red");
        
        // Indicate on the buzzer led if or not GPS fix was achieved
        if (_gb->hasdevice("buzzer") && this->fix()) _gb->getdevice("buzzer").play(".."); else _gb->getdevice("buzzer").play("--");

        // Send data to GDC
        _gb->getdevice("gdc").send("gdc-db", "gps=" + String(this->data.has_fix) + "," + String(this->data.lat) + "," + String(this->data.lng) + "," + String(this->data.attempts));

        // Return latest data
        return this->data;
    }
}

// Update and read the gps data
GPS_DATA GB_NEO_6M::read() {
    return this->read(false);
}

// Return if a fix was achieved
bool GB_NEO_6M::fix() {
    return this->data.has_fix;
}

// Return the speed information
float GB_NEO_6M::speed() {
    if (_neo.speed.isValid()) {
        return _neo.speed.mph();
    }
    
    return 0;
}

// Read the GPS data
void GB_NEO_6M::_read_nmea() {

    // Clear the serial buffer
    for (unsigned long start = millis(); millis() - start < 500;) while (_gb->serial.hardware->available()) char c = _gb->serial.hardware->read();

    // Read data from GPS over serial
    for (unsigned long start = millis(); millis() - start < 2000;) {
        while (_gb->serial.hardware->available()) {
            char c = _gb->serial.hardware->read();
            // _gb->log(c, false);
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
    _gb->getmcu().watchdog("enable");
    

    // Read the serial data
    this->_read_nmea();
    
    // Allow more attempts if the device just started collecting data.
    if (_gb->globals.ITERATION < 5) this->MAX_ATTEMPTS = 40;
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

    ) 
    {
        
        // Reset watchdog timer
        _gb->getmcu().watchdog("reset");
        
        _gb->log("  Attempt: " + String(counter + 1) + ", Valid: " + String(_neo.location.isValid() ? "Yes" : "No") + ", Updated: " + String(_neo.location.isUpdated() ? "Yes" : "No"));
        
        // // Log to SD card
        // if (_gb->hasdevice("sd")) _gb->getdevice("sd").debug("operations", "  Attempt: " + String(counter + 1) + ", Valid: " + String(_neo.location.isValid() ? "Yes" : "No") + ", Updated: " + String(_neo.location.isUpdated() ? "Yes" : "No"));

        // Read the serial data
        this->_read_nmea();

        // Toggle led
        if (lasttoggle == 0 || millis() - lasttoggle > 1000) {
            if (_gb->hasdevice("rgb") && ledstate) _gb->getdevice("rgb").on("yellow");
            else _gb->getdevice("rgb").off();
            ledstate = !ledstate;
        }
        
        // Increment the counter
        counter++;
    }


    // Log to console
    _gb->log("Looped out, Valid: " + String(_neo.location.isValid() ? "Yes" : "No") + ", Updated: " + String(_neo.location.isUpdated() ? "Yes" : "No"));


    // If we have a valid GPS location set data
    if (_neo.location.isValid() && _neo.location.isUpdated()){
        this->_last_fix_timestamp = millis();

        this->data.has_fix = true;
        this->data.valid = _neo.location.isValid();
        this->data.updated = _neo.location.isUpdated();
        this->data.lat = _neo.location.lat();
        this->data.lng = _neo.location.lng();
        this->data.satellites = (_neo.satellites.isValid() ? _neo.satellites.value() : 0);
        this->data.hdop = _neo.hdop.value();
        this->data.accuracy = (_neo.hdop.isValid() ? (this->data.hdop < 2 ? "accurate" : (this->data.hdop < 5 ? "moderate" : "poor")) : "unknown");
        this->data.age = _neo.location.age();
        this->data.date = 0;
        this->data.time = 0;
        this->data.id = 0;
        this->data.attempts = counter;
        this->data.speed = _neo.speed.mph();

        // INFO:
        // The following code causes the FLASH memory to overflow
        
        // // Calculate distance moved
        // float distancemoved = _neo.distanceBetween(this->_prev_lat, this->_prev_lng, this->data.lat, this->data.lng);

        // // Check if distance moved is greater than the threshold
        // if (distancemoved > MOVEMENT_THRESHOLD) {
        //     this->_prev_lat = this->data.lat;
        //     this->_prev_lng = this->data.lng;
        // }

        // // Check if 5 minutes have passed without movement
        // if (millis() - this->_last_fix_timestamp > MOVEMENT_CHECK_INTERVAL) {
        //     Serial.println("No movement detected in 5 minutes.");
        // }
    }
    else this->reset();
    
    
    // Disable watchdog timer
    _gb->getmcu().watchdog("disable");
    
    this->off();
    return this->data.has_fix;
}

#endif