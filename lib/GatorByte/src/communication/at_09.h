#ifndef GB_AT_09_h
#define GB_AT_09_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_AT_09 : public GB_DEVICE {
    public:
        GB_AT_09(GB &gb);
        
        DEVICE device = {
            "bl",
            "AT-09"
        };

        struct PINS {
            bool mux;
            int enable;
            int comm;
        } pins;

        GB_AT_09& initialize();
        bool initialized();
        GB_AT_09& configure(PINS);
        GB_AT_09& setup(String name, String pass);
        GB_AT_09& persistent();
        GB_AT_09& on();
        GB_AT_09& on(String);
        GB_AT_09& off();
        GB_AT_09& off(String);
        
        bool testdevice();
        String status();

        typedef void (*callback_t_func)(String);
        GB_AT_09& listen(callback_t_func);

        GB_AT_09& setname(String);
        GB_AT_09& setpin(String);
        String getname();
        String getpin();

        GB_AT_09& br();
        void print(String message);
        void print(String message, bool new_line);
        typedef void(*handler_t) (String);

        // AT commands
        String read_at_command_response();
        String send_at_command(String command);

    private:
        void _purge_buffer();
        GB *_gb;
        String _name;
        String _pin;
        int _baud = 9600;
        int _at_baud_hc05 = 38400;
        int _at_baud_at09 = 9600;
        bool _persistent = false;
        bool _initialized = false;
};

GB_AT_09::GB_AT_09(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.bl = this;
}

// Test the device
bool GB_AT_09::testdevice() { 
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_AT_09::status() { 

    if (this->device.detected) {
        String namedata = this->send_at_command("AT+NAME");
        String name = namedata.substring(namedata.indexOf('=') + 1, namedata.length());
        this->off("comm");
        return name;
    }
    return "not-detected" + String(":") + device.id;
}

// Initialize the module
GB_AT_09& GB_AT_09::initialize() { 
    _gb->init();

    this->on();
    _gb->log("Initializing AT-09 BL module", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    // Disable stray watchdog timers
    this->_gb->getmcu().watchdog("disable");

    // Begin serial communication
    _gb->serial.hardware->begin(this->_baud);

    // Clear buffer
    this->_purge_buffer();
    // Test connection
    bool error = this->send_at_command("AT") != "OK";
    
    this->device.detected = !error;
    this->_initialized = !error;

    if (this->device.detected) {
        
        if (_gb->globals.GDC_CONNECTED) {
            _gb->log(" -> Done");
            this->off("comm");
            return *this;
        }

        String namedata = this->send_at_command("AT+NAME");
        String name = namedata.substring(namedata.indexOf('=') + 1, namedata.length());
        
        String pindata = this->send_at_command("AT+PIN");
        String pin = pindata.substring(pindata.indexOf('=') + 1, pindata.length());
        
        if (name == "BT05") {
            _gb->log(" -> Default name detected.", false);

            if (_gb->globals.DEVICE_NAME.length() > 0) {
                
                _gb->log(" -> Changing to 'GB/" + _gb->globals.DEVICE_NAME + "'", false);

                delay(100); bool namesuccess = this->send_at_command("AT+NAMEGB/" + _gb->globals.DEVICE_NAME) != "ERROR";
                delay(500);
                namedata = this->send_at_command("AT+NAME");
                name = namedata.substring(namedata.indexOf('=') + 1, namedata.length());
                this->_name = name;

                delay(100); bool pinsuccess = this->send_at_command("AT+PIN5555") != "ERROR";
                delay(100);
                pindata = this->send_at_command("AT+PIN");
                pin = pindata.substring(pindata.indexOf('=') + 1, pindata.length());
                this->_pin = pin;

                _gb->log(namesuccess && pinsuccess ? " -> Success" : " -> Failed", false);
            }
        }

        if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("-..").wait(250).play("-..");
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("green").wait(250).revert(); 

        _gb->log(" -> Name: " + name, false);
        _gb->log(", PIN: " + pin);
    }

    else {
        _gb->log(" -> Not detected");

        if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("-..").wait(250).play("-..").wait(250).play("---");
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("green").wait(250).revert(); 
    }

    this->off("comm");
    
    return *this;
}

bool GB_AT_09::initialized() { 
    return this->_initialized;
}

// Configure device pins
GB_AT_09& GB_AT_09::configure(PINS pins) {
    this->pins = pins;

    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    return *this;
}

GB_AT_09& GB_AT_09::setname(String name) {
    this->on();
    delay(100);

    _gb->log("Updating BL module's name", false);
    
    // Set device BL name
    this->send_at_command("AT+NAME" + name);
    this->_name = name;
    
    _gb->log(" -> Done");
    return *this;
}

GB_AT_09& GB_AT_09::setpin(String pin) {
    this->on();
    delay(100);

    _gb->log("Updating BL module's PIN", false);
    this->_pin = pin;
    
    // Set device BL name
    this->send_at_command("AT+PIN" + pin);
    
    _gb->log(" -> Done");
    return *this;
}

String GB_AT_09::getname() {
    return this->_name;
}

String GB_AT_09::getpin() {
    return this->_pin;
}

//TODO:Can't change name and pin together
GB_AT_09& GB_AT_09::setup(String name, String pin) {
    _gb->log("Updating BL module's name and PIN", false);
    this->on();delay(500);
    
    // Set device BL name
    this->send_at_command("AT+NAME" + name);delay(500);
    this->_name = name;

    // Set device BL PIN
    this->send_at_command("AT+PIN" + pin);delay(500);
    this->_pin = pin;

    _gb->log(" -> Done");

    this->off("comm");
    return *this;
}

/*
    Keeps device persistently on
*/
GB_AT_09& GB_AT_09::persistent() {
    this->_persistent = true;
    return *this;
}

// Turn on the module
GB_AT_09& GB_AT_09::on(String type) {
    if(this->pins.mux) {
        if (type == "power") _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
        if (type == "comm") _gb->getdevice("ioe").writepin(this->pins.comm, HIGH);
    }
    else {
        if (type == "power") digitalWrite(this->pins.enable, HIGH);
        if (type == "comm") digitalWrite(this->pins.comm, HIGH);
    }
}

GB_AT_09& GB_AT_09::on() {
    this->on("comm");
    this->on("power");
    return *this;
}

// Turn off the module
GB_AT_09& GB_AT_09::off(String type) {
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
GB_AT_09& GB_AT_09::off() {
    this->off("comm");
    this->off("power");
    return *this;
}

GB_AT_09& GB_AT_09::listen(callback_t_func callback) {
    if (this->_gb->serial.hardware->available()) {
        String command = this->_gb->serial.hardware->readStringUntil('\n');
        command.trim();
        if (command != "E" && command != "ERROR" && command != "EERROR") callback (command);
    }
    return *this;
}

GB_AT_09& GB_AT_09::br() {
    this->print("", true);
    return *this;
}

void GB_AT_09::print(String message) {
    this->print(message, true);
}

void GB_AT_09::print(String message, bool new_line) {

    // Power on the device if not already
    if(digitalRead(this->pins.enable) == LOW) this->on("power");

    // Enable communication
    this->on("comm");
    
    // Print the message
    if(new_line) this->_gb->serial.hardware->println(message);
    else this->_gb->serial.hardware->print(message);

    // Turn off the communication only
    this->off("comm");
}

String GB_AT_09::read_at_command_response() {
    String response;
    int counter = 0;
    int start = millis();
    while (!_gb->serial.hardware->available() && counter++ < 200) delay(5);
    
    response = _gb->serial.hardware->readStringUntil('\n');
    response.trim();

    if (response != "ERROR" && response.length() != 3  && response.length() != 0) return response;

    while (response.length() == 3) {
        delay(200);
        response = _gb->serial.hardware->readStringUntil('\n');
        response.trim();
    }
    
    if (response != "ERROR" && response.length() != 3  && response.length() != 0) return response;

    while (_gb->serial.hardware->available() && millis() - start < 10000) {
        response = _gb->serial.hardware->readStringUntil('\n');
        response.trim();
    }
    
    return response;
}

String GB_AT_09::send_at_command(String command) {
    this->on(); delay(100);
    _gb->serial.hardware->begin(_at_baud_at09); delay(100);
    _gb->serial.hardware->println(command); delay(100);
    return this->read_at_command_response();
}

void GB_AT_09::_purge_buffer() {
    String response;
    int counter = 0;
    int start = millis();
    while (!_gb->serial.hardware->available() && counter++ < 200) delay(5);
    
    response = _gb->serial.hardware->readStringUntil('\n');
    response.trim();
    while (response == "ERROR" || response.length() == 3) {
        delay(200);
        response = _gb->serial.hardware->readStringUntil('\n');
        response.trim();
    }
}

#endif