#ifndef GB_SERMUX_h
#define GB_SERMUX_h

/* Import the core GB library */
#ifndef GB_h
    #include "../GB.h"
#endif

class GB_SERMUX : public GB_DEVICE {
    public:

        struct PINS {
            bool mux;
            int power;
            int comm;
            bool persistent;
        } pins;
        
        GB_SERMUX(GB &gb);

        DEVICE device = {
            "sermux",
            "Virtual multiplexer"
        };

        void createchannel(PINS pins, int channel);
        void selectexclusive(int channel);
        void select(int channel);
        void resetmux();

        GB_SERMUX& initialize();
        GB_SERMUX& configure();
        GB_SERMUX& configure(PINS pins);

    private:
        GB *_gb;
        uint8_t _max_channels = 4;
        PINS _bus[4];
};

// Constructor
GB_SERMUX::GB_SERMUX(GB &gb) { 
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.sermux = this;
}

/*
    ! Device configuration
    Configures the device. Optionally, provide the reset pin.
*/
GB_SERMUX& GB_SERMUX::configure() { return *this; }

/*
    ! Initialize device
    Initialize the device. Begins the 'Wire.h' library.
*/
GB_SERMUX& GB_SERMUX::initialize() { 
    _gb->init();
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    return *this;
}

/*
    ! Create channel
    Builds a channel in the multiplexer. Provide a channel number between 0 and 3.
*/
void GB_SERMUX::createchannel(PINS pins, int channel) {

    // Return if invalid channel number was provided
    if (channel < 0 || channel >= _max_channels) return; 

    _gb->log("Creating USART channel: " + String(channel));
    
    // Set bus/channel pins
    _bus[channel] = pins;

    return; 
}

/*
    ! Channel select
    Selects a channel in the multiplexer (Turns off comm only if available).
    Provide a channel number between 0 and 3.
*/
void GB_SERMUX::selectexclusive(int channel) {

    // Return if invalid channel number was provided
    if (channel < 0 || channel > _max_channels) return; 

    _gb->log("Selecting channel: " + String(channel));

    // Turn off all channels except for the provided channel
    for (int i = 0; i < _max_channels; i++) {
        PINS channelpins = _bus[i];

        // _gb->log(String(channel) + ", " + String(i) + ", " + String(channel != i));
        
        if (channel != i) {
                        
            // Turn off the power pin
            if (!channelpins.persistent) {
                _gb->log("Turning off channel: " + String(i));
                if (channelpins.mux) _gb->getdevice("ioe").writepin(channelpins.power, LOW);
                else digitalWrite(channelpins.power, LOW);
            }
            
            // Turn off the comm pin
            if (channelpins.mux) _gb->getdevice("ioe").writepin(channelpins.comm, LOW);
            else digitalWrite(channelpins.comm, LOW);

        }
        else {
            
            // Turn on the power pin
            if(channelpins.mux) _gb->getdevice("ioe").writepin(channelpins.power, HIGH);
            else digitalWrite(channelpins.power, HIGH);
            
            // Turn on the comm pin
            if(channelpins.mux) _gb->getdevice("ioe").writepin(channelpins.comm, HIGH);
            else digitalWrite(channelpins.comm, HIGH);
        }
        
    }
    delay(25);

    return; 
}

/*
    ! Reset multiplexer
    Resets the multiplexer.
*/
void GB_SERMUX::resetmux() { 
    

    return; 
}

#endif