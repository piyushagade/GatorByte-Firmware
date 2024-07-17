/*
	File: esp32.h
	Project: microcontrollers
	
	Notes:
	Insert notes here
	
	Created: Wednesday, 23rd December 2020 12:29:40 am by Piyush Agade
	Modified: Friday, 8th January 2021 11:38:39 am by Piyush Agade
	
	Copyright (c) 2021 Piyush Agade
	
    ! Usage example
    mcu.i2c().debug(Serial, 115200).serial(Serial1, 9600).configure("Hakuna Matata", "aprilfox").connect("wifi");

*/

#ifdef ESP32

    #define MCU_HAS_WIFI
    #define MCU_HAS_I2C
    #define MCU_HAS_SPI
    #define MCU_HAS_UART
    #define MCU_HAS_BL

    
    #define GB_ESP32_h

    // Include required libraries
    #ifndef Arduino_h
        #include "Arduino.h"
    #endif
    
    // Polyfill for ESP32
    #ifndef _ESP32_ANALOG_WRITE_
        #include "analogWrite.h"
    #endif

    #ifndef ArduinoHttpClient_h
        #include "ArduinoHttpClient.h"
    #endif

    #ifndef Wire
        #include "Wire.h"
    #endif

    #ifndef GB_h
        #include "../GB.h"
    #endif

    #ifndef _ARDUINO_LOW_POWER_H_
        #include "ArduinoLowPower.h"
    #endif

    #ifndef WiFi_h
        #include "WiFi.h"
    #endif

    class GB_ESP32: public GB_MCU {
        public:
            GB_ESP32(GB &gb);
            
            DEVICE device = {
                "esp32",
                "ESP32S",
                "ESP32S microcontroller with Wi-Fi",
                "Espressif"
            };

            GB_ESP32& configure(String ssid, String password);
            GB_ESP32& i2c();

            //TODO: Fix serial
            GB_ESP32& debug(HardwareSerial &ser, int);
            GB_ESP32& serial(HardwareSerial &ser, int);
            GB_ESP32& wait(int);
            void i2cscan();
            
            // Power management
            void sleep(int);
            void reset();

            // Wi-Fi functions
            bool connect(String type);
            bool disconnect(String type);
            bool reconnect(String type);
            bool reconnect();
            String get(String);
            String post(String, String);

            String help();
            Client& client();

        private:
            GB *_gb;
            GB_MCU *_mcu;

            // Wi-Fi Client
            WiFiClient _client;

            bool _wifi_connected = false;

            _SER_PORTS _serial = {&Serial, &Serial1};
            _BAUD_RATES _baudrate = {9600, 9600};
            String _SSID, _PASS;
    };

    GB_ESP32::GB_ESP32(GB &gb) {
        // Set GatorByte object pointer
        _gb = &gb;
        _gb->includelibrary(this->device.id, this->device.name);
        _gb->devices.mcu = this;
        _gb->serial = {&Serial, &Serial1};
    }


    GB_ESP32& GB_ESP32::configure(String ssid, String password) {
        this->_SSID = ssid;
        this->_PASS = password;
    }

    Client& GB_ESP32::client() {
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
        
        ser.begin(baud_rate);

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

    bool GB_ESP32::connect(String type) {
        if(type == "wifi") {
            _gb->log("Connecting to WiFi SSID: " + this->_SSID, false);
            if(_gb->globals.OFFLINE_MODE) { _gb->arrow().log("Skipped"); return false; }
            if(WiFi.status() == WL_CONNECTED) { _gb->arrow().log("Already connected"); return true; }

            if(this->_SSID.length() == 0) { _gb->log("Wi-Fi SSID not provided. See documentation for more information."); return false; }
            if(this->_PASS.length() == 0) { _gb->log("Wi-Fi password not provided. See documentation for more information."); return false; }

            // Connect to WiFi 
            WiFi.begin(_gb->s2c(this->_SSID), _gb->s2c(this->_PASS));
            int wait_seconds = 10, counter = 0;
            while (WiFi.status() != WL_CONNECTED && counter++ < wait_seconds) delay(1000);

            this->_wifi_connected = WiFi.status() == WL_CONNECTED;
            if (this->_wifi_connected) {
                _gb->arrow().log("Done"); 
                CONNECTED_TO_NETWORK = true;
                
                // TODO: Add check for internet connectivity
                CONNECTED_TO_INTERNET = true;
                return true;
            }
            else {
                _gb->arrow().log("Failed. Status code: " + String(WiFi.status())); 
                CONNECTED_TO_NETWORK = false;
                CONNECTED_TO_INTERNET = false;
                return false;
            }
        }
        return false;
    }

    bool GB_ESP32::disconnect(String type) {
        if(type == "wifi") {
            WiFi.disconnect();
            return WiFi.status() != WL_CONNECTED;
        }
        return false;
    }

    // Reconnect default network
    bool GB_ESP32::reconnect() {
        return this->reconnect("wifi");
    }

    // Reconnect wifi network
    bool GB_ESP32::reconnect(String type) {
        if(type == "wifi") {
            this->disconnect("wifi");
            this->connect("wifi");
            return WiFi.status() == WL_CONNECTED;
        }
        return false;
    }

    void GB_ESP32::sleep(int milliseconds) {
        _gb->log("\nEntering low-power mode. Sleeping for " + String(milliseconds / 60 / 1000) + " min.", true); delay(1000);

        this->_serial.debug->end();
        this->_serial.hardware->end();

        //TODO: Figure lowpower library out for ESP32
        // LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, setup, CHANGE);

        // // Enter low power-mode
        // LowPower.sleep(milliseconds);
    }

    void GB_ESP32::reset() { 
        //TODO: Figure reset for ESP32
        // void(* resetFunc) (void) = 0;
        // resetFunc();
    }

    String GB_ESP32::get(String path) { 

        String result = "";
        HttpClient httpclient = HttpClient(this->client(), this->SERVER_IP, this->SERVER_PORT);
        unsigned long start = millis();

        // Connect to network if not connected
        int counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter++ < 10) { this->connect("wifi"); delay(1000); }

        if (WiFi.status() != WL_CONNECTED) {
            _gb->arrow().log("Couldn't connect to Wi-Fi SSID: " + this->_SSID);
            return result;
        }
        else {
            // Send request
            httpclient.beginRequest();
            int state = httpclient.get("/" + this->API_VERSION + path);
            httpclient.sendHeader("x-device-id", "mkr-gb-prototype");
            httpclient.endRequest();

            // Get response
            if (state == HTTP_SUCCESS) {
                _gb->arrow().log("Done");

                _gb->log("Getting response", false);
                int code = httpclient.responseStatusCode();
                if (code == 200) {
                    _gb->arrow().log("Received: " + String(code));
                    result = httpclient.responseBody();
                }
                else {
                    _gb->arrow().log("Error: " + String(code));
                    result = "";
                }
                httpclient.stop();
            }
            else {
                _gb->arrow().log("Failed");
                httpclient.stop();
            }

            int difference = millis() - start;
            _gb->log("Time taken: " + String(difference / 1000) + " seconds");
            
            return result;
        }
    }

    String GB_ESP32::post(String path, String data) { 
        // Connect to network if not connected
        this->connect("wifi");

        HttpClient httpclient = HttpClient(this->client(), this->SERVER_IP, this->SERVER_PORT);
        unsigned long start = millis();
        String result = "";

        // Send request
        httpclient.beginRequest();
        int state = httpclient.post("/" + this->API_VERSION + path, "plain/text", data);
        httpclient.sendHeader("x-device-id", "mkr-gb-prototype");
        httpclient.beginBody();
        httpclient.print(data);
        httpclient.endRequest();

        // Get response
        if (state == HTTP_SUCCESS) {
            _gb->arrow().log("Done");

            _gb->log("Getting response", false);
            int code = httpclient.responseStatusCode();
            if (code == 200) {
                _gb->arrow().log("Received: " + String(code));
                result = httpclient.responseBody();
            }
            else {
                _gb->arrow().log("Error: " + String(code));
                result = "";
            }
            httpclient.stop();
        }
        else {
            _gb->arrow().log("Failed");
            httpclient.stop();
        }

        int difference = millis() - start;
        _gb->log("Time taken: " + String(difference / 1000) + " seconds");
        return result;
    }

    String GB_ESP32::help() { 
        return "mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure(\"serving wifi realness\", \"mousequeen\").connect(\"wifi\");";
    }

#endif