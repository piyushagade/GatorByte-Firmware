#ifndef GB_SIM7XXX_h
#define GB_SIM7XXX_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_SIM7XXX {
    public:
        GB_SIM7XXX(GB &gb);
        
        DEVICE device = {
            "sim7xxx",
            "SIM7XXX LTE module",
            "SIM7XXX family of external LTE modules",
            "SIMCom"
        };

        struct PINS {
            bool mux;
            int enable;
        } pins;

        void test();
        GB_SIM7XXX& initialize();
        GB_SIM7XXX& configure(PINS);
        GB_SIM7XXX& setup(String name, String pass);
        GB_SIM7XXX& on();
        GB_SIM7XXX& off();

    private:
        GB *_gb;
        PCF8575 _ioe = PCF8575(0x20);
        int _baud = 9600;
        int _at_baud = 38400;
};

GB_SIM7XXX::GB_SIM7XXX(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
}

// Initialize the module
GB_SIM7XXX& GB_SIM7XXX::initialize() { 
    this->on();
    _gb->log("Initializing SIM7XXX module", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    _gb->log(" -> Done");
    this->off();
    return *this;
}

// Configure device pins
GB_SIM7XXX& GB_SIM7XXX::configure(PINS pins) {
    this->pins = pins;

    // Set pin mode
    if(this->pins.mux) _ioe.pinMode(this->pins.enable, OUTPUT);
    else pinMode(this->pins.enable, OUTPUT);

    return *this;
}

GB_SIM7XXX& GB_SIM7XXX::setup(String name, String pass) {
    _gb->log("\nTo configure Bluetooth name and pin, press and hold \"reset\" button on HC-05 before the countdown ends.");
    for(int i = 5; i >= 0; i--) { delay(1000); _gb->log(String(i) + "  ", false); }
    _gb->log("\n");
    this->on();delay(500);

    // Set device BL name
    this->_send_at_command("AT+NAME=" + String(name));

    // Set device BL PIN
    this->_send_at_command("AT+PSWD=" + String(pass));

    this->off();
    delay(500);
    return *this;
}

GB_SIM7XXX& GB_SIM7XXX::on() {
    if(this->pins.mux) _ioe.write(this->pins.enable, HIGH); 
    else digitalWrite(this->pins.enable, HIGH); 
    return *this;
}

GB_SIM7XXX& GB_SIM7XXX::off() {
    if(this->pins.mux) _ioe.write(this->pins.enable, LOW); 
    else digitalWrite(this->pins.enable, LOW); 
    return *this;
}

void GB_SIM7XXX::print(String message) {
    this->log(message, true);
}

void GB_SIM7XXX::print(String message, bool new_line) {
    if(digitalRead(this->pins.enable) == LOW) this->on();
    if(new_line) this->_gb->serial.hardware->println(message);
    else this->_gb->serial.hardware->print(message);
    this->off();
}

void GB_SIM7XXX::_send_at_command(String command) {
    _gb->serial.hardware->end();delay(100); 
    _gb->serial.hardware->begin(_at_baud);delay(100);
    _gb->serial.hardware->write(command + "\r");delay(1000);
    this->off();delay(500);this->on();delay(500);

    _gb->serial.hardware->flush();
    _gb->serial.hardware->end();delay(100);
    _gb->serial.hardware->begin(_baud);delay(100);
}

#endif