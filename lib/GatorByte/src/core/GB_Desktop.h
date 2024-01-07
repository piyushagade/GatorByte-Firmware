#ifndef GB_DESKTOP_h
#define GB_DESKTOP_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_DESKTOP : public GB_DEVICE {
    public:
        GB_DESKTOP(GB &gb);
        
        DEVICE device = {
            "gdc",
            "GatorByte Desktop Client"
        };

        GB_DESKTOP& detect() override;
        GB_DESKTOP& detect(bool lock);
        GB_DESKTOP& loop() override;
        
        void process(string);
        bool contains(String, String);
        GB_DESKTOP& enter();
        GB_DESKTOP& exit();
        
        String cfget(String message);
        void cfset(String message);
        void cfprint(String message);
        
        void send(String data);
        void send(String category, String data);
        void sendfile(String category, String data);
        
        bool testdevice();
        String status();

    private:
        GB *_gb; 
        String _prev_mode;
        String _state = "";
        bool lock = false;
        String _received_config_str = "";
        GB_DESKTOP& _busy(bool busy);
};

// Constructor 
GB_DESKTOP::GB_DESKTOP(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name); 
    _gb->devices.gdc = this;
    
    this->device.detected = true;
}

// Test the device
bool GB_DESKTOP::testdevice() { 
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_DESKTOP::status() { 
    return "detected";
}

GB_DESKTOP& GB_DESKTOP::detect() { return this->detect(true); }
GB_DESKTOP& GB_DESKTOP::detect(bool lock) { 

    this->lock = lock;

    // Return if the GDC is already connected
    if (_gb->globals.GDC_CONNECTED) {
        if (lock) return this->enter();
        else this->loop();

        return *this;
    }

    // Init serial if not already done
    this->_gb->serial.debug->begin(9600);
    
    if (_gb->serial.debug->available()) {
        String response = _gb->serial.debug->readString();

        if (response.indexOf("##CL-GDC-PING##") != -1) {
            _gb->globals.GDC_CONNECTED = true;
            Serial.println("##CL-GDC-PONG##");
            if (_gb->globals.GDC_SETUP_READY) Serial.println("##CL-GB-READY##");

            this->send("gdc-db", "power=awake");

            if (lock) this->enter();
            else this->loop();
        }
        else {
            _gb->globals.GDC_CONNECTED = false;
        }
    }
    else _gb->globals.GDC_CONNECTED = false;
    return *this;
}

GB_DESKTOP& GB_DESKTOP::_busy(bool busy) {
        if (busy) {
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red").wait(100).revert();
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play(",.");
        }
        else  {
            if (_gb->hasdevice("rgb") )_gb->getdevice("rgb").revert();
        }
    return *this;
}

// Enter console mode
GB_DESKTOP& GB_DESKTOP::enter() {

    _gb->log("GDC mode enabled. GatorByte is now locked in this mode.");

    uint8_t last_led_act_ts = millis();
    bool led_state = false;
    while (_gb->globals.GDC_CONNECTED) {
        
        // Look for new commands
        this->loop();

        if (millis() - last_led_act_ts >= 1000) {
            last_led_act_ts = millis();
            led_state = !led_state;
            if (_gb->hasdevice("rgb")) {
                if (led_state) _gb->getdevice("rgb").on("cyan");
                else _gb->getdevice("rgb").off();
            }
        }

        delay(250);
    }

    return *this;
}

// Turn off the module
GB_DESKTOP& GB_DESKTOP::exit() {
    _gb->log("Exiting comand mode. Reverting to \"" + this->_prev_mode + "\" mode.\n");
    this->_gb->globals.MODE = this->_prev_mode;
    return *this;
}

// Check if there is a new char from computer/bl serial
GB_DESKTOP& GB_DESKTOP::loop() {

    // Return if GDC is not connected
    if (!_gb->globals.GDC_CONNECTED) return *this; 

    #if not defined (LOW_MEMORY_MODE)
        
        // Read any incoming commands
        string response;
        char c;
        while (_gb->serial.debug->available()) {
            c = _gb->serial.debug->read();
            response += String(c);
            response.trim();
        }

        // If a command is received, act on it
        if (response.length() > 0) {
            this->process(response);
        }
        
        // Set LED color to cyan
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("cyan");

    #endif
    return *this;
}

/*
    ! Get a value/state from GatorByte configurator or console-based GatorByte datastore 
*/
String GB_DESKTOP::cfget(String key) { 
    
    String prefix = "##SOF##", suffix = "##EOF##";

    //! Send the command to GatorByte Configurator
    this->_gb->serial.debug->println(prefix + key + suffix);

    //! Get the response from the GatorByte Configurator
    int counter = 0;
    while (!this->_gb->serial.debug->available() && counter++ < 10) delay(500);

    String response = "";
    String value = "";
    while (_gb->serial.debug->available()) {
        response += _gb->serial.debug->readString();
        response.trim();

        // Check if a string is coming from the GB Configurator 
        if (response.indexOf("$$GBCG$$") == 0) {
            
            // Strip the prefix
            response = response.substring(response.indexOf("$$GBCG$$") + 8, response.length());

            // Check if the response is a key-value pair
            if (response.indexOf(":") > -1) {
                String key = response.substring(0, response.indexOf(":"));
                value = response.substring(response.indexOf(":") + 1, response.length());
                response = value;
            }
        }  
    }
    return response;
}

void GB_DESKTOP::cfprint(String message) { 
    String prefix = "##SOF##", suffix = "##EOF##";

    //! Send the command to GatorByte Configurator
    // this->_gb->serial.debug->println(prefix + "log-message::" + message + suffix);
    this->_gb->serial.debug->println(message);
}

// Process the incoming command
void GB_DESKTOP::process(string command) {

    // _gb->br().log("Processing command: " + command, false);
    // _gb->log(" -> Current state: " + String(this->_state.length() > 0 ? this->_state : "None"));
    
    /*
        ! Ping
    */
    if (command.contains("gdc-ping")) {
        Serial.println("##CL-GDC-PONG##"); delay(50);
        if (_gb->globals.GDC_SETUP_READY) Serial.println("##CL-GB-READY##");
    }
    
    #if not defined (LOW_MEMORY_MODE)

        // Set as busy
        this->_busy(true);

        _gb->log("Received GDC command: " + command);

        /*
            ! Check command validity
        */
        if (
            this->_state == "" && 
            !command.contains("##GB##") && 
            !command.startsWith("cfg") && 
            !command.startsWith("sdf")
        ) return;

        /*
            ! GB lock/unlock
        */
        if (command.contains("##CL-GDC-LOCK##")) {
            this->lock = true;
        };
        if (command.contains("##CL-GDC-UNLOCK##")) {
            this->lock = false;
        };

        /*
            ! Global configuration download/upload
        */
        if (command.startsWith("cfg")) {

            // Configuration download request
            if (command.contains("cfgdl:")) {

                String filename = "/config/" + command.substring(command.indexOf("dl:") + 3, command.indexOf(","));

                int initialcharindex = command.substring(command.indexOf(",") + 1, command.length()).toInt();
                int charsatatime = 30;

                String data = _gb->getdevice("sd").readLinesFromSD(filename, charsatatime, initialcharindex);
                this->sendfile("gdc-cfg", "fdl:" + data);
            }
            
            // Configuration upload request
            else if (command.contains("cfgupl:")) {
            
                String filename = "/config/config.ini";
                string data = command.substring(command.indexOf("upl:") + 4, command.indexOf("^"));
                int initialcharindex = command.substring(command.indexOf("^") + 1, command.length()).toInt();

                while(data.contains("~")) {
                    data.replace("~", "\n");
                }

                while(data.contains("`")) {
                    data.replace("`", " ");
                }

                // Delete preexisting file if the upload has just started.
                if (initialcharindex == 0) _gb->getdevice("sd").rm(filename);

                // Append data to the file
                _gb->getdevice("sd").writeLinesToSD(filename, data);

                // Pause
                delay(10);

                // Send acknowledgement
                this->send("gdc-cfg", "fupl:ack");
            }

            // Post config upload tasks
            else if (command.contains("cfgupd:")) {
            
                // Update config in the memory
                _gb->getdevice("sd").readconfig();

            }

            // Post config upload tasks
            else if (command.contains("hash")) {
            
                String configdata = _gb->getdevice("sd").readfile("/config/config.ini");

                // Compute hash
                unsigned int hash = 0;
                for (int i = 0; i < configdata.length(); i++) {
                    unsigned int charCode = static_cast<unsigned int>(configdata.charAt(i));
                    hash = (hash << 5) - charCode;
                }

                _gb->log("Computed hash from config on SD: " + String(hash));

                // Send acknowledgement
                this->send("gdc-cfg", "cfghash:" + String(hash));

            }
        }
        

        /*
            ! Create base folders and files on SD card if they don't exist
        */
        if (command.startsWith("sdf")) {
            
            if (command.contains("cr:all")) {
                if (!_gb->hasdevice("sd")) {
                    this->send("gdc-sdf", "sd:error");
                    return;
                }
                if (!_gb->getdevice("sd").initialized()) {
                    this->send("gdc-sdf", "sdinit:error");
                    return;
                }

                // Create folder
                String folders[] = {"config", "calibration", "control", "readings", "debug", "logs", "queue"};
                for (int i = 0; i < sizeof(folders) / sizeof(folders[0]); i++) {
                    String foldername = folders[i];
                    if (!_gb->getdevice("sd").exists("/" + foldername)) {
                        _gb->log(foldername + " doesn't exist. Creating directory", false);
                        bool success = _gb->getdevice("sd").mkdir("/" + foldername);
                        _gb->arrow().log(success ? "Done" : "Failed");
                    }
                }

                // Create config file
                if (!_gb->getdevice("sd").exists("/config/config.ini")) {
                    _gb->log("File config.ini doesn't exist. Creating file.");
                    _gb->getdevice("sd").writeLinesToSD("/config/config.ini", "");
                }

                // Create control file
                if (!_gb->getdevice("sd").exists("/control/variables.ini")) {
                    _gb->log("File variables.ini doesn't exist. Creating file.");
                    _gb->getdevice("sd").writeLinesToSD("/control/variables.ini", "");
                }

                // Send acknowledgement
                this->send("gdc-sdf", "success");
            }
        }


        /*
            ! SD file list / File download
        */
        if (this->_state == "files-list" || command.contains("fl")) {
            this->_state = "files-list";

            // Strip prefix and suffix
            command.replace("##GB##fl", "");
            command.replace("#EOF#", "");
            
            // List the folders
            if (command.contains(":list")) {

                String action = command.substring(0, command.indexOf(","));
                
                // Get the name of the folder to get file list
                String root = command.substring(command.indexOf(",") + 1, command.length());
                _gb->log("Listing folder: " + root);
                
                String filelistdata = _gb->getdevice("sd").getfilelist(root);
                int filecount = filelistdata.substring(0, filelistdata.indexOf("::")).toInt();
                String list = filelistdata.substring(filelistdata.indexOf("::") + 2, filelistdata.length());

                // Test SD
                bool success = _gb->hasdevice("sd") ? _gb->getdevice("sd").testdevice() : false;

                // Send acknowledgment
                if (success) {
                    String file = "";

                    if (filecount == 0) {
                        this->send("gdc-dfl", "error:nofile");
                    }
                    else {
                        for (int i = 0; i < list.length(); i++) {
                            String c = String(list[i]);

                            if (c != "," && i < list.length() - 1) file += c;
                            else {
                                if (i == list.length() - 1) file += c;
                                this->send("gdc-dfl", "file:" + file + "<br>");
                                file = "";
                                delay(20);
                            }
                        }
                    }
                }
                else {
                    this->send("gdc-dfl", "error:nodevice");
                }
            }

            // Download a file
            if (command.contains("dl:")) {

                // dl:caip-trial_readings.csv,0

                String file = command.substring(command.indexOf(":") + 1, command.indexOf(","));
                int initialcharindex = command.substring(command.indexOf(",") + 1, command.length()).toInt();
                int charsatatime = 30;
                String data = _gb->getdevice("sd").readLinesFromSD(file, charsatatime, initialcharindex);
                
                this->sendfile("gdc-dfl", "fdl:" + data);
            }

            // Delete a file
            if (command.contains("rm:")) {
                String file = command.substring(command.indexOf("rm:") + 3, command.length());
                _gb->log("Removing file: " + file);
                _gb->getdevice("sd").rm(file);
            }

            // Delete a folder
            if (command.contains("rmd:")) {
                String file = command.substring(command.indexOf("rmd:") + 4, command.length());
                _gb->log("Removing folder: " + file);
                _gb->getdevice("sd").rmdir(file);
            }

            // Make directory
            if (command.contains("mkdir:")) {
                String dirname = command.substring(command.indexOf("mkdir:") + 3, command.length());
                _gb->log("Creating directory: " + dirname);
                _gb->getdevice("sd").mkdir(dirname);
            }

            // Preview file
            if (command.contains("pv:")) {

            }
        }
        
        /*
            ! Calibration
        */
        if (this->_state == "calibration" || command.contains("calibration")) {
            this->_state = "calibration";

            // Set state to calibration
            if (command.contains(":enter")) {

                // Strip prefix
                command.replace(":enter", "");

                // Get sensor name
                String sensor = command;

                // Send acknowledgment
                this->send("gdc-cal", "ack");
            }

            else if (command.contains(":calibrate")) {

                // Strip prefix
                command.replace(":calibrate", "");
                
                // Get sensor name
                // String sensor = strtok(_gb->s2c(command), ":");
                // String remainder = strtok(NULL, ":");

                _gb->log("Here 0: " + command);

                String sensor = command.substring(0, command.indexOf(":"));
                String level = command.substring(command.indexOf(":") + 1, command.indexOf(","));
                int value = command.substring(command.indexOf(",") + 1, command.length()).toInt();

                // Get calibration configuration
                // String level = strtok(_gb->s2c(remainder), ",");
                // int value = String(strtok(NULL, ",")).toInt();
                
                _gb->log("Processed calibration request for: " + sensor);
                _gb->log("Level: ", false);
                _gb->log(level);
                
                // Calibrate
                int status = _gb->getdevice(sensor).calibrate(level, value);
                
                // Send acknowledgment
                this->send("gdc-cal", "ack");
                this->send("gdc-cal", "result" + String(":") + sensor + String(":") + String(status));

                // Update "last calibrated" date on SD card
                if (level != "status" && level != "clear") {
                    _gb->getdevice("sd").rm("/calibration/" + sensor + ".ini");
                    _gb->getdevice("sd").writeString("/calibration/" + sensor + ".ini", _gb->getdevice("rtc").timestamp());
                }
            }

            // Get continuous readings
            else if (command.contains(":cread")) {

                // Strip prefix, remainder will be "ph:30"
                command.replace(":cread", "");

                // Get sensor name
                String sensor = command.substring(0, command.indexOf(":"));
                int NUMBER_OF_READINGS = command.substring(command.indexOf(":") + 1, command.length()).toInt();

                _gb->br().log("Reading " + String(NUMBER_OF_READINGS) + " continuous values from " + sensor);
                int starttime = millis();
                int count = 0;
                for (count = 0; count < NUMBER_OF_READINGS; count++) {
                    float sensorvalue = _gb->getdevice(sensor).readsensor("next");

                    // Push reading to GDC
                    this->send("gdc-cal", "cread" + String(":") + String(count) + String(":") + String(sensorvalue));
                    delay(10);
                }
            }

            // Get last calibration perform information
            else if (command.contains(":lpi")) {

                // Strip prefix
                command.replace(":lpi", "");

                // Get sensor name
                String sensor = command;
                String lpi = _gb->getdevice("sd").readfile("/calibration/" + sensor + ".ini");
                
                // Send acknowledgment
                this->send("gdc-cal", "lpi" + String(":") + String(lpi));
            }
        }

        /* 
            ! Dashboard
        */
        if (this->_state == "dashboard" || command.contains("db")) {
            this->_state = "dashboard";

            if (command.contains(":enter")) {
                NBModem _nbModem;

                // Device status
                this->send("gdc-db", "power=awake");

                // Check MODEM status
                MODEM_INITIALIZED = _nbModem.begin();
                this->send("gdc-db", "modem=" + String(MODEM_INITIALIZED ? "active" : "not-responding"));
                
                // MODEM firmware
                this->send("gdc-db", "modem-fw=" + _gb->getmcu().getfirmwareinfo());
                
                // MODEM IMEI
                this->send("gdc-db", "modem-imei=" + _gb->getmcu().getimei());
                
                // SIM ICCID
                this->send("gdc-db", "sim-iccid=" + _gb->getmcu().geticcid());

                // Network strength
                this->send("gdc-db", "rssi=" + String(_gb->getmcu().getrssi()));

                // Operator
                String cops = _gb->getmcu().getoperator();
                cops = cops.substring(cops.indexOf("\"") + 1, cops.lastIndexOf("\""));
                this->send("gdc-db", "cops=" + String(cops));
            }
            
            else if (command.contains("fsr:")) {

                if (command.contains(":all")) {
                    String result = "";

                    result += "rtd:" + String(_gb->getdevice("rtd").readsensor()) + String(",");
                    result += "ph:" + String(_gb->getdevice("ph").readsensor()) + String(",");
                    result += "dox:" + String(_gb->getdevice("dox").readsensor()) + String(",");
                    result += "ec:" + String(_gb->getdevice("ec").readsensor());

                    // The following line is not needed because the individual functions above send this info on their own
                    this->send("gdc-db", "fsr=" + result);
                }
                else if (command.contains(":rtd")) {
                    String result = "";

                    result += "rtd:" + String(_gb->getdevice("rtd").readsensor());
                    this->send("gdc-db", "fsr=" + result);
                }
                else if (command.contains(":ph")) {
                    String result = "";

                    result += "ph:" + String(_gb->getdevice("ph").readsensor());
                    this->send("gdc-db", "fsr=" + result);
                }
                else if (command.contains(":dox")) {
                    String result = "";

                    result += "dox:" + String(_gb->getdevice("dox").readsensor());
                    this->send("gdc-db", "fsr=" + result);
                }
                else if (command.contains(":ec")) {
                    String result = "";

                    result += "ec:" + String(_gb->getdevice("ec").readsensor());
                    this->send("gdc-db", "fsr=" + result);
                }
            }

        }
        
        /* 
            ! Configure GatorByte
        */
        if (this->_state == "configuration" || command.contains("cfgb")) {
            this->_state = "configuration";

            // Strip prefix and suffix
            command.replace("##GB##cfgb", "");
            command.replace("##GB##cfg", "");
            command.replace("#EOF#", "");

            // if (command.contains("cfgdl:")) {
            //     _gb->log("A request for config file has been received. File: ", false);

            //     String filename = "/config/" + command.substring(command.indexOf("dl:") + 3, command.indexOf(","));
            //     _gb->log(filename);

            //     int initialcharindex = command.substring(command.indexOf(",") + 1, command.length()).toInt();
            //     int charsatatime = 30;

            //     String data = _gb->getdevice("sd").readLinesFromSD(filename, charsatatime, initialcharindex);
            //     this->sendfile("gdc-cfg", "fdl:" + data);
            // }
            
            // else if (command.contains("cfgupl:")) {
            
            //     String filename = "/config/config.ini";
            //     string data = command.substring(command.indexOf("upl:") + 4, command.indexOf("^"));
            //     int initialcharindex = command.substring(command.indexOf("^") + 1, command.length()).toInt();

            //     while(data.contains("~")) {
            //         data.replace("~", "\n");
            //     }

            //     while(data.contains("`")) {
            //         data.replace("`", " ");
            //     }

            //     // Delete preexisting file if the upload has just started.
            //     if (initialcharindex == 0) _gb->getdevice("sd").rm(filename);

            //     // Append data to the file
            //     _gb->getdevice("sd").writeLinesToSD(filename, data);

            //     // Pause
            //     delay(10);

            //     // Send acknowledgement
            //     this->send("gdc-cfg", "fupl:ack");
            // }

            // // Post config upload tasks
            // else if (command.contains("upd:")) {
            
            //     // Update config in the memory
            //     _gb->getdevice("sd").readconfig();

            // }
            
            //! RTC action
            if (command.contains("rtc:")) {

                // Remove "rtc:" substring
                command = command.substring(4, command.length());

                //! Sync RTC
                if (command.contains("sync")) {

                    _gb->log("Syncing RTC time -> ", false);
                    bool success = _gb->hasdevice("rtc") && _gb->getdevice("rtc").testdevice();

                    if (success) {
                        String month = command.substring(4, 7);
                        String date = command.substring(8, 10);
                        String year = command.substring(11, 15);
                        
                        // syncDec-07-202217-32-42
                        String hour = command.substring(15, 17);
                        String minute = command.substring(18, 20);
                        String second = command.substring(21, 23);

                        String fulldate = month + " " +  date + " " + year;
                        String fulltime = hour + ":" +  minute + ":" + second;

                        _gb->log(fulldate + ", ", false);
                        _gb->log(fulltime);

                        _gb->getdevice("rtc").sync(_gb->s2c(fulldate), _gb->s2c(fulltime));
                        this->send("gdc-cfg", "ack");
                    }
                    else {
                        
                        _gb->log(" -> Skipped");
                        this->send("gdc-cfg", "nack");
                    }
                }

                //! Get RTC timestamp
                if (command.contains("get")) {
                    
                    _gb->log("Getting RTC time -> ", false);
                    bool success = _gb->hasdevice("rtc") && _gb->getdevice("rtc").testdevice();
                    
                    String data;
                    if (success) data = _gb->getdevice("rtc").timestamp();
                    else data = "not-detected";
                    _gb->log(data);

                    // Send RTC time
                    this->send("gdc-cfg", "rtc:" + data);
                }
            }

            //! Bluetooth action
            if (command.contains("bl:")) {

                _gb->log("Command: " + command);
                if (command.contains("getconfig")) {
                    
                    bool success = _gb->hasdevice("bl") && _gb->getdevice("bl").testdevice();
                    if (success) {
                        String namedata = _gb->getdevice("bl").send_at_command("AT+NAME");
                        String name = namedata.substring(namedata.indexOf('=') + 1, namedata.length());
                        
                        String pindata = _gb->getdevice("bl").send_at_command("AT+PIN");
                        String pin = pindata.substring(pindata.indexOf('=') + 1, pindata.length());

                        // Send BL config
                        this->send("gdc-cfg", "bl:" + name + ";" + pin);
                    }
                    else {
                        this->send("gdc-cfg", "bl:not-detected");
                    }
                }

                if (command.contains("setconfig")) {
                    command.replace("bl:setconfig", "");

                    bool success = _gb->hasdevice("bl") && _gb->getdevice("bl").testdevice();
                    if (success) {
                        String name = command.substring(0, command.indexOf(';'));
                        String pin = command.substring(command.indexOf(';') + 1, command.length());

                        _gb->log("Setting BL name to " + name);
                        _gb->log("Setting BL PIN to " + pin);

                        // Update BL name and pin by sending AT commands
                        _gb->getdevice("bl").send_at_command("AT+NAME" + name);
                        _gb->getdevice("bl").send_at_command("AT+PIN" + pin);

                        // Send BL config
                        delay(250); this->send("gdc-cfg", "bl:" + name + ";" + pin);
                    }
                    else {
                        this->send("gdc-cfg", "bl:not-detected");
                    }
                }
            }
        }
        
        /* 
            ! Device diagnostics
        */
        if (this->_state == "diagnostics" || command.contains("dgn")) {
            this->_state = "diagnostics";
            
            // Strip prefix and suffix
            command.replace("##GB##dgn", "");
            command.replace("#EOF#", "");

            if (command.contains("rtc")) {
                String rtctimestamp = _gb->hasdevice("rtc") ? _gb->getdevice("rtc").timestamp() : "";
                
                // Send RTC time
                this->send("gdc-dgn", "rtc:" + rtctimestamp);
            }
            
            if (command.contains("mem")) {
                bool success = _gb->hasdevice("mem") ? _gb->getdevice("mem").get(0) == "formatted": false;
                
                // Send response
                this->send("gdc-dgn", "mem:" + String(success ? "true" : "false"));
            }
            
            if (command.contains("sd")) {
                bool success = _gb->hasdevice("sd") ? _gb->getdevice("sd").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "sd:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("sd").status());
            }
            
            if (command.contains("eadc")) {
                bool success = _gb->hasdevice("eadc") ? _gb->getdevice("eadc").testdevice(): false;
                
                // Send response
                this->send("gdc-dgn", "eadc:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("eadc").status());
            }
            
            if (command.contains("fram")) {
                bool success = _gb->hasdevice("fram") ? _gb->getdevice("fram").testdevice() : false;

                // Send response
                this->send("gdc-dgn", "fram:" + String(success ? "true" : "false"));
            }
            
            if (command.contains("bl")) {
                bool success = _gb->hasdevice("bl") ? _gb->getdevice("bl").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "bl:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("bl").status());
            }
            
            if (command.contains("gps")) {
                bool success = _gb->hasdevice("gps") ? _gb->getdevice("gps").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "gps:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("gps").status());
            }
            
            if (command.contains("sntl")) {
                bool success = _gb->hasdevice("sntl") ? _gb->getdevice("sntl").testdevice() : false;
                
                // Send response
                // this->send("gdc-dgn", "sntl:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("sntl").status());
                this->send("gdc-dgn", "sntl:" + String("true") + ":..:" + _gb->getdevice("sntl").status());
            }
            
            if (command.contains("aht")) {
                bool success = _gb->hasdevice("aht") ? _gb->getdevice("aht").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "aht:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("aht").status());
            }
            
            if (command.contains("relay")) {
                bool success = _gb->hasdevice("relay") ? _gb->getdevice("relay").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "relay:" + String(success ? "true" : "false"));
            }
            
            if (command.contains("rgb")) {
                bool success = _gb->hasdevice("rgb") ? _gb->getdevice("rgb").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "rgb:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("rgb").status());
            }
            
            if (command.contains("buzzer")) {
                bool success = _gb->hasdevice("buzzer") ? _gb->getdevice("buzzer").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "buzzer:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("buzzer").status());
            }
            
            if (command.contains("uss")) {
                bool success = _gb->hasdevice("uss") ? _gb->getdevice("uss").testdevice() : false;

                // Send response
                this->send("gdc-dgn", "uss:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("uss").status());
            }
            
            if (command.contains("ph")) {
                bool success = _gb->hasdevice("ph") ? _gb->getdevice("ph").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "ph:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("ph").status());
            }

            if (command.contains("ec")) {
                bool success = _gb->hasdevice("ec") ? _gb->getdevice("ec").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "ec:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("ec").status());
            }
            
            if (command.contains("dox")) {
                bool success = _gb->hasdevice("dox") ? _gb->getdevice("dox").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "dox:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("dox").status());
            }
            
            if (command.contains("rtd")) {
                bool success = _gb->hasdevice("rtd") ? _gb->getdevice("rtd").testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "rtd:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("rtd").status());
            }
            
            if (command.contains("lipo")) {
                bool success = _gb->getmcu().testbattery();
                
                // Send response
                this->send("gdc-dgn", "lipo:" + String(success ? "true" : "false") + ":..:" + _gb->getmcu().batterystatus());
            }
        }
    
    #endif
}

/*
    ! Send a string to GatorByte configurator or console-based GatorByte datastore 
*/
void GB_DESKTOP::cfset(String key) { 
    
    String prefix = "##SOF##", suffix = "##EOF##";
    this->_gb->serial.debug->println(prefix + key + suffix);
}

/*
    ! Send a response to GatorByte Desktop Client
*/
void GB_DESKTOP::send(String data) { return this->send("", data); } 
void GB_DESKTOP::send(String category, String data) { 
    if (!_gb->globals.GDC_CONNECTED) return;

    String prefix = "##CL##", suffix = "#EOF#";

    //! Send the command to GatorByte Configurator
    delay(5);
    this->_gb->serial.debug->println(
        "<br>" + prefix + 
        category + "::" + data + 
        suffix
    );
    
    // Unset busy
    this->_busy(false);

    // this->_gb->serial.debug->flush();
    delay(10);
}

void GB_DESKTOP::sendfile(String category, String data) { 
    if (!_gb->globals.GDC_CONNECTED) return;

    String prefix = "##CL##", suffix = "#EOF#";

    //! Send the command to GatorByte Configurator
    delay(5);
    this->_gb->serial.debug->println(
        prefix + 
        category + "::" + data + 
        suffix
    );
    // this->_gb->serial.debug->flush();
    delay(10);
}

#endif