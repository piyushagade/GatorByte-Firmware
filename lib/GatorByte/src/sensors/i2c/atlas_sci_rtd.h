#ifndef GB_AT_SCI_RTD_h
#define GB_AT_SCI_RTD_h

#ifndef GB_h
    #include "../../GB.h"
#endif

/* Import I/O expander */
#ifndef GB_74HC595_h
    #include "../misc/74hc595.h"
#endif

class GB_AT_SCI_RTD : public GB_DEVICE {
    public:
        GB_AT_SCI_RTD(GB &gb);

        struct PINS {
            bool pwrmux;
            int enable;
            bool commux;
            int muxchannel;
        } pins;
        
        DEVICE device = {
            "rtd",
            "RTD OEM module"
        };
        
        struct ADDRESSES {
            int bus;
        };
        struct REGISTERS {
            int data;
            int led;
            int hibernation;
            int new_reading;
            int calibration_control;
            int calibration_confirmation;
            int calibration_request;
        };

        ADDRESSES addresses = {0x68};
        REGISTERS registers = {0x0E, 0x05, 0x06, 0x07, 0x08, 0x0D, 0x0C};

        GB_AT_SCI_RTD& initialize();
        GB_AT_SCI_RTD& initialize(bool testdevice);
        GB_AT_SCI_RTD& configure(PINS);
        GB_AT_SCI_RTD& configure(PINS, ADDRESSES);
        GB_AT_SCI_RTD& persistent();
        GB_AT_SCI_RTD& on();
        GB_AT_SCI_RTD& off();
        GB_AT_SCI_RTD& activate();
        GB_AT_SCI_RTD& deactivate();
        GB_AT_SCI_RTD& led(String);
        GB_AT_SCI_RTD& calibrate();
        GB_AT_SCI_RTD& calibrate(int);
        int calibrate(String, int value);
        
        bool testdevice();
        String status();

        float readsensor();
        float readsensor(String mode);
        float quickreadsensor(int times);
        float lastvalue();

    private:
        GB *_gb;
        union sensor_mem_handler {
            byte i2c_data[4];
            long answ;
        } _acquired_data;
        bool _persistent = false;
        float _latest_value = 25;

        float _sendCommand(String);
        void _write_byte(byte, byte);
        void _write_long(byte, unsigned long);
        void _read_register(byte, byte);
        String _device_info();
        void _led_control(String);
        int _calibrate(String, int value);
        float _read();
        bool _test_connection();
        
};

GB_AT_SCI_RTD::GB_AT_SCI_RTD(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.rtd = this;
}

// Test the device
bool GB_AT_SCI_RTD::testdevice() { 
    
    this->device.detected = this->_test_connection();
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_AT_SCI_RTD::status() { 
    return this->device.detected ? this->_device_info() : "not-detected" + String(":") + device.id;
}

GB_AT_SCI_RTD& GB_AT_SCI_RTD::initialize () { this->initialize(false); } 
GB_AT_SCI_RTD& GB_AT_SCI_RTD::initialize (bool testdevice) { 
    _gb->init();
    
    _gb->log("Initializing " + this->device.name, false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    this->device.detected = this->_test_connection();
    if (testdevice) {
        if (this->device.detected) {
            this->on();
            _gb->log(" -> " + this->_device_info());
            this->off();
        }
        else _gb->log(" -> Not detected");
    }
    else _gb->log(this->device.detected ? " -> Done" : " -> Not detected");
    return *this;
}

GB_AT_SCI_RTD& GB_AT_SCI_RTD::configure(PINS pins) { return this->configure(pins, this->addresses); }
GB_AT_SCI_RTD& GB_AT_SCI_RTD::configure(PINS pins, ADDRESSES addresses) { 
    this->pins = pins;
    this->addresses = addresses;

    // Set pin modes of the device
    if(!this->pins.pwrmux) pinMode(this->pins.enable, OUTPUT);
    
    return *this;
}

/*
    Keeps device persistently on
*/
GB_AT_SCI_RTD& GB_AT_SCI_RTD::persistent() {
    this->_persistent = true;
    return *this;
}

// Turn on the module
GB_AT_SCI_RTD& GB_AT_SCI_RTD::on() {
    delay(2);
    
    // Reset mux
    _gb->getdevice("tca").resetmux();
    
    // Power on the device
    if(this->pins.pwrmux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);

    // Select I2C mux channel
    if(this->pins.commux) _gb->getdevice("tca").selectexclusive(pins.muxchannel);

    delay(50);
    return *this;
}

// Turn off the module
GB_AT_SCI_RTD& GB_AT_SCI_RTD::off() {
    if (this->_persistent) return *this;
    delay(10);
    if(this->pins.pwrmux) this->_gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    delay(2);
    return *this;
}

// Activate the module
GB_AT_SCI_RTD& GB_AT_SCI_RTD::activate() {
    this->_write_byte(this->registers.hibernation, 0x01);
    this->_read_register(this->registers.hibernation, 0x01);
    delay(200);
    return *this;
}

// Deactivate the module
GB_AT_SCI_RTD& GB_AT_SCI_RTD::deactivate() {
    this->_write_byte(this->registers.hibernation, 0x00);
    delay(100);
    return *this;
}

// LED control
GB_AT_SCI_RTD& GB_AT_SCI_RTD::led(String state) {
    this->_led_control(state);
    return *this;
}

// Read the sensor
float GB_AT_SCI_RTD::_read() {
    
    // Enable watchdog
    _gb->getmcu().watchdog("enable");

    float sensor_value = -1;
    float divident = 1000;
    int DELAY_BETWEEN_SENSOR_READS = 420;

    // Sensor's temporal resolution (from datasheet)
    delay(DELAY_BETWEEN_SENSOR_READS);

    this->_read_register(this->registers.new_reading, 0x01);
    bool new_data_available = _acquired_data.i2c_data[0];
    
    // If new data is available
    if (new_data_available) {
        
        // Read sensor value
        this->_read_register(this->registers.data, 0x04);
        sensor_value = _acquired_data.answ / divident; 
        this->_latest_value = sensor_value;

        // The new data available register needs to be manually reset to 0 according to the datasheet
        this->_write_byte(this->registers.new_reading, 0x00);
    }
    
    // Disable watchdog
    _gb->getmcu().watchdog("disable");

    return sensor_value;
}

float GB_AT_SCI_RTD::readsensor(String mode) {
    
    float sensor_value = -1;
    if (mode == "continuous") {
        int NUMBER_OF_READINGS = 10;

        _gb->br().log("Reading " + String(NUMBER_OF_READINGS) + " values from " + this->device.name);
        this->activate();
        for (int i = 0; i < NUMBER_OF_READINGS; i++) {
            sensor_value = sensor_value = this->_read(); 
            _gb->log(String(i) + ": " + String(sensor_value));
        }
        this->deactivate();
    }

    else if (mode == "single") {
        return this->readsensor();
    }

    else if (mode == "next") {
        this->activate();
        float reading = this->_read();
        this->deactivate();
        return reading;
    }

    return sensor_value;
}


float GB_AT_SCI_RTD::quickreadsensor(int times) {

    this->on();
    this->activate();

    // Variables
    String sensor_mode = _gb->globals.SENSOR_MODE;
    float sensor_value = -1;
    float stability_delta = 0.5;
    int min_stable_reading_count = 5;
    float previous_reading = 0;
    int stability_counter = 0;

    // Determine number of times to read the sensor
    int NUMBER_OF_SENSOR_READS = 10;
    int MAX_ATTEMPTS = 10;
    int ATTEMPT_COUNT = 0;

    // Get readings
    int timer = millis();
    float min = 99, max = 0, avg = 0;

    // Read sensors until readings are stable
    this->_gb->log("Reading " + this->device.name + " " + String(NUMBER_OF_SENSOR_READS) + " times", false);
    
    // Read sensor a fixed number of times
    for(int i = 0; i < NUMBER_OF_SENSOR_READS; i++, ATTEMPT_COUNT++) {

        // Get sensor value
        sensor_value = this->_read();

        _gb->log(sensor_value);
        
        // Calculate statistics
        if (sensor_value < min) min = sensor_value;
        if (max < sensor_value) max = sensor_value;
        avg = (avg * (ATTEMPT_COUNT - 1) + sensor_value) / ATTEMPT_COUNT;

        // If max number of read attempts were made, exit
        if (ATTEMPT_COUNT >= MAX_ATTEMPTS) {
            i = NUMBER_OF_SENSOR_READS;
        }
    }

    this->deactivate();
    this->off();
    
    // Print statistics
    this->_gb->log(String(" -> ") + String(min) + " |--- " + String(avg) + " ---| " + String(max));

    _gb->getdevice("gdc").send("gdc-db", "rtd=" + String(sensor_value));

    return sensor_value;
}

// Read the sensor
float GB_AT_SCI_RTD::readsensor() {

    // Loop mqtt
    // if (this->_gb->hasdevice("mqtt")) this->_gb->getdevice("mqtt").update();
    // this->_gb->breathe();
        
    // Return a dummy value if "dummy" mode is on
    if (this->_gb->globals.MODE == "dummy") {
        this->_gb->log("Reading " + this->device.name, false);
        float value = random(5, 29) + random(0, 100) / 100.00;
        this->_gb->log(" -> Dummy value: " + String(value));
        _gb->getdevice("gdc").send("gdc-db", "rtd=" + String(value));
        return value;
    }

    // Return if device not detected
    this->device.detected = this->_test_connection();
    if (!this->device.detected) {
        this->_gb->log("Reading " + this->device.name, false);
        this->_gb->log(" -> Device not detected");
        _gb->getdevice("gdc").send("gdc-db", "rtd=" + String(-1));
        return -1;
    }
    
    this->on();
    this->activate();
    delay(100);

    // Variables
    String sensor_mode = _gb->globals.SENSOR_MODE;
    float sensor_value = -1;
    float stability_delta = 0.5;
    int min_stable_reading_count = 5;
    float previous_reading = 0;
    int stability_counter = 0;

    // Determine number of times to read the sensor
    int NUMBER_OF_SENSOR_READS = 10;
    int MAX_ATTEMPTS = 10;
    int ATTEMPT_COUNT = 0;

    // Get readings
    int timer = millis();
    if (sensor_mode == "stability") {
        
        this->_gb->log("Reading " + this->device.name + " until stable", false);
        
        // Read sensors until readings are stable
        while (stability_counter < min_stable_reading_count && ATTEMPT_COUNT++ < MAX_ATTEMPTS) {
            
            // Get updated sensor value
            sensor_value = this->_read();

            // Check if the readings are stable
            float difference = abs(sensor_value - previous_reading);
            if (difference <= stability_delta) {
                stability_counter++;
                if (_gb->hasdevice("rgb")) { _gb->getdevice("rgb").on("green"); delay (250); _gb->getdevice("rgb").on("magenta"); }
            }
            else {
                stability_counter = 0;
                if (_gb->hasdevice("rgb")) { _gb->getdevice("rgb").on("yellow"); delay (250); _gb->getdevice("rgb").on("magenta"); }
            }
            previous_reading = sensor_value;
        }
    }

    else if (sensor_mode == "iterations") {
            
        this->_gb->log("Reading " + this->device.name + " " + String(NUMBER_OF_SENSOR_READS) + " times", false);

        // Read sensor a fixed number of times
        for(int i = 0; i < NUMBER_OF_SENSOR_READS; i++, ATTEMPT_COUNT++) {

            // GB breathe
            this->_gb->breathe();

            // Get sensor value
            sensor_value = this->_read();

            // If max number of read attempts were made, exit
            if (ATTEMPT_COUNT >= MAX_ATTEMPTS) {
                i = NUMBER_OF_SENSOR_READS;
            }
        }
    }

    else this->_gb->log("Reading " + this->device.name + " -> Sensor read mode not provided", false);
    
    this->_gb->log(" -> " + String(sensor_value) + (sensor_value == -1023.00 ? " -> The sensor might not be connected." : "") + " (" + String((millis() - timer) / 1000) + " seconds)");
    
    this->deactivate();
    this->off();
    // this->_gb->log(" -> " + String(sensor_value));
    _gb->getdevice("gdc").send("gdc-db", "rtd=" + String(sensor_value));

    return sensor_value;
}

// Read the sensor
float GB_AT_SCI_RTD::lastvalue() {
    return this->_latest_value < 0 ? 25 : this->_latest_value;
}

// Manual sensor calibration
GB_AT_SCI_RTD& GB_AT_SCI_RTD::calibrate() {

    #if not defined (LOW_MEMORY_MODE)
        int step = 0;
        String cmd = "c";
        while (step <= 1 && cmd != "q") {
            if (cmd == "w") {
                _gb->log("\nReading 10 values from " + this->device.name);
                _gb->log("-----------------------------------------------");
                for (int i = 0; i < 10; i++) {
                    _gb->log(this->readsensor());
                    delay(500);
                }
                _gb->log("Done. Waiting for a new command.");

                // Wait for serial input
                while(!_gb->serial.debug->available());

                if (_gb->serial.debug->available()) cmd = _gb->serial.debug->readStringUntil('\n');
            }
            else if (cmd == "c") {
                this->calibrate(step);
                
                // Wait for serial input
                while(!_gb->serial.debug->available());

                // Increment the step count
                step++;

                if (_gb->serial.debug->available()) cmd = _gb->serial.debug->readStringUntil('\n');
            }
            if (cmd == "s") {
                this->calibrate(step++);
                
                // Wait for serial input
                while(!_gb->serial.debug->available());

                if (_gb->serial.debug->available()) cmd = _gb->serial.debug->readStringUntil('\n');
            }
            else if (cmd == "r") {
                _gb->log(this->device.name + " value: " + this->readsensor());
                
                // Wait for serial input
                while(!_gb->serial.debug->available());

                if (_gb->serial.debug->available()) cmd = _gb->serial.debug->readStringUntil('\n');
            }
            else if (cmd == "x") {
                this->_calibrate("clr", 0);
                cmd = "c";
                step = 0;
            }
            else if (cmd == "/") {
                // _gb->log("\nCalibration status: ", false);
                // this->_calibrate("?");
                cmd = "c";
                step = 0;
            }
            else if (cmd == "q") {
                _gb->log("\nExiting calibration mode.\n");
                cmd = "c";
                step = 0;
                break;
            }
            else {
                delay(500);
                if (_gb->serial.debug->available()) cmd = _gb->serial.debug->readStringUntil('\n');
            }
        }
    #else
        _gb->log("GatorByte operating in low power mode. Calibration is disabled.");
    #endif

    return *this;
}

// Sensor calibration
int GB_AT_SCI_RTD::calibrate(String action, int value) {
    if (action == "clear") return this->_calibrate("clr", 0);
    else if (action == "mid") return this->_calibrate("mid", value);
    return this->_calibrate("?", 0);
}

// Sensor calibration
GB_AT_SCI_RTD& GB_AT_SCI_RTD::calibrate(int step) {

    #if not defined (LOW_MEMORY_MODE)
        if(step > 0) {
            _gb->log("\nStep: " + String(step));
            _gb->log("-------");
        }
        else _gb->log("");

        if(step == 0) {
            _gb->log("You have entered calibration mode for " + this->device.name);
            _gb->log("Current status: ", false);
            this->_calibrate("?", 0);
            _gb->log("Calibration not required for this sensor. Click \"next\" to exit.");

            // // Calibration complete
            // this->calibrate(1);
        }
        else if(step == 1) {
            _gb->log("The sensor is ready to be used. No action required.");
        }
    #endif
    return *this;
}

// Calibration procedure
int GB_AT_SCI_RTD::_calibrate(String cmd, int value) {

    return 0;
}

// Write a byte to an OEM register
void GB_AT_SCI_RTD::_write_byte(byte reg, byte data) {
    this->on();
    Wire.beginTransmission(this->addresses.bus);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
    // this->off();
}

// Write a long to an OEM register
void GB_AT_SCI_RTD::_write_long(byte reg, unsigned long data) {
    
    union local_sensor_mem_handler {
        byte bdata[4];
        long ldata;
    } _local_data;
    _local_data.ldata = data;
    
    this->on();
    Wire.beginTransmission(this->addresses.bus);
    Wire.write(reg);
    for (int i = 3; i >= 0; i--) Wire.write(_local_data.bdata[i]);
    Wire.endTransmission();
    // this->off();
}

// Read OEM device register
void GB_AT_SCI_RTD::_read_register(byte register_address, byte number_of_bytes_to_read) {
    this->on();
    byte i;
    Wire.beginTransmission(this->addresses.bus);
    Wire.write(register_address);
    Wire.endTransmission();

    Wire.requestFrom(this->addresses.bus, (byte) number_of_bytes_to_read);
    for (i = number_of_bytes_to_read; i > 0; i--){
        // Byte 0 is LSB
        byte data = Wire.read();
        _acquired_data.i2c_data[i - 1] = data;
    }
    Wire.endTransmission();
    this->off();
}

// Sensor info function
bool GB_AT_SCI_RTD::_test_connection() {
    this->on();
    // delay(2000);
    for (int i = 0; i < 3; i++) { delay(30); this->_read_register(this->registers.led, 0x01); }
    if (_acquired_data.i2c_data[0] <= 1) {
        this->off();
        return true;
    }
    else {
        this->off();
        return false;
    }
}

// Sensor info function
String GB_AT_SCI_RTD::_device_info() {
    this->on();

    const byte device_type_register = 0x00;
    const byte device_address_register = 0x03;
    this->_read_register(device_type_register, 0x02);
    if(_acquired_data.i2c_data[1] == 255) {
        _gb->log("Error reading device info.");
        return "ID: N/A vN/A";
    }

    String data = "ID: " + String(_acquired_data.i2c_data[1]) + " v" + String(_acquired_data.i2c_data[0]);

    this->off();
    return data;
}

// Sensor LED control
void GB_AT_SCI_RTD::_led_control(String cmd) {
    byte led_control = cmd == "on" ? 0x01 : 0x00;
    
    this->on();
    if (cmd == "?") {
        this->_read_register(this->registers.led, 0x01);
        if (_acquired_data.i2c_data[0] == 0) _gb->log("LED is OFF.");
        else if (_acquired_data.i2c_data[0] == 1) _gb->log("LED is ON.");
        else _gb->log("Error reading LED state.");
        return;
    }

    this->_write_byte(this->registers.led, led_control);
    this->_read_register(this->registers.led, 0x01);
    if (_acquired_data.i2c_data[0] != led_control) _gb->log("Error changing LED state.");
    this->off();
}

#endif