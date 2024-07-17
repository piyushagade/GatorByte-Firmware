#ifndef GB_SNTL_h
#define GB_SNTL_h

/* Import the core GB library */
#ifndef GB_h
    #include "../GB.h"
#endif

class GB_SNTL : public GB_DEVICE {
    public:
        GB_SNTL(GB &gb);
        bool debug = false;

        struct PINS {
            bool commux;
            int muxchannel;
        } pins;

        DEVICE device = {
            "sntl",
            "Sentinel"
        };

        GB_SNTL& timer(uint8_t);
        GB_SNTL& enable();
        GB_SNTL& enable(bool);
        GB_SNTL& disable();
        GB_SNTL& disable(bool);
        GB_SNTL& enablebeacon(uint8_t);
        GB_SNTL& disablebeacon();
        GB_SNTL& triggerbeacon();
        GB_SNTL& blow();

        GB_SNTL& formatmemory();
        uint16_t readmemory(int);

        GB_SNTL& interval(String, int);
        uint16_t tell(int);
        uint16_t tell(int, int);
        GB_SNTL& setack(bool);
        uint16_t ask();
        GB_SNTL& configure();
        GB_SNTL& configure(PINS pins, int address);
        GB_SNTL& initialize();
        
        bool testdevice();
        String status();
        
        GB_SNTL& sauron(bool enable);

        typedef void (*callback_t_func)();
        GB_SNTL& watch(int, callback_t_func);
        GB_SNTL& watch(int, callback_t_func, bool);
        GB_SNTL& kick();
        
        uint16_t reboot();
        uint16_t shutdown();
        GB_SNTL& fetchfaultcounters();
        uint16_t resetfaultcounters();
        bool ping();
        float fwversion();
        
    private:
        GB *_gb;
        int _address = 0x9;
        bool _logresponse = false;
        bool _initialized = false;
        bool _no_ack = false;
        float _fw_version = 0;
        uint8_t _comm_attempts = 0;

        uint16_t _tell(int, int, int);
        bool _parse_response(int);
        uint16_t _lockconfig();
        uint16_t _unlockconfig();
        void _enablelog();
        void _disablelog();
};

GB_SNTL::GB_SNTL(GB &gb) { 
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.sntl = this;
}

/*
    Configure Sentinel library instance
*/
GB_SNTL& GB_SNTL::configure() { 
    return *this;
}
GB_SNTL& GB_SNTL::configure(PINS pins, int address) { 
    this->_address = address;
    this->pins = pins;
    return *this;
}

/*
    Initialize device
*/
GB_SNTL& GB_SNTL::initialize() { 
    _gb->init();
    
    _gb->log("Initializing " + this->device.name, false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    //! Begin I2C
    Wire.begin();

    // Disable stray watchdogs
    _gb->getmcu()->watchdog("disable");

    //! Send ping to Sentinel
    int counter = 1; 
    if (_gb->globals.GDC_CONNECTED) {
        counter = 0;
    }
    bool success = this->ping();
    while (!success && counter-- >= 0) { delay(250); success = this->ping(); }
    this->device.detected = success;

    _gb->arrow().log(success ? "Done" : "Not detected", false);

    if (_gb->globals.GDC_CONNECTED) {
        _gb->log();

        if (success) this->disable();
        return *this;
    }
    
    if (_gb->hasdevice("buzzer"))_gb->getdevice("buzzer")->play(success ? "x.x.." : "xx");
    if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on(success ? "green" : "red").wait(250).revert(); 

    //! Turn off sentinence on initialize
    if (success) {
        this->disable();
    }

    //! Get Sentinel firmware version, and fault counts
    if (success) {

        // Get F/W version
        delay(0);
        this->fwversion();
        _gb->log(success ? " (F/W v" + String(this->_fw_version) + "), " : ", ", false);

        // // Fetch fault counters
        // this->fetchfaultcounters();

        // // int primaryfaultcounter; while (primaryfaultcounter > 45000) { primaryfaultcounter = this->tell(0x8, 1); delay(100); }
        // // int secondaryfaultcounter; while (secondaryfaultcounter > 45000) { secondaryfaultcounter = this->tell(0x9, 1); delay(100); }

        // // _gb->globals.FAULTS_PRIMARY = primaryfaultcounter;
        // // _gb->globals.FAULTS_SECONDARY = secondaryfaultcounter;

        // // _gb->arrow().log("Fault counters: " + String(_gb->globals.FAULTS_PRIMARY) + ", " + String(_gb->globals.FAULTS_SECONDARY));

        // Get ACK status (for the location refer to Sentinel's F/W)
        uint8_t ackstatuslocation = 0x0E;
        this->_no_ack = this->readmemory(ackstatuslocation) == 0;

        _gb->log("ACK: " + String(this->_no_ack ? " Disabled" : " Enabled"));
        
    }

    else {
        _gb->arrow().log("Failed");
        _gb->globals.INIT_REPORT += this->device.id;
    }
    // this->_enablelog();
    
    return *this;
}

// Test the device
bool GB_SNTL::testdevice() { 
    this->device.detected = this->ping(); delay(100);
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));

    return this->device.detected;
}
String GB_SNTL::status() {
    
    String fwinfo;
    if (this->device.detected) {
    
        fwinfo = this->fwversion();
        Serial.println(fwinfo);
    
        int code = 0x16;
        this->tell(code, 5);
    }

    return this->device.detected ? String(fwinfo) : "not-detected" + String(":") + device.id;
}

/*
    Check communication with the Sentinel
*/
bool GB_SNTL::ping() { 
    
    int code = 0x0;
    int response;
    if (_gb->globals.GDC_CONNECTED) {
        response = this->tell(code, 3);
    } 
    else {
        response = this->tell(code);
    }
    
    if (this->_parse_response(response)) {
        uint16_t secondaryfaultcounter = this->tell(0x9, 15);
        _gb->globals.FAULTS_SECONDARY = secondaryfaultcounter;
    }

    return response == 0x03;
}

/*
    Get Sentinel's firmware version
*/
float GB_SNTL::fwversion() { 
    
    int code = 0x3;
    float response = this->tell(code, 5);
    this->_fw_version = response / 100.00;
    return this->_fw_version;
}

/*
    Request a shutdown
*/
uint16_t GB_SNTL::shutdown() { 
    if (!this->device.detected) {
        _gb->log("Sentinel uninitialized. Skipping shutdown");
        return -1;
    }
    else {
        _gb->log("Sending shutdown command to sentinel", false);
    }
    int code = 0x12;
    uint16_t response = this->tell(code);
    bool success = this->_parse_response(response);
    _gb->arrow().log(success ? "Done (" + String(response) + ")" : "Failed (" + String(response) + ")");
    return success;
}

/*
    Request a reboot
*/
uint16_t GB_SNTL::reboot() { 
    if (!this->device.detected) {
        _gb->br().color("yellow").log("Sentinel uninitialized. Skipping reboot");
        return -1;
    }
    else {

        // Disable sentinence so that the primary isn't turned back on again
        this->disable();
        delay(1000);

        _gb->br().color("red").log("Sending reboot command to sentinel.", false).color();
    
        // Send a reboot request
        int rebootcode = 0x06;
        uint16_t response = this->tell(rebootcode, 1);
        bool success = this->_parse_response(response);
        _gb->arrow().log(success ? "Done (" + String(response) + ")" : "Failed (" + String(response) + ")");

        return success;
    }
}

/*
    Reset fault counters on the Sentinel
*/
uint16_t GB_SNTL::resetfaultcounters() { 
    if (!this->device.detected) return -1;
    _gb->log("Resetting fault counters to 0", false);
    
    int code = 0x07;
    u_int16_t response = this->tell(code);

    _gb->arrow().log(this->_parse_response(response) ? "Done" : "Failed");
    return response;
}

/*
    Get fault counters
*/
GB_SNTL& GB_SNTL::fetchfaultcounters() { 
    if (this->debug) _gb->log("Fetching fault counters", false);
    if (!this->device.detected) { 
        if (this->debug) _gb->arrow().log("Not initialized"); 
        return *this; 
    }

    int primaryfaultcounter = this->tell(0x8, 10); delay(100);
    int secondaryfaultcounter = this->tell(0x9, 10); delay(100);
    // int secondaryfaultcounter = 0;

    _gb->globals.FAULTS_PRIMARY = primaryfaultcounter;
    _gb->globals.FAULTS_SECONDARY = secondaryfaultcounter;

    if (this->debug) _gb->arrow().log("Fault counters: " + String(primaryfaultcounter) + ", " + String(secondaryfaultcounter));

    return *this;
}

/*
    Select timer
*/
GB_SNTL& GB_SNTL::timer(uint8_t timerid) { 
    if (timerid < 0 || timerid > 2) _gb->log("Invalid timer ID: " + String(timerid));
    if (this->debug) _gb->log("Selecting timer ID: " + String(timerid), false);
    if (!this->device.detected) { if (this->debug) _gb->arrow().log("Not initialized"); return *this; }
    
    uint16_t response;
    bool success = false;
    uint8_t code = 33 + timerid;
    response = this->tell(code, 10); 
    success = response == 0;

    int counter = 5;
    while (!success && counter-- >= 0) {
        delay(2000);
        
        if (this->debug) _gb->color("yellow").log(" . ", false).color();
        response = this->tell(code, 5);
        success = response == 0 || response == 6;
    }

    if (this->debug) _gb->arrow().log(success ? "Done (" + String(this->_comm_attempts) + ", " + String(response) + ")" : "Failed (" + String(this->_comm_attempts) + ", " + String(response) + ")");
    if (success) {
        if (this->_comm_attempts >= 3) _gb->globals.LOGPREFIX = "\033[1;30;43m \033[0m";
        else _gb->globals.LOGPREFIX = "\033[1;30;46m \033[0m";
    }
    else _gb->globals.LOGPREFIX = "\033[1;37;41m \033[0m";

    return *this;
}


/*
    Blow the fuse (enable master timer)
*/
GB_SNTL& GB_SNTL::blow() { 
    this->tell(38);
    return *this;
}

/*
    Enable sentinence
*/
GB_SNTL& GB_SNTL::enable() { 
    return enable(true);
}

/*
    Enable sentinence
    If 'stubborn' is set true the function will ensure the sentinence
    is enabled.
*/
GB_SNTL& GB_SNTL::enable(bool stubborn) { 
    if (this->debug) _gb->log("Enabling Sentinel monitor", false);
    if (!this->device.detected) { if (this->debug) _gb->arrow().log("Not initialized"); return *this; }
    
    uint16_t response;
    bool success = false;
    if (!stubborn) {
        response = this->tell(0x1E, 10); 
    }
    else {
        response = this->tell(0x1E, 5); 
        success = response == 0 || response == 6;

        int counter = 5;
        while (!success && counter-- >= 0) {
            delay(2000);
            
            if (this->debug) _gb->color("yellow").log(" . ", false).color();
            response = this->tell(0x1E, 5);
            success = response == 0 || response == 6;
        }
    }

    if (this->debug) _gb->arrow().log(success ? "Done (" + String(this->_comm_attempts) + ", " + String(response) + ")" : "Failed (" + String(this->_comm_attempts) + ", " + String(response) + ")");
    if (success) {
        if (this->_comm_attempts >= 3) _gb->globals.LOGPREFIX = "\033[1;30;43m \033[0m";
        else _gb->globals.LOGPREFIX = "\033[1;30;46m \033[0m";
    }
    else _gb->globals.LOGPREFIX = "\033[1;37;41m \033[0m";

    return *this;
}

/*
    Disable sentinence
*/
GB_SNTL& GB_SNTL::disable() { 
    return this->disable(true);
}
GB_SNTL& GB_SNTL::disable(bool stubborn) { 
    if (this->debug) _gb->log("Disabling Sentinel monitor", false);
    if (!this->device.detected) { if (this->debug) _gb->arrow().log("Not initialized"); return *this; }
    
    uint16_t response;
    bool success = false;

    if (!stubborn) {
        response = this->tell(0x1F, 10);
    }
    else {
        response = this->tell(0x1F, 5); 
        success = response == 0 || response == 7;

        int counter = 5;
        while (!success && counter-- >= 0) {
            delay(2000);
            
            if (this->debug) _gb->color("yellow").log(String(counter) + ". ", false).color();
            response = this->tell(0x1F, 5);
            success = response == 0 || response == 7;
        }
    }

    if (this->debug) _gb->arrow().log(success ? "Done (" + String(this->_comm_attempts) + ", " + String(response) + ")" : "Failed (" + String(this->_comm_attempts) + ", " + String(response) + ")");
    if (success) _gb->globals.LOGPREFIX = "\033[1;30;40m \033[0m";
    
    return *this;
}

/*
    Set sentinence interval (in seconds)
*/
GB_SNTL& GB_SNTL::interval(String type, int seconds) { 

    // Figure out a base and a multiplier that works for the provided interval
    int baseindex, multiplierindex;
    int bases[] = { 1, 5, 15, 15, 30, 150, 90 };
    int multipliers[20];
    for (int i = 0; i < 20; i++) multipliers[i] = i + 1;
    
    if (seconds > bases[sizeof (bases) / sizeof (bases[0]) - 1] * multipliers[sizeof (multipliers) / sizeof (multipliers[0]) - 1]) {
        _gb->log("Interval should be smaller than " + String(bases[sizeof (bases) / sizeof (bases[0]) - 1] * multipliers[sizeof (multipliers) / sizeof (multipliers[0]) - 1]) + " seconds", true);
    }

    bool found = false;
    for (int i = 0; i < sizeof (bases) / sizeof (bases[0]); i++) {
        if (found) continue;
        baseindex = i;
        
        for (int j = 0; j < sizeof (multipliers) / sizeof (multipliers[0]); j++) {
            if (found) continue;
            multiplierindex = j;

            // 12
            if (bases[baseindex] * multipliers[multiplierindex] >= seconds) {
                found = true;
            }
        }
    }

    // Return if not initialized
    if (!this->device.detected) { return *this; }

    int basecommand = -1, multipliercommand = -1;

    // The adjusted interval
    seconds = bases[baseindex] * multipliers[multiplierindex];
    
    // Unlock config
    this->_unlockconfig();
    
    if (type == "sentinence") {
        if (this->debug) _gb->log("Sentinence interval set to " + String(seconds) + " seconds", false);

        // Set interval setting to sentinence
        this->tell(0x0B); delay(200);
    }
    else if (type == "sleep") {
        if (this->debug) _gb->log("Sleep interval set to " + String(seconds) + " seconds", false);
        
        // Set interval setting to power saving mode (sleep)
        this->tell(0x0C); delay(200);
    }

    // Set commands to send
    basecommand = 40 + baseindex;
    multipliercommand = 50 + multiplierindex;

    // Make 5 attempts to send the request
    uint8_t resbase = this->tell(basecommand);
    uint8_t resmult = this->tell(multipliercommand);
    if (this->debug) _gb->arrow().log(this->_parse_response(resbase) && this->_parse_response(resmult) ? "Done (" + String(this->_comm_attempts) + ", " + String(resbase) + ", " + String(resmult) + ")" : "Failed(" + String(this->_comm_attempts) + ", " + String(resbase) + ", " + String(resmult) + ")");

    // Lock config
    this->_lockconfig();

    return *this;
}

/*
    Format Sentinel's EEPROM
*/
GB_SNTL& GB_SNTL::formatmemory() {

    _gb->log("Formatting EEPROM", false);

    // Make 3 attempts to send the request
    uint16_t response = this->tell(0x15, 1);
    delay(8000);
    _gb->arrow().log(response != -1 && response != 65535 ? "Done" : "Failed");

    return *this;
}

/*
    Read EEPROM location
*/
uint16_t GB_SNTL::readmemory(int location) { 

    uint16_t baseindex = 70;
    uint16_t code = baseindex + location;

    // _gb->log("EEPROM read: " + String(location), false);

    // Make 3 attempts to send the request
    uint16_t response = this->tell(code, 1);
    // _gb->arrow().log("" + String(response));

    return response;
}

/*
    One shot, no acknowledge
*/
GB_SNTL& GB_SNTL::setack(bool state) { 
    
    uint8_t code = state ? 0x13 : 0x14;
    this->tell(code, 2); 
    this->_no_ack = !state;
    delay(100);
    return *this;
}

/*
    Send commands to the Sentinel
*/
uint16_t GB_SNTL::tell(int data) { 
    
    // Flush communication channel
    this->ask(); delay(10);

    // Make attempts to send the request
    return this->tell(data, 15); 
}

uint16_t GB_SNTL::tell(int data, int attempts) {  

    // Flush communication channel
    this->ask(); delay(10);

    this->_comm_attempts = 0;
    return this->_tell(data, attempts, attempts);
}

/*
    Read 2 byte response from the Sentinel
*/
uint16_t GB_SNTL::ask() { 

    // Reset mux
    if (this->pins.commux) _gb->getdevice("tca")->resetmux();
    
    // Select I2C mux channel
    if (this->pins.commux) _gb->getdevice("tca")->selectexclusive(pins.muxchannel);

    // Request first byte from Sentinel
    Wire.requestFrom(this->_address, 1);

    // Read low byte
    delay(50); byte low = Wire.read();
    
    // Request second byte from Sentinel
    Wire.requestFrom(this->_address, 1);

    // Read high byte
    delay(50); byte high = Wire.read();

    uint16_t response = (high << 8) | low;

    // Combine bytes into uint16_t
    return response;
}


/*
    Enable the Eye of Sauron
*/
GB_SNTL& GB_SNTL::sauron(bool enable) { 
    return enable ? this->timer(3).enable().timer(0) : this->timer(3).disable().timer(0);
}

/*
    Monitor function/scope
    Duration in seconds
*/
GB_SNTL& GB_SNTL::watch(int duration_sec, callback_t_func function) { 
    return this->watch(duration_sec, function, true);
}
GB_SNTL& GB_SNTL::watch(int duration_sec, callback_t_func function, bool stubborn) { 
    delay(100); 
    
    // Set sentinence interval and enable sentinel
    // this->disable(); delay(20);
    
    this->interval("sentinence", duration_sec); delay(100);
    this->enable(stubborn); delay(100);

    // Call the function/scope/block
    function(); delay(100);

    // Disable the sentinel monitor
    this->disable(stubborn); delay(100);

    return *this;
}

GB_SNTL& GB_SNTL::kick() { 
    
    // Set the sentinel's timer to 0, and reset the sentinel
    this->disable();
    this->enable();
    
    // // _gb->log("Kicking Sentinel monitor", false);
    // if (!this->device.detected) { 
    //     // _gb->arrow().log("Not initialized"); 
    //     return *this; 
    // }
    // uint16_t response = this->tell(0x20, 5); 
    // // _gb->arrow().log(response != -1 && response != 65535 ? "Done (" + String(response) + ")" : "Failed (" + String(response) + ")");
    
    return *this;
}


/*
    Enable beacon
*/
GB_SNTL& GB_SNTL::enablebeacon(uint8_t mode) { 
    _gb->log("Enabling beacon", false);
    if (!this->device.detected) { _gb->arrow().log("Not initialized"); return *this; }
    
    // Enable beacon
    uint16_t enresponse = this->tell(0x0F, 8); 

    // Set mode
    bool validmode = 0 <= mode && mode <= 3;
    if (!validmode) {
        _gb->arrow().log("Invalid mode provided.", false);
        mode = 1;
    }
    else {
        _gb->arrow().log("Setting mode: " +  String(mode), false);
    }
    mode += 100;
    uint16_t moderesponse = this->tell(mode, 5); 
    _gb->arrow().log(this->_parse_response(enresponse) && this->_parse_response(moderesponse) ? "Done (" + String(enresponse) + ", " + String(moderesponse) + ")" : "Failed (" + String(enresponse) + ", " + String(moderesponse) + ")");

    return *this;
}

/*
    Disable beacon
*/
GB_SNTL& GB_SNTL::disablebeacon() { 
    _gb->log("Disabling beacon", false);
    if (!this->device.detected) { _gb->arrow().log("Not initialized"); return *this; }
    uint16_t response = this->tell(0x10, 2); 
    _gb->arrow().log(this->_parse_response(response) ? "Done (" + String(response) + ")" : "Failed (" + String(response) + ")");

    return *this;
}


/*
    Trigger beacon
*/
GB_SNTL& GB_SNTL::triggerbeacon() { 
    _gb->log("Triggering beacon", false);
    if (!this->device.detected) { _gb->arrow().log("Not initialized"); return *this; }
    uint16_t response = this->tell(0x11, 5); 
    _gb->arrow().log(response != -1 && response != 65535 ? "Done (" + String(response) + ")" : "Failed (" + String(response) + ")");

    return *this;
}

/*
    Send a command to the Sentinel
*/
uint16_t GB_SNTL::_tell(int data, int maxattempts, int currentattempt) {
    this->_comm_attempts++;

    bool logcondition = false && (data == 0x00 || data == 0x09); 
    // logcondition = true || (data == 0x00 || data == 0x09); 

    int8_t numberofattempts = maxattempts - currentattempt;
    if (logcondition) {
        _gb->log(String(data) + ": " + String(numberofattempts <= 3 ? 250 : (numberofattempts <= 6 ? 250 : (numberofattempts <= 10 ? 500 : (numberofattempts <= 12 ? 1500 : 2000)))) + ": ", false);
    }
    
    // Reset mux
    if (this->pins.commux) _gb->getdevice("tca")->resetmux();
    
    // Select I2C mux channel
    if (this->pins.commux) _gb->getdevice("tca")->selectexclusive(pins.muxchannel);
    
    // If maximum number of attempts have been made, return.
    if (currentattempt < 0) return 1;

    unsigned long start = millis();

    // Convert the int to byte; allowing any number between 0 and 127 (both including) be sent as a byte
    byte b = (byte) data;

    // Try sending/resending the data
    delay(100);
    Wire.beginTransmission(this->_address);
    Wire.write(b);
    Wire.endTransmission();
    delay(10);

    if (!this->_no_ack) {

        // If ack not received, try sending again
        uint16_t x = this->ask();

        if (logcondition) {
            _gb->log(String(x));
        }

        int end = millis();
        // if (this->debug) _gb->log("Reception time w/ ACK: " + String(end - start) + " ms");

        // If Sentinel is not connected/communicating
        if (x == 65535) {
            delay(1000);

            // Send command again
            Wire.beginTransmission(this->_address);
            Wire.write(b);
            Wire.endTransmission();
            delay(10);
            uint16_t x = this->ask();
            return x;
        }

        // The reponse codes are 0 to 7, firmware version can be anything
        if (x > 1000) {
            // delay(numberofattempts <= 3 ? 250 : (numberofattempts <= 6 ? 250 : (numberofattempts <= 10 ? 500 : (numberofattempts <= 12 ? 1500 : 2000))));
            delay(numberofattempts <= 7 ? 0 : (numberofattempts <= 14 ? 250 : 500));
            return this->_tell(data, maxattempts, currentattempt - 1);
        }
        return x;
    }
    else {

        // _gb->log(" No response expected ", false);

        // // Reset noack state
        // this->_no_ack = false;

        unsigned long end = millis();
        // _gb->log("Reception time w/o ACk: " + String(end - start) + " ms");

        return 65534;
    }
}

/*
    Parse sentinel's response
*/
bool GB_SNTL::_parse_response(int response) { 
    bool error = response == 1 || response == 255 || response > 65000;
    return !error;
}

/*
    Unlock config on Sentinel
*/
uint16_t GB_SNTL::_unlockconfig() { 
    
    if (!this->device.detected) return-1;

    uint8_t code = 0x01;
    uint16_t response = this->tell(code, 5);
    return response;
}

/*
    Lock config on Sentinel
*/
uint16_t GB_SNTL::_lockconfig() { 
    
    if (!this->device.detected) return -1;

    uint8_t code = 0x02;
    uint16_t response = this->tell(code, 5);
    return response;
}

void GB_SNTL::_enablelog() { 
    this->_logresponse = true;
}

#endif