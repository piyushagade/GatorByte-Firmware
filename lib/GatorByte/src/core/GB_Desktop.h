#ifndef GB_DESKTOP_h
#define GB_DESKTOP_h

/*
    ! Uses ~9% flash memory
*/

#ifndef GB_h
    #include "../GB.h"
#endif

#define LOGCONTROL false

class GB_DESKTOP : public GB_DEVICE {
    public:
        GB_DESKTOP(GB &gb);
        
        DEVICE device = {
            "gdc",
            "Desktop Client"
        };

        GB_DESKTOP& detect() override;
        GB_DESKTOP& detect(bool lock);
        GB_DESKTOP& loop() override;
        
        void process(string);
        GB_DESKTOP& enter();
        GB_DESKTOP& exit();
        
        bool send(String data);
        bool send(String category, String data);
        void sendfile(String category, String data);
        
        bool testdevice();
        String status();

    private:
        GB *_gb; 
        String _prev_mode;
        String _state = "";
        bool lock = false;
        GB_DESKTOP& _busy(bool busy);
        
        // Temporary data
        String tempstring[5];
        int tempint[5];
        bool tempbool[5];
        float tempfloat[5];
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
    #if LOGCONTROL
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    #endif
    return this->device.detected;
}
String GB_DESKTOP::status() { 
    return "detected";
}

GB_DESKTOP& GB_DESKTOP::detect() { return this->detect(true); }
GB_DESKTOP& GB_DESKTOP::detect(bool lock) { 

    // Set the fuse if requested
    if (digitalRead(A6) == HIGH) {
        pinMode(A6, INPUT_PULLDOWN);
        _gb->getdevice("sntl")->setfuse();
        _gb->getdevice("buzzer")->play("---").wait(250).play("-.-.-");
    }

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
            Serial.println("##CL-GDC-PONG##"); delay(50);
            Serial.println("##CL-GDC-SN::" + _gb->getmcu()->getsn() + "##"); delay(50);

            if (_gb->globals.GDC_SETUP_READY) {
                Serial.println("##CL-GB-READY##");
                
                // Get device environment from memory
                if (_gb->hasdevice("mem") &&  _gb->getdevice("mem")->get(0) == "formatted") {
                    String savedenv = _gb->getdevice("mem")->get(1);
                    _gb->env(savedenv);
                    Serial.println("##CL-GDC-ENV::" + _gb->env() + "##"); delay(50);
                }
            }

            // this->send("gdc-db", "power=awake");

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
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on(1).wait(100).revert();
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play(",.");
        }
        else  {
            if (_gb->hasdevice("rgb") )_gb->getdevice("rgb")->revert();
        }
    return *this;
}

// Enter GDC mode
GB_DESKTOP& GB_DESKTOP::enter() {

    _gb->log("Entering GDC mode");

    uint8_t last_led_act_ts = millis();
    bool led_state = false;
    while (_gb->globals.GDC_CONNECTED) {
        
        // Look for new commands
        this->loop(); 

        if (millis() - last_led_act_ts >= 1000) {
            last_led_act_ts = millis();
            led_state = !led_state;
            if (_gb->hasdevice("rgb")) {
                if (led_state) _gb->getdevice("rgb")->on(4);
                else _gb->getdevice("rgb")->off();
            }
        }

        delay(250);
    }

    return *this;
}

// Turn off the module
GB_DESKTOP& GB_DESKTOP::exit() {
    #if LOGCONTROL
    _gb->log("Exiting commnd mode. Reverting to \"" + this->_prev_mode + "\" mode.\n");
    #endif
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
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on(4);

    #endif
    return *this;
}

// Process the incoming command
void GB_DESKTOP::process(string command) {

    #if LOGCONTROL
    // _gb->br().log("Processing command: " + command, false);
    // _gb->arrow().log("Current state: " + String(this->_state.length() > 0 ? this->_state : "None"));
    #endif
    
    /*
        ! Ping
    */
    if (command.contains("gdc-ping")) {
        Serial.println("##CL-GDC-PONG##"); delay(50);
        Serial.println("##CL-GDC-SN::" + _gb->getmcu()->getsn() + "##"); delay(50);
        
        if (_gb->hasdevice("sd")) {
            if (!_gb->getdevice("sd")->testdevice()) Serial.println("##CL-GB-SD-UINT##");
            if (_gb->globals.GDC_SETUP_READY) {
                Serial.println("##CL-GB-READY##");
                
                // Get device environment from memory
                if (_gb->hasdevice("mem") &&  _gb->getdevice("mem")->get(0) == "formatted") {
                    String savedenv = _gb->getdevice("mem")->get(1);
                    _gb->env(savedenv);
                    Serial.println("##CL-GDC-ENV::" + _gb->env() + "##"); delay(50);
                }
            }
        }
        else if (!_gb->hasdevice("sd")) {
            Serial.println("##CL-GB-SD-UINT##");
        }
    }
    
    //! Uses 8% flash
    #if not defined (LOW_MEMORY_MODE)

        String CONFIGPATH = "/config/config.ini";
        String CVPATH = "/control/variables.ini";

        // Set as busy
        this->_busy(true);

        #if LOGCONTROL
          _gb->log("Received GDC command: " + command);
        #endif

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
            ! Global configuration download/upload; Uses 0.5% flash
        */
        if (command.startsWith("cfg")) {

            // Configuration download request
            if (command.contains("cfgdl:")) {

                // String filename = "/config/" + command.substring(command.indexOf("dl:") + 3, command.indexOf(","));
                // String configdata = _gb->getdevice("sd")->readfile(filename);
                
                String configdata = _gb->getconfig();
                for (unsigned int i = 0; i < configdata.length(); i += 30) {
                    
                    // Extract a chunk of 30 characters
                    String chunk = configdata.substring(i, i + 30);

                    // Send the chunk over Serial
                    this->sendfile("gdc-cfg", "fdl:" + chunk);

                    // Add a delay if needed to prevent data loss
                    delay(10);
                }
                this->sendfile("gdc-cfg", "fdl:#EOF#"); delay(10);
            }
            
            // Configuration upload request
            else if (command.contains("cfgupl:")) {
            
                String filename = CONFIGPATH;
                string data = command.substring(command.indexOf("upl:") + 4, command.indexOf("^"));
                int initialcharindex = command.substring(command.indexOf("^") + 1, command.length()).toInt();

                while(data.contains("~")) {
                    data.replace("~", "\n");
                }

                while(data.contains("`")) {
                    data.replace("`", " ");
                }

                // Delete preexisting file if the upload has just started.
                if (initialcharindex == 0) _gb->getdevice("sd")->rm(filename);

                // Append data to the file
                _gb->getdevice("sd")->writeLinesToSD(filename, data);

                // Pause
                delay(10);

                // Send acknowledgement
                this->send("gdc-cfg", "fupl:ack");
            }

            // Post config upload tasks
            else if (command.contains("cfgupd:")) {
            
                // Get the updated config from SD
                String configdata = _gb->getdevice("sd")->readconfig();

                // // Sync config in the EEPROM
                // _gb->getdevice("mem")->writeconfig(configdata);

                // Updata the config in the Arduino's flash memory
                _gb->processconfig(configdata);

            }

            // Post config upload tasks
            else if (command.contains("hash")) {
            
                String configdata = _gb->getdevice("sd")->readfile(CONFIGPATH);

                // Compute hash
                unsigned int hash = _gb->s2hash(configdata);

                // Send acknowledgement
                this->send("gdc-cfg", "cfghash:" + String(hash)); delay(10);

            }

            // Battery status
            else if (command.contains("batt")) {
                #if LOGCONTROL
                    _gb->log("Battery level requested.");
                #endif
            
                float level = _gb->getmcu()->fuel("level");

                // Send acknowledgement
                this->send("gdc-cfg", "cfgbatt:" + String(level)); delay(10);

            }
        }
        
        /*
            ! Create base folders and files on SD card if they don't exist; Uses 0.3% flash
        */
        if (command.startsWith("sdf")) {
            
            if (command.contains("cr:all")) {
                if (!_gb->hasdevice("sd")) {
                    this->send("gdc-sdf", "sd:error");
                    return;
                }
                if (!_gb->getdevice("sd")->initialized()) {
                    this->send("gdc-sdf", "sdinit:error");
                    return;
                }

                // Create folder
                String folders[] = {"config", "calibration", "control", "readings", "debug", "logs", "queue"};
                for (unsigned int i = 0; i < sizeof(folders) / sizeof(folders[0]); i++) {
                    String foldername = folders[i];
                    if (!_gb->getdevice("sd")->exists("/" + foldername)) {
                        #if LOGCONTROL
                            _gb->log(foldername + " doesn't exist. Creating directory", false);
                        #endif
                        bool success = _gb->getdevice("sd")->mkdir("/" + foldername);
                        #if LOGCONTROL
                          _gb->arrow().log(success ? "Done" : "Failed");
                        #endif
                    }
                }

                // Create config file
                if (!_gb->getdevice("sd")->exists(CONFIGPATH)) {
                    #if LOGCONTROL
                        _gb->log("File config.ini doesn't exist. Creating file.");
                    #endif
                    _gb->getdevice("sd")->writeLinesToSD(CONFIGPATH, "");
                }

                // Create control file
                if (!_gb->getdevice("sd")->exists(CVPATH)) {
                    #if LOGCONTROL
                        _gb->log("File variables.ini doesn't exist. Creating file.");
                    #endif
                    _gb->getdevice("sd")->writeLinesToSD(CVPATH, "");
                }

                // Send acknowledgement
                this->send("gdc-sdf", "success");
            }
        }

        /*
            ! SD file list / File download; Uses 1% flash
        */
        if (this->_state == "fl" || command.contains("fl")) {
            this->_state = "fl";

            // Strip prefix and suffix
            command.replace("##GB##fl", "");
            command.replace("#EOF#", "");
            
            // List the folders
            if (command.contains(":list")) {

                String action = command.substring(0, command.indexOf(","));
                
                // Get the name of the folder to get file list
                String root = command.substring(command.indexOf(",") + 1, command.length());
                // _gb->log("Listing folder: " + root);
                
                String filelistdata = _gb->getdevice("sd")->getfilelist(root);
                int filecount = filelistdata.substring(0, filelistdata.indexOf("::")).toInt();
                String list = filelistdata.substring(filelistdata.indexOf("::") + 2, filelistdata.length());

                // Test SD
                bool success = _gb->hasdevice("sd") ? _gb->getdevice("sd")->testdevice() : false;

                // Send acknowledgment
                if (success) {
                    String file = "";

                    if (filecount == 0) {
                        this->send("gdc-dfl", "error:nofile");
                    }
                    else {
                        for (unsigned int i = 0; i < list.length(); i++) {
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

                String filename = command.substring(command.indexOf(":") + 1, command.indexOf(","));
                
                File file = _gb->getdevice("sd")->openFile("read", filename);
                if (!file) {
                    #if LOGCONTROL
                        _gb->log("Error opening file!");
                    #endif
                    return;
                }

                const int bufferSize = 40;
                char buffer[bufferSize];

                while (file.available()) {
                    int bytesRead = file.read(buffer, bufferSize);

                    // Process the data in the buffer
                    String chunk = "";
                    for (int i = 0; i < bytesRead; i++) {
                        chunk += String(buffer[i]);
                    }
                    this->sendfile("gdc-dfl", "fdl:" + String(chunk)); delay(25);
                }

                #if LOGCONTROL
                _gb->log("File upload completed: " + filename);
                #endif
                this->sendfile("gdc-dfl", "fdl:#DLEOF#"); delay(10);
            }

            // Delete a file
            if (command.contains("rm:")) {
                String file = command.substring(command.indexOf("rm:") + 3, command.length());
                #if LOGCONTROL
                _gb->log("Removing file: " + file);
                #endif
                _gb->getdevice("sd")->rm(file);
            }

            // Delete a folder
            if (command.contains("rmd:")) {
                String file = command.substring(command.indexOf("rmd:") + 4, command.length());
                #if LOGCONTROL
                _gb->log("Removing folder: " + file);
                #endif
                _gb->getdevice("sd")->rmdir(file);
            }

            // Make directory
            if (command.contains("crd:")) {
                String dirname = command.substring(command.indexOf("crd:") + 4, command.length());
                #if LOGCONTROL
                _gb->log("Creating directory: " + dirname);
                #endif
                
                String foldername = dirname;
                if (!_gb->getdevice("sd")->exists("/" + foldername)) {
                    bool success = _gb->getdevice("sd")->mkdir("/" + foldername);
                }
            }
            
            // Rename directory
            if (command.contains("rnd:")) {
                String stripped = command.substring(command.indexOf("rnd:") + 4, command.length());
                String oldname = stripped.substring(0, stripped.indexOf("#"));
                String newname = stripped.substring(stripped.indexOf("#") + 1, stripped.length());
                #if LOGCONTROL
                _gb->log("Renaming directory: " + oldname + " to " + newname);
                #endif
                
                if (_gb->getdevice("sd")->exists("/" + oldname)) {
                    bool success = _gb->getdevice("sd")->renamedir(oldname , newname);
                }
            }

            // Preview file
            if (command.contains("pv:")) {

            }

            // Upload file meta
            if (command.contains("fuplm:")) {
                String filepath = command.substring(String("fuplm:").length(), command.length());
                this->tempstring[0] = filepath;

                bool exists = _gb->getdevice("sd")->exists(filepath);
                if (exists) {
                    #if LOGCONTROL
                    _gb->log("File already exists. Deleting file.");
                    #endif
                    _gb->getdevice("sd")->rm(filepath);
                }
                
                #if LOGCONTROL
                _gb->log("Setting file path: " + filepath);
                #endif
            }

            // Upload file
            if (command.contains("fupl:")) {
                // command = command.substring(command.indexOf("fupl:") + 1, command.length());

                String filepath = this->tempstring[0];

                String target = "fupl:";
                String replacement = "";
                
                String data = _gb->sreplace(command, target, replacement);
                
                if (filepath.length() == 0) {
                    #if LOGCONTROL
                    _gb->log("File path not set");
                    #endif
                }

                else {
                    #if LOGCONTROL
                    _gb->log("File upload request received: ", false);
                    _gb->log(filepath);
                    #endif

                    while(data.contains("~")) {
                        data.replace("~", "\n");
                    }

                    while(data.contains("`")) {
                        data.replace("`", " ");
                    }

                    _gb->getdevice("sd")->writeLinesToSD(filepath, data);

                    this->send("gdc-dfl", "upl:ack");
                }
            }
        
        }
        
        /*
            ! Calibration; Uses 1% flash
        */
        if (this->_state == "cal" || command.contains("calibration")) {
            this->_state = "cal";

            // Set state to calibration
            if (command.contains(":enter")) {

                // Strip prefix
                command.replace(":enter", "");

                // Get sensor name
                String sensor = command;

                // Send acknowledgment
                this->send("gdc-cal", "ack");
            }
            
            // Power control
            else if (command.contains(":pcon:")) {

                // Strip prefix
                command.replace(":pcon", "");

                // Get sensor name
                String sensor = command.substring(0, command.indexOf(":"));
                String action = command.substring(command.indexOf(":") + 1, command.length());

                if (action == "on") _gb->getdevice(sensor)->on();
                else _gb->getdevice(sensor)->off();
                
                // // Send acknowledgment
                // this->send("gdc-cal", "ack");
            }

            else if (command.contains(":calibrate")) {

                // Strip prefix
                command.replace(":calibrate", "");
                
                // Get sensor name
                // String sensor = strtok(_gb->s2c(command), ":");
                // String remainder = strtok(NULL, ":");

                String sensor = command.substring(0, command.indexOf(":"));
                String level = command.substring(command.indexOf(":") + 1, command.indexOf(","));
                int value = command.substring(command.indexOf(",") + 1, command.length()).toInt();

                // Get calibration configuration
                // String level = strtok(_gb->s2c(remainder), ",");
                // int value = String(strtok(NULL, ",")).toInt();
                
                #if LOGCONTROL
                _gb->log("Processed calibration request for: " + sensor);
                _gb->log("Level: ", false);
                _gb->log(level);
                #endif
                
                // Calibrate
                int status = _gb->getdevice(sensor)->calibrate(level, value);
                
                // Send acknowledgment
                this->send("gdc-cal", "ack");
                this->send("gdc-cal", "result" + String(":") + sensor + String(":") + String(status));

                // Update "last calibrated" date on SD card
                if (level != "status" && level != "clear") {
                    _gb->getdevice("sd")->rm("/calibration/" + sensor + ".ini");
                    _gb->getdevice("sd")->writeString("/calibration/" + sensor + ".ini", _gb->getdevice("rtc")->timestamp());
                }
            }

            // Get continuous readings
            else if (command.contains(":cread")) {

                // Strip prefix, remainder will be "ph:30"
                command.replace(":cread", "");

                // Get sensor name
                String sensor = command.substring(0, command.indexOf(":"));
                int NUMBER_OF_READINGS = command.substring(command.indexOf(":") + 1, command.length()).toInt();

                #if LOGCONTROL
                _gb->br().log("Reading " + String(NUMBER_OF_READINGS) + " continuous values from " + sensor);
                #endif
                int count = 0;

                for (count = 0; count < NUMBER_OF_READINGS; count++) {

                    float sensorvalue = _gb->getdevice(sensor)->readsensor("next");

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
                String lpi = _gb->getdevice("sd")->readfile("/calibration/" + sensor + ".ini");
                
                // Send acknowledgment
                this->send("gdc-cal", "lpi" + String(":") + String(lpi));
            }
        
        }

        /* 
            ! Dashboard; Uses 1% flash
        */
        if (this->_state == "db" || command.contains("db")) {
            this->_state = "db";

            if (command.contains(":enter")) {
                NBModem _nbModem;

                // Device status
                // this->send("gdc-db", "power=awake");

                // Check MODEM status
                MODEM_INITIALIZED = _nbModem.begin();
                // this->send("gdc-db", "modem=" + String(MODEM_INITIALIZED ? "active" : "not-responding"));
                
                // MODEM firmware
                // this->send("gdc-db", "modem-fw=" + _gb->getmcu()->getfirmwareinfo());
                
                // MODEM IMEI
                // this->send("gdc-db", "modem-imei=" + _gb->getmcu()->getimei());
                
                // SIM ICCID
                // this->send("gdc-db", "sim-iccid=" + _gb->getmcu()->geticcid());

                // Network strength
                // this->send("gdc-db", "rssi=" + String(_gb->getmcu()->getrssi()));

                // Operator
                String cops = _gb->getmcu()->getoperator();
                cops = cops.substring(cops.indexOf("\"") + 1, cops.lastIndexOf("\""));
                // this->send("gdc-db", "cops=" + String(cops));
            }
            
            // Sensor readings
            else if (command.contains("fsr:")) {

                if (command.contains(":all")) {
                    String result = "";

                    result += "rtd:" + String(_gb->getdevice("rtd")->readsensor()) + String(",");
                    result += "ph:" + String(_gb->getdevice("ph")->readsensor()) + String(",");
                    result += "dox:" + String(_gb->getdevice("dox")->readsensor()) + String(",");
                    result += "ec:" + String(_gb->getdevice("ec")->readsensor());

                    // The following line is not needed because the individual functions above send this info on their own
                    // this->send("gdc-db", "fsr=" + result);
                }
                else if (command.contains(":rtd")) {
                    String result = "";

                    result += "rtd:" + String(_gb->getdevice("rtd")->readsensor());
                    // this->send("gdc-db", "fsr=" + result);
                }
                else if (command.contains(":ph")) {
                    String result = "";

                    result += "ph:" + String(_gb->getdevice("ph")->readsensor());
                    // this->send("gdc-db", "fsr=" + result);
                }
                else if (command.contains(":dox")) {
                    String result = "";

                    result += "dox:" + String(_gb->getdevice("dox")->readsensor());
                    // this->send("gdc-db", "fsr=" + result);
                }
                else if (command.contains(":ec")) {
                    String result = "";

                    result += "ec:" + String(_gb->getdevice("ec")->readsensor());
                    // this->send("gdc-db", "fsr=" + result);
                }
            }

        }
        
        /* 
            ! Configure GatorByte; Uses 1% flash
        */
        if (this->_state == "cfg" || command.contains("cfgb")) {
            this->_state = "cfg";

            // Strip prefix and suffix
            command.replace("##GB##cfgb", "");
            command.replace("##GB##cfg", "");
            command.replace("#EOF#", "");

            
            //! Sentinel configuration
            if (command.contains("sntl:")) {

                #if LOGCONTROL
                _gb->log("Command: " + command);
                #endif


                // Get fuse status
                if (command.contains("sf:get")) {
                    
                    bool success = _gb->hasdevice("sntl") ? _gb->getdevice("sntl")->testdevice() : false;            
                    uint8_t code = 39;
                    uint8_t status = _gb->getdevice("sntl")->tell(code, 5);
                    this->send("gdc-cfg", "sf:get:" + String(status ? "true" : "false"));
                }

                // Set fuse
                if (command.contains("sf:set:")) {
                    
                    command = command.substring(12, command.length());
                    bool success = _gb->hasdevice("sntl") ? _gb->getdevice("sntl")->testdevice() : false;            
                    uint8_t code = command == "set" ? 37 : 38;
                    if (success) success = _gb->getdevice("sntl")->tell(code, 5) == 0;
                    this->send("gdc-cfg", "sf:" + String(success ? "true" : "false"));
                }
            }

            //! RTC action
            if (command.contains("rtc:")) {

                // Remove "rtc:" substring
                command = command.substring(4, command.length());

                //! Sync RTC
                if (command.contains("sync")) {
                    
                    #if LOGCONTROL
                    _gb->log("Syncing GatorByte time to ", false);
                    #endif
                    bool success = true || _gb->hasdevice("rtc") && _gb->getdevice("rtc")->testdevice();

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

                        #if LOGCONTROL
                        _gb->log(fulldate + ", ", false);
                        _gb->log(fulltime);
                        #endif

                        _gb->getdevice("rtc")->sync(_gb->s2c(fulldate), _gb->s2c(fulltime));
                        this->send("gdc-cfg", "ack");
                    }
                    else {
                        #if LOGCONTROL
                        _gb->arrow().log("Skipped");
                        #endif
                        this->send("gdc-cfg", "nack");
                    }
                }

                //! Get RTC timestamp
                if (command.contains("get")) {
                    
                    bool success = true || _gb->hasdevice("rtc") && _gb->getdevice("rtc")->testdevice();
                    
                    String data;
                    if (success) data = _gb->getdevice("rtc")->timestamp();
                    else data = "not-detected";

                    // Send RTC time
                    this->send("gdc-cfg", "rtc:" + data + "-" + _gb->getdevice("rtc")->getsource());

                    Serial.println("rtc:" + data + "::" + _gb->getdevice("rtc")->getsource()); 
                }
            }

            //! Bluetooth action
            if (command.contains("bl:")) {

                #if LOGCONTROL
                _gb->log("Command: " + command);
                #endif
                if (command.contains("getconfig")) {
                    
                    bool success = _gb->hasdevice("bl") && _gb->getdevice("bl")->testdevice();
                    if (success) {
                        String namedata = _gb->getdevice("bl")->send_at_command("AT+NAME");
                        String name = namedata.substring(namedata.indexOf('=') + 1, namedata.length());
                        
                        String pindata = _gb->getdevice("bl")->send_at_command("AT+PIN");
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

                    bool success = _gb->hasdevice("bl") && _gb->getdevice("bl")->testdevice();
                    if (success) {
                        String name = command.substring(0, command.indexOf(';'));
                        String pin = command.substring(command.indexOf(';') + 1, command.length());

                        #if LOGCONTROL
                        _gb->log("Setting BL name to " + name);
                        _gb->log("Setting BL PIN to " + pin);
                        #endif

                        // Update BL name and pin by sending AT commands
                        _gb->getdevice("bl")->send_at_command("AT+NAME" + name);
                        _gb->getdevice("bl")->send_at_command("AT+PIN" + pin);

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
            ! Control variables; Uses 1% flash
        */
        if (this->_state == "cv" || command.contains("cv")) {
            this->_state = "cv";

            if (!_gb->hasdevice("sd")) {
                this->send("gdc-sdf", "sd:error");
                return;
            }
            if (!_gb->getdevice("sd")->initialized()) {
                this->send("gdc-sdf", "sdinit:error");
                return;
            }

            // Strip prefix and suffix
            command.replace("##GB##cv", "");
            command.replace("#EOF#", "");

            // Configuration download request
            if (command.contains("cvdl:")) {
                
                String filename = CVPATH;

                // int initialcharindex = command.substring(command.indexOf(":") + 1, command.length()).toInt();
                // int charsatatime = 30;

                // String data = _gb->getdevice("sd")->readLinesFromSD(filename, charsatatime, initialcharindex);
                // this->sendfile("gdc-cv", "fdl:" + data);

                String data = _gb->getdevice("sd")->readfile(filename);
                for (int i = 0; i < data.length(); i += 30) {
                    // Extract a chunk of 30 characters
                    String chunk = data.substring(i, i + 30);

                    // Send the chunk over Serial
                    this->sendfile("gdc-cv", "fdl:" + chunk);

                    // Add a delay if needed to prevent data loss
                    delay(10);
                }
                this->sendfile("gdc-cv", "fdl:#EOF#"); delay(10);
            }
            
            // Control variables upload request
            else if (command.contains("cvupl:")) {
            
                String filename = CVPATH;
                string data = command.substring(command.indexOf("upl:") + 4, command.indexOf("^"));
                int initialcharindex = command.substring(command.indexOf("^") + 1, command.length()).toInt();

                while(data.contains("~")) {
                    data.replace("~", "\n");
                }

                while(data.contains("`")) {
                    data.replace("`", " ");
                }

                // Delete preexisting file if the upload has just started.
                if (initialcharindex == 0) _gb->getdevice("sd")->rm(filename);

                // Append data to the file
                _gb->getdevice("sd")->writeLinesToSD(filename, data);

                // Pause
                delay(10);

                // Reset global variable
                _gb->controls.reset();

                // Send acknowledgement
                this->send("gdc-cv", "fupl:ack");
            }

            // Post config upload tasks
            else if (command.contains("cvupd:")) {

                // Reset global variable
                _gb->controls.reset();
            
                // Update config in the memory
                _gb->getdevice("sd")->readconfig();

            }

            //! Get hash from stored file
            if (command.contains("cv:hash")) {

                String cvdata = _gb->getdevice("sd")->readfile(CVPATH);
                int hash = _gb->s2hash(cvdata);

                #if LOGCONTROL
                _gb->log("Control variables hash: " + String(hash));
                #endif

                // Send response
                this->send("gdc-cv", "hash:" + String(hash));
            }
            
            //! Add a variable
            else if (command.contains("add:")) {
                command = command.substring(4, command.length());

                String variablename = command.substring(0, command.indexOf(":"));
                command = command.substring(variablename.length() + 1, command.length());

                String variabletype = command.substring(0, command.indexOf(":"));
                command = command.substring(variabletype.length() + 1, command.length());

                String variablevalue = command;

                // Write to variables file and update
                typedef void (*callback_t_on_control)(JSONary data);
                callback_t_on_control func = [](JSONary data){};
                if (variabletype == "string") {
                    _gb->controls.set(variablename, variablevalue);
                    _gb->getdevice("sd")->updatecontrolstring(variablename, variablevalue, func);
                }
                else if (variabletype == "bool") {
                    _gb->controls.set(variablename, variablevalue);
                    _gb->getdevice("sd")->updatecontrolbool(variablename, variablevalue == "true", func);
                }
                else if (variabletype == "int")  {
                    _gb->controls.set(variablename, variablevalue);
                    _gb->getdevice("sd")->updatecontrolint(variablename, variablevalue.toInt(), func);
                }
                else if (variabletype == "float")  {
                    _gb->controls.set(variablename, variablevalue);
                    _gb->getdevice("sd")->updatecontrolfloat(variablename, variablevalue.toDouble(), func);
                }
                else _gb->controls.set(variablename, variablevalue);

                #if LOGCONTROL
                _gb->log("Control variables updated.");
                #endif
                _gb->getdevice("sd")->readcontrol();
                #if LOGCONTROL
                _gb->log(_gb->getdevice("sd")->readfile(CVPATH));
                #endif

                // Send response
                this->send("gdc-cv", "ack:true");
            }

        }
        
        /* 
            ! Device diagnostics; Uses 2% flash
        */
        if (this->_state == "dgn" || command.contains("dgn")) {
            this->_state = "dgn";
            
            // Strip prefix and suffix
            command.replace("##GB##", "");
            command.replace("#EOF#", "");

            if (command.contains("rtc")) {
                String rtctimestamp = _gb->hasdevice("rtc") ? _gb->getdevice("rtc")->timestamp() : "";
                
                // Send RTC time
                this->send("gdc-dgn", "rtc:" + rtctimestamp);
            }
            
            if (command.contains("mem")) {
                bool success = _gb->hasdevice("mem") ? _gb->getdevice("mem")->get(0) == "formatted": false;
                
                // Send response
                this->send("gdc-dgn", "mem:" + String(success ? "true" : "false"));
            }
            
            if (command.contains("sd")) {
                bool success = _gb->hasdevice("sd") ? _gb->getdevice("sd")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "sd:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("sd")->status());
            }
            
            if (command.contains("eadc")) {
                bool success = _gb->hasdevice("eadc") ? _gb->getdevice("eadc")->testdevice(): false;
                
                // Send response
                this->send("gdc-dgn", "eadc:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("eadc")->status());
            }
            
            if (command.contains("fram")) {
                bool success = _gb->hasdevice("fram") ? _gb->getdevice("fram")->testdevice() : false;

                // Send response
                this->send("gdc-dgn", "fram:" + String(success ? "true" : "false"));
            }
            
            if (command.contains("bl")) {
                bool success = _gb->hasdevice("bl") ? _gb->getdevice("bl")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "bl:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("bl")->status());
            }
            
            if (command.contains("gps")) {
                bool success = _gb->hasdevice("gps") ? _gb->getdevice("gps")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "gps:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("gps")->status());
            }
            
            if (command.contains("sntl")) {
                bool success = _gb->hasdevice("sntl") ? _gb->getdevice("sntl")->testdevice() : false;
                
                // Send response
                // this->send("gdc-dgn", "sntl:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("sntl")->status());
                this->send("gdc-dgn", "sntl:" + String("true") + ":..:" + _gb->getdevice("sntl")->status());
            }
            
            if (command.contains("aht")) {
                bool success = _gb->hasdevice("aht") ? _gb->getdevice("aht")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "aht:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("aht")->status());
            }
            
            if (command.contains("relay")) {
                bool success = _gb->hasdevice("relay") ? _gb->getdevice("relay")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "relay:" + String(success ? "true" : "false"));
            }
            
            if (command.contains("rgb")) {
                bool success = _gb->hasdevice("rgb") ? _gb->getdevice("rgb")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "rgb:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("rgb")->status());
            }
            
            if (command.contains("buzzer")) {
                bool success = _gb->hasdevice("buzzer") ? _gb->getdevice("buzzer")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "buzzer:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("buzzer")->status());
            }
            
            if (command.contains("uss")) {
                bool success = _gb->hasdevice("uss") ? _gb->getdevice("uss")->testdevice() : false;

                // Send response
                this->send("gdc-dgn", "uss:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("uss")->status());
            }
            
            if (command.contains("ph")) {
                bool success = _gb->hasdevice("ph") ? _gb->getdevice("ph")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "ph:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("ph")->status());
            }

            if (command.contains("ec")) {
                bool success = _gb->hasdevice("ec") ? _gb->getdevice("ec")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "ec:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("ec")->status());
            }
            
            if (command.contains("dox")) {
                bool success = _gb->hasdevice("dox") ? _gb->getdevice("dox")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "dox:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("dox")->status());
            }
            
            if (command.contains("rtd")) {
                bool success = _gb->hasdevice("rtd") ? _gb->getdevice("rtd")->testdevice() : false;
                
                // Send response
                this->send("gdc-dgn", "rtd:" + String(success ? "true" : "false") + ":..:" + _gb->getdevice("rtd")->status());
            }
            
            if (command.contains("lipo")) {
                bool success = _gb->getmcu()->testbattery();
                
                // Send response
                this->send("gdc-dgn", "lipo:" + String(success ? "true" : "false") + ":..:" + _gb->getmcu()->batterystatus());
            }
            
            if (command.contains("comm:all")) {
                NBModem _nbModem;

                // Check MODEM status
                MODEM_INITIALIZED = _nbModem.begin();
                this->send("gdc-dgn", "modem=" + String(MODEM_INITIALIZED ? "active" : "not-responding"));
                
                // MODEM firmware
                this->send("gdc-dgn", "modem-fw=" + _gb->getmcu()->getfirmwareinfo());
                
                // MODEM IMEI
                this->send("gdc-dgn", "modem-imei=" + _gb->getmcu()->getimei());
                
                // SIM ICCID
                this->send("gdc-dgn", "sim-iccid=" + _gb->getmcu()->geticcid());

                // Network strength
                this->send("gdc-dgn", "rssi=" + String(_gb->getmcu()->getrssi()));

                // Operator
                String cops = _gb->getmcu()->getoperator();
                cops = cops.substring(cops.indexOf("\"") + 1, cops.lastIndexOf("\""));
                this->send("gdc-dgn", "cops=" + String(cops));
            }

            if (command.endsWith("comm:modem")) {
                NBModem _nbModem;

                // Check MODEM status
                MODEM_INITIALIZED = _nbModem.begin();
                this->send("gdc-dgn", "modem=" + String(MODEM_INITIALIZED ? "active" : "not-responding"));
                
                // MODEM firmware
                this->send("gdc-dgn", "modem-fw=" + _gb->getmcu()->getfirmwareinfo());
                
                // MODEM IMEI
                this->send("gdc-dgn", "modem-imei=" + _gb->getmcu()->getimei());
            }

            if (command.endsWith("comm:modem:rb")) {
                #if LOGCONTROL
                _gb->log("Rebooting MODEM");
                #endif
                
                NB _nb;
                NBModem _nbModem;

                // Turn off MODEM
                _nb.secureShutdown();

                // Turn MODEM on
                int counter = 0;
                bool result = false;
                while (!(result = _nb.begin()) && counter++ < 5) { delay(2000); }
                MODEM_INITIALIZED = result;
            }

            if (command.contains("comm:sim")) {
                
                // SIM ICCID
                this->send("gdc-dgn", "sim-iccid=" + _gb->getmcu()->geticcid());
            }

            if (command.contains("comm:nw")) {
                
                // Network strength
                this->send("gdc-dgn", "rssi=" + String(_gb->getmcu()->getrssi()));

                // Operator
                String cops = _gb->getmcu()->getoperator();
                cops = cops.substring(cops.indexOf("\"") + 1, cops.lastIndexOf("\""));
                this->send("gdc-dgn", "cops=" + String(cops));
            }

            if (command.contains("comm:conn")) {
                
                //! Connect to network
                _gb->getmcu()->connect();
                
                //! Connect to MQTT servver
                _gb->getdevice("mqtt")->connect();
                    
                this->send("gdc-dgn", "conn=init");
            }
        
        }
    
    #endif
}

/*
    ! Send a response to GatorByte Desktop Client
*/
bool GB_DESKTOP::send(String data) { return this->send("", data); } 
bool GB_DESKTOP::send(String category, String data) { 
    if (!_gb->globals.GDC_CONNECTED) return true;

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

    return true;
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
    // delay(10);
}

#endif