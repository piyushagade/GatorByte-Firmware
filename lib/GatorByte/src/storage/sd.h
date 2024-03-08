#ifndef GB_SD_h
#define GB_SD_h

#ifndef GB_h
    #include "../GB.h"
#endif

#ifndef CSVary_h
    #include "CSVary.h"
#endif

#ifndef GB_RGB_h
    #include "../misc/rgb.h"
#endif

#ifndef SdFat_h
    #include "SdFat.h"
    #include "sdios.h"
#endif

#define O_RDONLY 0

class GB_SD : public GB_DEVICE {
    public:
        GB_SD(GB &gb);

        DEVICE device = {
            "sd",
            "Level-shifting uSD card",
            false
        };
        
        struct PINS {
            bool mux;
            int cd;
            int ss;
            int enable;
        } pins;

        String sn;

        GB_SD& configure(PINS);
        GB_SD& initialize();
        GB_SD& initialize(String speed);
        bool initialized();
        GB_SD& on();
        GB_SD& off();
        
        bool testdevice();
        String status();
        
        // SD operations
        void ls();
        bool sderase();
        bool gbformat();
        bool sdformat();
        bool rwtest();
        bool detected();

        // Set state
        GB_SD& state(String, bool);
        GB_SD& state(String, int);
        GB_SD& state(String, float);
        GB_SD& state(String, String);

        // Count funtions
        // int count(String type, String root);
        int count(String contains, String folder);
        int count(String contains);
        String getfilelist(String root);

        // Queuing functions
        int getqueuecount();
        bool isqueueempty();
        String getfirstqueuefilename();
        String getlastqueuefilename();
        String getavailablequeuefilename();
        void removequeuefile(String);
        String readqueuefile(String file_name);
        void writequeuefile(String filename, String data);
        void writequeuefile(String filename, CSVary csv);

        // Write functions
        void writefile(String filename, String data);
        void writeString(String filename, String data);
        void writeCSV(CSVary csv);
        void writeCSV(String filename, CSVary csv);
        void writeCSV(String filename, String data, String header);
        void writeJSON(String filename, String data);

        // // Write messages for debugging
        // bool debug(String action, String category);
        // bool debug(String action, String category, String message);

        // File operations
        bool rmdir(String foldername);
        bool rm(String filename);
        File openFile(String reason, String file_name);
        void closeFile(File file_name);
        String getfirstfilenamecontaining(String contains);
        String getfirstfilenamecontaining(String contains, String folder);
        String getlastfilenamecontaining(String contains);
        String getlastfilenamecontaining(String contains, String folder);
        bool exists(String path);
        bool mkdir(String path);
        bool renamedir(String originalfolder, String newfolder);
        String download(String file_name);
        String readLinesFromSD(String file_name, int lines_at_a_time, int starting_line);
        String writeLinesToSD(String file_name, String data);
        String readfile(String file_name);

        
        GB_SD& readconfig();
        String readconfig(String type);
        String readconfig(String file, String type);
        void updateconfig(String type, String data);
        void updateconfig(String file, String type, String data);

        typedef void (*callback_t_on_control)(JSONary data);
        GB_SD& readcontrol();
        GB_SD& readcontrol(callback_t_on_control callback);
        GB_SD& updateallcontrol(String keyvalue, callback_t_on_control callback);
        GB_SD& updatesinglecontrol(String keyvalue, callback_t_on_control callback);

        GB_SD& updatecontrolstring(String key, String value);
        GB_SD& updatecontrolstring(String key, String value, callback_t_on_control callback);
        GB_SD& updatecontrolint(String key, int value);
        GB_SD& updatecontrolint(String key, int value, callback_t_on_control callback);
        GB_SD& updatecontrolbool(String key, bool value);
        GB_SD& updatecontrolbool(String key, bool value, callback_t_on_control callback);
        GB_SD& updatecontrolfloat(String key, double value, callback_t_on_control callback);
        GB_SD& updatecontrolfloat(String key, double value);

    private:
        bool _use_mux = false;
        String _split_string(String data, char separator, int index);
        void set_file_date_time_callback(uint16_t* date, uint16_t* time);

        SdFat _sd;
        cid_t _cid;
        File _root;
        bool _sd_card_present = false;
        GB *_gb;
        bool _rebegin_on_restart = false;
        int _run_counter = 0;
        int _sck_speed = SPI_HALF_SPEED;
        bool _initialized = false;
        bool SKIP_CHIP_DETECT = false;
        callback_t_on_control _on_control_update;

        // SD Formatter
        uint32_t _cardSectorCount = 0;
        uint8_t  _sectorBuffer[512];
        SdCardFactory _cardFactory;
        SdCard* m_card = nullptr;

        bool _write(String filename, String data);

};

GB_SD::GB_SD(GB &gb) {
    this->_gb = &gb;
    this->_gb->includelibrary(this->device.id, this->device.name);     
    this->_gb->devices.sd = this;
}

/*
    ! Create the GB's SD directories and files
    TODO: Finish this
*/
bool GB_SD::gbformat() { 

    //! Create folders
    _gb->log("Creating folders in the SD storage");
    String folders[] = {"readings", "queue", "config", "calibration", "logs", "control", "debug"};
    
    // Check and create folders if needed
    for (int i = 0; i < sizeof(folders)/sizeof(folders[0]); i++) {
        String folder = folders[i].substring(0, folders[i].indexOf("."));
        String subfolder = folders[i].substring(folders[i].indexOf(".") + 1, folders[i].length());
        
        String path = "/" + folder;
    
        // Check if debug folder needs to be created
        _gb->space(4).log("Creating directory: " + path, false);
        _gb->log(!exists(path) ? (mkdir(folder) ? " -> Done" : " -> Failed") : " -> Already exists");
    }

    //! Create config file
    if (!exists("/config/config.ini")) {
        _gb->space(4).log("Creating file: /config/config.ini", false);

        String configdata = "";
        if (!exists("/config/config.ini")) {
            this->writeString("/config/config.ini", configdata);
            _gb->log(" -> Done. Please complete the configuration using the desktop client.");
        }
        else {
            _gb->log(" -> Already exists");
        }
    }

    return true;
}

bool GB_SD::sdformat() { 
    this->on();
    bool success = true;

    _gb->log("Formatting: " + this->device.name);
    SdSpiConfig SD_CONFIG = SdSpiConfig(this->pins.ss, DEDICATED_SPI);

    // Get card object
    m_card = _cardFactory.newCard(SD_CONFIG);
    if (!m_card || m_card->errorCode()) {
        this->_gb->log(" -> Couldn't initialize card.");
        success = false;
    }

    // Get sector count
    this->_cardSectorCount = m_card->sectorCount();
    if (!this->_cardSectorCount) {
        this->_gb->log(" -> Couldn't get sector count.");
        success = false;
    }

    this->_gb->log(("Card size: ") + String(this->_cardSectorCount * 5.12e-7) + " GB");
    this->_gb->log(("Card will be formated to: "), false);
    if (this->_cardSectorCount > 67108864) this->_gb->log(("exFAT"));
    else if (this->_cardSectorCount > 4194304) this->_gb->log(("FAT32"));
    else this->_gb->log(("FAT16"));

    ExFatFormatter exFatFormatter;
    FatFormatter fatFormatter;
    success = 
        this->_cardSectorCount > 67108864 ?
        exFatFormatter.format(m_card, this->_sectorBuffer, &Serial) :
        fatFormatter.format(m_card, this->_sectorBuffer, &Serial);

    if (success) this->_gb->log(("The operation was successful."));
    else this->_gb->log(("Formatting was not successful."));

    this->off();
    return success;
}

bool GB_SD::sderase() { 
    this->on();
    bool success = true;

    _gb->log("Erase: " + this->device.name);
    SdSpiConfig SD_CONFIG = SdSpiConfig(this->pins.ss, DEDICATED_SPI);

    // Get card object
    m_card = _cardFactory.newCard(SD_CONFIG);
    if (!m_card || m_card->errorCode()) {
        this->_gb->log(" -> Couldn't initialize card.");
        success = false;
    }

    // Get sector count
    this->_cardSectorCount = m_card->sectorCount();
    if (!this->_cardSectorCount) {
        this->_gb->log(" -> Couldn't get sector count.");
        success = false;
    }

    this->_gb->log(("Card size: ") + String(this->_cardSectorCount * 5.12e-7) + " GB");

    uint32_t const ERASE_SIZE = 262144L;
    uint32_t firstBlock = 0;
    uint32_t lastBlock;
    uint16_t n = 0;

    do {
        lastBlock = firstBlock + ERASE_SIZE - 1;
        if (lastBlock >= this->_cardSectorCount) {
            lastBlock = this->_cardSectorCount - 1;
        }
        if (!m_card->erase(firstBlock, lastBlock)) success = false;
        if ((n++)%64 == 63) {}
        firstBlock += ERASE_SIZE;
    } while (firstBlock < this->_cardSectorCount);

    if (!m_card->readSector(0, this->_sectorBuffer)) _gb->log(("Read block"));
        
    this->_gb->log("All data set to " + String(this->_sectorBuffer[0]));

    if (success) this->_gb->log(("The operation was successful."));
    else this->_gb->log(("Erasing was not successful."));

    this->off();
    return success;
}

GB_SD& GB_SD::configure(PINS pins) {
    this->pins = pins;

    // Set pin modes of the device
    if(!this->pins.mux) {
        digitalWrite(this->pins.enable, OUTPUT);
        digitalWrite(this->pins.cd, INPUT);
    }

    return *this;
}

GB_SD& GB_SD::on() {

    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);

    // Initialize the connection
    if (this->_rebegin_on_restart) {
        _sd.begin(this->pins.ss, this->_sck_speed);
    }

    return *this;
}

GB_SD& GB_SD::off() {
    
    // Safety delay
    delay(250);

    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    this->_rebegin_on_restart = true;

    return *this;
}

GB_SD& GB_SD::state(String name, bool value) {
    if(name.compareTo("SKIP_CHIP_DETECT") == 0) this->SKIP_CHIP_DETECT = value;
    return *this;
}

// Detect SD card and set varialbe
bool GB_SD::detected() {
    if(this->SKIP_CHIP_DETECT) this->_sd_card_present = true;
    else {
        if(this->pins.mux) this->_sd_card_present = _gb->getdevice("ioe").readpin(this->pins.cd);
        else this->_sd_card_present = digitalRead(this->pins.cd);
        return this->_sd_card_present;
    }
}

// Test the device
bool GB_SD::testdevice() { 
    if (!this->device.detected) this->device.detected = _sd.begin(this->pins.ss, this->_sck_speed) && this->rwtest();
    return this->device.detected;
}
String GB_SD::status() {
    String size = "-";
    
    if (this->device.detected) {
        SdSpiConfig SD_CONFIG = SdSpiConfig(this->pins.ss, DEDICATED_SPI);
        m_card = _cardFactory.newCard(SD_CONFIG);
        size = String(m_card->sectorCount() * 5.12e-7) + " GB";
    }
    return this->device.detected ? size : (String("not-detected") + String(":") + device.id);
}

// Initialize SD card
GB_SD& GB_SD::initialize() { this->initialize("quarter"); }
GB_SD& GB_SD::initialize(String speed) { 
    _gb->init();
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);
    
    this->on();
    
    /*
        ! Set SPI speed
    */
    // For AVR
    if (speed.indexOf("eight") > -1) this->_sck_speed = SPI_EIGHTH_SPEED;  
    else if (speed.indexOf("quarter") > -1) this->_sck_speed = SPI_QUARTER_SPEED; 
    else if (speed.indexOf("half") > -1) this->_sck_speed = SPI_HALF_SPEED; 
    else if (speed.indexOf("full") > -1) this->_sck_speed = SPI_FULL_SPEED;

    // For Due
    else if (speed.indexOf("third") > -1) this->_sck_speed = SPI_DIV3_SPEED;
    else if (speed.indexOf("sixth") > -1) this->_sck_speed = SPI_DIV6_SPEED;

    // Default speed value
    else this->_sck_speed = SPI_HALF_SPEED;

    _gb->log("Initializing SD card module", false);
    if(_gb->globals.WRITE_DATA_TO_SD){

        if(!detected()){
            _gb->log(" -> SD card absent", true);
            _sd_card_present = false;
            this->device.detected = false;

            if (_gb->globals.GDC_CONNECTED) {
                Serial.println("##CL-GB-SD-ABS##");
                this->off();
                return *this;
            }

            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("-").play("---");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red").wait(100).on("red").wait(100).revert();

        }
        else if(!_sd.begin(this->pins.ss, this->_sck_speed)) {
            _gb->log(" -> Not detected", true);
            this->device.detected = false;

            if (_gb->globals.GDC_CONNECTED) {
                Serial.println("##CL-GB-SD-UINT##");
                this->off();
                return *this;
            }

            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("-").wait(500).play("---");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red").wait(100).on("red").wait(100).revert();
        }
        else {
            // Open root folder
            _root.open("/");
            _sd_card_present = true;

            // Perform R/W test
            bool result = this->rwtest();
            _gb->log(result ? " -> Done" : "-> R/W test failed", true);
            this->_initialized = result;
            this->device.detected = result;
            
            // Read SD serial number
            _sd.card()->readCID(&this->_cid);
            this->sn = String(this->_cid.psn);
            
            if (_gb->globals.GDC_CONNECTED) {
                if (this->device.detected) Serial.println("##CL-GB-SD-READY##");
                else  Serial.println("##CL-GB-SD-RWF##");
                this->off();
                return *this;
            }
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("-").wait(500).play("...");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("blue").wait(100).revert();
        }
    }
    else _gb->log(" -> Disabled", true);
    this->off();
    return *this;
}

// Open file for operation
File GB_SD::openFile(String reason, String file_name) {
    this->on();
    if(!this->SKIP_CHIP_DETECT && digitalRead(this->pins.cd) == LOW) _sd_card_present = false;
    else if(!this->SKIP_CHIP_DETECT && digitalRead(this->pins.cd) == HIGH) _sd_card_present = true;
    else if (this->SKIP_CHIP_DETECT) _sd_card_present = true;

    File file;
    if(strcmp(_gb->s2c(reason), "write") == 0) {
        if (!_sd_card_present) _gb->log(" -> SD card absent", false);
        else if(!file.open(_gb->s2c(file_name), FILE_WRITE)) {
            // File couldn't be opened
            // _gb->log("Error opening file");
        }
        else {
            // File opened successfully
            // _gb->log("Done opening file");
        }
    }
    else if(strcmp(_gb->s2c(reason), "read") == 0) {
        if (!_sd_card_present)
            _gb->log(" -> SD card absent", false);
        else if(!file.open(_gb->s2c(file_name), FILE_READ)) {
            // File couldn't be opened
            // _gb->log("Error opening file");
        }
        else {
            // File opened successfully
            // _gb->log("Done opening file");
        }
    }
    return file;
}

// Close a file 
void GB_SD::closeFile(File file){
    file.close();
}

// LS
void GB_SD::ls(){
    FsFile file;
    while (file.openNext(&_root, O_RDONLY)) {
        char name[25];
        file.getName(name, 25);

        _gb->log(name, false);
        
        // Indicate a directory.
        if (file.isDir()) Serial.write('/');
        _gb->log();
        
        file.close();
    }
    if (_root.getError()) {
        _gb->log("openNext failed");
    } else {
        _gb->log("Done!");
    }
}

// Returns the first file found that contains the specified string (returns filename)
String GB_SD::getfirstfilenamecontaining(String contains){
    return this->getfirstfilenamecontaining(contains, "/");
}

String GB_SD::getfirstfilenamecontaining(String contains, String folder){
    if (!this->device.detected) return "";

    this->on();

    int count = 0;
    FsFile file;
    
    // Open root folder
    File root;
    root.open(_gb->s2c(folder));

    // Lookup in root folder
    while (file.openNext(&root, O_RDONLY)) {
        char name[25];
        file.getName(name, 25);

        if (!file.isDir()) {
            if (_gb->ca2s(name).indexOf(contains) != -1) {
                return name;
            }
            file.close();
        }
    }
    
    this->off();
    return "";
}

// Returns the last file found that contains the specified string (returns filename)
String GB_SD::getlastfilenamecontaining(String contains){
    return this->getlastfilenamecontaining(contains, "/");
}
String GB_SD::getlastfilenamecontaining(String contains, String folder){
    if (!this->device.detected) return "";

    this->on();

    int count = 0;
    FsFile file;
    
    // Open root folder
    File root;
    root.open(_gb->s2c(folder));

    // Lookup in root folder
    String filename = "";
    while (file.openNext(&root, O_RDONLY)) {
        char name[25];
        file.getName(name, 25);

        if (!file.isDir()) {
            if (_gb->ca2s(name).indexOf(contains) != -1) {
                filename = _gb->ca2s(name);
            }
            file.close();
        }
    }

    this->off();
    return filename;
}

// Write a string to a file (i.e. a CSV without a header)
void GB_SD::writefile(String filename, String data) {
    this->writeString(filename, data);
}
void GB_SD::writeString(String filename, String data) {
    this->writeCSV(filename, data, "");
}

// // Log a message to a file
// bool GB_SD::debug(String action, String category) {
//     return this->debug(action, category, "");
// }

// // Log a message to a file
// bool GB_SD::debug(String action, String category, String message) {
//     // _gb->logd(" sdb1", false);

//     if (!this->device.detected) return false;
//     String filename = "/debug/" + category + ".dbg";
//     bool success = false;
//     // _gb->logd(" sdb2", false);
    
//     this->on();
//     if (action == "write") {
//         // _gb->logd(" sdb3.wr", false);
//         File file = this->openFile("write", filename);
//         if(file) {
//             file.print(message.length() > 0 ? message + "\n" : "");
//             file.close();
//             delay(15);
//             success = true;
//             // _gb->logd(" sdb3.wr.1", false);
//         }
//         else {
//             success = false;
//             // _gb->logd(" sdb3.wr.2", false);
//         }
//     }
//     else if (action == "exists") {
//         // _gb->logd(" sdb3.exst", false);
//         success = this->exists(filename);
//     }
//     else if (action == "remove") {
//         // _gb->logd(" sdb3.del", false);
//         if (this->exists(filename)) {
//             success = this->rm(filename);
//             // _gb->logd(" sdb3.del.1", false);
//         }
//         else {
//             success = true;
//             // _gb->logd(" sdb3.del.2", false);
//         }
//     }
//     // _gb->logd(" sdb4", false);
//     delay(50);
//     this->off();
//     return success;
// }

bool GB_SD::initialized() { 
    return this->_initialized;
}

// Write to a CSV file
void GB_SD::writeCSV(CSVary csv) {
    String filename = "/readings/" + (_gb->globals.DEVICE_NAME.length() > 0 ? _gb->globals.DEVICE_NAME + "_" : "") + "readings.csv";
    this->writeCSV(filename, csv); 
}
void GB_SD::writeCSV(String filename, CSVary csv) { 
    this->writeCSV(filename, csv.getrows(), csv.getheader()); 
    }
void GB_SD::writeCSV(String filename, String data, String header) {
    
    if (!this->device.detected) return;
    if(!_gb->globals.WRITE_DATA_TO_SD) return;
    
    // Enable watchdog
    _gb->getmcu().watchdog("enable");
    
    this->on();
    this->_run_counter++;

    int start = millis();

    // // Write to a different file in dummy mode
    // if (this->_gb->globals.MODE == "dummy") filename = "dummy-" + filename;
    
    _gb->log("Writing data to: " + filename, false);
    bool erroroccured = false && !this->rwtest();

    // If error occured during the R/W test
    if (erroroccured) {
        _gb->log(" -> Skipped due to error in R/W test");
        if (this->_gb->hasdevice("rgb")) this->_gb->getdevice("rgb").on("red");
    }
    else {
        _gb->log(" -> File" + String(this->exists(filename) ? " " : " not ") + "found", false);

        // Check if file doesn't exists, write header
        if(header.length() > 0 && !this->exists(filename)) {
            File new_file = this->openFile("write", _gb->s2c(filename));
            if(new_file) {
                new_file.print(String(header));
                this->closeFile(new_file);
                delay(15);
            }
        }

        File file = this->openFile("write", filename);
        if(file) {

            file.print((header.length() > 0 ? "\n" : "") + data);
            
            // // Force failure
            // if (this->_run_counter % 5 == 0) this->off();

            file.close();

            delay(15);

            int end = millis();
            _gb->log(" -> Write complete -> " + String((end - start)) + " milliseconds");
        }
        else{
            _gb->log(" -> Write failed (Couldn't open file)");
            if (this->_gb->hasdevice("rgb")) this->_gb->getdevice("rgb").on("red");
            // this->_gb->getmcu().reset("mcu");
        }
    }

    this->off();
    
    // Disable watchdog
    _gb->getmcu().watchdog("disable");
}

// Write to a JSON file
void GB_SD::writeJSON(String filename, String data) {
    if (!this->device.detected) return;
    if(!_gb->globals.WRITE_DATA_TO_SD) return;

    this->on();
    
    _gb->log("Writing data to: " + filename, false);
    bool erroroccured = false && !this->rwtest();

    // If error occured during the R/W test
    if (erroroccured) {
        _gb->log(" -> Skipped due to error in R/W test");
    }
    else {

        // Check if file doesn't exists, write an empty array
        if(!exists(filename)) {
            File new_file = this->openFile("write", filename);
            if(new_file) {
                // new_file.print("");
                this->closeFile(new_file);
            }
        }

        File file = this->openFile("write", filename);
        if(file) {
            file.print(data);
            this->closeFile(file);
            
            _gb->log(" -> Write complete");
        }
        else{
            _gb->log(" -> Write failed");
        }
    }
    this->off();
}

// Read content from SD card
String GB_SD::readfile(String filename) {
    
    // Enable watchdog
    _gb->getmcu().watchdog("enable");

    this->on();

    if(this->exists(filename)){
        File file = this->openFile("read", filename);
        delay(100);
        if(file){
            char char_code;
            String result = "";
            while (file.available()) {
                char_code = file.read();
                if(char_code == 44) result += ",";
                else result += String(char_code);
            }
            this->closeFile(file);
            
            // Disable watchdog
            _gb->getmcu().watchdog("disable");
            
            this->off();
            return result;
        }
        else{
            this->off();
            
            // Disable watchdog
            _gb->getmcu().watchdog("disable");
    
            return "";
        }
    }
    else {
        this->off();
            
        // Disable watchdog
        _gb->getmcu().watchdog("disable");

        return "";
    }
}

bool GB_SD::_write(String filename, String data) {
    
    this->on();
    File file = this->openFile("write", filename);
    if(file) {
        file.print(data);
        file.close();
        delay(15);
        return true;
    }
    else{
        if (this->_gb->hasdevice("rgb")) this->_gb->getdevice("rgb").on("red");
        return false;
    }
    this->off();
}

/*
    Read config file and set device and survey variables
*/
GB_SD& GB_SD::readconfig() {

    _gb->log("Reading config file from SD", false);

    if (!this->_initialized || !this->device.detected) {
        _gb->log(" -> Skipped");
        
        _gb->getdevice("sntl").disable();
        _gb->br().color("red").log("SD card not found. Configuration couldn't be read. Suspending operation until issue is fixed.").br();

        uint8_t now = millis();
        while (!_sd.begin(this->pins.ss, this->_sck_speed)) {
            _gb->getdevice("gdc").detect(true);
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on(((millis() - now) / 1000) % 2 ? "red" : "blue");
            
            this->off();delay(1000);
            this->on(); delay(50);
        }
        if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("....");
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").off();
        
        // Reinitialize
        this->initialize();
        return *this;
    }

    // // Enable watchdog
    // _gb->getmcu().watchdog("enable");

    this->on();

    // Create config folder if not exists
    if (!this->exists("/config")) this->mkdir("/config");

    String filename = "/config/config.ini";
    
    // If requested config file exists
    if(!this->exists(filename)){
        _gb->arrow().log("Config file not found.");
        
        _gb->getdevice("sntl").disable();
        _gb->br().color("red").log("Configuration absent on SD card. Suspending operation.");
        _gb->color("white").log("Please use the GatorByte Desktop Client to configure this device.");
        uint8_t now = millis();
        while (!_gb->globals.GDC_CONNECTED) {
            _gb->getdevice("gdc").detect(true);
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on(((millis() - now) / 1000) % 2 ? "red" : "green");
            delay(500);
        }
    }
    
    File file = this->openFile("read", filename);
    delay(100);

    // If the file opened successfully
    if(file){
        char char_code;
        String line = "";
        String category;
        while (file.available()) {
            char_code = file.read();
            delay(5);

            if(char_code == '\n' || !file.available()) {
                if (!file.available()) line += String(char_code);
                
                // If the current line is level 1 header
                if (line.indexOf(" ") == -1) {
                    line.trim();
                    if (line.indexOf("data") == 0) category = line;
                    else if (line.indexOf("device") == 0) category = line;
                    else if (line.indexOf("survey") == 0) category = line;
                    else if (line.indexOf("sleep") == 0) category = line;
                    else if (line.indexOf("server") == 0) category = line;
                    else if (line.indexOf("sim") == 0) category = line;
                    else category = "";
                    line = "";
                }
                else {
                    if (
                        category.indexOf("data") == 0 || 
                        category.indexOf("device") == 0 || 
                        category.indexOf("server") == 0 || 
                        category.indexOf("sim") == 0 || 
                        category.indexOf("survey") == 0 || 
                        category.indexOf("sleep") == 0
                    ) {
                        string key = line.substring(0, line.indexOf(":")); key.trim(); key.toLowerCase();
                        string value = line.substring(line.indexOf(":") + 1, line.length()); value.trim();

                        // _gb->log(category + ":" + line);

                        // Remove trailing '\n'
                        if (key.indexOf("\n") >= 0) key.substring(0, key.length() - 1);
                        if (value.indexOf("\n") >= 0) value.substring(0, value.length() - 1);

                        if (category == "survey") {
                            if (key == "mode") _gb->globals.SURVEY_MODE = value;
                            if (key == "id") _gb->globals.PROJECT_ID = value;
                            if (key == "tz") _gb->globals.TIMEZONE = value;
                            if (key == "realtime") {}
                        }

                        if (category == "data") {
                            if (key == "mode") _gb->globals.MODE = value;
                            if (key == "readuntil") _gb->globals.SENSOR_MODE = value;
                        }

                        if (category == "device") {
                            if (key == "name") _gb->globals.DEVICE_NAME = value;
                            if (key == "env") {
                                _gb->env(value);
                                if (_gb->hasdevice("mem")) _gb->getdevice("mem").write(1, value);

                                if (_gb->globals.GDC_SETUP_READY) {
                                    Serial.println("##CL-GDC-ENV::" + _gb->env() + "##"); delay(50);
                                }
                            }

                            // if (key == "id") _gb->globals.DEVICE_SN = value;
                            if (key == "devices") _gb->globals.DEVICES_LIST = value;
                        }

                        if (category == "sleep") {
                            if (key.contains("mode")) _gb->globals.SLEEP_MODE = value;
                            if (key.contains("duration")) _gb->globals.SLEEP_DURATION = (value).toInt();
                        }

                        if (category == "server") {
                            if (key.contains("url")) _gb->globals.SERVER_URL = value;
                            if (key.contains("port")) _gb->globals.SERVER_PORT = (value).toInt();
                            if (key.contains("enabled")) _gb->globals.OFFLINE_MODE = value == "disabled";
                        }

                        if (category == "sim") {
                            if (key.contains("apn")) _gb->globals.APN = value;
                            if (key.contains("rat")) _gb->globals.RAT = value;
                        }
                    }
                    line = "";
                }
            }
            else line += String(char_code);
        }
        
        this->closeFile(file);
        this->off();
        _gb->log(" -> Done");

        // If device SN not set
        if (_gb->globals.DEVICE_SN == "" || _gb->globals.DEVICE_SN.contains("-")) {
            _gb->getdevice("sntl").disable();
            _gb->br().color("red").log("Configuration doesn't specify the device's SN. Suspending operation.");
            _gb->color("white").log("Please use the GatorByte Desktop Client to configure this device.");
            uint8_t now = millis();
            while (!_gb->globals.GDC_CONNECTED) {
                _gb->getdevice("gdc").detect(true);
                if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on(((millis() - now) / 1000) % 2 ? "red" : "yellow");
                delay(500);
            }
        }

        if (_gb->globals.GDC_CONNECTED) {
            return *this;
        }
        
        _gb->br();
        _gb->heading("Configuration extracted from SD");
        _gb->log("Environment: " + _gb->env());
        _gb->log("Project ID: " + _gb->globals.PROJECT_ID);
        _gb->log("Device SN: " + _gb->globals.DEVICE_SN);
        _gb->log("SD SN: " + this->sn);
        _gb->log("Device name: " + _gb->globals.DEVICE_NAME);
        _gb->log("Survey mode: " + _gb->globals.SURVEY_MODE);
        _gb->log("Local timezone: " + _gb->globals.TIMEZONE);
        _gb->log("Server URL: " + _gb->globals.SERVER_URL + ":" + _gb->globals.SERVER_PORT);
        _gb->log("APN: " + (_gb->globals.APN.length() > 0 ? _gb->globals.APN : "Not set"));
        _gb->log("Sleep settings: " + _gb->globals.SLEEP_MODE + ", " + (String(_gb->globals.SLEEP_DURATION / 60 / 1000) + " min " + String(_gb->globals.SLEEP_DURATION / 1000 % 60) + " seconds."));
        _gb->br();

        return *this;
    }
    
    // If requested file could not be opened
    else{
        this->off();
        
        // // Disable watchdog
        // _gb->getmcu().watchdog("disable");

        return *this;
    }

    delay(100);
    return *this; 
}

// Read config file and return a line by type
String GB_SD::readconfig(String type) { return this->readconfig("/config/config.ini", type); }
String GB_SD::readconfig(String filename, String type) { 
    
    // Detect GDC without lock
    if (_gb->hasdevice("gdc")) _gb->getdevice("gdc").detect(false);

    // Enable watchdog
    _gb->getmcu().watchdog("enable");
    this->on();

    // If requested config file exists
    if(!this->exists(filename)){
        _gb->log("config file not found.");
        delay(100);
    }
        
    File file = this->openFile("read", filename);
    delay(100);

    // If the file opened successfully
    if(file){
        char char_code;
        String line = "";
        while (file.available()) {
            char_code = file.read();
            if(char_code == '\n') {
                
                // Check if the current read line is the type requested
                if (line.indexOf(type + "=") == 0) break;
                else line = "";
            }
            else line += String(char_code);
        }
        
        this->closeFile(file);
        this->off();
        return line;
    }
    
    // If requested file could not be opened
    else{
        this->off();
        
        // Disable watchdog
        _gb->getmcu().watchdog("disable");

        return "";
    }
}

// Update config file by type
void GB_SD::updateconfig(String type, String data) { return this->updateconfig("/config/config.ini", type, data); }
void GB_SD::updateconfig(String filename, String type, String data) { 

    // Enable watchdog
    _gb->getmcu().watchdog("enable");
    this->on();

    String configdata = "";

    // If requested config file exists
    if(this->exists(filename)){
        File file = this->openFile("read", filename);
        delay(100);

        // If the file opened successfully
        if(file){
            char char_code;
            String line = "";
            bool updated = false;
            while (file.available()) {
                char_code = file.read();

                // If it's the end of the line or end of the file
                if(char_code == '\n' || !file.available()) {
                    if (!file.available()) line += String(char_code);

                    // Check if the current read line is the type requested
                    if (line.indexOf(type + "=") == 0) {
                        line = data;
                        updated = true;
                    }
                    configdata = (configdata.length() == 0 ? "" : configdata + "\n") + line;
                    line = "";
                }
                else line += String(char_code);
            }

            if (updated) {

                // Remove config file
                this->rm("/config/config.ini");

                // Re-write the config file
                this->_write("/config/config.ini", configdata);
                delay(100);
            }
            
            this->closeFile(file);
            this->off();
            return;
        }
        
        // If requested file could not be opened
        else{
            this->off();
            
            // Disable watchdog
            _gb->getmcu().watchdog("disable");
    
            return;
        }
    }
 
    // If requested config file does not exist
    else{
        this->off();
        
        // Disable watchdog
        _gb->getmcu().watchdog("disable");

        return;
    }
}

String GB_SD::getfilelist(String root) {
    if (!this->device.detected) return "SD device not initialized.";

    this->on();

    String list = "";
    int filecount = 0;
    File dir = this->_sd.open(root, O_RDONLY);

    while (true) {
        File file = dir.openNextFile();
        if (!file) break;

        int max_characters = 25;
        char f_name[max_characters];
        file.getName(f_name, max_characters);

        if (String(f_name).length() > 0) { 
            filecount++;

            // Get file size in bytes
            int size = file.size();

            if (false && !file.isDir()) {
                _gb->br().log("File: " + String(f_name));
                _gb->log("Size: " + String(size));
            }

            list += String(f_name).indexOf("FOUND.") == -1 ? 
                        (f_name) + String(file.isDir() ? "/" : "") + 
                        String(":") + String(size) +
                        String(",")
                    : "";
        }
        file.close();
    }

    // Remove the last ','
    list = list.substring(0, list.length() -1);

    // Prepend filecount
    list = String(filecount) + "::" + list;

    this->off();
    return list;
}

/*
    ! Count files that contain the specified string in the filename.
*/
int GB_SD::count(String contains, String folder) {
    this->on();

    int count = 0;
    FsFile file;
    
    // Open root folder
    File root;
    root.open(_gb->s2c(folder));

    // Iterate over all files in the folder
    while (file.openNext(&root, O_RDONLY)) {
        char name[25];
        file.getName(name, 25);
        if (!file.isDir()) {
            if (_gb->ca2s(name).indexOf(contains) != -1) {
                count++;
            }
            file.close();
        }
    }
    
    this->off();
    
    return count;
}

// Count files tjat contains the provided string in the file name.
int GB_SD::count(String contains){
    return this->count(contains, "/");
}

// Get queue count
int GB_SD::getqueuecount() {
    if (!this->device.detected) return 0;

    // Create queue folder if not exists
    if (!this->exists("/queue")) this->mkdir("/queue");
    if (!this->exists("/queue/sent")) this->mkdir("/queue/sent");

    return this->count("queue", "/queue");
}

// Check if queue is empty
bool GB_SD::isqueueempty() {
    if (!this->device.detected) return true;

    // Create queue folder if not exists
    if (!this->exists("/queue")) this->mkdir("/queue");
    if (!this->exists("/queue/sent")) this->mkdir("/queue/sent");
    
    return this->getqueuecount() == 0;
}

String GB_SD::getfirstqueuefilename() {
    return this->getfirstfilenamecontaining("queue", "/queue");
}

String GB_SD::getlastqueuefilename() {
    return this->getlastfilenamecontaining("queue", "/queue");
}

String GB_SD::getavailablequeuefilename() {
    String lastfilename = this->getlastfilenamecontaining("queue", "/queue");
    String availablefilename = "queue_" + 
                                String(
                                    (lastfilename.substring(lastfilename.indexOf("queue_") + 6, lastfilename.indexOf("queue_") + 8)).toInt() + 1
                                ) + 
                                ".csv";
    int count = 0;
    while (this->getfirstfilenamecontaining(availablefilename, "/queue").length() > 0) {
        availablefilename = "queue_" + 
                            String(
                                (lastfilename.substring(lastfilename.indexOf("queue_") + 6, lastfilename.indexOf("queue_") + 8)).toInt() + 1 + ++count
                            ) + 
                            ".csv";
    }
    return availablefilename;
}

// Delete a queue file
void GB_SD::removequeuefile(String filename) {
    // _gb->log("Deleting queue file " + filename);
    this->rm("/queue/" + filename);
}

// Write a string to a queue file (i.e. a CSV without a header)
void GB_SD::writequeuefile(String filename, String data) {
    this->writeCSV("/queue/" + filename, data, "");
}
// Write CSV to queue file
void GB_SD::writequeuefile(String filename, CSVary csv) {
    this->writeCSV("/queue/" + filename, csv); 
}

// Read content from a queue file
String GB_SD::readqueuefile(String filename) {
    return this->readfile("/queue/" + filename);
}

// Read content from SD card and encode for transfer over Serial USB
// TODO: Test/redo/deprecate
String GB_SD::download(String file_name){
    File file = this->openFile("read", file_name);
    int char_code;

    String result = "";
    while ((char_code = file.read()) >= 0){
        if(char_code == 44) result += ",";
        else result += "!" + String(char_code);
    }
    _gb->log("###" + String(result), true);
    this->closeFile(file);
    return result;
}

/*
    Read "chars_at_a_time" number of characters starting from "starting_char_index"
*/
String GB_SD::readLinesFromSD(String file_name, int chars_at_a_time, int starting_char_index) {
    File file = this->openFile("read", file_name);
    this->on();
    String result = "";
    int charnumber = 0;

    bool log = file_name.indexOf("readings") != -1;

    // Read lines one at a time
    while(file.available()) {
        // Read characters
        char c = file.read();
        charnumber++;
        if (starting_char_index <= charnumber && charnumber < starting_char_index + chars_at_a_time) result += String(c);
        // delay(1);
    }
    
    this->closeFile(file);
    this->off();
    return result;
}

/*
    Writes data to a file
*/
String GB_SD::writeLinesToSD(String file_name, String data) {
    File file = this->openFile("write", file_name);

    if(file) {
        file.print(data);
        this->closeFile(file);
        delay(5);
    }
    else {
        _gb->log("File creation failed");
    }
    return "";
}

// Deletes folder from SD
bool GB_SD::rmdir(String folderpath){
    
    this->on();
    bool erroroccured = false;

    // Open the parent folder
    String parentpath = folderpath.substring(1, folderpath.lastIndexOf("/"));
    String foldername = folderpath.substring(folderpath.lastIndexOf("/") + 1, folderpath.length());
    
    // Delete contents first
    SdFile delfile;
    File dir;
    dir.open(folderpath.c_str());
    while (delfile.openNext(&dir, O_READ)) {
        char fileName[25];
        delfile.getName(fileName, 25);

        // Create a new file with the same name in the new folder
        delfile.close();
        if (delfile.isDir()) this->rmdir(fileName);
        else this->rm(folderpath + "/" + fileName);
    }

    dir.open(_gb->s2c(folderpath));
    
    // Check if folder exists, and delete it if it does
    if (this->exists(folderpath)) {

        // Traverse to parent folder
        _sd.chdir(parentpath);

        // If the delete action was unsuccessful
        if (!_sd.rmdir(foldername)) {
            erroroccured = true;
        }
                
        // Deleted successfully
        else erroroccured = false;
    }

    // The the file does not exist
    else {
        erroroccured = true;
    }

    delay(15);
    this->off();
    return erroroccured ? false : true;
}

// Deletes file from SD
bool GB_SD::rm(String filename){
    
    this->on();
    bool erroroccured = false;

    // Check if file exists, and delete it if it does
    if (this->exists(filename)) {

        // If the delete action was unsuccessful
        if (!this->_sd.remove(filename)) {
            // _gb->log(" -> Failed (code 4) (" + filename + ")");
            erroroccured = true;
        }
                
        // Deleted successfully
        else erroroccured = false;
    }

    // The the file does not exist
    else {
        // _gb->log(" -> Failed (code 5) (" + filename + ")");
        erroroccured = true;
    }

    delay(15);
    this->off();
    return erroroccured ? false : true;
}

bool GB_SD::exists(String path) {

    this->on();
    bool exists = _sd.exists(path);
    
    return exists;
}

bool GB_SD::mkdir(String path) {
    bool success = _sd.mkdir(_gb->s2c(path), true);
    return success;
}


bool GB_SD::renamedir(String originalFolder, String newFolder) {

    // Create a new folder with the new name
    if (!_sd.mkdir(newFolder)) {
        // Handle folder creation failure
        return false;
    }

    SdFile file;
    SdFile dir;

    // Open the original folder
    if (!dir.open(originalFolder.c_str())) {
        // Handle failure to open the original folder
        return false;
    }

    // Iterate through the contents of the original folder
    while (file.openNext(&dir, O_READ)) {
        char fileName[25];
        file.getName(fileName, 25);

        // Create a new file with the same name in the new folder
        File newFile;
        if (newFile.open(_gb->s2c(newFolder + "/" + fileName), O_CREAT | O_WRITE | O_APPEND)) {
            // Copy the content of the file
            file.rewind();
            uint8_t buf[128];
            int bytesRead;
            while ((bytesRead = file.read(buf, sizeof(buf))) > 0) {
                newFile.write(buf, bytesRead);
            }
            newFile.close();
        } else {
            // Handle failure to create a new file in the new folder
            return false;
        }
        file.close();
    }

    // // Iterate through the files of the original folder and delete them
    // SdFile delfile;
    // dir.open(originalFolder.c_str());
    // while (delfile.openNext(&dir, O_READ)) {
    //     char fileName[25];
    //     delfile.getName(fileName, 25);

    //     // Create a new file with the same name in the new folder
    //     delfile.close();
    //     if (delfile.isDir()) this->rmdir(fileName);
    //     else this->rm(originalFolder + "/" + fileName);
    // }

    dir.close();



    // Delete the original folder and its contents
    if (this->rmdir(originalFolder.c_str())) {
        // Handle failure to delete the original folder
        return false;
    }

    return true;
}
String GB_SD::_split_string(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool GB_SD::rwtest() {

    // // Skip R/W test
    // return true;

    _gb->log(" -> SD R/W test", false);
    bool _erroroccurred = false;

    if(this->exists("test")) {
        _gb->log(" -> Test file wasn't deleted in previous run.", false);

        // Delete the file
        this->rm("test");

        _erroroccurred = true;
    }
    
    String prompt = "teststring";
    File file = this->openFile("write", "test");
    if (file) {

        // Write to the file
        file.print(prompt);
        file.close();
        delay(15);

        // Read the file
        String response = this->readfile("test");

        // Delete the file
        this->rm("test");

        if (response == prompt) {
            _erroroccurred = false;
        }
        else {
            _gb->log(" -> R/W code mismatch", false);
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red");
            _erroroccurred = true;
        }
    }
    else {
        _gb->log(" -> Failed (code 1)", false);
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red");
        _erroroccurred = true;
    }

    return !_erroroccurred;
}

/*
    ! Read control file
    The following function parses key-value file into a json object
*/
GB_SD& GB_SD::readcontrol() {
    callback_t_on_control func = [](JSONary data){};
    return this->readcontrol(func);
}
GB_SD& GB_SD::readcontrol(callback_t_on_control callback) {
    
    // Detect GDC without lock
    if (_gb->hasdevice("gdc")) _gb->getdevice("gdc").detect(false);

    _gb->log("Reading control file from SD", false);

    if (!this->_initialized) {
        _gb->log(" -> Skipped");
        return *this;
    }

    this->on();

    if (!this->detected()) {
        _gb->log(" -> Skipped");
        return *this;
    }
    
    // Create control folder if not exists
    if (!this->exists("/control")) this->mkdir("/control");

    // Control file path
    String filename = "/control/variables.ini";
    
    // If requested config file exists
    if(!this->exists(filename)){
        _gb->color("white").log(" -> Control file not found. Creating file.").color();
        this->writeCSV(filename, "", "");
        delay(100);
    }
    
    File file = this->openFile("read", filename);
    delay(100);

    /*
        ! Sample control.ini file

        REBOOT_FLAG:false
        RESET_VARIABLES_FLAG:false
        QUEUE_UPLOAD_INTERVAL:60000
        WLEV_SAMPLING_INTERVAL:30000
    */

    // If the file opened successfully
    if(file) {
        char char_code;
        String line = "";
        int counter = 0;
        String category;
        while (file.available()) {
            char_code = file.read();
            delay(5);
            if(char_code != '\n' || !file.available()) {
                line += String(char_code);
                // _gb->log(line);
            }

            if(char_code == '\n' || !file.available()) {
                
                counter++;
                line.trim();
                String key = line.substring(0, line.indexOf(":"));
                String value = line.substring(line.indexOf(":") + 1, line.length());
                key.trim(); value.trim();

                // Check the variable type of 'value'
                bool isinteger = false;
                bool isboolean = false;
                bool isstring = false;
                bool isfloat = false;

                if (value == "true" || value == "false") isboolean = true;
                else if (value.indexOf(".") > -1) {
                    String predecimal = value.substring(0, value.indexOf("."));
                    String postdecimal = value.substring(value.indexOf(".") + 1, value.length());
                    if (_gb->isnumber(predecimal) && _gb->isnumber(postdecimal)) isfloat = true;
                }
                else if (_gb->isnumber(value)) isinteger = true;
                else isstring = true;
                
                if (isboolean) _gb->controls.set(key, value);
                else if (isfloat) _gb->controls.set(key, value.toFloat());
                else if (isinteger) _gb->controls.set(key, value);
                else _gb->controls.set(key, value);

                line = "";
            }
        }

        this->closeFile(file);
        this->off();
        if (counter > 0) _gb->arrow().color("green").log("Read " + String(counter) + " control variables.");
        else {
            _gb->arrow().color("red").log("No control variables found.");
        }

        // Callback
        if (this->initialized() && _gb->controls.get() != "{}") callback(_gb->controls);

        return *this;
    }
    
    // If requested file could not be opened
    else{
        this->off();
        _gb->arrow().color("red").log("File not found.");
        return *this;
    }

    return *this; 
}

/*
    ! This function updates one control variable on the SD and the global JSONary object.

    'keyvalue' string should have the following format:
    ----------------
    key1=value1
    ...
*/
GB_SD& GB_SD::updatesinglecontrol(String keyvalue, callback_t_on_control callback) {
    if(keyvalue.length() == 0) return *this;
    
    _gb->log("Updating single control variables on SD: ", false);

    int cursor = 0;
    String state = "key";
    String key = "";
    String value = "";
    do { 
        String newchar = String(keyvalue.charAt(cursor));

        if (state == "key") {
            if (newchar == "=") {
                state = "value";
                value = "";
            }
            else {
                key += newchar;
            }
        }

        if (state == "value") {
            if (newchar != "=") { 
                if (newchar == "," || newchar == "") {
                    state = "key";
                }
                else {
                    value += newchar;
                }
            }
        }

    } while (String(keyvalue.charAt(cursor++)).length() > 0);
    
    _gb->log(key + " = " + value, false);
    
    _gb->arrow().color("green").log("Done");

    // Update the variable on the SD and the global variable
    updatecontrolstring(key, value);

    // Update variables in runtime memory
    callback(_gb->controls);

    return *this;
}

/*
    ! This function updates the control file and the global JSONary object.

    'keyvalues' should have the following format:
    ----------------
    key1=value1
    key2=value2
    key3=value3
    ...
*/
GB_SD& GB_SD::updateallcontrol(String keyvalues, callback_t_on_control callback) {
    if(keyvalues.length() == 0) return *this;

    _gb->log("Updating all control variables on SD", false);
        
    int MAX_KEYS = 20;
    String keys[MAX_KEYS];
    String values[MAX_KEYS];
    int count = 0;
    int cursor = 0;
    String state = "key";

    String key = "";
    String value = "";
    do { 
        String newchar = String(keyvalues.charAt(cursor));

        if (state == "key") {
            if (newchar == "=") {
                state = "value";
                keys[count] = key;
                value = "";
            }
            else {
                key += newchar;
            }
        }

        if (state == "value") {
            if (newchar != "=") { 
                if (newchar == "," || newchar == "") {
                    state = "key";
                    values[count] = value;
                    key = "";
                    count++;
                }
                else {
                    value += newchar;
                }
            }
        }

    } while (String(keyvalues.charAt(cursor++)).length() > 0);

    _gb->arrow().color("green").log("Done");

    // Get control data from SD
    this->readcontrol();

    // Update values in the gb.controls object
    for (int i = 0; i < MAX_KEYS; i++) {
        String key = keys[i];
        
        if (key != "") {
            _gb->controls.set(key, values[i]);
        }
    }

    // Update the controls file on SD
    String filename = "/control/variables.ini";
    
    // Delete previous control file
    this->rm(filename);

    // Replace '=' with ':'
    keyvalues.replace("=", ":");
    keyvalues.replace(",", "\n");
    keyvalues = keyvalues.substring(0, keyvalues.length());
    
    // Re-write the control file
    this->_write(filename, keyvalues);

    // Update variables in runtime memory
    callback(_gb->controls);

    return *this;
}

GB_SD& GB_SD::updatecontrolstring(String key, String value, callback_t_on_control callback) {

    // Update control
    this->updatecontrolstring(key, value);

    // Update variables in runtime memory
    callback(_gb->controls);

    return *this;
}
GB_SD& GB_SD::updatecontrolstring(String key, String value) {

    // Get control data from SD
    this->readcontrol();

    // Update/add key-value pair to the control object
    _gb->controls.set(key, value);

    // Convert JSON to key-value
    String keyvalue = _gb->controls.toKeyValue();

    String filename = "/control/variables.ini";
    
    // Delete previous control file
    this->rm(filename);

    // Re-write the control file
    this->_write(filename, keyvalue);

    return *this;
}

GB_SD& GB_SD::updatecontrolint(String key, int value, callback_t_on_control callback) {
    
    // Update control
    return this->updatecontrolint(key, value);

    // Update variables in runtime memory
    callback(_gb->controls);
}
GB_SD& GB_SD::updatecontrolint(String key, int value) {
    return this->updatecontrolstring(key, String(value));
}

GB_SD& GB_SD::updatecontrolbool(String key, bool value) {
    return this->updatecontrolstring(key, value ? "true" : "false");
}
GB_SD& GB_SD::updatecontrolbool(String key, bool value, callback_t_on_control callback) {
    
    // Update control
    this->updatecontrolbool(key, value);

    // Update variables in runtime memory
    callback(_gb->controls);

    return *this;
}

GB_SD& GB_SD::updatecontrolfloat(String key, double value, callback_t_on_control callback) {
    
    // Update control
    return this->updatecontrolfloat(key, value);

    // Update variables in runtime memory
    callback(_gb->controls);
}
GB_SD& GB_SD::updatecontrolfloat(String key, double value) {
    return this->updatecontrolstring(key, String(value));
}

#endif


/**
 * !Known issues
 * 1. SD card gets corrupted after a while.
 *  * Try using updated library -> Didn't do anything.
 *  * Try removing rwtest() before every write -> Didn't do anything.
 *  * Try using another SD card format.
 *  * Count the number of read/writes it takes for the error to resurface.
 *  * Try running an example code over-night.
 *  * I think the problem was using cheap SD cards. Switching to Sandisk solved the issue.
 *  * Also added a delay in the off() to give SD enough time to finish its operations before it is turned off.
 * 
 * !Resources
 * 1. https://github.com/greiman/SdFat/issues/96
 * 2. https://github.com/greiman/SdFat/issues/272
*/ 