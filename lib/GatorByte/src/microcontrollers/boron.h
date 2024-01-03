#ifndef GB_BORON_h
#define GB_BORON_h

//TODO: Rewrite this library to be same as other microcontrollers
#if (defined(PARTICLE) || defined(SPARK)) && (defined(PLATFORM_ID) && PLATFORM_ID == 13)
    #include "Particle.h"

    #ifndef Wire
        #include "Wire.h"
    #endif

    #ifndef GB_h
        #include "../GB.h"
    #endif

    // Timer serverTimeoutTimer(45000, on_http_request_timeout);

    class GB_BORON {
        public:
            GB_BORON(GB &gb);
            
            DEVICE device = {
                "boron",
                "Particle Boron",
                "Microcontroller with 4G",
                "N/A"
            };

            GB_NB1500& i2c();
            GB_NB1500& debug(Serial_ &ser, int);
            GB_NB1500& serial(Uart &ser, int);
            GB_NB1500& wait(int);
            
            
            // Power management
            void sleep(int);
            void reset();

            // Cellular functions
            void connectToParticleCloud();
            void safemode();
            void connect(String type);
            void disconnect(String type);

            Client& client();

            // Inbuilt LED
            void blink();

        private:
            GB *_gb;
            GB_PCF8575 _ioe = GB_PCF8575();
            FuelGauge _fuel;
    };

    GB_BORON::GB_BORON(GB &gb) {
        // Set GatorByte object pointer
        _gb = &gb;
        _gb->includelibrary(this->device.id, this->device.name);
        _gb->devices.mcu = this;
        _gb->serial = {&Serial, &Serial1};
        
        // HTTP client
        // TODO: May need to make changes
        this->_client = TCPClient();

        // Set pinMode for builtin LED
        pinMode(D7, OUTPUT);
    }

    Client& GB_NB1500::client() {
        return this->_client;
    }


    GB_ESP32& GB_ESP32::i2c() {
        // Initialize I2C for peripherals
        Wire.begin();

        return *this;
    }

    GB_ESP32& GB_ESP32::debug(HardwareSerial &ser, int baud_rate) {
        // Enable serial port for debugging
        _serial.debug = &ser;
        _baudrate.debug = baud_rate;
        _serial.debug->begin(_baudrate.debug);

        _gb->serial = _serial;

        return *this;
    }

    GB_ESP32& GB_ESP32::serial(HardwareSerial &ser, int baud_rate) {
        // Enable serial port for sensors (peripherals)
        _serial.hardware = &ser;
        _baudrate.hardware = baud_rate;
        _serial.hardware->begin(_baudrate.hardware);

        _gb->serial = _serial;
        return *this;
    }

    void GB_BORON::connect(String type) {
        if(type == "cloud") {
            if(!_gb->CONNECT_TO_PARTICLE_CLOUD) return;
            _gb->log("Connecting to particle cloud.", false);
            Particle.connect(); 
            _gb->log(" -> Done", true);
        }
        else if(type == "cellular") {
            if(_gb->globals.OFFLINE_MODE) return;
            _gb->log("Looking for network asynchronously.", true); 

            // Turn cellular module on
            Cellular.on();

            // Connect to a cellular network
            int time_elapsed_while_connecting_to_network = 0;
            while(!Cellular.ready() && time_elapsed_while_connecting_to_network <= _gb->globals.TIMEOUT_CELLULAR_CONNECTION){
                if(!Cellular.connecting()) Cellular.connect();
                if(Cellular.ready()) _gb->log("Device connected to the internet.", true);
                delay(1000);
                time_elapsed_while_connecting_to_network += 1000;
            }
        }
    }

    void GB_BORON::disconnect(String type) {
        if(type == "cloud") Particle.disconnect();
        else if(type == "cellular") Cellular.off();
    }

    void GB_BORON::sleep(int milliseconds) {
        _gb->log("\nEntering power saving mode. Sleeping for " + String(duration_ms / 60 / 1000) + " min.", true);
        delay(2000);

        System.sleep(D1, RISING, duration_ms / 1000);
    }

    void GB_BORON::safemode() {
        System.enterSafeMode();
    }

    void GB_BORON::reset() { 
        System.reset();
    }

    #endif
#endif