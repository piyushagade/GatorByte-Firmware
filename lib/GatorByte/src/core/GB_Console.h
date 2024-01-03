#ifndef GB_CONSOLE_h
#define GB_CONSOLE_h

#ifndef GB_h
    #include "./GB_Primary.h"
#endif

class GB_CONSOLE : public GB_DEVICE {
    public:
        GB_CONSOLE(GB &gb);

        DEVICE device = {
            "command",
            "Command mode"
        };

        GB_CONSOLE& initialize();
        GB_CONSOLE& loop();
        void process(String);
        GB_CONSOLE& enter();
        GB_CONSOLE& exit();
        
        String post(String message);
        bool log(String message);

        bool confirm_discard_changes();
        bool confirm_discard_changes(int timer);
        bool confirm_action(int timer);
        String get_string_from_console();
        String get_string_from_console(int timer);
        String get_string_from_console(bool mirror);
        String get_string_from_console(bool mirror, int timer);

    private:
        GB *_gb; 
        GB_DEVICE *_led;
        String _prev_mode;
        bool _state = false;
};

GB_CONSOLE::GB_CONSOLE(GB &gb) {
    _gb = &gb;
    _led = &_gb->getdevice("rgb");
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.command = this;
}

GB_CONSOLE& GB_CONSOLE::initialize() { 
    _gb->log("Initializing " + this->device.name + ": ", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    this->_gb->serial.debug->begin(9600);
    return *this;
}

// Enter console mode
GB_CONSOLE& GB_CONSOLE::enter() {
    this->_prev_mode = this->_gb->globals.MODE;
    this->_gb->globals.MODE = "command";

    // Wait for command input
    String str = "";
    if(_gb->serial.debug->available()) {
        str = _gb->serial.debug->readStringUntil('\n');
    };
    if(str.length() > 0) {
        _gb->log("You have entered: ", false);
        _gb->log(str);
        this->process(str);
    }

    return *this;
}

// Turn off the module
GB_CONSOLE& GB_CONSOLE::exit() {
    _gb->log("Exiting comand mode. Reverting to \"" + this->_prev_mode + "\" mode.\n");
    this->_gb->globals.MODE = this->_prev_mode;
    this->_led->revert();
    return *this;
}

GB_CONSOLE& GB_CONSOLE::loop() {

    //! Set RGB color
    if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("yellow");

    // // Clear the serial buffer
    // while (_gb->serial.debug->available()) char c = _gb->serial.debug->read();

    /*
        1. The following code works with sm3.py
    */
    String response;
    char c;
    while (_gb->serial.debug->available()) {
        c = _gb->serial.debug->read();
        response += String(c);
        response.trim();
    }

    if (response.length() > 0) {
        if (response == "m") {
            this->process("menu");
        }
        else {
            this->process(response);
        }
    }
}

// // Check if there is a new char from computer/bl serial
// GB_CONSOLE& GB_CONSOLE::loop() {

//     #if not defined (LOW_MEMORY_MODE)

//         String str = "";
//         if (_gb->serial.debug->available() > 0 || this->_gb->globals.MODE == "command") {
            
//             // Set device mode
//             if (this->_gb->globals.MODE != "command") this->_prev_mode = this->_gb->globals.MODE;
//             this->_gb->globals.MODE = "command";
    
//             //! Set RGB color
//             if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("green");

//             // Clear the serial buffer
//             while (_gb->serial.debug->available()) char c = _gb->serial.debug->read();

//             //! Show menu
//             this->process("menu");
            
//             // String c = "";
//             // bool flag = false;
//             // str += c;
//             // if (!str.endsWith("\n") && !str.endsWith("\r")) {
//             //     _gb->log("\nEnter a command to continue.");
//             //     while (!str.endsWith("\n") && !str.endsWith("\r")) {
//             //         c = _gb->serial.debug->readString();
//             //         str += c;
                        
//             //         if (c.length() > 0) {
//             //             if (!flag) {
//             //                 _gb->log("You have entered: ", false);
//             //                 flag = true;
//             //             }
//             //             _gb->log(c, false); c = "";
//             //         }
//             //         delay(10);
//             //     }
//             // }
//             // else str += _gb->serial.debug->readString();
//             // str.trim();
//             // this->process(str);
//         }

//         String response = "";
//         while (_gb->serial.debug->available() && !response.endsWith("$$END$$")) {
//             response += _gb->serial.debug->readString();
//             response.trim();

//             // Check if a string is coming from the GB Configurator 
//             if (response.indexOf("$$") == 0) {
                
//                 if (_state) {
//                     this->_gb->getdevice("ph").off();
//                     _state = false;
//                 }
//                 else {
//                     this->_gb->getdevice("ph").on();
//                     _state = true;
//                 }

//                 // Strip the prefix and suffix
//                 response = response.substring(response.indexOf("$$CONFIG$$") + 10, response.indexOf("$$END$$"));

//                 String key = response.substring(0, response.indexOf(":"));
//                 String value = response.substring(response.indexOf(":") + 1, response.length());

//                 this->post("##LOG##" + key);
//                 this->post("##LOG##" + value);
//             }  
//         }
//     #endif
//     return *this;
// }

/*
    ! Sned a string to GatorByte configurator or console-based GatorByte datastore 
*/
String GB_CONSOLE::post(String message) { 
    
    String prefix = "##SOF##", suffix = "##EOF##";
    this->_gb->log(prefix + message + suffix);

    // Get response from the GatorByte Configurator
    int counter = 0;
    while (!this->_gb->serial.debug->available() && counter++ < 50) delay(200);

    String response = "";
    while (_gb->serial.debug->available()) {
        response += _gb->serial.debug->readString();
        response.trim();

        // Check if a string is coming from the GB Configurator 
        if (response.indexOf("$$GBCG$$") == 0) {
            
            // if (_state) {
            //     this->_gb->getdevice("ph").off();
            //     _state = false;
            // }
            // else {
            //     this->_gb->getdevice("ph").on();
            //     _state = true;
            // }

            // Strip the prefix
            response = response.substring(response.indexOf("$$GBCG$$"), response.length());

            String key = response.substring(0, response.indexOf(":"));
            String value = response.substring(response.indexOf(":") + 1, response.length());

            this->log(key);
            this->log(value);
        }  

    }

    return response;
}

bool GB_CONSOLE::log(String message) { 
    this->post("log-message::" + message);
    return true;
}

bool GB_CONSOLE::confirm_discard_changes() { this->confirm_discard_changes(0); }
bool GB_CONSOLE::confirm_discard_changes(int timer) {
    _gb->log("\nDo you want to apply these changes?");
    _gb->log("Enter \"y\" to apply or \"n\" to discard the changes.");
    _gb->log();
    String response = this->get_string_from_console(true, timer);
    if (response == "y") {
        _gb->log("Applying changes."); return true;
    }
    else if (response == "n") {
        _gb->log("Discarding changes."); return false;
    }
    else {
        _gb->log("Invalid response."); return false; 
    }
}

bool GB_CONSOLE::confirm_action(int timer) {
    String response = this->get_string_from_console(true, timer);
    if (response == "y") return true;
    else if (response == "n") return false;
    else return false; 
}


// TODO: Fix this
String GB_CONSOLE::get_string_from_console() { return this->get_string_from_console(false, 0);}
String GB_CONSOLE::get_string_from_console(bool instantaneous) { return this->get_string_from_console(instantaneous, 0);}
String GB_CONSOLE::get_string_from_console(int timer) { return this->get_string_from_console(false, timer);}
String GB_CONSOLE::get_string_from_console(bool instantaneous, int timer) {
    String str, c;
    int then = 0;
    while (timer >= 0) {
        
        c = _gb->serial.debug->readString();
        str += c;

        if (str.endsWith("\n") || str.endsWith("\r")) {
            break;
        }
        else if (!instantaneous && c.length() > 0) {
            _gb->log(c, false);
            c = "";
        }
        else if (instantaneous && str.length() > 0) {
            break;
        }

        if (timer < 0) {
            Serial.println("Here 4");
            if (str.length() > 0) {
                _gb->log("No input provided before timer expired.");
            }
            break;
        }

        timer--;
        delay(1);
    }
    
    if (!instantaneous) _gb->log("");  
    str.trim();
    if (str == "!") {
        _gb->log("Menu requested");
        // this->process("menu");
        return "";
    }
    if (str == "!!") {
        this->exit();
        return "";
    }
    return str;
}

// Process the incoming command
void GB_CONSOLE::process(String command) {

    #if not defined (LOW_MEMORY_MODE)

        _gb->log("Received command: ", false);
        _gb->log(command);

        if (command == "##GB##files-list:/#EOF#") {
            String root = command.substring(command.indexOf(":") + 1, command.indexOf("#EOF#"));
            _gb->log("File list requested in folder: " + root);
            String list = _gb->getdevice("sd").getfilelist("/");

            _gb->log(list);

            // _gb->getdevice("gdc").send("highlight-yellow", list);
            // _gb->getdevice("gdc").send("gdc-dfl", list);
        }

        //! Quit mode
        if (command.endsWith("quit")) {
            this->exit();
        }
        
        //! Device configuration
        else if (command.startsWith("menu")) {
            _gb->log("\nMenu");
            _gb->log("-----");
            _gb->log("Select an option from the menu below.");
            _gb->log("For instance, if you want to select option 1. and suboption 3 (if available), enter '1:3' and press 'Enter'.");
            _gb->log("Options marked with '(*)' use EEPROM to store the configuration. If the device does not have an EEPROM, the changes will not be saved.");
            _gb->log("Enter '!' to return to main menu or '!!' to exit command mode.");
            _gb->log("   1. Restart microcontroller");
            _gb->log("   2. Change device identifier (*)");
            _gb->log("   3. Change survey identifier (*)");
            _gb->log("   4. Sensor calibration");
            _gb->log("   5. Modules configuration");
            _gb->log("      1. Show added devices");
            _gb->log("      2. Real-time Clock (RTC) setup");
            _gb->log("         1. Sync to compilation time");
            _gb->log("         2. Set time zone offset");
            _gb->log("         3. Report current time");
            _gb->log("      3. EEPROM setup");
            _gb->log("         1. Show mapping");
            _gb->log("         2. Erase");
            _gb->log("      4. SD storage");
            _gb->log("         1. Format");
            _gb->log("         2. Erase");
            _gb->log("      5. Bluetooth module");
            _gb->log("         1. Change name");
            _gb->log("         2. Change PIN (minimum 6 digits)");

            _gb->log("\nEnter your selection: ", false);
            String selection = this->get_string_from_console();

            if (selection.indexOf("1") == 0) {
                _gb->log("Restarting the microcontroller. Please wait..."); delay(3000);
                _gb->getmcu().reset("mcu");
            }
            else if (selection.indexOf("2") == 0) {
                _gb->log("\nThe device's name will identify this device on the web dashboard. A device can have multiple surveys associated with it.");
                _gb->log("Enter new name: ", false);
                String devicename = this->get_string_from_console();

                bool confirm = this->confirm_discard_changes();
                if (confirm) {
                    this->_gb->globals.DEVICE_TYPE = "gatorbyte";
                    _gb->getdevice("mem").write(1, _gb->globals.DEVICE_TYPE);
                    this->_gb->globals.DEVICE_NAME = devicename;
                    _gb->getdevice("mem").write(2, _gb->globals.DEVICE_NAME);
                }
            }
            else if (selection.indexOf("3") == 0) {
                _gb->log("\nThe survey's id will identify the data collected on the web dashboard. The survey will be associated with the following device: " + this->_gb->globals.DEVICE_NAME);
                _gb->log("Enter new survey identifier: ", false);
                String surveyname = this->get_string_from_console();

                bool confirm = this->confirm_discard_changes();
                if (confirm) {
                    this->_gb->globals.PROJECT_ID = surveyname;
                    _gb->getdevice("mem").write(3, _gb->globals.PROJECT_ID);
                }
            }
            else if (selection.indexOf("4") == 0) {
                _gb->log("\nThe device has entered calibration mode. Please read the documentation for more information.");
                _gb->log("Enter the device you wish to calibrate: ", false);
                String device = this->get_string_from_console();

                if (_gb->hasdevice(device)) _gb->getdevice(device).calibrate();
                else _gb->log("Device '" + device + "' not initialized. Read documentation for more information.");

            }
            else if (selection.indexOf("5") == 0) {
                
                if (selection.indexOf(".") == -1) {
                    _gb->log("You did not enter the first suboption");
                }
                else {
                    String suboption = selection.substring(selection.indexOf(".") + 1, selection.length());

                    if (suboption.indexOf("1") == 0) {
                        _gb->log(_gb->_all_included_gb_libraries);
                    }
                    else if (suboption.indexOf("2") == 0) {
                        String subsuboption = suboption.substring(suboption.indexOf(".") + 1, suboption.length());

                        if (_gb->hasdevice("rtc")) {
                            if (subsuboption.indexOf("1") == 0) {
                                _gb->getdevice("rtc").sync(__DATE__,  __TIME__);
                            }
                            else if (subsuboption.indexOf("2") == 0) {

                            }
                            else if (subsuboption.indexOf("3") == 0) {
                                _gb->log("Current date: " + _gb->getdevice("rtc").date("MMM DDth, YYYY"));
                                _gb->log("Current time: " + _gb->getdevice("rtc").time("hh:mm:ss a"));
                                _gb->log("Current timestamp: " + String(_gb->getdevice("rtc").timestamp()));
                            }
                        }
                        else _gb->log("RTC not initialized");
                    }
                    else if (suboption.indexOf("3") == 0) {
                    }
                    else if (suboption.indexOf("4") == 0) {
                        String subsuboption = suboption.substring(suboption.indexOf(".") + 1, suboption.length());

                        if (_gb->hasdevice("sd")) {
                            if (subsuboption.indexOf("1") == 0) {
                                _gb->getdevice("sd").sdformat();
                            }
                            else if (subsuboption.indexOf("2") == 0) {
                                _gb->getdevice("sd").sderase();
                            }
                        }
                        else _gb->log("SD not initialized");
                    }
                    else if (suboption.indexOf("5") == 0) {
                        String subsuboption = suboption.substring(suboption.indexOf(".") + 1, suboption.length());
                        if (_gb->hasdevice("bl")) {
                            if (subsuboption.indexOf("1") == 0) {
                                _gb->log("Enter a new name: ", false);
                                String name = this->get_string_from_console();
                                
                                bool confirm = this->confirm_discard_changes();
                                if (confirm) {
                                    this->_gb->getdevice("bl").setname(name);
                                }
                            }
                            else if (subsuboption.indexOf("2") == 0) {
                                _gb->log("Enter a new PIN: ", false);
                                String pin = this->get_string_from_console();
                                
                                bool confirm = this->confirm_discard_changes();
                                if (confirm) {
                                    this->_gb->getdevice("bl").setname(pin);
                                }
                            }
                        }
                        else _gb->log("Bluetooth module not initialized");
                    }
                }
            }
        }

        //! Calibration
        else if (command.startsWith("cal.")) {
            String device = command.substring(command.lastIndexOf(".") + 1, command.length());
            if (_gb->hasdevice(device)) _gb->getdevice(device).calibrate();
            else _gb->log("Device " + device + " not found");
        }
        
        //! Power
        else if (command.startsWith("power.")) {
            String device = command.substring(command.lastIndexOf(".") + 1, command.length());
            if (_gb->hasdevice(device)) {
                if (command.indexOf(".on.") > -1) _gb->getdevice(device).on();
                else if (command.indexOf(".off.") > -1) _gb->getdevice(device).off();
            }
            else _gb->log("Device " + device + " not found");
        }
        
        //! RTC
        else if (_gb->hasdevice("rtc") && command.startsWith("rtc.")) {
        
            if (command.indexOf(".sync") > -1) {
                _gb->getdevice("rtc").sync(__DATE__,  __TIME__);
            }
            else {
                _gb->log("Current date: " + _gb->getdevice("rtc").date("MMM DDth, YYYY"));
                _gb->log("Current time: " + _gb->getdevice("rtc").time("hh:mm:ss a"));
                _gb->log("Current timestamp: " + String(_gb->getdevice("rtc").timestamp()));
            }
        }
        
        //! SD
        else if (_gb->hasdevice("sd") && command.startsWith("sd.")) {
        
            if (command.indexOf(".format") > -1) {
                _gb->getdevice("sd").sdformat();
            }
            else if (command.indexOf(".erase") > -1) {
                _gb->getdevice("sd").sderase();
            }
            else if (command.indexOf(".test") > -1) {
                _gb->getdevice("sd").rwtest();
            }
        }

        // ! Invalid command
        else {
            _gb->log("Invalid command: " + command);
            
            //! Revert RGB color
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").revert();

            this->loop();
        }
        
        this->loop();
    #endif
}

#endif