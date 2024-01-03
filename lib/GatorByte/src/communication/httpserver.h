/*
    This module needs to be mcu agnostic. 
    This module handles HTTP requests to the API

*/

#ifndef GB_HTTP_h
#define GB_HTTP_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_HTTP : public GB_DEVICE {
    public:
        GB_HTTP(GB &gb);
        
        // TODO: Add logic for when sd is not available
        // GB_HTTP(GB &gb, GB_MCU &mcu, GB_SD &sd);

        DEVICE device = {
            "http",
            "Server API"
        };

        String SERVER_IP = "";
        int SERVER_PORT = -1;

        GB_HTTP& configure(String IP, int port);

        // int time(String);
        String post(String, String);
        String get(String);
        String help();

    private:
        GB *_gb;
        GB_MCU *_mcu;

        void _save_data_queue(String, String);
        void _upload_data_queue();
        void _count_data_queue_items();
        bool _sync_data_queue();
        String _files[50];
        
};

GB_HTTP::GB_HTTP(GB &gb) {
    _gb = &gb;
    _mcu = &_gb->getmcu();

    _gb->includelibrary(this->device.id, this->device.name);
}

GB_HTTP& GB_HTTP::configure(String ip, int port) {
    
    _gb->includedevice(this->device.id, this->device.name);
    
    _gb->log("Configuring server: " + ip + ":" + port, false);
    this->SERVER_IP = ip;
    _mcu->SERVER_IP = ip;
    this->SERVER_PORT = port;
    _mcu->SERVER_PORT = port;

    if(ip.length() == 0) _gb->log(" -> Failed. Invalid Server IP");
    else if(port <= 0) _gb->log(" -> Failed. Invalid port");
    else _gb->log(" -> Done");

    return *this;
}

// Low level get request + response function
String GB_HTTP::get(String path) {
    
    // Call the get method specific to the mcu
    return _mcu->get(path);
}

// // Get time from the server
// int GB_HTTP::time(String type) {
//     int result = this->get("/time/" + type).toInt();
//     return result;
// }

String GB_HTTP::help() { 
    return "api.configure(\"api.ezbean-lab.com\", 80);";
}


#endif