#ifndef GB_BOOSTER_h
#define GB_BOOSTER_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_BOOSTER {
    public:
        GB_BOOSTER(GB &gb);

        DEVICE device = {
            "booster",
            "5V booster"
        };

        struct PINS {
            bool mux;
            int enable;
        } pins;

        GB_BOOSTER& initialize();
        GB_BOOSTER& configure(PINS);
        GB_BOOSTER& sync(char[], char[]);
        GB_BOOSTER& on();
        GB_BOOSTER& off();

        String timestamp();
        String date();
        String time();

    private:
        GB *_gb;
};

GB_BOOSTER::GB_BOOSTER(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
}

GB_BOOSTER& GB_BOOSTER::configure(PINS pins) {
    this->on(); 
    this->pins = pins;
    _gb->includedevice(this->device.id, this->device.name);
    
    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    this->off();
    return *this;
}


GB_BOOSTER& GB_BOOSTER::on() {
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    return *this;
}

GB_BOOSTER& GB_BOOSTER::off() {
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    return *this;
}

#endif