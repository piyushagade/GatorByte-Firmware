#ifndef GB_PWRMTR_h
#define GB_PWRMTR_h

/* Import the core GB library */
#ifndef GB_h
    #include "../GB.h"
#endif

class GB_PWRMTR : public GB_DEVICE {
    public:
               
        GB_PWRMTR(GB &gb);

        DEVICE device = {
            "powermeter",
            "GatorByte Power meter"
        };

        void write(String message);
        int read();
        GB_PWRMTR& initialize(int address);

    private:
        GB *_gb;
        int _address = 4;
};

// Constructor
GB_PWRMTR::GB_PWRMTR(GB &gb) { 
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.pwr = this;
}


GB_PWRMTR& GB_PWRMTR::initialize(int address) { 
    _gb->log("Initializing " + this->device.name, false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    this->_address = address;
    _gb->log(" -> Address set to " + String(this->_address));
    return *this;
}

void GB_PWRMTR::write(String string) { 
    char* cstring = _gb->s2c(string);
    int len = sprintf(cstring, "%s", cstring);

    for (int i = 0; i < len; i++) {
        Wire.beginTransmission(4);
        Wire.write(cstring[i]);
        Wire.endTransmission();
        delay(20);
    }
    Wire.beginTransmission(this->_address);
    Wire.write("\n");
    Wire.endTransmission();
    delay(100); 
}

int GB_PWRMTR::read() { 
    int reading = 0;
    return reading;
}

#endif