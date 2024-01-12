#ifndef GB_SR04_h
#define GB_SR04_h

#ifndef GB_h
    #include "../../GB.h"
#endif

class GB_SR04 {
    public:
        GB_SR04(GB&);
        
        DEVICE device = {
            "sr04",
            "SR04 Ultrasonic sensor"
        };
        
        struct PINS {
            int enable;
            int data;
            bool mux;
        } pins;

        GB_SR04& configure(PINS);
        GB_SR04& on();
        GB_SR04& off();
        float read();

    private:
        GB *_gb;
};

GB_SR04::GB_SR04(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
}

GB_SR04& GB_SR04::configure(PINS) { 
    this->pins = pins;

    // Set pin modes
    pinMode(this->pins.data, INPUT);
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);
    return *this;
}

GB_SR04& GB_SR04::on() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH); 
    return *this;
}

GB_SR04& GB_SR04::off() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, HIGH);
    return *this;
}

float GB_SR04::read() {
    this->on();
    delay(100);
    float mm = pulseIn(this->pins.data, HIGH);
    float inch = mm * 0.0393701;
    return inch;
    this->off();
}

#endif