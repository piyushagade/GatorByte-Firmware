#ifndef GB_TCA9548A_h
#define GB_TCA9548A_h

/* Import the core GB library */
#ifndef GB_h
    #include "../GB.h"
#endif

class GB_TCA9548A : public GB_DEVICE {
    public:

        struct PINS {
            int reset;
        } pins;
        
        GB_TCA9548A(GB &gb);

        DEVICE device = {
            "tca",
            "TCA9548A"
        };

        void selectexclusive(int channel);
        void select(int channel);
        bool isselected(int channel);
        void deselect();
        void resetmux();

        GB_TCA9548A& initialize();
        GB_TCA9548A& configure();
        GB_TCA9548A& configure(PINS pins);

    private:
        GB *_gb;
        int _tca_address = 0x70;
        byte _control_byte = 0b0;
};

// Constructor
GB_TCA9548A::GB_TCA9548A(GB &gb) { 
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.tca = this;
}

/*
    ! Device configuration
    Configures the device. Optionally, provide the reset pin.
*/
GB_TCA9548A& GB_TCA9548A::configure() { return *this; }
GB_TCA9548A& GB_TCA9548A::configure(PINS pins) { 
    this->pins = pins;

    // Set pin mode
    pinMode(this->pins.reset, OUTPUT);
    digitalWrite(this->pins.reset, HIGH);

    return *this;
}

/*
    ! Initialize device
    Initialize the device. Begins the 'Wire.h' library.
*/
GB_TCA9548A& GB_TCA9548A::initialize() { 
    _gb->init();
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    _gb->log("Initializing " + this->device.name, false);

    Wire.begin();
    Wire.beginTransmission(this->_tca_address);
    bool success = Wire.endTransmission() == 0;
    this->device.detected = success;

    _gb->arrow().log(success ? "Done" : "Failed");

    return *this;
}

/*
    ! Exclusive channel select
    Selects a channel exclusively in the multiplexer. Provide a channel number between 0 and 7.
*/
void GB_TCA9548A::selectexclusive(int channel) {

    bool log = false;

    // Return if invalid channel number was provided
    if (channel < 0 || channel > 7) return; 

    if (log) _gb->log("Exclusively selecting channel: " + String(channel), false);

    // Select single channel
    bool success = false; int counter = 3;
    while (!success && counter-- > 0) {
        Wire.beginTransmission(this->_tca_address);
        Wire.write(1 << channel);
        success = Wire.endTransmission() == 0;
    }

    // Check if the channel was successfully selected
    success = this->isselected(channel);

    if (success) {

        // Update control byte
        this->_control_byte = 1 << channel;

        if (log) _gb->arrow().log("Done (" + String(this->_control_byte) + ")");
    }
    else {
        if (log) _gb->arrow().log("Failed");
    }
    
    delay(25);

    return; 
}

/*
    ! Channel select
    Selects a channel in the multiplexer. Provide a channel number between 0 and 7.
*/
void GB_TCA9548A::select(int channel) {

    bool log = false;

    _gb->log("Selecting channel: " + String(channel));

    // Return if invalid channel number was provided
    if (channel < 0 || channel > 7) return; 

    if (log) _gb->log("Exclusively selecting channel: " + String(channel), false);

    // Select single channel
    bool success = false; int counter = 3;
    while (!success && counter-- > 0) {
        Wire.beginTransmission(this->_tca_address);
        Wire.write(this->_control_byte | (1 << channel));
        success = Wire.endTransmission();
    }

    // Check if the channel was successfully selected
    success = this->isselected(channel);

    if (success) {

        // Update control byte
        this->_control_byte |= 1 << channel;

        if (log) _gb->arrow().log("Done (" + String(this->_control_byte) + ")");
    }
    else {
        if (log) _gb->arrow().log("Failed");
    }

    delay(25);

    return; 
}

/*
    ! Check if a channel is selected
    Reads the control register in the TCA9548A.
*/
bool GB_TCA9548A::isselected(int channel) {

    // Return if invalid channel number was provided
    if (channel < 0 || channel > 7) return false; 

    // Select single channel
    Wire.requestFrom(this->_tca_address, 1);
    if (Wire.available()) {
        this->_control_byte = Wire.read();
    }
    return (this->_control_byte & (1 << channel)) != 0;
}

void GB_TCA9548A::deselect() {

    // _gb->log("Deselecting channel: " + String(channel));

    // Deselect all channels
    bool success = false; int counter = 3;
    while (!success && counter-- > 0) {
        Wire.beginTransmission(this->_tca_address);
        Wire.write(0);
        success = Wire.endTransmission();

        // Try resetting the TCA9548A
        if (!success) this->resetmux();
    }

    // Update control byte
    if (success) this->_control_byte = 0b0;

    delay(25);

    return; 
}

/*
    ! Reset multiplexer
    Resets the multiplexer.
*/
void GB_TCA9548A::resetmux() { 

    // Set pin low (active low)
    digitalWrite(this->pins.reset, LOW);
    delay(25);
    
    // Set pin high (active low)
    digitalWrite(this->pins.reset, HIGH);
    delay(25);

    return; 
}

#endif