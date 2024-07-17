#ifndef GB_AT24_h
#define GB_AT24_h

/*
    EEPROM addressing standard for GatorByte

    ----------------------------------------------------------------
        Location    +   Description
    ----------------------------------------------------------------
        0           |  Format status. If the value is "formatted", 
                    |   that indicates that the device has been
                    |   initialized.
    ----------------------------------------------------------------
        1           |  Environment
                    |   E.g. gatorbyte
    ----------------------------------------------------------------
        2           |  Device unique ID/SN. 
                    |   E.g. cV0XdX9
    ----------------------------------------------------------------
        3           |  Survey ID. 
                    |   E.g. gb-lry
    ----------------------------------------------------------------
        4           |  Bluetooth name. 
                    |  E.g. gb-swb-luna
    ----------------------------------------------------------------
        5           |  Bluetooth PIN
                    |   E.g. 5555
    ----------------------------------------------------------------
        6           |  Failure count
                    |   E.g. 3
                    |  Should reset to 0 after a successful loop() execution 
    ----------------------------------------------------------------
        7           | Last failure type
                    | E.g. Cellular
    ----------------------------------------------------------------
        8           | Iteration count
                    | E.g. 211
    ----------------------------------------------------------------
        9           | Last loop execution start timestamp
                    | E.g. 56499
    ----------------------------------------------------------------
        10          | Cumulative hard resets of SARA-R410 MODEM 
                    | E.g. 4
                    | Needs manual counter reset
    ----------------------------------------------------------------
        11          | Cumulative hard microcontroller reboots forced due to some error (that is caught) 
                    | E.g. 2
                    | Needs manual counter reset
    ----------------------------------------------------------------
        12          | Last reading timestamp 
                    | E.g. 1670873987
                    | Needs manual counter reset
    ----------------------------------------------------------------
*/

/*
    Failure ID table

    ----------------------------------------------------------------
        ID          +   Description
    ----------------------------------------------------------------
        0           |  Format status. If the value is "formatted", 
                    |   that indicates that the device has been
                    |   initialized.
    ----------------------------------------------------------------

*/


/*
    ! New schematic
    The EEPROM has 32 kBytes space.

    A key can only have 16 Bytes (16 characters); No spaces; Spaces will be replaced with a hyphen '-'.
    Ex. device-id, last-read-time

    Key - Location pairs will be stored in the first 4 kB. This means there can be (4096 / 16 =) 256 keys.
    Location - Value pairs will be stored in the next 28 kB. This means each value can be (28 * 1024 / 256 =) 112 Bytes each. 

*/

#include "Eeprom_at24c256.h"

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_AT24 : public GB_DEVICE {
    public:
        GB_AT24(GB &gb);
        
        DEVICE device = {
            "mem",
            "AT24X EEPROM"
        };

        struct PINS {
            bool mux;
            int enable;
        } pins;

        GB_AT24& initialize();
        GB_AT24& initialize(bool test);
        GB_AT24& configure(PINS);
        GB_AT24& on();
        GB_AT24& off();
        GB_AT24& persistent();
        GB_AT24& format();
        bool memtest();
        
        bool testdevice();
        String status();

        String get(int location);
        void print(int location);
        GB_AT24& write(int, char*);
        GB_AT24& write(int, String);
        GB_AT24& remove(int);
        void debug(String);

        bool hasconfig();
        GB_AT24& writeconfig(String);
        String getconfig();

        bool hascontrolvariables();
        GB_AT24& writecontrolvariables(String);
        String getcontrolvariables();

    private:
        GB *_gb;
        uint8_t _address = 0x50;
        uint8_t _chunksize = 32;
        uint8_t _chunkscount = 30;
        bool _persistent = false;

        Eeprom_at24c256 _eeprom = Eeprom_at24c256(_address);

        int _get_data_location(String key);
        GB_AT24& _set_data(String key, String value);

        uint8_t _addresses_store_start_location = 0;
        uint16_t _addresses_store_size = 4 * 1024;
        uint8_t _data_store_start_location = 0;
        int _date_store_size = 28 * 1024;
        uint8_t _max_key_size = 16;
        uint16_t _max_value_size = 112;

        uint8_t MEMLOC_BOOT_COUNTER = 41;
};

/*
    Lookup the addresses store and find the location id at which
    the data corresponding to the provided key is stored
    TODO: Complete this
*/
int GB_AT24::_get_data_location(String key) {

    return 0;
}


// Constructor 
GB_AT24::GB_AT24(GB &gb) {
    this->_gb = &gb;
    this->_gb->includelibrary(this->device.id, this->device.name); 
    this->_gb->devices.mem = this;
}

// Configure device pins
GB_AT24& GB_AT24::configure(PINS pins) {
    this->pins = pins;

    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    return *this;
}

bool GB_AT24::memtest() { 
    Serial.println("Testing: " + this->device.name);
    return true;
}

// Test the device
bool GB_AT24::testdevice() {
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_AT24::status() { 
    return this->device.detected ? String(_gb->s2c(this->get(0)) == "formatted" ? "formatted" : "not-formatted") + ":.:" + "32" : "not-detected" + String(":") + device.id;
}

// Initialize the module
GB_AT24& GB_AT24::initialize() { 
    return this->initialize(false);
}
GB_AT24& GB_AT24::initialize(bool test) { 
    _gb->init();
    this->on();
    _gb->log("Initializing " + this->device.name, false);

    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    int counter = 3;
    uint8_t error = 1;
    while (error != 0 && counter-- >= 0) { 
        Wire.begin();
        Wire.beginTransmission(_address);
        error = Wire.endTransmission();
        if (error != 0) delay(1000); 
    }

    // If no error encountered
    if (error == 0) {
        _gb->arrow().log("Done", !test);
        this->device.detected = true;

        // // Increment the boot counter
        // String bootcounter = String(this->get(this->MEMLOC_BOOT_COUNTER).toInt() + 1);
        // this->write(this->MEMLOC_BOOT_COUNTER, bootcounter);

        // Disable stray watchdog timers
        this->_gb->getmcu()->watchdog("disable");

        //! Format EEPROM's memory content (if first use)
        if(strcmp(_gb->s2c(this->get(0)), "formatted") != 0) this->format();
        
        if (_gb->globals.GDC_CONNECTED) {
            _gb->log("");
            this->off();
            return *this;
        }
        
        if (test) {
            
            //! Speed test
            unsigned long start = millis();
            String readdata = _gb->s2c(this->get(0));
            int readtime = millis() - start;
            _gb->arrow().log("Read: " + String(readtime) + " ms", false);
            start = millis();
            String writtendata = "formatted";
            this->write(0, writtendata);
            int writetime = millis() - start;
            _gb->log(", Write: " + String(writetime) + " ms", false);

            //! R/W test
            _gb->arrow().log("R/W test: ", false);
            _gb->log(readdata == writtendata ? "Passed" : "Failed");
        }


        // Get device SN from memory
        if (this->get(0) == "formatted") {
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("--").wait(250).play("...");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on("green").wait(250).revert(); 

            _gb->log("Fetching SN", false);
            String savedsn = this->get(2);
            if (savedsn.length() == 0) {
                this->write(2, _gb->globals.DEVICE_SN);
                _gb->arrow().color("yellow").log("SN written to EEPROM.");
            }
            else _gb->arrow().color("white").log(savedsn);
            
            if (savedsn.length() > 0 && savedsn != _gb->globals.DEVICE_SN) {
                this->write(2, _gb->globals.DEVICE_SN);
                _gb->color("yellow").log("New microcontroller detected. Updating the SN in EEPROM.");
                _gb->log("Old SN: " + savedsn);
                _gb->log("New SN: " + _gb->globals.DEVICE_SN);
            }
        
            // Get device environment from memory
            _gb->log("Fetching environment", false);
            String savedenv = this->get(1);
            if (savedenv.length() > 0) {
                _gb->env(savedenv);
                _gb->arrow().color("white").log(_gb->env());
            }
            else {
                this->write(1, "development");
                _gb->env("development");
                _gb->arrow().color("yellow").log("Environment written to EEPROM", false).arrow().log("development");
            }
        } 
    }
    else {
        this->device.detected = false;
        _gb->globals.INIT_REPORT += this->device.id;
        _gb->arrow().log("Not detected", true);
        
        if (_gb->globals.GDC_CONNECTED) {
            this->off();
            return *this;
        }
        
        if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer")->play("--").wait(250).play("---");
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on("red").wait(250).revert();
    }

    this->off();
    return *this;
}

// Format the contents in the EEPROM
GB_AT24& GB_AT24::format() { 
    this->on();
    if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on("yellow");

    // // Start watchdog timer
    // this->_gb->getmcu()->watchdog("enable");

    Wire.begin();
    _gb->br().log("Formatting EEPROM module", false);

    if (!this->device.detected) {
        _gb->arrow().log("Skipped", true);
        this->off();
        return *this;
    }

    // Insert 'formatted' flag at location 0
    this->write(0, _gb->s2c("formatted"));

    // Insert 'env' information
    this->write(1, _gb->s2c("development"));

    for(int i = 1; i < this->_chunkscount; i++) {
        this->write(i, _gb->s2c(""));
        delay(2);
    }
    
    // Ensure the EEPROM has been formatted and also working properly
    if (strcmp(_gb->s2c(this->get(0)), "formatted") == 0) _gb->arrow().log("Done with verification", true);
    else _gb->arrow().log("Failed", true);
    
    // // Disable watchdog timer
    // this->_gb->getmcu()->watchdog("disable");
    
    if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->revert();

    this->off();
    return *this;
}

// Turn on the module
GB_AT24&  GB_AT24::on() { 
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    delay(1000);
    return *this;
}

// Turn off the module
GB_AT24&  GB_AT24::off() { 

    if (this->_persistent) return *this;
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    return *this;
}

GB_AT24&  GB_AT24::persistent() {
    
    this->_persistent = true;
    return *this;
}

// Read a location by id on EEPROM module
String GB_AT24::get(int id){
    this->on();

    // Start watchdog timer
    this->_gb->getmcu()->watchdog("enable");

    String data = "";
    char message[this->_chunksize];
    _eeprom.read(id * this->_chunksize, (char *) message, sizeof(message));
    for (int z = 0; z < this->_chunksize; z++) {
        delay(10);
        data += String(message[z]);
    }
    
    // Disable watchdog timer
    this->_gb->getmcu()->watchdog("disable");
    
    this->off();
    return data;
}

// Print the value at id
void GB_AT24::print(int id){
    this->_gb->log("Value at " + String(id) + " on EEPROM: " + this->get(id));
}

// Write to an EEPROM location by id
GB_AT24& GB_AT24::write(int id, char* data) { return this->write(id, _gb->ca2s(data)); }
GB_AT24& GB_AT24::write(int id, String data){
    this->on(); 

    // Start watchdog timer
    this->_gb->getmcu()->watchdog("enable");

    String data_to_write = data;

    // _gb->log("Writing location: " + String(id));
    // _gb->log("Writing: " + data);

    // Reserve memory
    char writebuffer[this->_chunksize];

    // Convert string to character array
    for (int x = 0; x < this->_chunksize; x++) writebuffer[x] = data_to_write[x];

    // Write to EEPROM location
    _eeprom.write(id * this->_chunksize, (char *)writebuffer, sizeof(writebuffer));
    
    // Disable watchdog timer
    this->_gb->getmcu()->watchdog("disable");

    this->off();
    return *this;
}

/*
    Write config to EEPROM
*/
GB_AT24& GB_AT24::writeconfig(String data){
    this->on(); 

    // // Start watchdog timer
    // this->_gb->getmcu()->watchdog("enable");

    
    _gb->color("yellow").log("Writing config to EEPROM");

    String data_to_write = data;
    uint16_t configflaglocationid = 290;
    uint16_t configlocationid = 300;
    uint8_t pagesize = 4;
    String configsize = String(512);
    char configflag[4] = "1";

    // // Write config size to EEPROM
    // for (uint16_t y = 0; y < sizeof(String); y++) {
        _eeprom.write(configflaglocationid, (char *) configflag, sizeof(configflag));
    // }

    int counter = 0;
    for (uint16_t y = 0; y < (configsize).toInt() / pagesize; y++) {

        // Reserve memory
        char writebuffer[pagesize];
        String chunk = "";

        // Convert string to character array
        for (int x = 0; x < pagesize; x++) {
            counter++;
            writebuffer[x] = data_to_write[(y - 1) * pagesize + x];
            // chunk += data_to_write[(y - 1) * pagesize + x];
        }
        
        // Serial.print("\n" + String(configlocationid + y * pagesize) + ": " + String(writebuffer));

        // Write to EEPROM location
        _eeprom.write(configlocationid + y * pagesize, _gb->s2c(String(writebuffer)), sizeof(writebuffer));
        // _eeprom.write(configlocationid + y * pagesize, _gb->s2c(chunk), sizeof(writebuffer));
        delay(10);
    }

    // Disable watchdog timer
    this->_gb->getmcu()->watchdog("disable");
    _gb->color("yellow").log("Done (" + String(counter) +")");

    this->off();
    return *this;
}

// Check if config exists on EEPROM
bool GB_AT24::hasconfig() {
    this->on();

    // Start watchdog timer
    this->_gb->getmcu()->watchdog("enable");

    // Read config size
    uint16_t configflaglocationid = 290;
    char sizeflag[1];
    _eeprom.read(configflaglocationid, (char *) sizeflag, sizeof(sizeflag));

    // Disable watchdog timer
    this->_gb->getmcu()->watchdog("disable");
    this->off();

    return sizeflag[0] == '1';
}

// Read config
String GB_AT24::getconfig(){
    this->on();
    
    _gb->log("Reading config from EEPROM", false);

    // Start watchdog timer
    this->_gb->getmcu()->watchdog("enable");

    // Read config size
    uint16_t configflaglocationid = 290;
    uint8_t pagesize = 4;
    char sizeflag[1];
    _eeprom.read(configflaglocationid, (char *) sizeflag, sizeof(sizeflag));

    uint16_t configlocationid = 300;
    uint16_t configsize = 512;

    String data = "";

    for (uint16_t y = 0; y < configsize / pagesize; y++) {
        char readbuffer[pagesize];
        _eeprom.read(configlocationid + y * pagesize, (char *) readbuffer, sizeof(readbuffer));
        delay(5);
        
        // _gb->log("\n" + String(configlocationid + y * pagesize) + ": " + readbuffer, false);
        
        for (int z = 0; z < pagesize; z++) {
            data += String(readbuffer[z]);
        }
    }

    _gb->arrow().log("Done");

    // Disable watchdog timer
    this->_gb->getmcu()->watchdog("disable");
    this->off();

    return data;
}

/*
    Write control variables to EEPROM
*/
GB_AT24& GB_AT24::writecontrolvariables(String data){
    this->on(); 

    // // Start watchdog timer
    // this->_gb->getmcu()->watchdog("enable");

    
    _gb->color("yellow").log("Writing control variables to EEPROM");

    String data_to_write = data;
    uint16_t datalocationid = 5000;
    uint16_t flaglocationid = datalocationid - 1;
    uint8_t pagesize = 4;
    uint16_t datasize = 512;
    char dataflag[4] = "1";

    // Write flag to EEPROM
    _eeprom.write(flaglocationid, (char *) dataflag, sizeof(dataflag));

    // Write data to EEPROM
    for (uint16_t y = 0; y < datasize / pagesize; y++) {

        // Reserve memory
        char writebuffer[pagesize];

        // Convert string to character array
        for (int x = 0; x < pagesize; x++) {
            writebuffer[x] = data_to_write[(y - 1) * pagesize + x];
        }
        
        // Serial.print("\n" + String(configlocationid + y * pagesize) + ": " + String(writebuffer));

        // Write to EEPROM location
        _eeprom.write(datalocationid + y * pagesize, _gb->s2c(writebuffer), sizeof(writebuffer));
        // _eeprom.write(configlocationid + y * pagesize, " f(writebuffer));
        delay(5);
    }

    // Disable watchdog timer
    this->_gb->getmcu()->watchdog("disable");

    this->off();
    return *this;
}


// Read control variables
String GB_AT24::getcontrolvariables(){
    this->on();
    
    _gb->log("Reading control variables from EEPROM", false);

    // Start watchdog timer
    // this->_gb->getmcu()->watchdog("enable");

    
    // Read config size
    uint8_t pagesize = 4;
    uint16_t datalocationid = 5000;
    uint16_t datasize = 512;

    String data = "";

    Serial.println("\nPages: " + String(datasize / pagesize));
    for (uint16_t y = 0; y < datasize / pagesize; y++) {
    // for (uint16_t y = 0; y < 1; y++) {

        
        for (int z = 0; z < pagesize; z++) {
            char readbuffer[pagesize];
            _eeprom.read(datalocationid + y * pagesize + z, (char *) readbuffer, 1);
            _gb->log(String(datalocationid + y * pagesize + z) + ": " + String(readbuffer));
            delay(15);
            data += String(readbuffer[z]);
        }
    }

    _gb->arrow().log("Done");

    // Disable watchdog timer
    this->_gb->getmcu()->watchdog("disable");
    this->off();

    _gb->log(data);

    return data;
}

// Delete a location on EEPROM by id
GB_AT24&  GB_AT24::remove(int id){
    this->on();
    
    // Start watchdog timer
    this->_gb->getmcu()->watchdog("enable");

    String data_to_write = "";
    
    char writebuffer[16];
    
    // Convert string to character array
    for (int x = 0; x < 16; x++) writebuffer[x] = data_to_write[x];
    
    // Write blank string to EEPROM
    _eeprom.write(id * 16, (char *)writebuffer, sizeof(writebuffer));
    delay(10);
    
    // Disable watchdog timer
    this->_gb->getmcu()->watchdog("disable");

    this->off();
    return *this;
}

// Log a message to a file
void GB_AT24::debug(String message) {
    // _gb->log("Writing to EEPROM: " + message, false);
    this->on();
    
    // // Write previous state to EEPROM
    // String prevstring = this->get(9);
    // delay(250);
    // this->write(10, prevstring);

    // // Write to current state to EEPROM
    // this->write(9, message);

    // delay(50);
    this->off();

    // _gb->arrow().log("Done");
}

#endif