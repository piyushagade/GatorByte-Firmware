#ifndef GB_CONFIGURATOR_h
#define GB_CONFIGURATOR_h

#ifndef GB_h
    #include "./GB_Primary.h"
#endif

class GB_CONFIGURATOR : public GB_DEVICE {
    public:
        GB_CONFIGURATOR(GB &gb);

        DEVICE device = {
            "configurator",
            "GatorByte Configurator"
        };

        GB_CONFIGURATOR& initialize();
        GB_CONFIGURATOR& loop();
        void process(String);
        GB_CONFIGURATOR& enter();
        GB_CONFIGURATOR& exit();
        
        String cfget(String message);
        void cfset(String message);
        void cfprint(String message);
        void upload(String data);

    private:
        GB *_gb; 
        GB_DEVICE *_led;
        String _prev_mode;
        bool _state = false;
};

GB_CONFIGURATOR::GB_CONFIGURATOR(GB &gb) {
    _gb = &gb;
    // _led = &_gb->getdevice("rgb");
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.configurator = this;
}

GB_CONFIGURATOR& GB_CONFIGURATOR::initialize() { 
    _gb->log("Initializing " + this->device.name + ": ", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    this->_gb->serial.debug->begin(9600);
    return *this;
}

// Enter console mode
GB_CONFIGURATOR& GB_CONFIGURATOR::enter() {
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
GB_CONFIGURATOR& GB_CONFIGURATOR::exit() {
    _gb->log("Exiting comand mode. Reverting to \"" + this->_prev_mode + "\" mode.\n");
    this->_gb->globals.MODE = this->_prev_mode;
    this->_led->revert();
    return *this;
}

// Check if there is a new char from computer/bl serial
GB_CONFIGURATOR& GB_CONFIGURATOR::loop() {

    #if not defined (LOW_MEMORY_MODE)

    #endif
    return *this;
}

/*
    ! Get a value/state from GatorByte configurator or console-based GatorByte datastore 
*/
String GB_CONFIGURATOR::cfget(String key) { 
    
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

void GB_CONFIGURATOR::cfprint(String message) { 
    String prefix = "##SOF##", suffix = "##EOF##";

    //! Send the command to GatorByte Configurator
    // this->_gb->serial.debug->println(prefix + "log-message::" + message + suffix);
    this->_gb->serial.debug->println(message);
}

// Process the incoming command
void GB_CONFIGURATOR::process(String command) {

    #if not defined (LOW_MEMORY_MODE)

    #endif
}


/*
    ! Send a string to GatorByte configurator or console-based GatorByte datastore 
*/
void GB_CONFIGURATOR::cfset(String key) { 
    
    String prefix = "##SOF##", suffix = "##EOF##";
    this->_gb->serial.debug->println(prefix + key + suffix);
}


/*
    ! Upload the data to configurator
*/
void GB_CONFIGURATOR::upload(String key) { 

    this->cfprint("Uploading data to configurator.");
    
    String prefix = "##SOF##", suffix = "##EOF##";
    this->_gb->serial.debug->println(prefix + "upload-data::" + key + suffix);
}

#endif