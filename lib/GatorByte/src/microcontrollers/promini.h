#ifdef ARDUINO_AVR_PRO

    #define MCU_HAS_I2C
    #define MCU_HAS_SPI
    #define MCU_HAS_UART

    #define GB_PROMINI_h

    // Include required libraries
    #ifndef Arduino_h
        #include "Arduino.h"
    #endif

    #ifndef Wire
        #include "Wire.h"
    #endif

    #ifndef GB_h
        #include "../GB.h"
    #endif

    class GB_PROMINI: public GB_MCU {
        public:
            GB_PROMINI(GB &gb);
            
            DEVICE device = {
                "promini",
                "Pro Mini",
                "3.3V Pro Mini microcontroller",
                "Arduino"
            };

            GB_PROMINI& configure(String ssid, String password);
            GB_PROMINI& i2c();

            //TODO: Fix serial
            GB_PROMINI& debug(HardwareSerial &ser, int);
            GB_PROMINI& serial(HardwareSerial &ser, int);
            GB_PROMINI& wait(int);
            void i2cscan();
            
            // Power management
            void sleep(int);
            void reset();

            String help();

            // Inbuilt LED
            void blink();

            

        private:
            GB *_gb;
            GB_MCU *_mcu;

            _SER_PORTS _serial = {&Serial};
            _BAUD_RATES _baudrate = {9600};
    };

    GB_PROMINI::GB_PROMINI(GB &gb) {
        // Set GatorByte object pointer
        _gb = &gb;
        _gb->includelibrary(this->device.id, this->device.name);
        _gb->devices.mcu = this;
        _gb->serial = {&Serial};

        // Set pinMode for builtin LED
        pinMode(2, OUTPUT);
    }


    void GB_PROMINI::blink() {
        int count = 8;
        for (int i = 0; i < count; i++) {
            digitalWrite(2, HIGH);
            delay(50);
            digitalWrite(2, LOW);
            delay(1000);
        }
    }

    GB_PROMINI& GB_PROMINI::i2c() {
        // Initialize I2C for peripherals
        Wire.begin();

        return *this;
    }

    GB_PROMINI& GB_PROMINI::debug(HardwareSerial &ser, int baud_rate) {
        // Enable serial port for debugging
        _serial.debug = &ser;
        _baudrate.debug = baud_rate;
        
        ser.begin(baud_rate);

        _gb->serial = _serial;

        return *this;
    }

    void GB_PROMINI::sleep(int milliseconds) {
        _gb->log("\nEntering low-power mode. Sleeping for " + String(milliseconds / 60 / 1000) + " min.", true); delay(1000);

        //TODO: Figure lowpower library out for Pro Mini
        // LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, setup, CHANGE);

        // // Enter low power-mode
        // LowPower.sleep(milliseconds);
    }

    void GB_PROMINI::reset() { 
        //TODO: Figure reset for Pro Mini
        // void(* resetFunc) (void) = 0;
        // resetFunc();
    }

    String GB_PROMINI::help() { 
        return "mcu.i2c().debug(Serial, 9600).configure();";
    }

#endif