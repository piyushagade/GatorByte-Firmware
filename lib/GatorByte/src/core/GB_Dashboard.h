#ifndef GB_DASHBOARD_h
#define GB_DASHBOARD_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_DASHBOARD : public GB_DEVICE {
    public:
        GB_DASHBOARD(GB &gb);
        
        DEVICE device = {
            "gwdb",
            "GatorByte Web Dashboard"
        };

        bool testdevice();
        String status();
        
        GB_DASHBOARD& configure(String method, String apiversion);
        GB_DASHBOARD& initialize();
        bool send(String, String);

    private:
        GB *_gb; 
};

// Constructor 
GB_DASHBOARD::GB_DASHBOARD(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name); 
    _gb->devices.gwdb = this;
    
    this->device.detected = true;
}

// Test the device
bool GB_DASHBOARD::testdevice() { 
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}

String GB_DASHBOARD::status() { 
    return "detected";
}


/*
    Configure device
*/
GB_DASHBOARD& GB_DASHBOARD::configure(String method, String apiversion) { 
    method.trim();
    method.toLowerCase();

    _gb->globals.SERVER_METHOD = method;
    _gb->globals.SERVER_API_VERSION = apiversion;
    return *this;
}

/*
    Initialize device
*/
GB_DASHBOARD& GB_DASHBOARD::initialize() { 
    _gb->init();

    if (_gb->globals.SERVER_METHOD == "mqtt") {
        _gb->globals.SERVER_URL = "mqtt.ezbean-lab.com";
        _gb->globals.SERVER_PORT = 1883;
    }

    else if (_gb->globals.SERVER_METHOD == "http") {
        _gb->globals.SERVER_URL = "api.ezbean-lab.com";
        _gb->globals.SERVER_PORT = 80;
    }
}

bool GB_DASHBOARD::send(String category, String data) { 

    if (_gb->globals.SERVER_METHOD == "http") {

        if (category == "control") {
            _gb->getmcu()->post("/" + _gb->globals.SERVER_API_VERSION + "/controls/set", "dasdsa");
        }
        else if (category == "state") {
            _gb->getmcu()->post("/" + _gb->globals.SERVER_API_VERSION + "/state/set", "dasdsa");
        }
        else if (category == "data") {
            _gb->getmcu()->post("/" + _gb->globals.SERVER_API_VERSION + "/data/set", "dasdsa");
        }
    }
}

#endif