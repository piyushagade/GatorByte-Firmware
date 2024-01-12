#ifndef GB_AT_SCI_EC_h
#define GB_AT_SCI_EC_h

#ifndef GB_h
    #include "../../GB.h"
#endif


class GB_AT_SCI_EC : public GB_DEVICE {
    public:
        GB_AT_SCI_EC(GB &gb);
        
        struct PINS {
            bool pwrmux;
            int enable;
            bool commux;
            int muxchannel;
        } pins;
        
        DEVICE device = {
            "ec",
            "EC OEM module"
        };
        
        struct ADDRESSES {
            int bus;
        };
        struct REGISTERS {
            int data;
            int led;
            int hibernation;
            int new_reading;
            int calibration_value;
            int calibration_confirmation;
            int calibration_request;
        };

        ADDRESSES addresses = {0x64};
        REGISTERS registers = {0x18, 0x05, 0x06, 0x07, 0x0A, 0x0F, 0x0E};

        GB_AT_SCI_EC& initialize();
        GB_AT_SCI_EC& initialize(bool testdevice);
        GB_AT_SCI_EC& configure(PINS);
        GB_AT_SCI_EC& configure(PINS, ADDRESSES);
        GB_AT_SCI_EC& persistent();
        GB_AT_SCI_EC& on();
        GB_AT_SCI_EC& off();
        GB_AT_SCI_EC& activate();
        GB_AT_SCI_EC& deactivate();
        GB_AT_SCI_EC& led(String);
        GB_AT_SCI_EC& calibrate();
        GB_AT_SCI_EC& calibrate(int);
        int calibrate(String, int value);
        
        bool testdevice();
        String status();
        
        float readsensor(String mode);
        float readsensor();
        
        float current_ec_reading = 65536;
        float previous_ec_reading = 0;
        float stable_ec_reading_count = 0;

    private:
        GB *_gb;
        union sensor_mem_handler {
            byte i2c_data[4];
            long answ;
        } _acquired_data;
        bool _persistent = false;

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

GB_AT_SCI_EC::GB_AT_SCI_EC(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.ec = this;
}

// Test the device
bool GB_AT_SCI_EC::testdevice() { 
    
    this->device.detected = this->_test_connection();
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_AT_SCI_EC::status() { 
    return this->device.detected ? this->_device_info() : "not-detected" + String(":") + device.id;
}

GB_AT_SCI_EC& GB_AT_SCI_EC::initialize () { this->initialize(false); } 
GB_AT_SCI_EC& GB_AT_SCI_EC::initialize (bool testdevice) { 
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

GB_AT_SCI_EC& GB_AT_SCI_EC::configure(PINS pins) { return this->configure(pins, this->addresses); }
GB_AT_SCI_EC& GB_AT_SCI_EC::configure(PINS pins, ADDRESSES addresses) { 
    this->pins = pins;
    this->addresses = addresses;

    // Set pin modes of the device
    if(!this->pins.pwrmux) pinMode(this->pins.enable, OUTPUT);
    
    return *this;
}

/*
    Keeps device persistently on
*/
GB_AT_SCI_EC& GB_AT_SCI_EC::persistent() {
    this->_persistent = true;
    return *this;
}

// Turn on the module
GB_AT_SCI_EC& GB_AT_SCI_EC::on() {
    delay(2);
    
    // Reset mux
    _gb->getdevice("tca").resetmux();

    // Power on the device
    if(this->pins.pwrmux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);

    // Select I2C mux channel
    if(this->pins.commux) _gb->getdevice("tca").selectexclusive(pins.muxchannel);;
    
    delay(40);

    return *this;
}

// Turn off the module
GB_AT_SCI_EC& GB_AT_SCI_EC::off() {
    if (this->_persistent) return *this;
    delay(30);
    if(this->pins.pwrmux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    delay(2);

    return *this;
}

// Activate the module
GB_AT_SCI_EC& GB_AT_SCI_EC::activate() {
    this->_write_byte(this->registers.hibernation, 0x01);
    this->_read_register(this->registers.hibernation, 0x01);
    delay(1000);
    return *this;
}

// Deactivate the module
GB_AT_SCI_EC& GB_AT_SCI_EC::deactivate() {
    this->_write_byte(this->registers.hibernation, 0x00);
    delay(100);
    return *this;
}

// LED control
GB_AT_SCI_EC& GB_AT_SCI_EC::led(String state) {
    this->_led_control(state);
    return *this;
}

// Read the sensor
float GB_AT_SCI_EC::_read() {
    
    // Enable watchdog
    _gb->getmcu().watchdog("enable");

    float sensor_value = -1;
    float divident = 100;
    int DELAY_BETWEEN_SENSOR_READS = 640;

    // Sensor's temporal resolution (from datasheet)
    delay(DELAY_BETWEEN_SENSOR_READS);

    this->_read_register(this->registers.new_reading, 0x01);
    bool new_data_available = _acquired_data.i2c_data[0];
    
    // If new data is available
    if (new_data_available) {
        
        // Read sensor value
        this->_read_register(this->registers.data, 0x04);
        sensor_value = _acquired_data.answ / divident; 

        // The new data available register needs to be manually reset to 0 according to the datasheet
        this->_write_byte(this->registers.new_reading, 0x00);
    }
    
    // Disable watchdog
    _gb->getmcu().watchdog("disable");

    return sensor_value;
}

float GB_AT_SCI_EC::readsensor(String mode) {
    
    float sensor_value = -1;
    if (mode == "continuous") {
        int NUMBER_OF_READINGS = 30;

        _gb->br().log("Reading " + String(NUMBER_OF_READINGS) + " values from " + this->device.name);
        this->activate();
        for (int i = 0; i < NUMBER_OF_READINGS; i++) {
            sensor_value = sensor_value = this->_read(); 
            _gb->log(String(i) + ": " + String(sensor_value));
        }
        this->deactivate();
    }

    else if (mode == "single") {
        sensor_value = this->readsensor();
    }

    else if (mode == "next") {
        this->activate();
        sensor_value = this->_read();
        this->deactivate();
    }

    return sensor_value;
}

// Read the sensor
float GB_AT_SCI_EC::readsensor() {

    // Loop mqtt
    // if (this->_gb->hasdevice("mqtt")) this->_gb->getdevice("mqtt").update();
    // this->_gb->breathe();
    
    // Return a dummy value if "dummy" mode is on
    if (this->_gb->globals.MODE == "dummy") {
        this->_gb->log("Reading " + this->device.name, false);
        float value = random(5, 29) + random(0, 100) / 100.00;
        this->_gb->log(" -> Dummy value: " + String(value));
        _gb->getdevice("gdc").send("gdc-db", "ec=" + String(value));
        return value;
    }

    // Return if device not detected
    this->device.detected = this->_test_connection();
    if (!this->device.detected) {
        this->_gb->log("Reading " + this->device.name, false);
        this->_gb->log(" -> Device not detected");
        _gb->getdevice("gdc").send("gdc-db", "ec=" + String(-1));
        return -1;
    }
    
    this->on();
    this->activate();

    // Variables
    String sensor_mode = _gb->globals.SENSOR_MODE;
    float sensor_value = -1;
    float stability_delta = 0.1;
    int min_stable_reading_count = 5;
    float previous_reading = 0;
    int stability_counter = 0;

    // Determine number of times to read the sensor
    int NUMBER_OF_SENSOR_READS = 20;
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

    this->_gb->log(" -> " + String(sensor_value) + (sensor_value == 0 ? " -> The sensor might not be connected." : "") + " (" + String((millis() - timer) / 1000) + " seconds)");

    this->deactivate();
    this->off();

    _gb->getdevice("gdc").send("gdc-db", "ec=" + String(sensor_value));

    return sensor_value;
}


// Sensor calibration
GB_AT_SCI_EC& GB_AT_SCI_EC::calibrate() {
    // this->_calibrate("clr");

    #if not defined (LOW_MEMORY_MODE)
        int step = 0;
        String cmd = "c";
        while (step < 7 && cmd != "q") {
            if (cmd == "a") {
                this->activate();
            }
            if (cmd == "d") {
                this->deactivate();
            }
            if (cmd == "pard") {
                this->on();
                delay(500);
                this->activate();
                _gb->log(this->device.name + ": " + this->_read());
                this->deactivate();
                this->off();
                cmd = "";
            }
            if (cmd == "ard") {
                this->activate();
                _gb->log(this->device.name + ": " + this->_read());
                this->deactivate();
                cmd = "";
            }
            if (cmd == "+") {
                this->on();
            }
            if (cmd == "-") {
                this->off();
            }
            if (cmd == "w" || cmd == "i") {
                int NUMBER_OF_READINGS = cmd == "w" ? 30 : 1000;
                _gb->log("\nReading " + String(NUMBER_OF_READINGS) + " values from " + this->device.name);

                this->activate();
                for (int i = 0; i < NUMBER_OF_READINGS; i++) {
                    
                    float sensor_value = sensor_value = this->_read(); 
                    _gb->log(String(i) + ": " + String(sensor_value));
                }
                this->deactivate();
                _gb->log("Done. Waiting for a new command.");

                // Wait for serial input
                while(!_gb->serial.debug->available());

                // // Increment the step count
                // step++;

                if (_gb->serial.debug->available()) cmd = _gb->serial.debug->readStringUntil('\n');
            }
            else if (cmd == "1" || cmd == "2" || cmd == "3" || cmd == "4" || cmd == "5" || cmd == "6" || cmd == "7") {
                this->_calibrate(cmd, 0);
                
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
                _gb->log("\nCalibration status: ", false);
                this->_calibrate("?", 0);
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
int GB_AT_SCI_EC::calibrate(String action, int value) {
    _gb->log("Calibrating EC: " + action + ", " + value);
    if (action == "clear") return this->_calibrate("clr", 0);
    else if (action == "low") return this->_calibrate("low", value);
    else if (action == "dry") return this->_calibrate("dry", value);
    else if (action == "high") return this->_calibrate("high", value);
    return this->_calibrate("?", 0);
}


GB_AT_SCI_EC& GB_AT_SCI_EC::calibrate(int step) {
    
    #if not defined (LOW_MEMORY_MODE)
        if(step > 0) {
            _gb->log("\nStep: " + String(step));
            _gb->log("-------");
        }
        else _gb->log("");

        if(step == 0) {
            _gb->log("You have entered calibration mode for " + this->device.name);
            _gb->log("This is a guided process. So, please follow the instructions that are shown below. You will need 84 uS, 100 uS, 1,413 uS, 12,880 uS, and/or 80,0000 uS calibration solutions. Make sure you are wearing gloves and safety glasses before you proceed.");
            _gb->log("Current status: ", false);
            this->_calibrate("?", 0);
            _gb->log("During this process, press 'c' to move on to next step, or press 'q' to quit.");
            _gb->log("\nYou can alternatively perform calibration out of sequence by pressing the number corresponding to the following options:");
            _gb->log("1. Dry calibration");
            _gb->log("2. Low-point calibration to 84 uS/cm KCl solution");
            _gb->log("3. Low-point calibration to 100 uS/cm KCl solution");
            _gb->log("4. Low-point calibration to 417 uS/cm KCl solution");
            _gb->log("5. High-point calibration to 1,413 uS/cm KCl solution");
            _gb->log("6. High-point calibration to 12,880 uS/cm KCl solution");
            _gb->log("7. High-point calibration to 80,000 uS/cm KCl solution");
        }
        else if(step == 1) {
            _gb->log("Now, let's calibrate the sensor to the atmosphere. Make sure the sensor is dry before continuing.");
        }
        else if(step == 2) {
            _gb->log("Calibrating to atmosphere: ", false);
            this->_calibrate("dry", 0);
        }
        else if(step == 3) {
            _gb->log("Now, let's calibrate the sensor to the 100 uS calibration solution. Set the sensor in the solution and stir gently for at least 10 seconds.");
        }
        else if(step == 4) {
            _gb->log("Calibrating to 100 uS solution: ", false);
            this->_calibrate("low", 0);
        }
        else if(step == 5) {
            _gb->log("Next, we will now calibrate the sensor to the 1,413 uS calibration solution. Run some distilled water over the sensor to rinse any remnants of previous solution. Then dip the sensor in the solution and stir gently for at least 10 seconds.");
        }
        else if(step == 6) {
            _gb->log("Calibrating to 1,413 uS solution: ", false);
            this->_calibrate("high", 0);

            // Go to step 7
            this->calibrate(7);
        }
        else if(step == 7) {
            _gb->log("All done. The sensor is now ready to be used. For high precision and accuracy, please calibrate the sensor every few months.");

            // TODO: Should the mode be set to "read" always?
            _gb->globals.MODE = "read";
        }
    #endif

    return *this;
}

// Write a byte to an OEM register
void GB_AT_SCI_EC::_write_byte(byte reg, byte data) {
    this->on();
    Wire.beginTransmission(this->addresses.bus);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
    // this->off();
}

// Write a long to an OEM register
void GB_AT_SCI_EC::_write_long(byte reg, unsigned long data) {
    
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
void GB_AT_SCI_EC::_read_register(byte register_address, byte number_of_bytes_to_read) {
    this->on();
    byte i;
    Wire.beginTransmission(this->addresses.bus);
    Wire.write(register_address);
    Wire.endTransmission();

    Wire.requestFrom(this->addresses.bus, (byte) number_of_bytes_to_read);
    for (i = number_of_bytes_to_read; i > 0; i--) {
        // Byte 0 is LSB
        byte data = Wire.read();
        _acquired_data.i2c_data[i - 1] = data;
    }
    Wire.endTransmission();
    // this->off();
}

// Sensor info function
bool GB_AT_SCI_EC::_test_connection() {
    this->on();
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
String GB_AT_SCI_EC::_device_info() {
    this->on();

    const byte device_type_register = 0x00;
    const byte device_address_register = 0x03;
    this->_read_register(device_type_register, 0x02);
    if(_acquired_data.i2c_data[1] == 255) {
        _gb->log("Error reading device info.");
        return "ID: N/A vN/A";
    }
    
    String data = "ID: " + String(_acquired_data.i2c_data[1]) + " v" + String(_acquired_data.i2c_data[0]);

    // _gb->log(" -> ", false);
    // _gb->log("ID: ", false);
    // _gb->log(String(_acquired_data.i2c_data[1]), false);
    // _gb->log(" v", false);
    // _gb->log(String(_acquired_data.i2c_data[0]), false);

    // this->_read_register(device_address_register, 0x01);
    // _gb->log(", Address: ", false);
    // _gb->log("0x" + String(_acquired_data.i2c_data[0], HEX), false);

    this->off();
    return data;
}

// Sensor LED control
void GB_AT_SCI_EC::_led_control(String cmd) {
    byte led_control = cmd == "on" ? 0x01 : 0x00;
    
    if (cmd == "?") {
        this->_read_register(this->registers.led, 0x01);
        if (_acquired_data.i2c_data[0] == 0) _gb->log("LED is OFF.");
        else if (_acquired_data.i2c_data[0] == 1) _gb->log("LED is ON.");
        else _gb->log(" -> Error reading LED state.");
        return;
    }

    this->_write_byte(this->registers.led, led_control);
    this->_read_register(this->registers.led, 0x01);
    if (_acquired_data.i2c_data[0] != led_control) _gb->log(" -> Error changing LED state to: " + cmd , false);
}

// Calibration procedure
int GB_AT_SCI_EC::_calibrate(String cmd, int value) {

    if (cmd.compareTo("?") == 0) {
        String status = "";
        this->_read_register(this->registers.calibration_confirmation, 0x01);
        if (this->_acquired_data.i2c_data[0] == 0) status += "No calibration. ";
        if (bitRead(this->_acquired_data.i2c_data[0], 0) == 1) status += "Dry calibration done. ";
        if (bitRead(this->_acquired_data.i2c_data[0], 1) == 1) status += "Single-point calibration done. ";
        if (bitRead(this->_acquired_data.i2c_data[0], 2) == 1) status += "Low-point calibration done. ";
        if (bitRead(this->_acquired_data.i2c_data[0], 3) == 1) status += "High-point calibration done. ";
        
        _gb->log(this->_acquired_data.i2c_data[0], false);
        _gb->log(" " + status);
    }

    if (cmd.compareTo("clr") == 0) {
        const byte CAL_TYPE = 0x01;
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(40);
        this->_read_register(this->registers.calibration_confirmation, 0x01);
        if (this->_acquired_data.i2c_data[0] == 0) _gb->log("\nCalibration cleared.");
    }
    
    if (cmd.compareTo("low") == 0) {
        const float EC_VALUE = value * 100;
        const byte CAL_TYPE = 0x04;

        // Write calibration solution value to register
        this->_write_long(this->registers.calibration_value, EC_VALUE);

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(100);

        _gb->log("Calibration status: ", false); this->_calibrate("?", 0);
    }
    
    if (cmd.compareTo("high") == 0) {
        const float EC_VALUE = value * 100;
        const byte CAL_TYPE = 0x05;

        // Write calibration solution value to register
        this->_write_long(this->registers.calibration_value, EC_VALUE);

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(100);

        _gb->log("Calibration status: ", false); this->_calibrate("?", 0);
    }

        
    if (cmd.compareTo("dry") == 0) {
        const byte CAL_TYPE = 0x02;

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(15);

        _gb->log("Calibration status: ", false); this->_calibrate("?", 0);
    }
        
    if (cmd.compareTo("1") == 0) {
        const byte CAL_TYPE = 0x02;

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(15);

        _gb->log("Calibration status: ", false); this->_calibrate("?", 0);
    }


    if (cmd.compareTo("2") == 0 || cmd.compareTo("3") == 0 || cmd.compareTo("4") == 0) {
        const int solutions[] = {84, 100, 417};
        const float EC_VALUE = solutions[cmd.toInt() - 2] * 100;
        const byte CAL_TYPE = 0x04;

        // Write calibration solution value to register
        this->_write_long(this->registers.calibration_value, EC_VALUE);

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(100);

        _gb->log("Calibration status: ", false); this->_calibrate("?", 0);
    }

    if (cmd.compareTo("5") == 0 || cmd.compareTo("6") == 0 || cmd.compareTo("7") == 0) {
        const int solutions[] = {1413, 12880, 80000};
        const float EC_VALUE = solutions[cmd.toInt() - 5] * 100;
        const byte CAL_TYPE = 0x05;

        // Write calibration solution value to register
        this->_write_long(this->registers.calibration_value, EC_VALUE);

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(100);

        _gb->log("Calibration status: ", false); this->_calibrate("?", 0);
    }

    return this->_acquired_data.i2c_data[0];
}

#endif