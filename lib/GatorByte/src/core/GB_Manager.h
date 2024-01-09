

//! Base class for GB_MCU
#ifndef GB_MCU_h
    #define GB_MCU_h

    class GB_MCU {
        public:
            GB_MCU() {};
            
            virtual bool testdevice() { return false; };
            virtual bool connected() { return false; };
            virtual bool connect(String) { return false; };
            virtual bool stopclient() { return false; };
            virtual bool disconnect(String) { return false; };
            virtual bool reconnect(String) { return false; };
            virtual bool reconnect() { return false; };
            virtual bool reset(String) { return false; };
            virtual void sleep(String, int) { return; };
            virtual bool on_wakeup() { return false; };
            virtual Client &newclient() {};
            virtual Client &deleteclient() {};
            virtual Client &newsslclient() {};
            virtual Client &deletesslclient() {};
            virtual Client &getclient() {};
            virtual Client &getsslclient() {};
            virtual String get(String) {};
            virtual String getfirmwareinfo() { return ""; };
            virtual String getimei() { return ""; };
            virtual String geticcid() { return ""; };
            virtual int getrssi() { return 0; };
            virtual String getoperator() { return ""; };
            virtual bool begin_modem() { return false; };
            virtual void watchdog(String) {};
            virtual void watchdog(String, int) {};
            virtual GB_MCU& startbreathtimer() { return *this; };
            virtual GB_MCU& stopbreathtimer() { return *this; };
            virtual bool testbattery() { Serial.println("test from Base class"); return false; };
            virtual String batterystatus() { Serial.println("status from Base class"); return ""; };

            // virtual String post(String, String) {};

            String SERVER_IP = "";
            int SERVER_PORT = -1;
            int LAST_SLEEP_DURATION = 0;

        private:
    };
#endif

#ifndef FsFile_h
    #include "SdFat.h"
#endif

//! Base class for GB_DEVICE
#ifndef GB_DEVICE_h
    #define GB_DEVICE_h
    class GB_DEVICE {
        public:
            GB_DEVICE() {};
            typedef void (*callback_t_on_control)(JSONary data);

            //! General functions
            virtual GB_DEVICE& on() { Serial.println("on from Base class"); return *this; };
            virtual GB_DEVICE& off() { Serial.println("off from Base class"); return *this; };
            virtual bool ison() { return false; };
            virtual GB_DEVICE& trigger(int) { return *this; };
            virtual bool initialized() { return false; };
            virtual bool testdevice() { Serial.println("test from Base class"); return false; };
            virtual String status() { Serial.println("status from Base class"); return ""; };

            //! IOE functions
            virtual int readpin(int) { return LOW; };
            virtual void writepin(int, int) { return; };

            //! TCA functions
            virtual void selectexclusive(int) { return; };
            virtual void resetmux() { return; };

            //! EADC functions
            virtual uint16_t getreading(uint8_t) { return 0; };

            //! Atlas Scientific sensors' functions
            virtual float readsensor() { return -1; };
            virtual float readsensor(String mode) { return -1; };
            virtual GB_DEVICE& calibrate() { return *this; };
            virtual GB_DEVICE& calibrate(int) { return *this; };
            virtual int calibrate(String, int) { return 0; };
            virtual float lastvalue() { return 0; };

            //! RGB functions
            virtual GB_DEVICE& on(int, int, int) { return *this; };
            virtual GB_DEVICE& on(String) { return *this; };
            virtual GB_DEVICE& revert() { return *this; };
            virtual GB_DEVICE& blink(String, int, int, int) { return *this; };
            virtual GB_DEVICE& rainbow(int) { return *this; };

            //! MQTT functions
            virtual GB_DEVICE& update() { return *this; };

            //! Buzzer functions
            virtual GB_DEVICE& play(String) { return *this; };
            virtual GB_DEVICE& wait(int) { return *this; };

            //! BL functions
            virtual GB_DEVICE& setname(String) { return *this; };
            virtual GB_DEVICE& setpin(String) { return *this; };
            virtual void print(String) { return; };
            virtual void print(String, bool) { return; };
            virtual GB_DEVICE& listen() { return *this; };
            virtual String read_at_command_response() { return ""; };
            virtual String send_at_command(String) { return ""; };

            
            //! RTC functions
            virtual GB_DEVICE& sync(char[], char[]) { return *this; };
            virtual GB_DEVICE& sync() { return *this; };
            virtual String timestamp() { return ""; };
            virtual String date() { return ""; };
            virtual String date(String) { return ""; };
            virtual String time() { return ""; };
            virtual String time(String) { return ""; };

            //! SD functions
            virtual bool detected() { return false; };
            virtual bool sdformat() { return false; };
            virtual bool sderase() { return false; };
            virtual bool rwtest() { return false; };
            virtual bool exists(String) { return false; };
            virtual bool sdtest() { return false; };
            virtual File openFile(String reason, String file_name) { File file; return file; }
            virtual String readLinesFromSD(String file_name, int lines_at_a_time, int starting_line) { return ""; }
            virtual String writeLinesToSD(String file_name, String data) { return ""; }
            virtual GB_DEVICE& readconfig() { return *this; };

            virtual bool mkdir(String path) { return false;}
            virtual bool rm (String filename) { return false; };
            virtual bool rmdir (String foldername) { return false; };
            virtual void writefile(String filename, String data) { return; };
            virtual void writeString(String filename, String data) { return; };
            virtual void writeCSV(String filename, String data, String header) { return; };
            virtual void writeCSV(String filename, CSVary csv) { return; };
            virtual void writeCSV(CSVary csv) { return; };
            virtual void writeJSON(String filename, String data) { return; };
            virtual bool debug(String action, String category) { return false; };
            virtual bool debug(String action, String category, String message) { return false; };
            virtual String getfilelist(String root) { return ""; };
            virtual String readfile(String filename, String folder) { return ""; };
            virtual String readfile(String filename) { return ""; };
            virtual GB_DEVICE& readcontrol() { return *this; };
            virtual GB_DEVICE& updatecontrolstring(String, String, callback_t_on_control) { return *this; };
            virtual GB_DEVICE& updatecontrolbool(String, bool, callback_t_on_control) { return *this; };
            virtual GB_DEVICE& updatecontrolint(String, int, callback_t_on_control) { return *this; };
            virtual GB_DEVICE& updatecontrolfloat(String, double, callback_t_on_control) { return *this; };
            virtual String readconfig(String filename, String type) { return ""; };
            virtual String readconfig(String type) { return ""; };
            virtual void updateconfig(String filename, String type, String data) { return; };
            virtual void updateconfig(String type, String data) { return; };
            virtual GB_DEVICE& initialize() { return *this; };

            //! EEPROM module functions
            virtual String get(int) { return ""; };
            virtual GB_DEVICE& write(int, String) { return *this; };
            virtual GB_DEVICE& write(int, char*) { return *this; };
            virtual GB_DEVICE& remove(int) { return *this; };
            virtual void debug(String message) { return; };
            virtual bool memtest() { return false; };
            
            //! GatorByte desktop client (GDC)
            virtual GB_DEVICE& detect() {  Serial.println("GDC detect from Base class"); return *this; };
            virtual GB_DEVICE& detect(bool) { Serial.println("GDC detect from Base class"); return *this; };
            virtual GB_DEVICE& loop() { Serial.println("GDC loop from Base class"); return *this; };
            virtual void send(String category, String data) { return; }
            
            //! GatorByte Sentinel (SNTL)
            virtual void tell(String command) { return; };
            virtual void tell(String command, int attempts) { return; };
            virtual GB_DEVICE& enable() { Serial.println("enable from Base class"); return *this; };
            virtual GB_DEVICE& disable() { Serial.println("disable from Base class"); return *this; };
            virtual uint16_t reboot() { return 0; }

        private:
    };
#endif

//! The following structure needs to be filled in for each peripheral/sensor
struct DEVICES {

    // No device
    GB_DEVICE *none;

    // Microcontroller
    GB_MCU *mcu;
    
    // Comms
    GB_DEVICE *mqtt;
    GB_DEVICE *http;
    GB_DEVICE *bl;
    
    // IO Expander
    GB_DEVICE *ioe;
    GB_DEVICE *tca;
    GB_DEVICE *sermux;

    // Command mode interfaces
    GB_DEVICE *command;
    GB_DEVICE *configurator;
    GB_DEVICE *eadc;
    GB_DEVICE *gdc;
    
    // Peripherals
    GB_DEVICE *sd;
    GB_DEVICE *fram;
    GB_DEVICE *mem;
    GB_DEVICE *gps;
    GB_DEVICE *rtc;
    GB_DEVICE *buzzer;
    GB_DEVICE *acc;
    GB_DEVICE *rgb;
    GB_DEVICE *pwr;
    GB_DEVICE *sntl;
    GB_DEVICE *relay;
    GB_DEVICE *rg11;
    GB_DEVICE *tpbck;

    // Sensors
    GB_DEVICE *rtd;
    GB_DEVICE *ph;
    GB_DEVICE *dox;
    GB_DEVICE *ec;
    GB_DEVICE *aht;
    GB_DEVICE *uss;
};