#ifndef GB_AT_SCI_PH_h
#define GB_AT_SCI_PH_h

/*
    ! Uses 2.5% flash memory
*/

#ifndef GB_h
    #include "../../GB.h"
#endif

class GB_AT_SCI_PH : public GB_DEVICE {
    public:
        GB_AT_SCI_PH(GB &gb);

        struct PINS {
            bool pwrmux;
            int enable;
            bool commux;
            int muxchannel;
        } pins;
        
        DEVICE device = {
            "ph",
            "pH OEM module"
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
            int compensation_value;
            int compensation_confirmation;
        };

        ADDRESSES addresses = {0x65};
        REGISTERS registers = {0x16, 0x05, 0x06, 0x07, 0x08, 0x0D, 0x0C, 0X0E, 0x12};

        GB_AT_SCI_PH& initialize();
        GB_AT_SCI_PH& initialize(bool testdevice);
        GB_AT_SCI_PH& configure(PINS);
        GB_AT_SCI_PH& configure(PINS, ADDRESSES);
        GB_AT_SCI_PH& persistent();
        GB_AT_SCI_PH& on();
        GB_AT_SCI_PH& off();
        GB_AT_SCI_PH& activate();
        GB_AT_SCI_PH& deactivate();
        GB_AT_SCI_PH& led(String);
        int calibrate(String, int);
        
        bool testdevice();
        String status();

        float readsensor();
        float readsensor(String mode);
        float quickreadsensor(int times);

        bool stablereadings = false;

    private:
        GB *_gb;
        union sensor_mem_handler {
            byte i2c_data[4];
            long answ;
            float answf;
        } _acquired_data;
        bool _persistent = false;

        void _write_byte(byte, byte);
        void _write_long(byte, unsigned long);
        void _read_register(byte, byte);
        String _device_info();
        void _led_control(String);
        int _calibrate(String, int);
        float _read();
        void _compensate(float value);
        bool _test_connection();
};


GB_AT_SCI_PH::GB_AT_SCI_PH(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.ph = this;
}

// Test the device
bool GB_AT_SCI_PH::testdevice() { 
    
    this->device.detected = this->_test_connection();
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_AT_SCI_PH::status() { 
    return this->device.detected ? this->_device_info() : "not-detected" + String(":") + device.id;
}

GB_AT_SCI_PH& GB_AT_SCI_PH::initialize () { this->initialize(false); } 
GB_AT_SCI_PH& GB_AT_SCI_PH::initialize (bool testdevice) { 
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

GB_AT_SCI_PH& GB_AT_SCI_PH::configure(PINS pins) { return this->configure(pins, this->addresses); }
GB_AT_SCI_PH& GB_AT_SCI_PH::configure(PINS pins, ADDRESSES addresses) { 
    this->pins = pins;
    this->addresses = addresses;

    // Set pin modes of the device
    if(!this->pins.pwrmux) pinMode(this->pins.enable, OUTPUT);
    
    return *this;
}

/*
    Keeps device persistently on
*/
GB_AT_SCI_PH& GB_AT_SCI_PH::persistent() {
    this->_persistent = true;
    return *this;
}

// Turn on the module
GB_AT_SCI_PH& GB_AT_SCI_PH::on() {
    delay(2);
    
    // Reset mux
    _gb->getdevice("tca")->resetmux();

    // Power on the device
    if(this->pins.pwrmux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);

    // Select I2C mux channel
    if(this->pins.commux) _gb->getdevice("tca")->selectexclusive(pins.muxchannel);

    delay(10);

    return *this;
}

// Turn off the module
GB_AT_SCI_PH& GB_AT_SCI_PH::off() {
    
    if (this->_persistent) return *this;
    delay(10);
    if(this->pins.pwrmux) _gb->getdevice("ioe")->writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    delay(2);
    return *this;
}

// Activate the module
GB_AT_SCI_PH& GB_AT_SCI_PH::activate() {
    this->_write_byte(this->registers.hibernation, 0x01);
    this->_read_register(this->registers.hibernation, 0x01);
    delay(200);
    return *this;
}

// Deactivate the module
GB_AT_SCI_PH& GB_AT_SCI_PH::deactivate() {
    this->_write_byte(this->registers.hibernation, 0x00);
    delay(100);
    return *this;
}

// LED control
GB_AT_SCI_PH& GB_AT_SCI_PH::led(String state) {
    this->_led_control(state);
    return *this;
}

// Perform temperature compensation
/*
    TODO: Needs fixing
*/
void GB_AT_SCI_PH::_compensate(float value) {


    // The device must be active before compensation is applied
    this->on();

    // Set new compensation value
    _acquired_data.answf = value;
    _gb->log(" -> Temperature compensation: " + String(_acquired_data.answf), false);
    this->_write_long(this->registers.compensation_value, _acquired_data.answ);
    delay(200);

    this->activate();
    
    // Get compensation data from the OEM chip
	float compensation;
    this->_read_register(this->registers.compensation_confirmation, 0x04);
    compensation = _acquired_data.answf;
    _gb->log(" -> " + String(compensation), false);
    compensation /= 100;
    _gb->log(" -> " + String(compensation == value ? "done" : "failed"), false);

    /* 
        ! Do not turn off the device. The compensation register are NOT retained after a power off.
    */
    this->deactivate();
}

// Read the sensor
float GB_AT_SCI_PH::_read() {
    
    // Enable watchdog
    _gb->getmcu()->watchdog("enable");

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

        // The new data available register needs to be manually reset to 0 according to the datasheet
        this->_write_byte(this->registers.new_reading, 0x00);
    }
    
    // Disable watchdog
    _gb->getmcu()->watchdog("disable");

    return sensor_value;
}

float GB_AT_SCI_PH::readsensor(String mode) {
    
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

float GB_AT_SCI_PH::quickreadsensor(int times) {

    this->on();
    this->activate();

    // Variables
    String sensor_mode = _gb->globals.SENSOR_MODE;
    float sensor_value = -1;
    float stability_delta = 0.025;
    int min_stable_reading_count = 10;
    float previous_reading = 0;
    int stability_counter = 0;
    
    // Variables
    int NUMBER_OF_SENSOR_READS = times;
    int MAX_ATTEMPTS = 40;
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

    _gb->getdevice("gdc")->send("gdc-db", "ph=" + String(sensor_value));

    return sensor_value;
}

float GB_AT_SCI_PH::readsensor() {

    // Loop mqtt
    // if (this->_gb->hasdevice("mqtt")) this->_gb->getdevice("mqtt").update();
    // this->_gb->breathe();

    // Return a dummy value if "dummy" mode is on
    if (this->_gb->globals.MODE == "dummy") {
        this->_gb->log("Reading " + this->device.name, false);
        float value = random(5, 29) + random(0, 100) / 100.00;
        this->_gb->log(" -> Dummy value: " + String(value));
        _gb->getdevice("gdc")->send("gdc-db", "ph=" + String(value));
        return value;
    }

    // Return if device not detected
    this->device.detected = this->_test_connection();
    if (!this->device.detected) {
        this->_gb->log("Reading " + this->device.name, false);
        this->_gb->log(" -> Device not detected");
        _gb->getdevice("gdc")->send("gdc-db", "ph=" + String(-1));
        return -1;
    }

    _gb->log("Compensation temperature value: " + String(_gb->getdevice("rtd")->lastvalue()));
    
    // // Get temperature sensor value for compensation.
    // this->_compensate(_gb->hasdevice("rtd") ? _gb->getdevice("rtd")->lastvalue() : 25) ;
    
    this->on();
    this->activate();

    // Variables
    String sensor_mode = _gb->globals.SENSOR_MODE;
    float sensor_value = -1;
    float stability_delta = 0.025;
    int min_stable_reading_count = 10;
    float previous_reading = 0;
    int stability_counter = 0;
    
    // Variables
    int NUMBER_OF_SENSOR_READS = 20;
    int MAX_ATTEMPTS = 40;
    int ATTEMPT_COUNT = 0;

    // Get readings
    int timer = millis();
    float min = 99, max = 0, avg = 0;

    if (sensor_mode == "stability") {
        
        this->_gb->log("Reading " + this->device.name + " until stable", false);
        
        /*
            If the mode is not 'persistent', pH module needs a 30 seconds warmup delay for accurate reading.
        */
        if (!this->_persistent) {
            this->_gb->log(" -> 30 seconds warm-up delay", false);

            int counter = 30;
            while (counter-- >= 0) { 
                delay(1000);

                // // Loop mqtt
                // if (this->_gb->hasdevice("mqtt")) this->_gb->getdevice("mqtt").update();
            }
        }

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
                if (_gb->hasdevice("rgb")) { _gb->getdevice("rgb")->on("green"); delay (250); _gb->getdevice("rgb")->on("magenta"); }
            }
            else {
                stability_counter = 0;
                if (_gb->hasdevice("rgb")) { _gb->getdevice("rgb")->on("yellow"); delay (250); _gb->getdevice("rgb")->on("magenta"); }
            }
            previous_reading = sensor_value;
        }

        if (ATTEMPT_COUNT == MAX_ATTEMPTS) {
            this->_gb->arrow().color("yellow").log("Stability not acheived");
            this->stablereadings = false;
        }
        else this->stablereadings = true;

    }
    else if (sensor_mode == "iterations") {
        
        this->_gb->log("Reading " + this->device.name + " " + String(NUMBER_OF_SENSOR_READS) + " times", false);
        
        /*
            If the mode is not 'persistent', pH module needs a 30 seconds warmup delay for accurate reading.
        */
        if (!this->_persistent) { this->_gb->log(" -> 30 seconds warm-up delay", false); delay(30 * 1000); }

        // Read sensor a fixed number of times
        for(int i = 0; i < NUMBER_OF_SENSOR_READS; i++, ATTEMPT_COUNT++) {

            // GB breathe
            this->_gb->breathe();

            // Get sensor value
            sensor_value = this->_read();

            // _gb->log(sensor_value);
            
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

    else this->_gb->log("Reading " + this->device.name + " -> Sensor read mode not provided", false);

    this->_gb->log(" -> " + String(sensor_value) + (sensor_value == 14 || sensor_value < 0 ? " -> The sensor might not be connected." : "") + " (" + String((millis() - timer) / 1000) + " seconds)", false);
    this->_gb->log(String(" -> ") + String(min) + " |--- " + String(avg) + " ---| " + String(max));
    
    this->deactivate();
    this->off();

    _gb->getdevice("gdc")->send("gdc-db", "ph=" + String(sensor_value));

    return sensor_value;
}

// Sensor calibration
int GB_AT_SCI_PH::calibrate(String action, int value) {
    if (action == "clear") return this->_calibrate("clr", 0);
    else if (action == "low") return this->_calibrate("low", value);
    else if (action == "mid") return this->_calibrate("mid", value);
    else if (action == "high") return this->_calibrate("high", value);
    return this->_calibrate("?", 0);
}

// Write a long to an OEM register
void GB_AT_SCI_PH::_write_long(byte reg, unsigned long data) {
    
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

// Write a byte to an OEM register
void GB_AT_SCI_PH::_write_byte(byte reg, byte data) {
    this->on();
    Wire.beginTransmission(this->addresses.bus);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
    // this->off();
}

// Read OEM device register
void GB_AT_SCI_PH::_read_register(byte register_address, byte number_of_bytes_to_read) {
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
bool GB_AT_SCI_PH::_test_connection() {
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
String GB_AT_SCI_PH::_device_info() {
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
void GB_AT_SCI_PH::_led_control(String cmd) {
    
    this->on();
    if (cmd == "?") {
        this->_read_register(this->registers.led, 0x01);
        if (_acquired_data.i2c_data[0] == 0) _gb->log("LED is OFF.");
        else if (_acquired_data.i2c_data[0] == 1) _gb->log("LED is ON.");
        else _gb->log("Error reading LED state.");
        return;
    }
    else {
        _gb->log("LED control: " + cmd);
        byte led_control = cmd == "on" ? 0x01 : 0x00;
        this->_write_byte(this->registers.led, led_control);
        this->_read_register(this->registers.led, 0x01);
        if (_acquired_data.i2c_data[0] != led_control) _gb->log("Error changing LED state.");
    }
    this->off();
}

// Calibration procedure
int GB_AT_SCI_PH::_calibrate(String cmd, int value) {

    if (cmd.compareTo("?") == 0) {
        String status = " ";
        this->_read_register(this->registers.calibration_confirmation, 0x01);
        if (this->_acquired_data.i2c_data[0] == 0) status = "No calibration.";
        if (bitRead(this->_acquired_data.i2c_data[0], 0) == 1) status += "Low-point calibration found. ";
        if (bitRead(this->_acquired_data.i2c_data[0], 1) == 1) status += "Mid-point calibration found. ";
        if (bitRead(this->_acquired_data.i2c_data[0], 2) == 1) status += "High-point calibration found. ";
        
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
        const long PH_VALUE = value * 1000;
        const byte CAL_TYPE = 0x02;

        // Write calibration solution value to register
        this->_write_long(this->registers.calibration_value, PH_VALUE);

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(165);

        _gb->log("Calibration status: ", false);
        this->_calibrate("?", 0);
    }

    if (cmd.compareTo("mid") == 0) {
        const long PH_VALUE = value * 1000;
        const byte CAL_TYPE = 0x03;

        // Write calibration solution value to register
        this->_write_long(this->registers.calibration_value, PH_VALUE);

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(165);
        
        // Check if the calibration was successful
        // this->_read_register(this->registers.calibration_confirmation, 0x01);
        // if (bitRead(this->_acquired_data.i2c_data[0], 1) == 1) _gb->log("Complete.");
        // else _gb->log("Failed.");
        
        _gb->log("Calibration status: ");
        this->_calibrate("?", 0);
    }

    if (cmd.compareTo("high") == 0) {
        const long PH_VALUE = value * 1000;
        const byte CAL_TYPE = 0x04;

        // Write calibration solution value to register
        this->_write_long(this->registers.calibration_value, PH_VALUE);

        // Commit the calibration
        this->_write_byte(this->registers.calibration_request, CAL_TYPE);
        delay(165);
        
        // Check if the calibration was successful
        // this->_read_register(this->registers.calibration_confirmation, 0x01);
        // if (bitRead(this->_acquired_data.i2c_data[0], 2) == 1) _gb->log("Complete.");
        // else _gb->log("Failed.");
        _gb->log("Calibration status: ");
        this->_calibrate("?", 0);
    }

    // Return the current calibration status
    return this->_acquired_data.i2c_data[0];
}

#endif