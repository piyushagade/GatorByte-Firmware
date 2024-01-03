#ifndef GB_USS_h
#define GB_USS_h

#ifndef GB_h
    #include "../../GB.h"
#endif

// #ifndef RS485_h
//     #include "RS485.h"
// #endif

#ifndef CRC16_H
    #include "CRC16.h"
#endif

class GB_USS : GB_DEVICE {
    public:
        GB_USS(GB&);

        DEVICE device = {
            "uss",
            "Seedstudio distance sensor"
        };
        
        struct PINS {
            bool mux;
            int enable;
            int control;
        } pins;
        
        struct REGISTERS {
            word ultrasonic = 0x01;
            word calculated = 0x0100;
            word realtime = 0x0101;
            word addressconfig = 0x0200;
            word buadrateconfig = 0x0201;
        } registers;

        GB_USS& configure(PINS);
        GB_USS& initialize();
        GB_USS& on();
        GB_USS& off();
        GB_USS& persistent();
        GB_USS& persistent(bool);
        
        bool testdevice();
        String status();

        uint32_t read();
        
        // Type definitions
        typedef void (*callback_t)();
        bool listener(callback_t callback);

    private:
        GB *_gb;
        CRC16 CRC;
        bool _persistent = false;
        long _fetch();
        void _request(byte Address, unsigned short Register);
        void _write(byte Address, unsigned short Register, byte value);
};

GB_USS::GB_USS(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.uss = this;
}

GB_USS& GB_USS::configure(PINS pins) {
    this->pins = pins;
    
    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);
    
    return *this;
}

GB_USS& GB_USS::initialize() {
    _gb->init();

    _gb->log("Initializing " + this->device.name, false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    // Disable stray watchdogs
    _gb->getmcu().watchdog("disable");

    // Initialize MODBUS library
    this->on();
    if (!this->_persistent) delay(3000);
    
    pinMode(this->pins.control, OUTPUT);
    digitalWrite(this->pins.control, LOW);

    // Read value
    long distance = this->read();
    _gb->log(" -> Distance: " + String(distance), false);

    bool success = distance > 0;
    _gb->log(success ? " -> Done" : " -> Failed");

    this->device.detected = success;

    this->off();
    return *this;
}

// Test the device
bool GB_USS::testdevice() { 
    this->device.detected = this->read() > 0;
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}

String GB_USS::status() { 
    return this->device.detected ? String(this->read()) : "not-detected" + String(":") + device.id;
}

GB_USS& GB_USS::on() { 
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    delay(20);
    return *this;
}

GB_USS& GB_USS::off() { 
    if (this->_persistent) return *this;
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    return *this;
}

/*
    Keeps device persistently on
    WARN: The device misbehaves when persistentance if off (when set to false)
*/
GB_USS& GB_USS::persistent() {
    this->_persistent = true;
    return *this;
}
GB_USS& GB_USS::persistent(bool state) {
    this->_persistent = state;
    return *this;
}

/*
    Reads distance in mm
*/
uint32_t GB_USS::read() {
    this->on();

    if (!this->_persistent) delay(3000);

    long distance = 0;
    int counter = 5;
    do {
        uint32_t sum = 0;
        for(uint32_t i = 0; i < 3; i++) { 
            this->_request(registers.ultrasonic, registers.calculated);
            distance = this->_fetch();
            sum += distance;
            uint32_t average = sum / (i + 1);

            // if (abs(distance - average) > 75) _gb->log("USS reading outlier detected: " + String(distance));
            delay(500);
        }
        if (distance <= 0) this->_request(registers.ultrasonic, registers.calculated);
    } while (distance <= 0 && counter-- > 0);
    
    this->off();
    return distance;
}

void GB_USS::_request(byte Address, unsigned short Register) {
    this->on();

    byte data[] = {Address, 0x03, highByte(Register), lowByte(Register), (byte)0, 0x01};
    unsigned short checksum = CRC.Modbus(data, 0, 6);
    digitalWrite(this->pins.control, HIGH);
    this->_gb->serial.hardware->write(data[0]);
    this->_gb->serial.hardware->write(data[1]);
    this->_gb->serial.hardware->write(data[2]);
    this->_gb->serial.hardware->write(data[3]);
    this->_gb->serial.hardware->write(data[4]);
    this->_gb->serial.hardware->write(data[5]);
    
    //CRC-16/MODBUS
    this->_gb->serial.hardware->write(lowByte(checksum));
    this->_gb->serial.hardware->write(highByte(checksum));

    //wait until transmission ends
    this->_gb->serial.hardware->flush();
    digitalWrite(this->pins.control, LOW);
}

void GB_USS::_write(byte Address, unsigned short Register, byte value) {
    this->on();

    byte data[] = {Address, 0x06, highByte(Register), lowByte(Register), (byte)0, value};
    unsigned short checksum = CRC.Modbus(data, 0, 6);
    digitalWrite(this->pins.control, HIGH);
    this->_gb->serial.hardware->write(data[0]);
    this->_gb->serial.hardware->write(data[1]);
    this->_gb->serial.hardware->write(data[2]);
    this->_gb->serial.hardware->write(data[3]);
    this->_gb->serial.hardware->write(data[4]);
    this->_gb->serial.hardware->write(data[5]);
    //CRC-16/MODBUS
    this->_gb->serial.hardware->write(lowByte(checksum));
    this->_gb->serial.hardware->write(highByte(checksum));
    //wait until transmission ends
    this->_gb->serial.hardware->flush();
    digitalWrite(this->pins.control, LOW);
}

long GB_USS::_fetch() {
    this->on();
    char buff[11];
    long dist = 0;

    if (this->_gb->serial.hardware->available() < 7) {
        return -1;
    }
    for (int i = 0; i < sizeof(buff); i++) {
        buff[i] = this->_gb->serial.hardware->read();
        delayMicroseconds(10);
    }
    dist = (buff[3] << 8) | buff[4];
    return dist;
}

#endif