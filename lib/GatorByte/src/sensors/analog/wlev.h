#ifndef GB_IWLS_h
#define GB_IWLS_h

#ifndef GB_h
    #include "../../GB.h"
#endif

#ifndef ADS1115_DRIVER
    #include "ADS1115-Driver.h"
#endif


class GB_IWLS {
    public:
        GB_IWLS(GB&);

        DEVICE device = {
            "wlev",
            "Throw-in water level sensor"
        };

        struct PINS {
            bool mux;
            int enable;
            int data
        } pins;

        struct ADDRESSES {
            int bus;
        };
        

        GB_IWLS& initialize();
        GB_IWLS& configure(PINS, ADDRESSES);
        GB_IWLS& on();
        GB_IWLS& off();
        float readsensor();
        
        int VREF = 3300;
        float CURRENT_INIT = 4.0;
        int RANGE = 5000;
        float DENSITY_FLUID = 1;
        int PRINT_INTERVAL = 1000;
        float ADC_VDD_VALUE = 32767.0;

    private:
        GB *_gb;
        ADS1115 _ads1115 = ADS1115(ADS1115_I2C_ADDR_GND);

        uint16_t _read_ads(uint8_t addressmodifier);

        int _rain_pulse_state;
        unsigned long _debounce_delay = 1500;
        bool _last_rain_pulse_state = LOW;
        bool _last_debounce_at = 0;
        int _rain_pulse_count = 0;
        // PCF8575 _ioe = PCF8575(0x20);
};

GB_IWLS::GB_IWLS(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);

    // TODO Verify this
    _gb->devices.wlev = this;
}

// TODO Write initialize functions

GB_IWLS& GB_IWLS::configure(PINS pins, ADDRESSES addresses) {
    this->pins = pins;
    this->addresses = addresses;
    _gb->includedevice(this->device.id, this->device.name);
    
    // Set pin modes
    pinMode(this->pins.data, INPUT);
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    return *this;
}

GB_IWLS& GB_IWLS::on() { 
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH); 
    else digitalWrite(this->pins.enable, HIGH); 
}

GB_IWLS& GB_IWLS::off() { 
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, LOW); 
    else digitalWrite(this->pins.enable, LOW); 
    return *this;
}

uint16_t GB_IWLS::_read_ads(uint8_t input) {

  // Set the channel
  _ads1115.setMultiplexer(input);

  // Get reading
  _ads1115.startSingleConvertion();

  // The ADS1115 needs to wake up from sleep mode
  delay(1); 

  // Wait while until reading is ready
  while (!_ads1115.getOperationalStatus());

  // Return the reading
  return _ads1115.readConvertedValue();
}

int GB_IWLS::read() { 
    this->on();
    
    // Read ADC channel
    int16_t value = this->_read_ads(ADS1115_MUX_AIN0_GND);

    // Convert the ADC reading to voltage (mV)
    float voltage = (value / ADC_VDD_VALUE) * VREF;

    // Convert the voltage to current (mA) using a sense resistor (120 ohm)
    float current= voltage / 120.0;

    // Caluculate depth in mm
    float depth = (current - CURRENT_INIT) * (RANGE / DENSITY_FLUID / 16.0);

    // Ignore invalid readings
    if (depth < 0) depth = 0.0;
    
    this->off();
    return depth;
}

#endif