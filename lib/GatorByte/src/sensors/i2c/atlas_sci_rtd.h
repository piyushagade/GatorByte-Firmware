#ifndef GB_AT_SCI_RTD_h
#define GB_AT_SCI_RTD_h

/*
    ! Uses 2.5% flash memory
*/

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
            uint8_t bus;
        };
        struct REGISTERS {
            uint8_t data;
            uint8_t led;
            uint8_t hibernation;
            uint8_t new_reading;
            uint8_t calibration_control;
            uint8_t calibration_confirmation;
            uint8_t calibration_request;
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

GB_AT_SCI_RTD& GB_AT_SCI_RTD::initialize () { return this->initialize(false); } 
GB_AT_SCI_RTD& GB_AT_SCI_RTD::initialize (bool testdevice) { 
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
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on("green").wait(250).revert();
        }
        else {
            _gb->arrow().log("Not detected");
            _gb->globals.INIT_REPORT += this->device.id;
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("..").wait(250).play("---");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on("red").wait(250).revert();
        }
    }
    else _gb->arrow().log(this->device.detected ? "Done" : "Not detected");
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
    _gb->getdevice("tca")->resetmux();
    
    // Power on the device
    if(this->pins.pwrmux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);

    // Select I2C mux channel
    if(this->pins.commux) _gb->getdevice("tca")->selectexclusive(pins.muxchannel);

    delay(50);
    return *this;
}

// Turn off the module
GB_AT_SCI_RTD& GB_AT_SCI_RTD::off() {
    if (this->_persistent) return *this;
    delay(10);
    if(this->pins.pwrmux) this->_gb->getdevice("ioe")->writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    delay(2);
    return *this;
}

// Activate the module
GB_AT_SCI_RTD& GB_AT_SCI_RTD::activate() {
    this->on();
    this->_write_byte(this->registers.hibernation, 0x01);
    this->_read_register(this->registers.hibernation, 0x01);
    delay(200);
    return *this;
}

// Deactivate the module
GB_AT_SCI_RTD& GB_AT_SCI_RTD::deactivate() {
    this->on();
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
    _gb->getmcu()->watchdog("enable");

    float sensor_value = -1;
    float divident = 1000;
    int DELAY_BETWEEN_SENSOR_READS = 0;

    // Sensor's temporal resolution (from datasheet)
    delay(DELAY_BETWEEN_SENSOR_READS);

    this->_read_register(this->registers.new_reading, 0x01);
    bool new_data_available = _acquired_data.i2c_data[0];

    // _gb->log("New data available: " + String(new_data_available));
    
    // If new data is available
    if (new_data_available) {
        
        // Read sensor value
        this->_read_register(this->registers.data, 0x04);

        // if (_acquired_data.i2c_data[1] != 99 && _acquired_data.i2c_data[0] == 232) {
            // _acquired_data.i2c_data[3] = 0;
            // _acquired_data.i2c_data[2] = 0;
        // }

        sensor_value = _acquired_data.answ / divident; 
        this->_latest_value = sensor_value;

        // Serial.print("New data value: ");
        // Serial.print(String(_acquired_data.i2c_data[3], HEX) + " ");
        // Serial.print(String(_acquired_data.i2c_data[2], HEX) + " ");
        // Serial.print(String(_acquired_data.i2c_data[1], HEX) + " ");
        // Serial.println(String(_acquired_data.i2c_data[0], HEX));

        // Serial.println(sensor_value);

        // The 'new data available register' needs to be manually reset to 0 according to the datasheet
        this->_write_byte(this->registers.new_reading, 0x00);
    }
    
    // Disable watchdog
    _gb->getmcu()->watchdog("disable");

    return sensor_value;
}

float GB_AT_SCI_RTD::readsensor(String mode) {
    
    float sensor_value = -1;

    // Take continuos readings
    if (mode == "continuous") {
        int NUMBER_OF_READINGS = 10;

        _gb->br().log("Reading " + String(NUMBER_OF_READINGS) + " values from " + this->device.name);
        this->activate();
        for (int i = 0; i < NUMBER_OF_READINGS; i++) {
            sensor_value = this->_read(); 
            _gb->log(String(i) + ": " + String(sensor_value));
        }
        this->deactivate();
    }

    // Take one reading
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

    // Determine number of times to read the sensor
    int NUMBER_OF_SENSOR_READS = 10;
    int MAX_ATTEMPTS = 10;
    int ATTEMPT_COUNT = 0;

    // Get readings
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
    this->_gb->arrow().log(String(min) + " |--- " + String(avg) + " ---| " + String(max));

    _gb->getdevice("gdc")->send("gdc-db", "rtd=" + String(sensor_value));

    return sensor_value;
}

// Read the sensor (w/ filtering)
float GB_AT_SCI_RTD::readsensor() {

    // Loop mqtt
    // if (this->_gb->hasdevice("mqtt")) this->_gb->getdevice("mqtt").update();
    // this->_gb->breathe();
        
    // Return a dummy value if "dummy" mode is on
    if (this->_gb->globals.MODE == "dummy") {
        this->_gb->log("Reading " + this->device.name, false);
        float value = random(5, 29) + random(0, 100) / 100.00;
        this->_gb->arrow().log("Dummy value: " + String(value));
        _gb->getdevice("gdc")->send("gdc-db", "rtd=" + String(value));
        this->off();
        return value;
    }

    // Return if device not detected
    this->device.detected = this->_test_connection();
    if (!this->device.detected) {
        this->_gb->log("Reading " + this->device.name, false);
        this->_gb->arrow().log("Device not detected");
        _gb->getdevice("gdc")->send("gdc-db", "rtd=" + String(-1));
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
            
            bool dummy = _gb->env() == "development";
            // if (digitalRead(A6) == HIGH) {
            //     _gb->arrow().color("yellow").log("Override", false);
            //     dummy = true;
            // }
            
            if (dummy) {
                _gb->arrow().log("Dummy data");
                sensor_value = 8.88;
                return sensor_value;
            }
            else {

                // Get updated sensor value
                sensor_value = this->_read();

                // Check if the readings are stable
                float difference = abs(sensor_value - previous_reading);
                if (difference <= stability_delta) {
                    stability_counter++;
                    if (_gb->hasdevice("rgb")) { _gb->getdevice("rgb")->on("green"); delay (250); _gb->getdevice("rgb")->on("magenta"); }
                    if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play(".");
                }
                else {
                    stability_counter = 0;
                    if (_gb->hasdevice("rgb")) { _gb->getdevice("rgb")->on("yellow"); delay (250); _gb->getdevice("rgb")->on("magenta"); }
                    if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("-");
                }
                previous_reading = sensor_value;
            }
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
    
    this->_gb->arrow().log("" + String(sensor_value) + (sensor_value == -1023.00 ? " -> The sensor might not be connected." : "") + " (" + String((millis() - timer) / 1000) + " seconds)");
    
    this->deactivate();
    this->off();
    // this->_gb->arrow().log("" + String(sensor_value));
    _gb->getdevice("gdc")->send("gdc-db", "rtd=" + String(sensor_value));

    return sensor_value;
}

// Read the sensor
float GB_AT_SCI_RTD::lastvalue() {
    return this->_latest_value < 0 ? 25 : this->_latest_value;
}

// Sensor calibration
int GB_AT_SCI_RTD::calibrate(String action, int value) {
    if (action == "clear" || action == "clr") return this->_calibrate("clr", 0);
    else if (action == "single") return this->_calibrate("single", value);
    return this->_calibrate("?", 0);
}

// Calibration procedure
int GB_AT_SCI_RTD::_calibrate(String cmd, int value) {

    const byte calibration_value_register = 0x08; 
	const byte calibration_request_register = 0x0C; 
	const byte calibration_confirmation_register = 0x0D; 
	const byte cal_clear = 0x01; 
	const byte calibrate = 0x02; 
	float calibration = 0; 


	if (cmd == "?") {								
		Serial.print("Calibration status: ");
		this->_read_register(calibration_confirmation_register, 0x01); 
		if (_acquired_data.i2c_data[0] == 0) Serial.println("No calibration");
		if (_acquired_data.i2c_data[0] == 1) Serial.println("Single-point calibration found");
	}

	if (cmd == "clr") {								
		_write_byte(calibration_request_register, cal_clear); 
		delay(40);											 
		_read_register(calibration_confirmation_register, 0x01); 
		if (_acquired_data.i2c_data[0] == 0) Serial.println("Calibration cleared");
	}

	if (cmd == "single") { 
		calibration = value; 
		calibration *= 1000; 
		_acquired_data.answ = calibration; 
		_write_long(calibration_value_register, _acquired_data.answ); 
		_write_byte(calibration_request_register, calibrate); 
		delay(50); 
		_read_register(calibration_confirmation_register, 0x01); 
		if (_acquired_data.i2c_data[0] == 1) Serial.println("Calibrated to: " + String(_acquired_data.answ / 1000) + " C");
	}

    return 0;
}

// Write a byte to an OEM register
void GB_AT_SCI_RTD::_write_byte(byte reg, byte data) {
    this->on();
    delay(10);
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
    // this->off();
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
    // this->off();
}

#endif