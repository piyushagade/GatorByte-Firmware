#ifndef GB_MPU6050_h
#define GB_MPU6050_h

#ifndef GB_h
    #include "../../GB.h"
#endif

#ifndef _MPU6050_H_
    #include "../../lib/MPU6050/src/MPU6050.h"
#endif

class GB_MPU6050 : public GB_DEVICE {
    public:
        GB_MPU6050(GB&);

        DEVICE device = {
            "mpu6050",
            "Accelerometer"
        };

        struct PINS {
            bool mux;
            int enable;
        };
        PINS pins;

        GB_MPU6050& configure(PINS);
        GB_MPU6050& initialize();
        GB_MPU6050& mpucalibrate(String type);
        GB_MPU6050& on();
        GB_MPU6050& off();

        int16_t read(String type, String direction);

    private:

        MPU6050 _acc = MPU6050(0x69);
        bool _rebegin_on_restart = false;
        GB *_gb;
        
        GB_MPU6050& update();
        int16_t _ax, _ay, _az, _gx, _gy, _gz;
};

GB_MPU6050::GB_MPU6050(GB &gb) {
    _gb = &gb;
    _gb->devices.acc = this;
}

GB_MPU6050& GB_MPU6050::configure(PINS pins) { 
    this->pins = pins;

    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);
    
    return *this;
}

// Turn on the module
GB_MPU6050& GB_MPU6050::on() {
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    
    // Initialize object
    if (this->_rebegin_on_restart) {
        _acc.initialize();
    }

    return *this;
}

// Turn off the module
GB_MPU6050& GB_MPU6050::off() {
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    this->_rebegin_on_restart = true;
    return *this;
}

GB_MPU6050& GB_MPU6050::initialize() { 
    _gb->log("Initializing accelerometer", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    this->on();
    // _acc.initialize(); 
    bool success = _acc.testConnection();
    _gb->log(success ? " -> Done": " -> Failed");
    this->off();
    return *this;    
}

GB_MPU6050& GB_MPU6050::update() {
    this->on();
    _acc.getMotion6(&_ax, &_ay, &_az, &_gx, &_gy, &_gz);
    this->off();
    return *this;
}

GB_MPU6050& GB_MPU6050::mpucalibrate(String type) {
    this->on();
    if(type == "a") _acc.CalibrateAccel();
    if(type == "g") _acc.CalibrateGyro();
    this->off();
    return *this;
}

int16_t GB_MPU6050::read(String type, String direction){
    this->on();

    if(type == "a") {
        if(direction == "x") return _acc.getAccelerationX();
        else if(direction == "y") return _acc.getAccelerationY();
        else if(direction == "z") return _acc.getAccelerationZ();
    }
    else if(type == "r") {
        if(direction == "x") return _acc.getRotationX();
        else if(direction == "y") return _acc.getRotationY();
        else if(direction == "z") return _acc.getRotationZ();
    }
    
    this->off();
}

#endif