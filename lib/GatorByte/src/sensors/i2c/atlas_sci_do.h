#ifndef GB_AT_SCI_DO_h
#define GB_AT_SCI_DO_h

/*
    ! Uses 3% flash memory
*/

#ifndef GB_h
    #include "../../GB.h"
#endif

class GB_AT_SCI_DO : public GB_DEVICE {
    public:
        GB_AT_SCI_DO(GB &gb);

        struct PINS {
            bool pwrmux;
            int enable;
            bool commux;
            int muxchannel;
        } pins;
        
        DEVICE device = {
            "dox",
            "DO OEM module"
        };
        
        struct ADDRESSES {
            int bus;
        };

        struct REGISTERS {
            int data;
            int led;
            int hibernation;
            int new_reading;
            int calibration_confirmation;
            int calibration_request;
        };

        ADDRESSES addresses = {0x67};
        REGISTERS registers = {0x22, 0x05, 0x06, 0x07, 0x09, 0x08};

        GB_AT_SCI_DO& initialize();
        GB_AT_SCI_DO& initialize(bool testdevice);
        GB_AT_SCI_DO& configure(PINS);
        GB_AT_SCI_DO& configure(PINS, ADDRESSES);
        GB_AT_SCI_DO& persistent();
        GB_AT_SCI_DO& on();
        GB_AT_SCI_DO& off();
        GB_AT_SCI_DO& activate();
        GB_AT_SCI_DO& deactivate();
        GB_AT_SCI_DO& led(String);
        int calibrate(String, int value);
        
        bool testdevice();
        String status();
        
        float readsensor(String mode);
        float readsensor();

    private:
        GB *_gb;
        GB_AT_SCI_DO& _initialize(bool testdevice);
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

GB_AT_SCI_DO::GB_AT_SCI_DO(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.dox = this;
}

// Test the device
bool GB_AT_SCI_DO::testdevice() { 

    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(false);
    
    this->device.detected = this->_test_connection();
    return this->device.detected;
}
String GB_AT_SCI_DO::status() { 

    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(false);
    
    return this->device.detected ? this->_device_info() : "not-detected" + String(":") + device.id;
}

GB_AT_SCI_DO& GB_AT_SCI_DO::initialize() { return this->initialize(false); }
GB_AT_SCI_DO& GB_AT_SCI_DO::initialize (bool testdevice) {
    if (_gb->globals.GDC_CONNECTED) {
        this->off();
        return *this;
    }
    return this->_initialize(testdevice); 
}
GB_AT_SCI_DO& GB_AT_SCI_DO::_initialize (bool testdevice) { 
    _gb->init();
    
    _gb->log("Initializing " + this->device.name, false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
 
    this->device.detected = this->_test_connection();
    if (testdevice) {
        if (this->device.detected) {
            this->on();
            _gb->arrow().log("" + this->_device_info());
            this->off();
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("..").wait(250).play("...");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on(2).wait(250).revert();
        }
        else {
            _gb->arrow().log("Not detected");
            _gb->globals.INIT_REPORT += this->device.id;
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("..").wait(250).play("---");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on(1).wait(250).revert();
        }
    }
    else _gb->arrow().log(this->device.detected ? "Done" : "Not detected");

    return *this;
}

GB_AT_SCI_DO& GB_AT_SCI_DO::configure(PINS pins) { return this->configure(pins, this->addresses); }
GB_AT_SCI_DO& GB_AT_SCI_DO::configure(PINS pins, ADDRESSES addresses) { 
    this->pins = pins;
    this->addresses = addresses;

    // Set pin modes of the device
    if(!this->pins.pwrmux) pinMode(this->pins.enable, OUTPUT);
    
    return *this;
}

/*
    Keeps device persistently on
*/
GB_AT_SCI_DO& GB_AT_SCI_DO::persistent() {
    this->_persistent = true;
    return *this;
}

// Turn on the module
GB_AT_SCI_DO& GB_AT_SCI_DO::on() {
    delay(2);
    
    // Reset mux
    _gb->getdevice("tca")->resetmux();

    // Power on the device
    if(this->pins.pwrmux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);

    // Select I2C mux channel
    if(this->pins.commux) _gb->getdevice("tca")->selectexclusive(pins.muxchannel);;

    delay(30);

    return *this;
}

// Turn off the module
GB_AT_SCI_DO& GB_AT_SCI_DO::off() {
    if (this->_persistent) return *this;
    delay(30);
    if(this->pins.pwrmux) _gb->getdevice("ioe")->writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    delay(2);

    return *this;
}

// Activate the module
GB_AT_SCI_DO& GB_AT_SCI_DO::activate() {
    this->on();
    this->_write_byte(this->registers.hibernation, 0x01);
    this->_read_register(this->registers.hibernation, 0x01);
    delay(200);
    return *this;
}

// Deactivate the module
GB_AT_SCI_DO& GB_AT_SCI_DO::deactivate() {
    this->on();
    this->_write_byte(this->registers.hibernation, 0x00);
    delay(100);
    return *this;
}

// LED control
GB_AT_SCI_DO& GB_AT_SCI_DO::led(String state) {
    this->_led_control(state);
    return *this;
}

// Read the sensor
float GB_AT_SCI_DO::_read() {
    
    // Enable watchdog
    _gb->getmcu()->watchdog("enable");

    float sensor_value = -1;
    float divident = 100;
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

        // The new data available register needs to be manually reset to 0 according to the datasheet
        this->_write_byte(this->registers.new_reading, 0x00);
    }
    
    // Disable watchdog
    _gb->getmcu()->watchdog("disable");

    return sensor_value;
}

float GB_AT_SCI_DO::readsensor(String mode) {
    
    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(false);
    
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

// Read the sensor
float GB_AT_SCI_DO::readsensor() {
    
    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(false);

    // Loop mqtt
    // if (false && _gb->hasdevice("mqtt")) _gb->getdevice("mqtt").update();
    // _gb->breathe();

    _gb->log("Reading " + this->device.name + " (" + _gb->globals.SENSOR_MODE + ")", false);
    
    // Return a dummy value if "dummy" mode is on
    bool dummy = _gb->env() == "development";
    if (dummy || _gb->globals.MODE == "dummy") {
        float value = random(5, 29) + random(0, 100) / 100.00;
        _gb->arrow().log("Dummy value: " + String(value));
        // _gb->getdevice("gdc")->send("gdc-db", "dox=" + String(value));
        this->off();
        return value;
    }

    // Return if device not detected
    this->device.detected = this->_test_connection();
    if (!this->device.detected) {
        _gb->arrow().log("Device not detected");
        // _gb->getdevice("gdc")->send("gdc-db", "dox=" + String(-1));
        return -1;
    }
    
    // Turn on and activate
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
    int NUMBER_OF_SENSOR_READS = 10;
    int MAX_ATTEMPTS = 20;
    int ATTEMPT_COUNT = 0;
    
    // Get readings
    int timer = millis();
    float min = 99, max = 0, avg = 0;

    if (sensor_mode == "stability") {
        
        // Read sensors until readings are stable
        while (stability_counter < min_stable_reading_count && ATTEMPT_COUNT++ < MAX_ATTEMPTS) {
            
            // Get updated sensor value
            sensor_value = this->_read();

            // Calculate statistics
            if (sensor_value < min) min = sensor_value;
            if (max < sensor_value) max = sensor_value;
            avg = (avg * (ATTEMPT_COUNT - 1) + sensor_value) / ATTEMPT_COUNT;

            // Check if the readings are stable
            float difference = abs(sensor_value - previous_reading);
            if (difference <= stability_delta) {
                stability_counter++;
                if (_gb->hasdevice("rgb")) { _gb->getdevice("rgb")->on(2); delay (250); _gb->getdevice("rgb")->on(5); }
                if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play(".");
            }
            else {
                stability_counter = 0;
                if (_gb->hasdevice("rgb")) { _gb->getdevice("rgb")->on(6); delay (250); _gb->getdevice("rgb")->on(5); }
                if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("-");
            }
            previous_reading = sensor_value;
        }
    }

    else if (sensor_mode == "iterations") {
        
        // Read sensor a fixed number of times
        for(int i = 0; i < NUMBER_OF_SENSOR_READS; i++, ATTEMPT_COUNT++) {

            // Get sensor value
            sensor_value = this->_read();

            // Calculate statistics
            if (sensor_value < min) min = sensor_value;
            if (max < sensor_value) max = sensor_value;
            avg = (avg * (ATTEMPT_COUNT - 1) + sensor_value) / ATTEMPT_COUNT;

            // If max number of read attempts were made, exit
            if (ATTEMPT_COUNT >= MAX_ATTEMPTS) {
                i = NUMBER_OF_SENSOR_READS;
            }
        }
    }

    else if (sensor_mode == "single") {
            
        // Read sensor once
        sensor_value = this->_read();
        min = avg = max = sensor_value;
    }

    else _gb->arrow().color("red").log("Unknown mode.", false);
    
    _gb->arrow().log("" + String(sensor_value) + (abs(sensor_value - 48) <= 1 ? " -> The sensor might not be connected." : "") + " (" + String((millis() - timer) / 1000) + " seconds)", false);
    _gb->log(String(" -> ") + String(min) + " |--- " + String(avg) + " ---| " + String(max));

    // Deactivate and turn off
    this->deactivate();
    this->off();

    _gb->getdevice("gdc")->send("gdc-db", "dox=" + String(sensor_value));

    return sensor_value;
}

// Sensor calibration
int GB_AT_SCI_DO::calibrate(String action, int value) {
    
    // If device wasn't initialized/detected
    if (!this->device.detected) this->_initialize(false);

    if (action == "clear") return this->_calibrate("clr", 0);
    else if (action == "atm") return this->_calibrate("atm", value);
    else if (action == "zero") return this->_calibrate("zero", value);
    else if (action == "sat") return this->_calibrate("sat", value);
    return this->_calibrate("?", 0);
}

// Write a byte to an OEM register
void GB_AT_SCI_DO::_write_byte(byte reg, byte data) {
    this->on();
    Wire.beginTransmission(this->addresses.bus);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
    // this->off();
}

// Write a long to an OEM register
void GB_AT_SCI_DO::_write_long(byte reg, unsigned long data) {
    
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
void GB_AT_SCI_DO::_read_register(byte register_address, byte number_of_bytes_to_read) {
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
    // this->off();
}

// Sensor info function
bool GB_AT_SCI_DO::_test_connection() {
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
String GB_AT_SCI_DO::_device_info() {
    this->on();

    const byte device_type_register = 0x00;
    this->_read_register(device_type_register, 0x02);
    if(_acquired_data.i2c_data[1] == 255) {
        _gb->log("Error reading device info.");
        return "ID: N/A vN/A";
    }

    String data = "ID: " + String(_acquired_data.i2c_data[1]) + " v" + String(_acquired_data.i2c_data[0]);

    // _gb->arrow().log("", false);
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
void GB_AT_SCI_DO::_led_control(String cmd) {
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

// Calibration procedure
int GB_AT_SCI_DO::_calibrate(String cmd, int value) {

    if (cmd.compareTo("?") == 0) {
        this->_read_register(this->registers.calibration_confirmation, 0x01);

        String status = "";
        if (this->_acquired_data.i2c_data[0] == 0) status = "No calibration.";
        if (bitRead(this->_acquired_data.i2c_data[0], 0) == 1) status += "Atmospheric calibration found. ";
        if (bitRead(this->_acquired_data.i2c_data[0], 1) == 1) status += "Zero D.O. solution found. ";

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

    if (cmd.compareTo("atm") == 0) {
        const byte CAL_TYPE = 0x02;
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(40);
    
        _gb->log("Calibration status: ", false);
        this->_calibrate("?", 0);
    }

    if (cmd.compareTo("zero") == 0) {
        const byte CAL_TYPE = 0x03;
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(40);
    
        _gb->log("Calibration status: ", false);
        this->_calibrate("?", 0);
    }

    return this->_acquired_data.i2c_data[0];
}

#endif