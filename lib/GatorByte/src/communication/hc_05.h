#ifndef GB_HC_05_h
#define GB_HC_05_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_HC_05 : public GB_DEVICE {
    public:
        GB_HC_05(GB &gb);
        
        DEVICE device = {
            "bl",
            "HC-05"
        };

        struct PINS {
            bool mux;
            int enable;
            int comm;
        } pins;

        bool testdevice();
        GB_HC_05& initialize();
        bool initialized();
        GB_HC_05& configure(PINS);
        GB_HC_05& setup(String name, String pass);
        GB_HC_05& persistent();
        GB_HC_05& on();
        GB_HC_05& off();

        typedef void (*callback_t_func)(String);
        GB_HC_05& listen(callback_t_func);

        void print(String message);
        void print(String message, bool new_line);
        typedef void(*handler_t) (String);

        // void listen(handler_t handler);

    private:
        String _read_at_command_response();
        void _send_at_command(String command);
        GB *_gb;
        int _baud = 9600;
        int _at_baud_hc05 = 38400;
        int _at_baud_at09 = 9600;
        bool _persistent = false;
        bool _initialized = false;
};

GB_HC_05::GB_HC_05(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.bl = this;
}

// Initialize the module
GB_HC_05& GB_HC_05::initialize() { 
    this->on();
    _gb->log("Initializing Bluetooth module", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    _gb->serial.hardware->begin(this->_baud);
    _gb->log(" -> Done");
    
    // Test connection
    this->_send_at_command("AT");

    Serial.print("Response: ");
    Serial.println(this->_read_at_command_response());

    this->_initialized = true;

    this->off();
    return *this;
}

bool GB_HC_05::initialized() { 
    return this->_initialized;
}

// Configure device pins
GB_HC_05& GB_HC_05::configure(PINS pins) {
    this->pins = pins;

    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    return *this;
}

GB_HC_05& GB_HC_05::setup(String name, String pass) {
    _gb->log("\nTo configure Bluetooth name and pin, press and hold \"reset\" button on HC-05 before the countdown ends.");
    for(int i = 3; i >= 0; i--) { delay(1000); _gb->log(String(i) + "  ", false); }
    _gb->log("\n");
    this->on();delay(500);

    // // Test connection
    // this->_send_at_command("AT");
    // _gb->log(this->_read_at_command_response());

    // Set device BL name
    this->_send_at_command("AT+NAME=" + String(name));

    // Set device BL PIN
    this->_send_at_command("AT+PSWD=" + String(pass));

    this->off();
    delay(500);
    return *this;
}

/*
    Keeps device persistently on
*/
GB_HC_05& GB_HC_05::persistent() {
    this->_persistent = true;
    return *this;
}

GB_HC_05& GB_HC_05::on() {
    if(this->pins.mux) {
        _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
        _gb->getdevice("ioe").writepin(this->pins.comm, HIGH);
    }
    else {
        digitalWrite(this->pins.enable, HIGH);
        digitalWrite(this->pins.comm, HIGH);
    }
    return *this;
}

GB_HC_05& GB_HC_05::off() {
    if (this->_persistent) return *this;

    if(this->pins.mux) {
        _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
        _gb->getdevice("ioe").writepin(this->pins.comm, LOW);
    }
    else {
        digitalWrite(this->pins.enable, LOW);
        digitalWrite(this->pins.comm, LOW);
    }
    return *this;
}

GB_HC_05& GB_HC_05::listen(callback_t_func callback) {
    if (this->_gb->serial.hardware->available()) {
        callback (this->_gb->serial.hardware->readString());
    }
}

void GB_HC_05::print(String message) {
    this->print(message, true);
}

void GB_HC_05::print(String message, bool new_line) {
    if(digitalRead(this->pins.enable) == LOW) this->on();
    if(new_line) this->_gb->serial.hardware->println(message);
    else this->_gb->serial.hardware->print(message);
    this->off();
}

String GB_HC_05::_read_at_command_response() {
    String response;
    while (_gb->serial.hardware->available()) {
        response = _gb->serial.hardware->readString();
        break;
    }
    return response;
}

void GB_HC_05::_send_at_command(String command) {
    this->on();
    Serial.println("Here 1");
    _gb->serial.hardware->end();delay(100);
    _gb->serial.hardware->begin(_at_baud_hc05);delay(100);
    _gb->serial.hardware->print(command + "\r\n");delay(1000);    //! AG: Changed .write to .print
    Serial.println("Here 2");
    this->off();delay(100);this->on();delay(100);
    // _gb->serial.hardware->flush() ;
    _gb->serial.hardware->end();delay(100);
    _gb->serial.hardware->begin(_baud);delay(100);
    Serial.println("Here 3");
}


#endif