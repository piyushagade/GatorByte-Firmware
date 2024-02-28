/*
	File: GB.h
	Project: core
	
	Notes:
    !AG: Add the next to lines to Stream.h's public declarations
    virtual void begin(int) {};
    virtual void end() {};
    virtual void write(String) {};
    virtual size_t write(uint8_t) {};
	
	Created: Saturday, 26th December 2020 6:52:01 pm by Piyush Agade
	Modified: Friday, 8th January 2021 11:46:12 am by Piyush Agade
	
	Copyright (c) 2021 Piyush Agade
	
	Date      				Comments
	--------------------	---------------------------------------------------------
	08/01/21 10:45:39 am	Removed unneccessary headers
 */


#ifndef GB_h
#define GB_h

#define GB_DEBUG false

// Platform specific includes
#if defined(ARDUINO) && ARDUINO > 100
    #include "Arduino.h"
    #include "ArduinoHttpClient.h"
    #include "ArduinoMqttClient.h"
#elif defined(SPARK) || defined(PARTICLE)
    #include "Particle.h"
#endif

#ifndef CSVary_h
    #include "CSVary.h";
#endif

#ifndef JSONary_h
    #include "JSONary.h";
#endif

struct DEVICE {
    String id;
    String name;
    bool detected;
};

//! Global variables
#include "./GB_Globals.h"

//! Device base class
#include "./GB_Manager.h"

//! Compression
#ifndef MICROLZW_H
    #include "microlzw.h"
#endif

//! Global structures
// !TODO: Find a better place for these

class string : public String {
    public:
        string() : String() {}

    // Constructor with initialization
    string(char *cstr) : String(cstr) {}
    string(String cstr) : String(cstr) {}

    // Custom contains function
    bool contains(const String &substring) const {
        return indexOf(substring) != -1;
    }
};

struct _BAUD_RATES {
    int debug;
    int hardware;
};

#if defined(ARDUINO_SAMD_MKRNB1500)
    struct _SER_PORTS {
        Serial_ *debug;
        Uart *hardware;
    };
#else
    struct _SER_PORTS {
        Stream *debug;
        Stream *hardware;
    };
#endif


struct PINS {
    int enable;
    int data;
    int cd;
    int ss;
    int sck;
    bool mux;
} pins;

void breathe();

class GB {
    public:
        GB() {};

        DEVICE device = {
            "core",
            "GatorByte library"
        };

        struct GLOBALS {
            String DEVICE_TYPE = "gatorbyte", DEVICE_ID = "unset", DEVICE_NAME = "unset", DEVICE_SN = "unset";
            String DEVICES_LIST;
            String PROJECT_ID = "-", SURVEY_LOCATION = "-", SURVEY_START_DATE = "-", SURVEY_MODE = "stream";
            String TIMEZONE = "EST";
            String SENSOR_MODE = "iterations";
            String SLEEP_MODE = "shallow";
            
            String SERVER_METHOD = "mqtt";
            String SERVER_URL = "/";
            String SERVER_API_VERSION = "v3";
            int SERVER_PORT = 80;
            String PIN = "";
            String APN = "";
            String RAT = "catm";

            int TIMEOUT_CELLULAR_CONNECTION;
            int SLEEP_DURATION = 5 * 60 * 1000;
            int BREATHE_INTERVAL = 60 * 1000;
            bool OFFLINE_MODE;
            int FAULTS_PRIMARY = 0;
            int FAULTS_SECONDARY = 0;
            
            int WRITE_DATA_TO_SD = 1;
            // int LOG_DATA_TO_SERIAL;
            // int SEND_DATA_TO_SERVER;
            
            int INIT_SECONDS = 0;
            int ITERATION = 0;
            int LAST_READING_TIMESTAMP = 0;
            int SECONDS_SINCE_LAST_READING = 0;
            int LOOPTIMESTAMP = 0;
            bool READINGS_DUE = true;
            
            String MODE = "inactive"; 
            bool GDC_CONNECTED = false;
            bool GDC_SETUP_READY = false;
            int SETUPDELAY = 0;
            int LOOPDELAY = 0;
            
            String LOGPREFIX = "\033[1;30;40m \033[0m";
            String LOGCOLOR = "";
            bool SENTENCEENDED = false;
            bool NEWSENTENCE = true;

            bool ENFORCE_CONFIG = true;
        } globals;

        // Initialize mode
        // String MODE = "inactive"; 
        // int SETUPDELAY = 0;
        // int LOOPDELAY = 0;

        DEVICES devices;

        // Add devices
        GB& includelibrary(String device_id, String device_name);
        GB& includedevice(String device_id, String device_name);
        bool haslibrary(String device);
        bool hasdevice(String device);

        void configure(bool debug);
        void configure(bool debug, String device_id);

        // Get device by name
        GB_DEVICE& getdevice(String);
        GB_MCU& getmcu();

        // Functions
        GB& wait(int);
        GB& setup();
        GB& loop();
        GB& loop(int delay);
        String env();
        String env(String environment);
        GB& init();
        GB& breathe();
        GB& breathe(String);

        GB& bs();
        GB& br();
        GB& br(int);
        GB& del();
        GB& clr();
        GB& space(int count);
        GB& color();
        GB& color(String color);
        GB& log(String message, bool new_line);
        GB& log(char message, bool new_line);
        GB& log(int message, bool new_line);
        GB& log(float message, bool new_line);

        GB& log(String message);
        GB& log(char message);
        GB& log(int message);
        GB& log(float message);
        GB& log();

        GB& arrow();
        GB& heading(String message);

        GB& logd(String, bool new_line);
        GB& logd(String);

        GB& logb(String);
        GB& loga(String);
        GB& loge(String);

        String uuid();
        String uuid(int length);

        int s2hash(String);
        String sremove(String, String, String);
        String sreplace(String, String, String);
        String split(String, char, int);
        char* s2c(String str);
        String ca2s(char char_array[]);
        float ba2f(byte byte_array[]);
        bool isnumber(String);
        String rle(String);

        typedef void (*callback_t_func)();
        GB& closure(int, callback_t_func);
        GB& closure(callback_t_func);

        void i2cscan();

        bool USB_CONNECTED = false;
        JSONary controls;

        bool MODEMDEBUG = false;
        bool SERIALDEBUG = true;
        bool BLDEBUG = true;

        _SER_PORTS serial;
        _BAUD_RATES baudrate = {9600, 9600};

        String _all_included_gb_libraries = "";
        String _all_included_gb_devices = "";

        struct _FUNC {
            void test() {
                Serial.println("\nHello from struct\n");
            };
        };

    private:
        bool _debug = false;
        bool _low_memory_mode = false;
        int _boot_timestamp = 0;
        int _loop_counter = 0;
        int _loop_execute_timestamp = 0;
        bool _concat_print = false;
        String _env = "";
        
};

void GB::configure(bool debug) {
    this->configure(debug, "");
}
void GB::configure(bool debug, String device_name) {
    this->_debug = debug;
    this->globals.DEVICE_NAME = device_name;
}

// Add device library to the virtual GatorByte
GB& GB::includelibrary(String device_id, String device_name){
    device_id.toLowerCase();

    // Check if the library has already been included
    if (this->_all_included_gb_libraries.contains( device_id + ":" + device_name)) return *this;

    this->_all_included_gb_libraries += (this->_all_included_gb_libraries.length() > 0 ? "::" : "") + device_id + ":" + device_name;
    return *this;
}

// Add device to the virtual GatorByte
GB& GB::includedevice(String device_id, String device_name) {
    
    // Detect GDC without lock
    if (this->hasdevice("gdc")) this->getdevice("gdc").detect(false);

    device_id.toLowerCase();
    this->_all_included_gb_devices += (this->_all_included_gb_devices.length() > 0 ? "::" : "") + device_id;

    return *this;
}

/*
    ! Check if the device's library was added (#include-ed)
*/
bool GB::haslibrary(String device_name) {
    device_name.toLowerCase();
    return this->_all_included_gb_libraries.contains(device_name);
}

/*
    ! Check if the config file has device
*/
bool GB::hasdevice(String device_name) {

    if (this->globals.ENFORCE_CONFIG) {
        uint8_t errorcode = 0;
        
        // Force name to lower case
        device_name.toLowerCase();
        
        // If the requested device is GDC, IOE, or TCA, just check if it is constructed and initialized
        if (device_name == "gdc" || device_name == "ioe" || device_name == "tca") {
            return this->_all_included_gb_libraries.indexOf(device_name + ":");
        }
        
        // If the requested device is RGB, Buzzer just check if it is constructed and initialized
        if (device_name == "rgb" || device_name == "buzzer" || device_name == "mem") {
            return this->_all_included_gb_libraries.indexOf(device_name + ":") && this->_all_included_gb_devices.indexOf(device_name + ":");
        }
        
        // Check if SD is constructed and initialized
        bool sdinitialized =  this->_all_included_gb_libraries.indexOf("sd:") > -1;

        // If the requested device is SD, just check if it is constructed and initialized
        if (device_name == "sd") {
            if (!sdinitialized) errorcode = 1;
            else return true;
        }
        
        // If the requested device is not SD, but SD module is NOT initialized
        if (!sdinitialized) errorcode = 2;
    
        // If the requested device is not SD, and SD module is initialized
        else {

            if (device_name == "bl") {
                // Check if the device's constructor has been called (object has been initialized)
                if (this->_all_included_gb_libraries.indexOf(device_name + ":") > -1) {
                    return true;
                }
            }
            
            // Look for the device in the config file
            if (this->globals.DEVICES_LIST.indexOf(device_name) > -1) {

                // Check if the device's constructor has been called (object has been initialized)
                if (this->_all_included_gb_libraries.indexOf(device_name + ":") > -1) {
                    return true;
                }

                // If the device's constructor has NOT been called
                else {
                    errorcode = 3;
                }
            }

            // If device not specified in the SD configuration file
            else {

                // this->log("Device not found: " + device_name);
                // this->log(this->globals.DEVICES_LIST);
                errorcode = 4;
            }
        }

        // Check if an error has occurred
        bool failure = errorcode > 0;
        
        // if (device_name == "uss") Serial.println("Error code: " + String(errorcode));

        //! If an error has occurred
        if (failure) {

            if (errorcode == 1) {
                // this->log("Fatal exception in hasdevice(" + device_name + "): SD module not initialized.");
                while(true) delay(5);
                return false;
            }
            if (errorcode == 2) {
                // this->log("Exception in hasdevice(" + device_name + "): SD module not initialized.");
                return true;
            }
            if (errorcode == 3) {
                // this->log("Fatal exception in hasdevice(" + device_name + "): Device's constructor not called.");
                while(true) delay(5);
                return false;
            }
            if (errorcode == 4) {
                // this->log("Exception in hasdevice(" + device_name + "): Device not specified in config.ini.");
                return false;
            }

            // By default, return true so that the GB's operation is not crippled by a failed SD.
            return true;
        } 
    }
    
    else {
        //! The following code looks for the device in the libraries instantiated
        return this->_all_included_gb_devices.contains( "::" + device_name) || this->_all_included_gb_devices.contains(device_name);
    }
}

GB& GB::setup() {
    this->_boot_timestamp = millis();
    if (this->hasdevice("mem")) {
        // bool initialized = this->getdevice("mem").get
        // this->getdevice("mem").detect(false);
    }
    return *this;
}

String GB::env() {
    return this->_env;
}

String GB::env(String environment) {
    this->_env = environment;
    return this->_env;
}

GB& GB::init() {
    
    // Detect GDC without lock
    if (this->hasdevice("gdc")) this->getdevice("gdc").detect(false);

    return *this;
}

GB& GB::loop() { return this->loop(0);  }
GB& GB::loop(int wait) {
    bool LOG = false;

    // Send message to GDC
    
    if (this->hasdevice("gdc")) {
        this->globals.GDC_SETUP_READY = true;

        if (this->globals.GDC_CONNECTED) {
            Serial.println("Setup complete.");
            Serial.println("##CL-GB-READY##");
            
            // Get device environment from memory
            if (this->hasdevice("mem") &&  this->getdevice("mem").get(0) == "formatted") {
                String savedenv = this->getdevice("mem").get(1);
                this->env(savedenv);
                Serial.println("##CL-GDC-ENV::" + this->env() + "##"); delay(50);
            }
            
            if (this->hasdevice("buzzer")) this->getdevice("buzzer").play("-").wait(100).play("-");
        }

        // Enter GDC mode if GDC available
        this->getdevice("gdc").detect(true);
        this->getdevice("gdc").loop();
    }

    //! Post setup tasks
    if (this->_boot_timestamp > 0) {

        /*
            This block will be exectuded everytime the device reboots (hard reset).
            This will not be exectuded after waking up from sleep in NB1500.
        */

        this->_boot_timestamp = 0;
        
        // Calculate time elapsed for setup
        if (LOG) this->log("Setup took: " + String(millis()/1000 - this->_boot_timestamp/1000) + " seconds");
        this->globals.SETUPDELAY = millis()/1000 - this->_boot_timestamp/1000;
        if (this->hasdevice("mem")) this->getdevice("mem").debug("Power cycle setup complete.");

        if (LOG) this->log("Loop : " + String(this->globals.ITERATION));
        this->getdevice("gdc").send("highlight-cyan", "Loop: " + String(this->globals.ITERATION));
        if(this->hasdevice("mem")) this->getdevice("mem").debug("Loop iteration: " + String(this->globals.ITERATION));
        
        // Set interation count and save it to memory
        if (this->hasdevice("mem")) {
            if (this->getdevice("mem").get(8) == "") this->getdevice("mem").write(8, "0");
            this->getdevice("mem").write(8, "0");
            this->globals.ITERATION = this->getdevice("mem").get(8).toInt();
        }

        if(this->hasdevice("rtc")) this->globals.INIT_SECONDS = this->getdevice("rtc").timestamp().toDouble();
    }
    else {
        /*
            This block will be exectuded when the device boots directly into the 'loop()'
        */
        this->globals.SETUPDELAY = 0;
        
        if(this->hasdevice("mem")) this->getdevice("mem").debug("Loop iteration: " + String(this->globals.ITERATION));
    }

    this->globals.LOOPTIMESTAMP = this->getdevice("rtc").timestamp().toInt();

    if (LOG) this->log("Loop executing at: " + String(this->globals.LOOPTIMESTAMP));

    if (this->getdevice("mem").get(12) == "") this->getdevice("mem").write(12, "0");
    this->globals.LAST_READING_TIMESTAMP = this->getdevice("mem").get(12).toInt();

    // Handle invalid timestamps
    if (this->globals.LOOPTIMESTAMP - this->globals.LAST_READING_TIMESTAMP < 0) {
        if (LOG) this->log("Invalid timestamps detected. Resetting EEPROM values.");
        if (LOG) this->log("Loop timestamp: " + String(this->globals.LOOPTIMESTAMP));
        if (LOG) this->log("Last reading at: " + String(this->globals.LAST_READING_TIMESTAMP));
        this->getdevice("mem").write(12, "0");
        this->globals.LAST_READING_TIMESTAMP = this->getdevice("mem").get(12).toInt();
    }

    // Determine if readings are due
    if (
        this->globals.LAST_READING_TIMESTAMP == 0 || 
        (this->globals.LOOPTIMESTAMP - this->globals.LAST_READING_TIMESTAMP + 10 > this->globals.SLEEP_DURATION / 1000)
    ) {
        this->globals.READINGS_DUE = true;
            
        if (LOG) this->log("Loop timestamp: " + String(this->globals.LOOPTIMESTAMP));
        if (LOG) this->log("Last reading at: " + String(this->globals.LAST_READING_TIMESTAMP));
    }
    else {
        this->globals.READINGS_DUE = false;
        if (LOG) this->log("Loop timestamp: " + String(this->globals.LOOPTIMESTAMP));
        if (LOG) this->log("Last reading at: " + String(this->globals.LAST_READING_TIMESTAMP));
        if (LOG) this->log("Time elapsed since last reading: " + String((this->globals.LOOPTIMESTAMP - this->globals.LAST_READING_TIMESTAMP)) + " seconds");
    }

    if (USBDevice.connected() && USBDevice.configured()) {
        this->USB_CONNECTED = true;
        if (LOG) this->log("\nUSB device connected.\n");
    }
    else {
        this->USB_CONNECTED = false;
        if (LOG) this->log("\nUSB device NOT connected.\n");
    }
    
    // Calculate time taken to execute the previous loop
    if (this->getmcu().on_wakeup()) {
        this->globals.LOOPDELAY = (this->getdevice("rtc").timestamp().toInt() - this->_loop_execute_timestamp - this->getmcu().LAST_SLEEP_DURATION / 1000);
        if (LOG) this->log("\nLast loop took: " + String(this->globals.LOOPDELAY) + " seconds.\n");
    }
    else this->globals.LOOPDELAY = 0;
    this->_loop_execute_timestamp = this->getdevice("rtc").timestamp().toInt();
    this->_loop_counter++;

    // Start breath timer
    this->getmcu().startbreathtimer();

    // Increment iteration count
    if (this->hasdevice("mem")) {
        if (this->getdevice("mem").get(8) == "") this->getdevice("mem").write(8, "0");
        this->getdevice("mem").write(8, String(this->getdevice("mem").get(8).toInt() + 1));
        this->globals.ITERATION = this->getdevice("mem").get(8).toInt();
    }

    // RGB - Indicate if dummy mode
    if(this->globals.MODE == "dummy") if (this->hasdevice("rgb")) this->getdevice("rgb").on("green");
    else if (this->hasdevice("rgb")) this->getdevice("rgb").on("magenta");

    this->getdevice("gdc").send("gdc-db", "loopiteration=" + String(this->globals.ITERATION));

    return *this;
}

// GatorByte breathe function
GB& GB::breathe() { return this->breathe(""); }
GB& GB::breathe(String skipaction) {

    this->getdevice("gdc").detect();

    // // Listen for Bluetooth commands
    // if (this->hasdevice("bl") && skipaction.indexOf("bl") == -1) this->getdevice("bl").listen();

    // RGB LED - Flash white
    // if (this->hasdevice("rgb")) this->getdevice("rgb").on("white");delay(100);
    // if (this->hasdevice("rgb")) this->getdevice("rgb").revert(); delay(200);
    // if (this->hasdevice("rgb")) this->getdevice("rgb").on("white"); delay(100);
    // if (this->hasdevice("rgb")) this->getdevice("rgb").revert();

    // // Loop mqtt
    // if (this->hasdevice("mqtt") && skipaction.indexOf("mqtt") == -1) {
    //     this->getdevice("mqtt").update();
    // }

    // // RGB - Indicate if dummy mode
    // if(this->globals.MODE == "dummy") {
    //     if (this->hasdevice("rgb")) this->getdevice("rgb").on("green");
    // }
    // else if (this->hasdevice("rgb")) {
    //     this->getdevice("rgb").on("magenta");
    // }

    // if (Serial.available()) {
    //     Serial.print("Received command: ");
    //     delay(100);
    //     Serial.println(Serial.readString());
    // }

    return *this;
}

// Get device instance by name
GB_DEVICE& GB::getdevice(String name) {

    if (name == "rgb") return *this->devices.rgb;
    else if (name == "buzzer") return *this->devices.buzzer;
    else if (name == "ioe") return *this->devices.ioe;
    else if (name == "tca") return *this->devices.tca;
    else if (name == "eadc") return *this->devices.eadc;
    else if (name == "sd") return *this->devices.sd;
    else if (name == "fram") return *this->devices.fram;

    else if (name == "command") return *this->devices.command;
    else if (name == "configurator") return *this->devices.configurator;
    else if (name == "gdc") return *this->devices.gdc;

    else if (name == "mqtt") return *this->devices.mqtt; 
    else if (name == "http") return *this->devices.http; 

    else if (name == "bl") return *this->devices.bl; 
    else if (name == "mem") return *this->devices.mem;
    else if (name == "gps") return *this->devices.gps;
    else if (name == "rtc") return *this->devices.rtc;
    else if (name == "buzzer") return *this->devices.buzzer;
    else if (name == "pwr") return *this->devices.pwr;
    else if (name == "sntl") return *this->devices.sntl;
    else if (name == "relay") return *this->devices.relay;
    else if (name == "rg11") return *this->devices.rg11;
    else if (name == "tpbck") return *this->devices.tpbck;

    else if (name == "ec") return *this->devices.ec;
    else if (name == "rtd") return *this->devices.rtd;
    else if (name == "ph") return *this->devices.ph;
    else if (name == "dox") return *this->devices.dox;
    else if (name == "aht") return *this->devices.aht;
    else if (name == "uss") return *this->devices.uss;

    else {
        this->log("Device " + name + " not found in the 'getdevice' function.");
        return *this->devices.none;
    }
}

// Get microcontroller instance
GB_MCU& GB::getmcu() {
    return *this->devices.mcu;
}

GB& GB::closure(callback_t_func function) { 
    
    return this->closure(0, function);
}

GB& GB::closure(int duration, callback_t_func function) { 
    
    delay(duration);

    // Call the function/scope/block
    function();

    return *this;
}

// Include remaining base gb libraries
#include "GB_Helpers.h"
#include "SDU.h"

String BR = "\n"; 
String TAB = "\t"; 
String SPACE = " "; 

#endif
