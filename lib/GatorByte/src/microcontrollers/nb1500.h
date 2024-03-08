/*
	File: nb1500.h
	Project: microcontrollers
	
	Notes:
	Insert notes here
	
	Created: Wednesday, 23rd December 2020 12:29:40 am by Piyush Agade
	Modified: Friday, 8th January 2021 11:38:39 am by Piyush Agade
	
	Copyright (c) 2021 Piyush Agade
	
    ! Usage example
    mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "").connect("cellular");

*/

#ifdef ARDUINO_SAMD_MKRNB1500
#define MCU_HAS_4G
#define MCU_HAS_I2C
#define MCU_HAS_SPI
#define MCU_HAS_UART

#define GB_NB1500_h

// Include required libraries
#ifndef Arduino_h
    #include "Arduino.h"
#endif
#ifndef _ARDUINO_UNIQUE_ID_H_
    #include <ArduinoUniqueID.h>
#endif

#ifndef ArduinoHttpClient_h
    #include "ArduinoHttpClient.h"
#endif

#ifndef GB_h
    #include "../GB.h"
#endif

#ifndef GB_PIPER
    #include "../core/GB_Piper.h"
#endif

#ifndef _MKRNB_H_INCLUDED
    #include "MKRNB.h"
#endif

#ifndef Wire
    #include "Wire.h"
#endif

#ifndef _ARDUINO_LOW_POWER_H_
    #include "ArduinoLowPower.h"
#endif

#ifndef ADAFRUIT_SLEEPYDOG_H
    #include "SleepyDog.h"
#endif


// #ifndef SAMD_TIMERINTERRUPT_H
    // #include "SAMDTimerInterrupt.h"
    // #include "SAMD_ISR_Timer.h"
    // #define HW_TIMER_INTERVAL_MS 10

    // /*
    //     Other options: 
    //     TIMER_TC4, TIMER_TC5, TIMER_TCC, TIMER_TCC1, TIMER_TCC2
    // */
    // SAMDTimer ITimer(TIMER_TC3);

    // #define TIMER_INTERVAL_1S             1000L
    // #define TIMER_INTERVAL_2S             2000L
    // #define TIMER_INTERVAL_5S             5000L

    // SAMD_ISR_Timer ISR_Timer;

    // void _timer_handler(void) {
    //     ISR_Timer.run();
    // }
// #endif


class GB_NB1500 : public GB_MCU {
    public:
        GB_NB1500(GB &gb);

        DEVICE device = {
            "mcu",
            "MKR NB1500"
        };

        GB_NB1500& configure();
        GB_NB1500& configure(String, String);
        GB_NB1500& configure(String, String, int);
        GB_NB1500& i2c();
        GB_NB1500& debug(Serial_ &ser, int);
        GB_NB1500& serial(Uart &ser, int);
        GB_NB1500& serialpassthrough();
        GB_NB1500& startbreathtimer();
        GB_NB1500& stopbreathtimer();
        GB_NB1500& wait(int);
        void i2cscan();

        typedef void (*callback_t_on_sleep)();
        typedef void (*callback_t_on_wakeup)();
        typedef void (*callback_t_on_timer_interrupt)();
        
        // MODEM functions
        String getfirmwareinfo();
        String getimei();
        String getoperator();
        String gettime();
        String geticcid();
        int getrssi();
        bool getfplmn();
        bool clearfplmn();
        
        GB_NB1500& apn(String);
        GB_NB1500& pin(String pin);

        // Power management
        void sleep();
        void sleep(String level);
        void sleep(String level, int milliseconds);
        
        void sleep(callback_t_on_sleep callback);
        void sleep(callback_t_on_sleep callback, GB_PIPER piper);
        void sleep(String level, callback_t_on_sleep callback);
        void sleep(String level, int milliseconds, callback_t_on_sleep callback);
        
        void sleep(callback_t_on_sleep, callback_t_on_wakeup callback);
        void sleep(callback_t_on_sleep, callback_t_on_wakeup callback, GB_PIPER piper);
        void sleep(String level, callback_t_on_sleep, callback_t_on_wakeup callback);
        void sleep(String level, int milliseconds, callback_t_on_sleep, callback_t_on_wakeup callback);
        
        void watchdog(String action);
        void watchdog(String action, int milliseconds);
        
        bool reset();
        bool reset(String);
        bool testdevice();
        bool on_wakeup();
        bool checklist();
        bool checklist(string categories);
        bool checksignalviability(bool diagnostics);
        void diagnostics(); // TODO: Add this function to other microcontrollers, or the GB core library

        // Cellular functions
        bool connected();
        bool connect();
        bool connect(bool diagnostics);
        bool stopclient();
        bool disconnect(String type);
        bool reconnect(String type);
        bool reconnect();
        bool get(String);
        bool post(String, String);
        String httpresponse();

        bool testbattery();
        String batterystatus();
        float fuel(String);
        bool battery_connected();

        String help();
        Client& newclient();
        Client& deleteclient();
        Client& getclient();
        Client& newsslclient();
        Client& deletesslclient();
        Client& getsslclient();
        String send_at_command(String);
        String getsn();

        int CELL_SIGNAL_LOWER_BOUND = 5;
        int REBOOTCOUNTER = 0;
        int RSSI = 0;
        int CELL_FAILURE_COUNT_LIMIT = 3;

    private:
        GB *_gb;
        bool _ASLEEP = false;
        bool _HAS_WAKE_CALLBACK = false;
        callback_t_on_wakeup _wake_callback;
        callback_t_on_timer_interrupt _timer_callback;

        int _breath_timer_id;

        NB _nbAccess = NB(MODEM_DEBUG);
        NBScanner _nbScanner;
        NBModem _nbModem;
        GPRS _gprs;
        
        bool _cellular_connected = false;
        String _modem_fw = "";

        // HTTPS Client
        NBClient _client;
        NBSSLClient _ssl_client;
        _SER_PORTS _serial = {&Serial, &Serial1};
        _BAUD_RATES _baudrate = {9600, 9600};
        int _SARA_BAUD_RATE = 115200;

        String _last_at_response; 
        String _format_at_response(String);
        bool _wait_for_at_response();
        bool _wait_for_at_response(unsigned long);
        void _modem_initialize();
        String _sara_at_command(String);
        void _sleep(String, int);
        void _sleep(int);
        bool _register_network();
        bool _attach_gprs();

        String _httpresponse = "";

};

GB_NB1500::GB_NB1500(GB &gb) {
    this->_gb = &gb;
    // this->_gb->includelibrary(this->device.id, this->device.name);
    this->_gb->devices.mcu = this;
    
    // Set Serial channels
    this->_gb->serial = {&Serial, &Serial1};

    gb.globals.DEVICE_SN = this->getsn();
}

bool GB_NB1500::testdevice() { 
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return true;
}

Client& GB_NB1500::newclient() {
    this->_client = new NBClient();
    return this->_client;
}

Client& GB_NB1500::deleteclient() {
    this->_client = NULL;
    return this->_client;
}

bool GB_NB1500::stopclient() {
    this->_client.stop();
    return true;
}

Client& GB_NB1500::getclient() {
    return this->_client;
}

Client& GB_NB1500::newsslclient() {
    this->_ssl_client = new NBSSLClient();
    return this->_ssl_client;
}

Client& GB_NB1500::deletesslclient() {
    this->_ssl_client = NULL;
    return this->_ssl_client;
}


Client& GB_NB1500::getsslclient() {
    return this->_ssl_client;
}

void GB_NB1500::_modem_initialize() {
    bool restart = false;
    MODEM.begin(restart);    
}

String GB_NB1500::_sara_at_command(String command) {
    
    // Clear the serial
    delay(100);
    SerialSARA.readString();
    delay(50);
    
    String res;
    res.reserve(15);
    
    // Send command to MODEM and collect response
    MODEM.send(command);
    MODEM.waitForResponse(400, &res);
    this->_last_at_response = res;
    
    // _gb->log("Response: " + this->_last_at_response);

    // Extract the response string
    command.remove(0, 2);
    if (res.startsWith(command + ": ")) res.remove(0, (command + ": ").length());
    else if (command.indexOf("?") > -1) {
        command.remove(command.indexOf("?"), command.indexOf("?") + 1);
        if(res.startsWith(command + ": "))
            res.remove(0, (command + ": ").length());
    }
    return res;
}

String GB_NB1500::send_at_command(String command) {

    // _gb->log("Sending AT command: " + command, false);
    
    // Clear the serial
    delay(50);
    SerialSARA.readString();
    delay(20);
    
    // Send command to MODEM and collect response
    MODEM.send(command);
    
    this->_wait_for_at_response();
    String res = this->_last_at_response;
    
    // For commands like AT+CSQ?
    if (command.indexOf("?") > -1) {
        res.remove(0, (command).length());
        command.remove(0, 2);
        command.remove(command.length() - 1, command.length());
        res.replace(command + ": ", "");
        res = this->_format_at_response(res);
    }

    // For commands like AT+CMEE=1
    else if (command.indexOf("=") >= 0) {
        res.remove(0, (command).length() + 1);
        command.remove(0, 2);
        res.replace(command + ": ", "");
        res.replace(command, "");
        res = this->_format_at_response(res);
    }
    
    // For commands like AT+CCID
    else if (command.indexOf("=") == -1) {
        res.remove(0, (command).length() + 1);
        command.remove(0, 2);
        res.replace(command + ": ", "");
        res.replace(command, "");
        res = this->_format_at_response(res);
    }

    // For commands like AT, ATI, ATI9
    else {
        res.remove(0, (command).length() + 1);
        res = this->_format_at_response(res);
    }

    /*
        Extract data if applicable
        For responses for commands like AT+CCID, AT+CEREG?, AT+CSQ?
        The following block will convert 1,0;OK to 1,0
    */
    if (res.lastIndexOf(";OK") > 0) {
        res.remove(res.length() - 3, res.length());
    }

    // _gb->log(" -> " + res);

    return res;
}

String GB_NB1500::_format_at_response(String res) {
    res.trim();
    int counter = 20;
    while (res.contains("\r\n \r\n") && counter-- >= 0) res.replace("\r\n \r\n", ";");
    while (res.contains("\r\n\r\n") && counter-- >= 0) res.replace("\r\n\r\n", ";");
    while (res.indexOf("\r\n") != -1 && counter-- >= 0) res.replace("\r\n", ";");
    while (res.contains("\n") && counter-- >= 0) res.replace("\n", ";");
    while (res.contains("\r") && counter-- >= 0) res.replace("\r", ";");
    res.trim();
    return res;
}

bool GB_NB1500::_wait_for_at_response() {
    return _wait_for_at_response(10000);
}
bool GB_NB1500::_wait_for_at_response(unsigned long timeout) {
    this->_last_at_response = "";
    unsigned long startMillis = millis();
    String response;
    
    this->_last_at_response = "";
    while (millis() - startMillis < timeout) {
        if (Serial2.available()) {
            String result = SerialSARA.readString();
            this->_last_at_response = result;
            // if (_at_response.endsWith(expectedResponse)) {
            //     return true;
            // }
            return true;
        }
    }
    return false;
}

GB_NB1500& GB_NB1500::configure(String pin, String apn) {

    this->device.detected = true;

    // SIM configuration
    _gb->globals.PIN = pin;
    _gb->globals.APN = apn;

    return *this;
}
GB_NB1500& GB_NB1500::configure(String pin, String apn, int sleep_duration) {
    _gb->globals.SLEEP_DURATION = sleep_duration;
    return this->configure(pin, apn);
}
GB_NB1500& GB_NB1500::configure() {
    return this->configure("", "");
}

GB_NB1500& GB_NB1500::i2c() {

    // Initialize I2C for peripherals
    Wire.begin();
    
    this->device.detected = true;


    return *this;
}

GB_NB1500& GB_NB1500::debug(Serial_ &ser, int baud_rate) {
    
    // Enable serial port for debugging
    _serial.debug = &ser;
    _baudrate.debug = baud_rate;
    
    this->device.detected = true;


    _serial.debug->begin(_baudrate.debug);
    _gb->serial = _serial;

    // _gb->getdevice("gdc").detect(false);

    return *this;
}

GB_NB1500& GB_NB1500::serial(Uart &ser, int baud_rate) {
    
    // Enable serial port for sensors (peripherals)
    _serial.hardware = &ser;
    _baudrate.hardware = baud_rate;
    
    _serial.hardware->begin(_baudrate.hardware);
    
    this->device.detected = true;
    
    _gb->serial = _serial;
    
    return *this;
}

GB_NB1500& GB_NB1500::startbreathtimer() {
    return *this;

    _gb->log("Breath timer", false);

    // // Setup timer
    // this->_breath_timer_id = ITimer.attachInterruptInterval_MS(_gb->globals.BREATHE_INTERVAL, _timer_handler);

    // // Start the breath timer
    // ISR_Timer.setInterval(_gb->globals.BREATHE_INTERVAL, breathe);
    // ITimer.enableTimer();
    
    _gb->log(" -> Started");
    
    return *this;
}

GB_NB1500& GB_NB1500::stopbreathtimer() {
    return *this;

    _gb->log("Stopping breath timer", false);
    
    // ITimer.disableTimer();

    _gb->log(" -> Done");
    
    return *this;
}

GB_NB1500& GB_NB1500::serialpassthrough() {

    _gb->log("Entering passthrough mode. Restart to exit.");

    while (true) {
        if (_gb->serial.hardware->available()) {
            _gb->serial.debug->write(_gb->serial.hardware->read());
        }

        if (_gb->serial.debug->available()) {
            _gb->serial.hardware->write(_gb->serial.debug->read());
        }
    }
    
    return *this;
}

/*
    ! Network strength (AT+CSQ)
    Gets network strength/quality
*/

int GB_NB1500::getrssi() {
    this->watchdog("enable");
    String cesq = this->_sara_at_command("AT+CSQ");
    int rssi = cesq.substring(0, cesq.indexOf(",")).toInt();
    String rssi_dB = String(-20 + (.5 * rssi)) + " dB";

    // If the MODEM hasn't detected a signal yet
    if (rssi == 0 || rssi == 99) {
        
        // Reacquire the signal strength
        int counter = 0;
        while ((rssi == 0 || rssi == 99) && counter++ < 5) {
            cesq = this->_sara_at_command("AT+CSQ");
            rssi = cesq.substring(0, cesq.indexOf(",")).toInt();
            this->watchdog("reset");
            delay(2000);
        }

    }

    this->RSSI = rssi;

    this->watchdog("disable");
    return rssi;
}

/*
    ! FPLMN list
    nbAccess.begin doesn't work if FPLMN is not empty. So, first let's make sure to see if the list
    is empty or not using the AT+CRSM=176,28539,0,0,12 command.
    The empty list will give response: "FFFFFFFFFFFFFFFFFFFFFFFF"s

    The command to clear the FPLMN list is:
    AT+CRSM=214,28539,0,0,12,"FFFFFFFFFFFFFFFFFFFFFFFF"

    Reference: https://support.hologram.io/hc/en-us/articles/360035697373-Clear-the-FPLMN-forbidden-networks-list
*/

bool GB_NB1500::getfplmn() {
    
    if (SIM_DETECTED) {
        this->watchdog("enable", 4000);

        // Get the FPLMN list form the SIM memory
        String fplmnresponse = this->_sara_at_command("AT+CRSM=176,28539,0,0,12");
        fplmnresponse.remove(0,13);
        _gb->log("FPLMN list", false);

        // Check if the list is not empty. Clear the list if it is not empty
        if(fplmnresponse != "\"FFFFFFFFFFFFFFFFFFFFFFFF\"") {
            _gb->log(" -> " + fplmnresponse + " -> List not empty. Try clearing the list.", false);
            _gb->log(" -> Done");
            return false;
        }
        else {
            _gb->log(" -> " + fplmnresponse + " -> List is empty.");
            return true;
        }
        this->watchdog("disable");
    }
    else {
        _gb->log("FPLMN list -> Skipped reading from the SIM.");
    }
}

bool GB_NB1500::clearfplmn() {

    // Clear the list
    String clearresponse = this->_sara_at_command("AT+CRSM=214,28539,0,0,12,\"130184FFFFFFFFFFFFFFFFFF\"");
    
    if (!clearresponse.endsWith("+CRSM: 144,0,\"\"")) {
        _gb->getdevice("rgb").on("red");
        _gb->log(" -> Failed");
        return false;
    }
    
    // If clearing the list succeeded
    else {
        _gb->log(" -> Done. Restarting the device.");
        _gb->getdevice("rgb").on("red");
        delay(3000);
        _gb->getmcu().reset("mcu");
        
        return true;
    }
}

String GB_NB1500::getoperator() {
    this->watchdog("enable", 4000);
    String cops = this->_sara_at_command("AT+COPS?");
    this->watchdog("disable");
    return cops;
}

String GB_NB1500::gettime() {
    if (this->_modem_fw.indexOf("05.06") >= 0 || this->_modem_fw.indexOf("A.02.00") >= 0) return "Skipped. Outdated MODEM firmware.";

    // Read time
    String time = this->_sara_at_command("AT+CCLK?");
    return time;
}

String GB_NB1500::getfirmwareinfo() {
    if (!MODEM_INITIALIZED) return "MODEM not initialized";
    this->watchdog("enable", 8000);
    String response = ""; int count = 0; 
    // while (response == "" && count++ < 5) {
        response = this->_sara_at_command("ATI9"); delay(100);
    // }
    this->watchdog("disable");
    return response;
}

String GB_NB1500:: getimei() {
    if (!MODEM_INITIALIZED) return "MODEM not initialized";
    this->watchdog("enable", 4000);
    if (this->_modem_fw.indexOf("05.06") >= 0 || this->_modem_fw.indexOf("A.02.00") >= 0) {
        this->watchdog("disable");
        return "Skipped. Outdated MODEM firmware.";
    }
    
    String imei = ""; int count = 0; 
    // while (imei == "" && count++ < 5) {
    imei = this->_sara_at_command("AT+CGSN"); delay(100);
    // }
    this->watchdog("disable");
    return imei;
}

String GB_NB1500::geticcid() {
    if (!MODEM_INITIALIZED) return "MODEM not initialized";
    this->watchdog("enable", 4000);

    if (this->_modem_fw.indexOf("05.06") >= 0 || this->_modem_fw.indexOf("A.02.00") >= 0) {
        SIM_DETECTED = 1;
        return "Skipped. Outdated MODEM firmware.";
    }

    String simcheckresponse = this->_sara_at_command("AT+CCID");
    
    if (simcheckresponse.startsWith("ERROR")) {
        SIM_DETECTED = 0;
        this->watchdog("disable");
        return "SIM not detected";
    }
    else if (simcheckresponse.startsWith("+CME ERROR: SIM failure")) {
        SIM_DETECTED = 0;
        this->watchdog("disable");
        return "SIM failure";
    }
    else {
        SIM_DETECTED = 1;
        this->watchdog("disable");
        return simcheckresponse;
    }
}

/*
    Set PIN
*/
GB_NB1500& GB_NB1500::pin(String pin) { 
    _gb->globals.PIN = pin;
    return *this; 
}

/*
    Set APN
*/
GB_NB1500& GB_NB1500::apn(String apn) { 
    apn.trim();
    _gb->globals.APN = apn;
    if (_gb->globals.APN.length() > 0) _gb->log("Setting APN -> " + _gb->globals.APN);
    return *this; 
}

bool GB_NB1500::connected() { 
    return _gprs.status() == GPRS_READY;
}

bool GB_NB1500::connect() { return this->connect(false); }
bool GB_NB1500::connect(bool diagnostics) {
    
    if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("white");

    /*
        ! If the device is operating in offline mode
    */
    if(_gb->globals.OFFLINE_MODE) {
        _nbAccess.shutdown();
        MODEM_INITIALIZED = false;
        CONNECTED_TO_NETWORK = false;
        CONNECTED_TO_INTERNET = false;
        
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").revert();
        return false;
    }
    
    /*
        ! Check if already connected to the network and Internet
    */
    if (this->connected()) {
        return true;
    }
    else {
        _gb->color("yellow").log("Internet connection not detected. Attempting connection.");
    }

    // int counter;

    // // Kickstart the MODEM; And skip beyond 3 seconds
    // counter = 300; while (!MODEM.begin() && counter-- >= 0) delay(10);

    // // Test MODEM Serial communication
    // MODEM_INITIALIZED = this->send_at_command("AT").contains("OK");

    // // Check if SIM is inserted
    // this->send_at_command("AT+CMEE=0");
    // this->send_at_command("AT+CFUN=0");
    // bool SIMFLAG = this->send_at_command("AT+CPIN?").contains("READY");
    // _gb->log("SIM " + String(!SIMFLAG ? "NOT " : "") + "found.");

    // if (MODEM_INITIALIZED && SIMFLAG) {

    //     // Check if device is registered on the network
    //     bool NETREGFLAG = this->send_at_command("AT+CEREG?").contains(",1");
    //     _gb->log("Network " + String(!NETREGFLAG ? "NOT " : "") + "registered.");

    //     bool GPRSATTFLAG = this->send_at_command("AT+CGATT?").contains("1,1");
    //     _gb->log(String(!GPRSATTFLAG ? "NOT connected" : "Connected") + " to Internet.");
        
    //     while (!NETREGFLAG || !GPRSATTFLAG) {

    //         _gb->log("\nAttempting connection.");

    //         if (!NETREGFLAG) {
    //             this->send_at_command("AT+CMGF=1");
    //             this->send_at_command("AT+UDCONF=1,1");
    //             this->send_at_command("AT+CTZU=1");
    //             this->send_at_command("AT+CGDCONT=1,\"IP\",\"\"");
    //             this->send_at_command("AT+UAUTHREQ=1,0");
    //             this->send_at_command("AT+CFUN=1");
    //             this->send_at_command("AT+CEREG?");
                
    //             NETREGFLAG = this->send_at_command("AT+CEREG?").contains(",1");
    //             _gb->log("Network " + String(!NETREGFLAG ? "NOT " : "") + "registered.");
    //             CONNECTED_TO_NETWORK = NETREGFLAG;
    //         }
    //         else CONNECTED_TO_NETWORK = true;

    //         if (!GPRSATTFLAG) {
    //             this->send_at_command("AT+CGATT=1");
    //             GPRSATTFLAG = this->send_at_command("AT+CGACT?").contains("1,1");
    //             _gb->log(String(!GPRSATTFLAG ? "NOT connected" : "Connected") + " to Internet.");
    //             CONNECTED_TO_INTERNET = GPRSATTFLAG;

    //             this->send_at_command("AT+COPS?");

    //             return true;
    //         }
    //         else {
    //             CONNECTED_TO_INTERNET = true;
    //             return true;
    //         }
    //     }
    // }

    /*
        ! Go through pre-connection checklist
    */
    MODEM_INITIALIZED = this->checklist("iccid");
    
    /*
        ! Check network signal viability
    */
    SIGNAL_VIABLE = this->checksignalviability(diagnostics);
    
    /*
        ! Check if the cellular is already connected and if a reconnection is required
        TODO: Finish this ?
    */
    if (this->_cellular_connected &&_gprs.status() == GPRS_READY) { 

        _gb->br().color("green").log("Cellular connection detected.");
        
        // Check if signal is viable for good connection and connect
        if (SIGNAL_VIABLE) {
            int counter = 1; while (!this->_cellular_connected && counter-- > 0) {

                //! Register device on cellular network
                int start = 0;
                start = millis();
                bool nbstatus = this->_register_network();
                _gb->log(" -> Registration: " + String(nbstatus ? "Succeded" : "Failed"), false);
                _gb->log(" -> Delay: " + String((millis() - start) / 1000) + " seconds ", false);

                //! Attach GPRS
                bool gprsstatus = false;
                if (nbstatus) {
                    start = millis();
                    gprsstatus = this->_attach_gprs();
                    _gb->log(" -> GPRS: " + String(gprsstatus ? "Attached" : "Detached"), false);
                    _gb->log(" -> Delay: " + String((millis() - start) / 1000) + " seconds ", false);
                }
                else {
                    _gb->log(" -> GPRS: Skipped", false);
                }
                
                //! Get connection status
                this->_cellular_connected = nbstatus && gprsstatus;

                //! If a connection couldn't be established
                if (!this->_cellular_connected) _gb->log(" .", false);
            }
        }

        // If the signal is not viable
        else {
            _gb->arrow().color("red").log("Insufficient signal", false).color();
            this->_cellular_connected = false;
        }

        return this->_cellular_connected;
    }

    else {

        _gb->br().color("yellow").log("Cellular connection not detected.");

        /*
            ! Attempt connecting to the network
        */
        
        _gb->log("Connecting to cellular network", false);

        // Check if signal is viable for good connection and connect
        if (SIGNAL_VIABLE) {
            int counter = 1; while (!this->_cellular_connected && counter-- > 0) {

                //! Register device on cellular network
                bool nbstatus = this->_register_network();
                int start = 0;
                start = millis();
                _gb->log(" -> Registration: " + String(nbstatus ? "Succeded" : "Failed"), false);
                _gb->log(" -> Delay: " + String((millis() - start) / 1000) + " seconds ", false);

                //! Attach GPRS
                bool gprsstatus = false;
                if (nbstatus) {
                    start = millis();
                    gprsstatus = this->_attach_gprs();
                    _gb->log(" -> GPRS: " + String(gprsstatus ? "Attached" : "Detached"), false);
                    _gb->log(" -> Delay: " + String((millis() - start) / 1000) + " seconds ", false);
                }
                else {
                    _gb->log(" -> GPRS: Skipped", false);
                }

                //! Get connection status
                this->_cellular_connected = nbstatus && gprsstatus;

                //! If a connection couldn't be established
                if (!this->_cellular_connected) _gb->log(" .", false);
            }
        }

        // If the signal is not viable
        else {
            _gb->arrow().color("red").log("Insufficient signal", false).color();
            this->_cellular_connected = false;
        }
        
        //! If a cellular connection was successfully established
        if (this->_cellular_connected) {
            CONNECTED_TO_NETWORK = true;
            CONNECTED_TO_INTERNET = true;
            
            // Buzz
            if (_gb->hasdevice("buzzer"))_gb->getdevice("buzzer").play("..");
            
            // Slow blink green 3 times
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").blink("green", 3, 300, 200);

            _gb->log(" -> Done ");
            
            // Get cellular info
            String cops = this->getoperator();
            int modeint = cops.substring(0, cops.indexOf(",")).toInt();
            String mode = String(modeint == 0 ? "Automatic" : modeint == 1 ? "Manual" : "Other");
            int ratint = cops.substring(cops.lastIndexOf(",") + 1, cops.length()).toInt();
            String rat = String(ratint == 7 ? "LTE CAT-M1" : ratint == 8 ? "EC-GSM-IoT" : ratint == 9 ? "E-UTRAN" : "Other");
            cops = cops.substring(cops.indexOf("\"") + 1, cops.lastIndexOf("\""));
            _gb->log("Operator: " + cops + " -> Mode: " + mode + " -> RAT: " + rat);

            if (cops == "ERROR" || cops == "NO CARRIER") {
                this->_cellular_connected = false;
                CONNECTED_TO_NETWORK = false;
                CONNECTED_TO_INTERNET = false;
            }
            
            return true;
        }
        else {
            CONNECTED_TO_NETWORK = false;
            CONNECTED_TO_INTERNET = false;

            _gb->log(" -> Failed");
            
            // Buzz
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play(".-");

            // Blink red 3 times
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").blink("red", 3, 300, 200);
            return false;
        }
    }

    return false;
}

bool GB_NB1500::_register_network() {
    int _attempts = 2;
    bool nbstatus = false;
    // this->watchdog("enable");

    int count = 0;
    while (!nbstatus && count++ < _attempts) {
        if (_gb->globals.APN.length() > 0) nbstatus = _nbAccess.begin(_gb->s2c(_gb->globals.PIN), _gb->s2c(_gb->globals.APN), false, true) == NB_READY;
        else nbstatus = _nbAccess.begin("", false, true) == NB_READY;

        this->watchdog("reset");
        delay(100);
    }

    // this->watchdog("disable");
    return nbstatus;
}

bool GB_NB1500::_attach_gprs() {
    int _attempts = 2;
    bool gprsstatus = false;
    // this->watchdog("enable");
    
    int count = 0;
    while (!gprsstatus && count++ < _attempts) {
        gprsstatus = _gprs.attachGPRS() == GPRS_READY;
        this->watchdog("reset");
        delay(100);
    }

    // this->watchdog("disable");
    return gprsstatus;
}

bool GB_NB1500::disconnect(String type) {
    if(type == "cellular") {

        //! Set variables
        this->_cellular_connected = false;

        //! Disconnect from cellular network
        _gb->log("Detaching GPRS", false);
        if (MODEM_INITIALIZED && CONNECTED_TO_INTERNET) {
            _gprs.detachGPRS();
            CONNECTED_TO_NETWORK = false;
            CONNECTED_TO_INTERNET = false;
            _gb->log(" -> Done");
        }
        else _gb->log(" -> Skipping. MODEM not initialized");

        //! Shutdown MODEM
        _gb->log("Shutting down MODEM", false);
        if (MODEM_INITIALIZED) {
            bool success = _nbAccess.secureShutdown();
            MODEM_INITIALIZED = false;
            CONNECTED_TO_NETWORK = false;
            CONNECTED_TO_INTERNET = false;
            CONNECTED_TO_MQTT_BROKER = false;
            CONNECTED_TO_API_SERVER = false;
            _gb->log(" -> Done");
        }
        else _gb->log(" -> Skipping. MODEM not initialized");

        // Report MODEM's power state
        _gb->log("Powering MODEM off " + String(MODEM.isPowerOn() ? " -> Failed" : " -> Succeeded"));

        return _gprs.status() != GPRS_READY;
    }
    return false;
}

// Reconnect default network
bool GB_NB1500::reconnect() {
    return this->reconnect("cellular");
}

// Reconnect cellular network
bool GB_NB1500::reconnect(String type) {
    if(type == "cellular") {

        _gb->log("Reconnecting cellular -> ", false);

        // Reconnect to cellular network
        this->disconnect("cellular");
        this->connect();
        bool success = _gprs.status() == GPRS_READY;
        _gb->log(success ? "Done": "Failed");
        return success;
    }
    return false;
}

void GB_NB1500::sleep(callback_t_on_sleep callback) { this->sleep(_gb->globals.SLEEP_MODE, _gb->globals.SLEEP_DURATION, callback); }
void GB_NB1500::sleep(callback_t_on_sleep callback, GB_PIPER piper) { 
    // if (piper.)
    this->sleep(_gb->globals.SLEEP_MODE, _gb->globals.SLEEP_DURATION, callback); 
    }
void GB_NB1500::sleep(String level, callback_t_on_sleep callback) { this->sleep(level, _gb->globals.SLEEP_DURATION, callback); }
void GB_NB1500::sleep(String level, int milliseconds, callback_t_on_sleep callback) {
    this->_HAS_WAKE_CALLBACK = false;
    
    // Call user defined pre-sleep operations
    callback();

    // Enter low power-mode
    this->_sleep(level, milliseconds);
}

void GB_NB1500::sleep(callback_t_on_sleep callback_on_sleep, callback_t_on_wakeup callback_on_wakeup) { this->sleep(_gb->globals.SLEEP_MODE, _gb->globals.SLEEP_DURATION, callback_on_sleep, callback_on_wakeup); }
void GB_NB1500::sleep(String level, callback_t_on_sleep callback_on_sleep, callback_t_on_wakeup callback_on_wakeup) { this->sleep(level, _gb->globals.SLEEP_DURATION, callback_on_sleep, callback_on_wakeup); }
void GB_NB1500::sleep(String level, int milliseconds, callback_t_on_sleep callback_on_sleep, callback_t_on_wakeup callback_on_wakeup) {
    
    // Set wake up callback
    this->_HAS_WAKE_CALLBACK = true;
    this->_wake_callback = callback_on_wakeup;

    // Call user defined pre-sleep operations
    callback_on_sleep();

    // Enter low power-mode
    this->_sleep(level, milliseconds);
}

void GB_NB1500::sleep() { this->sleep(_gb->globals.SLEEP_MODE, _gb->globals.SLEEP_DURATION); }
void GB_NB1500::sleep(String level) { this->sleep(level, _gb->globals.SLEEP_DURATION); }
void GB_NB1500::sleep(String level, int milliseconds) {
    this->_HAS_WAKE_CALLBACK = false;

    // Enter low power-mode
    this->_sleep(level, milliseconds);
}

void GB_NB1500::_sleep(String level, int milliseconds) {
    
    // //! Turn off all the peripherals
    // TODO Exclude BL, GPS?
    // if (_gb->hasdevice("ioe")) {
    //     _gb->log("Turning off peripherals", false);
    //     for(int i = 0; i < 15; i++) {
    //         _gb->getdevice("ioe").writepin(i, LOW);
    //     }
    //     _gb->log(" -> Done"); delay(300);
    // }

    //! Conclude watchdog operations
    this->watchdog("disable");

    //! RGB rainbow
    if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").rainbow(5000).off();
    
    // Deduce sleep duration from the last timestamp when readings were taken
    _gb->br().log("Seconds since last reading: " + String(_gb->globals.SECONDS_SINCE_LAST_READING) + " seconds");
    milliseconds = _gb->globals.SLEEP_DURATION - _gb->globals.SECONDS_SINCE_LAST_READING * 1000;

    // Deduct setup from the sleep time
    milliseconds -= _gb->globals.SETUPDELAY * 1000;

    // Deduct loop delay from the sleep time
    milliseconds -= _gb->globals.LOOPDELAY * 1000;
    
    /*
        If sleep duration is negative. This might happen if the sleep duration set is smaller than
        the sum of setup delay and loop delay. In this case, the device sleeps for 3 seconds.
    */
    if (milliseconds <= 0) { 
        _gb->br().log("Iteration delay is more than the sleep duration. Setting sleep duration to 3 seconds.");    
        milliseconds = 3000;
    }
    
    /*
        If sleep duration is too big. 
    */
    if (milliseconds > 1000000000) { 
        _gb->br().log("Sleep duration is too big. Setting sleep duration to as specified in the config file.");    
        milliseconds = _gb->globals.SLEEP_DURATION;
    }

    this->LAST_SLEEP_DURATION = milliseconds;
    
    _gb->br().color("white").log("Entering low-power mode for " + String(milliseconds / 60 / 1000) + " min " + String(milliseconds / 1000 % 60) + " seconds. (" + String(milliseconds / 1000) + " seconds)", true);
    _gb->log("Setup delay: " + String(_gb->globals.SETUPDELAY) + " seconds");
    _gb->log("Loop delay: " + String(_gb->globals.LOOPDELAY) + " seconds");
    _gb->log("Low-power mode", false).arrow().color("yellow").log(level);

    this->_ASLEEP = true;

    /*
        ! End serial interfaces (this needs some work/bug fixing)
        * Issue: 
            1. The GPS serial reads nothing after wakeup.
        
        * Probable solution:
            1. Reinitialize serial interfaces using mcu.serial and mcu.debug functions
    */
    // this->_serial.debug->end();
    // this->_serial.hardware->end();

    //! Enter low power-mode
    // TODO: Test "deep" and "idle" sleep levels.

    /*
        'Fake' sleep uses delay() to emulate sleep behaviour.
    */
    if (level == "daydream") {

        _gb->log("Entering 'daydream' sleep mode.");
        
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").rainbow(milliseconds);
        _gb->log("Exiting 'daydream' sleep mode.");
    }
    
    /*
        'Reset' sleep uses delay() to emulate sleep behaviour, but resets the 
        microcontroller at the end of the 'sleep' duration.
    */
    else if (level == "reset") {

        _gb->log("Entering 'fake' and reset sleep mode.");
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").rainbow(milliseconds);
        _gb->log("Exiting 'fake' and reset sleep mode."); delay(2000);
        _gb->getmcu().reset("mcu");
    }
    
    /*
        'Deep' sleep uses LowPower library to put the Arduino in sleep mode.
        This mode detaches debug USB on sleep and restores on wake up.
        This sleep mode allows power optimization with a slowest wakeup time. 
        All peripherals but RTC are off.

        The CPU can be wakeup only using RTC or wakeup on interrupt capable pins.

        * Concerns:
            1. When the device is sleeping, MQTT messages cannot be received.
            2. Needs reconnection to the cellular network and MQTT broker 
               (and subscriptions) on wake up.

        * Issues:
            1. The implementation of LowPower.sleep() and LowPower.deepSleep() is
                exactly the same. Maybe this is a bug. A probable solution is to
                update the LowPower library.
    */
    else if (level == "deep") {

        USBDevice.detach();
        LowPower.deepSleep(milliseconds);
        USBDevice.attach();
    }
    
    /*
        'Shallow' sleep uses LowPower library to put the Arduino in sleep mode.
        This mode detaches debug USB on sleep and restores on wake up.
        This sleep mode allows power optimization with a slower wakeup time. 
        Only the chosen peripherals are on.

        * Concerns:
            1. When the device is sleeping, MQTT messages cannot be received.
            2. Needs reconnection to the cellular network and MQTT broker 
               (and subscriptions) on wake up.
    */
    else if (level == "shallow") {

        USBDevice.detach();
        LowPower.sleep(milliseconds);
        USBDevice.attach();
    }
    
    /*
        Puts the MCU in IDLE mode. This mode allows power optimization with the 
        fastest wake-up time. The CPU is stopped. To further reduce power consumption,
        the user can manually disable the clocking of modules and clock sources.

        * Issues:
            1. __WFI() not working
    */
    else if (level == "idle") {
        LowPower.idle(milliseconds);
    }
    
    /*
        'psm' mode puts the modem in Power Saving Mode using AT commands.
        This mode does not detach debug USB on 'sleep'.
    */
    else if (level == "psm") {
        // this->_sara_at_command("AT+CSPSM");
        delay(milliseconds);
        on_wakeup();
    }
}

void GB_NB1500::watchdog(String action) { 
    if (action == "enable") this->watchdog(action, 16 * 1000);
    else if (action == "reset") this->watchdog(action, 16 * 1000);
    else if (action == "disable") this->watchdog(action, 16 * 1000);
}

void GB_NB1500::watchdog(String action, int milliseconds) { 

    if (action == "enable") {
        Watchdog.disable();
        // _gb->log("Enabling watchdog timer with timeout: " + String(milliseconds) + " ms.");
        Watchdog.enable(milliseconds);
        WATCHDOG_ENABLED = true;
    }
    else if (action == "reset") {
        // _gb->log("Resetting watchdog timer");
        Watchdog.reset();
        WATCHDOG_ENABLED = true;
    }
    else if (action == "disable") {
        // if (!WATCHDOG_ENABLED) _gb->log("No outstanding watchdogs active");
        // _gb->log("Disabling watchdog timer");
        Watchdog.disable();
        WATCHDOG_ENABLED = false;
    }
    else if (action == "sleep") {
        _gb->log("Sleeping for " + String(milliseconds) + " ms");
        
		USBDevice.detach();
        Watchdog.sleep();
		USBDevice.attach();
    }
}

bool GB_NB1500::on_wakeup() { 
    if (this->_ASLEEP && this->_HAS_WAKE_CALLBACK) {
        this->_ASLEEP = false;
        this->_wake_callback();
        return true;
    }
    else return false;
}

bool GB_NB1500::checksignalviability(bool diagnostics) { 
    
    bool viable = false;
    
    // If SIM not detected
    if (!SIM_DETECTED) {
        this->_cellular_connected = false;
        _gb->color("red").log("SIM not detected.");
        return false;
    }

    // Get network signal strength
    int rssi = this->getrssi();
    
    if (rssi == 99 || rssi == 0) {
        _gb->log("Signal strength: No signal (" + String(rssi) + ")");
        viable = false;

        // Error handler
        if (++esc.no_cell_signal >= 3) _gb->getdevice("sntl").reboot();
    }
    else if (rssi <= this->CELL_SIGNAL_LOWER_BOUND) {
        _gb->log("Signal strength: Low signal strength detected (" + String(rssi) + ",");
        int counter = 5; while (counter-- >= 0) { rssi = this->getrssi(); delay(1000); }
        _gb->log(String(rssi) + ")", false);
        viable = false;
        
        // Error handler
        if (esc.low_cell_signal++ >= 3) {};
    }
    
    if (this->CELL_SIGNAL_LOWER_BOUND < rssi && rssi < 32) {
        _gb->log("Signal strength: " + String(rssi));
        viable = true;

        esc.no_cell_signal = 0;
        esc.low_cell_signal = 0;
    }

    return viable;
}

bool GB_NB1500::checklist() { this->checklist(""); } 
bool GB_NB1500::checklist(string categories) { 

    // Start watchdog timer
    this->watchdog("enable");

    //! Initialize MODEM
    _gb->log("Initializing MODEM", false);
    if (_nbModem.begin()) {
        _gb->log(" -> Succeeded", false);

        // Reset watchdog timer
        this->watchdog("reset");
        MODEM_INITIALIZED = true;
    }
    else {
        bool result = false;
        _gb->arrow().color("red").log("Couldn't initialize MODEM.", false).color();

        // Reboot the system
        if (++esc.modem_init_failure >= this->CELL_FAILURE_COUNT_LIMIT) {
            _gb->arrow().log("Rebooting GatorByte"); delay(1000);
            _gb->getdevice("sntl").reboot();
        }

        /* 
            Keep the following block for documentation
            There was an issue with MODEM firmware 5.06, where it would randomly freeze
            and cause NB1500 to freeze as well. 
            Numerous software-level solutions (see below) were tested, and none of them
            worked. The only solution was to power cycle the microcontroller viz. remove
            power source and reconnecting. Software reset (using watchdog or NVIC) did
            not work.

            Version 5.12 fixed the issue.
        */
        int counter = 0;
        if (false)
            while (!(result = _nbModem.begin()) && counter++ < 2) {
                _gb->log(" .", false);
                    
                // Reset watchdog timer
                this->watchdog("reset");

                // ! Reset MODEM
                MODEM.begin(true);       //* Didn't work
                this->_sara_at_command("AT+CFUN=0");    //* Didn't work
                _nbAccess.shutdown();    //* Didn't work
                _nbAccess.secureShutdown();    //* Didn't work
                MODEM.reset();          //* Didn't work
                MODEM.hardReset();      //* Didn't work
                
                // Wait before rechecking
                delay(5000);
            }

        MODEM_INITIALIZED = result;
        _gb->log(result ? " -> Succeeded" : " -> Failed (" + String(esc.modem_init_failure) + ", " + String(this->CELL_FAILURE_COUNT_LIMIT) + ")", false);
    }
    
    _gb->log(MODEM.isPowerOn() ? " -> On" : " -> Off");

    // Stop watchdog timer
    this->watchdog("disable");

    /*
        ! Get network time
        This function returns the network time
    */
    if (categories.contains("time")) {
        String time = this->gettime();
        if (time.length() == 0) _gb->log("Network time: Couldn't fetch network time");
        else if (time.length() == 22) {
            
            // Parse date
            String year = "20" + time.substring(1, 3);
            String month = time.substring(4, 6);
            String date = time.substring(7, 9);

            // Parse time       
            String hour = time.substring(10, 12);
            String minute = time.substring(13, 15);
            String second = time.substring(16, 18);
            
            _gb->log("Network time: " + year + "/" + month + "/" + date + " " + hour + ":" + minute + ":" + second);
        }
    }
    
    /*
        ! Check MODEM firmware version
        Report the MODEM firmware version. Please ugrade to version 5.12 if 
        the firmware version is below 5.12. Contact uBlox technical support for
        instructions on upgrading the firmware.
    */

    if (categories.contains("fw")) {
        String fwinfo = this->getfirmwareinfo();
        this->_modem_fw = fwinfo;
        _gb->log("MODEM firmware -> " + fwinfo);
    }
    
    /*
        ! Check if MODEM is working properly
        Checks MODEM functionality and set global variables accordingly
    */

    if (categories.contains("imei")) {
        String imei = this->getimei();
        _gb->log("Checking MODEM -> Succeeded -> IMEI: " + imei);
    }

    /*
        ! Check if is SIM is inserted properly
        Check for SIM card and set global variables accordingly
    */

    if (categories.contains("iccid")) {
        String iccid = this->geticcid();
        _gb->log("Checking SIM -> ICCID: " + iccid);
    }
    else SIM_DETECTED = 1;

    /*
        ! Check if FPLMN list is empty
    */

    if (categories.contains("flpmn")) {
        this->getfplmn();
    }

    return MODEM_INITIALIZED;
}

void GB_NB1500::diagnostics() { 
    if (_gb->hasdevice("mem")) {
        _gb->log("\nDiagnostics information: ");
        _gb->log("  > Last failure count: " + String(_gb->getdevice("mem").get(6)));
        _gb->log("  > Number of MODEM failures: " + String(_gb->getdevice("mem").get(10)));
        _gb->log("  > Number of microcontroller resets: " + String(_gb->getdevice("mem").get(11)));
        _gb->log();
    }
    else {
        _gb->log("\nDiagnostics information unavailable: EEPROM not found/added.");
        _gb->log();
    }
}

bool GB_NB1500::reset() { 
    return this->reset("mcu");
}
bool GB_NB1500::reset(String type) { 
    if (type == "mcu") {
        
        /*
            ! Error counter
              Increment counter that track number of times the MODEM had to be reset.
        */
        if (_gb->hasdevice("mem")) {
            _gb->log("Updating MODEM reboot counter");
            if (_gb->getdevice("mem").get(11) == "") _gb->getdevice("mem").write(11, "0");
            _gb->getdevice("mem").write(11, String(_gb->getdevice("mem").get(11).toInt() + 1));
        }
        delay(200);

        NVIC_SystemReset();
    }
    else if (type == "modem") {

        /*
            * Issue: 
                1. Sometimes, mqtt cannot reconnect to broker even after a software reset. The only way out is to remove all power and
                   reboot.
            
            * Good resource
                1. https://forum.arduino.cc/t/small-project-reliable-and-fault-tolerant-modem-start-sequence/704318/5
                2. https://1ot.mobi/resources/blog/finding-patterns-with-terminal

            * Solutions/Tests:
                1. reset() and begin(true): fixes mqtt reconnection issue only when the mcu is reset.
                2. reset() and begin(false): Untested
                3. Update SARA-R410 firmware to version 5.12 
        */
       
        // Start watchdog timer
        this->watchdog("enable", 8000);

        _gb->log("Resetting MODEM -> ", false);
        bool response = false;
        delay(100);

        /*
            ! Error counter
              Increment counter that track number of times the MODEM had to be reset.
        */
        if (_gb->hasdevice("mem")) {
            _gb->log("Updating MODEM reboot counter");
            if (_gb->getdevice("mem").get(10) == "") _gb->getdevice("mem").write(10, "0");
            _gb->getdevice("mem").write(10, String(_gb->getdevice("mem").get(10).toInt() + 1));
        }

        this->getclient().stop();
        this->getclient().flush();

        // Soft reset
        response = MODEM.reset();
        delay(100);

        // // MODEM begin
        // response = MODEM.begin(false);
        // _gb->log(response ? "Done" : "Failed");
        // delay(100);
        
        // Disable watchdog timer
        this->watchdog("disable");

        return response;
    }
    return true;
}

bool GB_NB1500::get(String path) { 
    
    // Connect to network if not connected
    this->connect("cellular");

    _gb->log("Sending GET: " + path + " to " + this->SERVER_IP + ", " + this->SERVER_PORT, false);
    
    HttpClient httpclient = HttpClient(this->getclient(), this->SERVER_IP, this->SERVER_PORT);
    int start = millis();
    String result = "";

    // Send request
    httpclient.beginRequest();
    int state = httpclient.get((path.indexOf("/") == 0 ? "" : "/") + path);
    httpclient.sendHeader("x-device-id", "mkr-gb-prototype");
    httpclient.endRequest();

    // Get response
    bool error = false;
    if (state == HTTP_SUCCESS) {
        _gb->arrow().color("green").log("Done");

        _gb->log("Getting response", false);
        int code = httpclient.responseStatusCode();
        if (code == 200) {
            _gb->arrow().log("Received: " + String(code));
            result = httpclient.responseBody();
            error = false;
        }
        else {
            _gb->arrow().color("red").log("Error: " + String(code));
            result = "";
            error = true;
        }
        httpclient.stop();
    }
    else {
        _gb->arrow().color("red").log("Failed");
        httpclient.stop();
        error = true;
    }

    int difference = millis() - start;
    _gb->log("Time taken: " + String(difference / 1000) + " seconds");

    // Set response
    this->_httpresponse = result;

    // Return
    return !error;
}

bool GB_NB1500::post(String path, String data) { 
    
    // Connect to network if not connected
    this->connect("cellular");

    _gb->log("Sending POST: " + path + " to " + this->SERVER_IP + ", " + this->SERVER_PORT, false);

    HttpClient httpclient = HttpClient(this->getclient(), this->SERVER_IP, this->SERVER_PORT);
    int start = millis();
    String result = "";

    // Send request
    httpclient.beginRequest();
    int state = httpclient.post((path.indexOf("/") == 0 ? "" : "/") + path, "plain/text", data);
    httpclient.sendHeader("device-sn", _gb->globals.DEVICE_SN);
    httpclient.beginBody();
    httpclient.print(data);
    httpclient.endRequest();

    // Get response
    bool error = false;
    if (state == HTTP_SUCCESS) {
        _gb->arrow().color("green").log("Done");

        _gb->log("Getting response", false);
        int code = httpclient.responseStatusCode();
        if (code == 200) {
            _gb->arrow().log("Received: " + String(code));
            result = httpclient.responseBody();
            error = false;
        }
        else {
            _gb->arrow().color("red").log("Error: " + String(code));
            result = "";
            error = true;
        }
        httpclient.stop();
    }
    else {
        _gb->arrow().color("red").log("Failed");
        httpclient.stop();
        error = true;
    }

    int difference = millis() - start;
    _gb->log("Time taken: " + String(difference / 1000) + " seconds");

    // Set response
    this->_httpresponse = result;

    // Return
    return !error;
}

String GB_NB1500::httpresponse() { 
    return this->_httpresponse;
}

String GB_NB1500::help() { 
    return "mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure(\"1234\", \"hologram\").connect(\"cellular\");";
}

void GB_NB1500::i2cscan() {
    _gb->log("\nScanning for I2C devices");

    byte error, address;
    int nDevices;

    nDevices = 0;
    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            _gb->log("Device found at 0x", false);
            if (address < 16) _gb->log("0", false);
            _gb->log(address, HEX);

            nDevices++;
        }
        else if (error == 4)
        {
            _gb->log("Unknown error at address 0x", false);
            if (address < 16)
                _gb->log("0", false);
            _gb->log(address, HEX);
        }
    }
    if (nDevices == 0)
        _gb->log("No I2C devices found. Ensure the devices are powered on.\n");
    else
        _gb->log("Scan complete.\n");
}

// Test the battery
bool GB_NB1500::testbattery() { 
    float prevvoltage = float(this->fuel("v")), currvoltage;
    float sumofdifference = 0, average = 0;
    for (int i = 0; i < 5; i++) {
        currvoltage = float(this->fuel("v"));
        sumofdifference += prevvoltage - currvoltage;
        delay(150);
    }
    average = sumofdifference / 5;

    return average < float(0.05);
}

String GB_NB1500::batterystatus() { 
    return this->testbattery() ? float(this->fuel("v")) : "not-detected" + String(":") + "battery";
}

// Battery functions
float GB_NB1500::fuel(String metric) {
    metric.toLowerCase();
    if (metric == "voltage" || metric == "v") return analogRead(ADC_BATTERY) * 3.3 / 1023.0 * 1.275;
    else if (metric == "level") {
        float sum = 0, avg;
        int counter = 6;
        while (counter-- > 0) {
            sum += analogRead(ADC_BATTERY) * (100 / 1023.0);
            delay(10);
        }
        avg = sum / 6;
        return avg;
    }
    else return this->fuel("v");
}

bool GB_NB1500::battery_connected() {
    return (float(this->fuel("v")) > float(0.5));
}

String GB_NB1500::getsn() {
    ArduinoUniqueID uid;
    String idstr = "";

    for (size_t i = 0; i < 16; i++) {    
        int number = uid.id[i];    
        int counter = 1;
        
        while (
            (number < 65) ||
            (91 <= number && number < 97) ||
            (123 <= number)
        ) {
            if (number < 65) number += (3 + i) + counter * (i + 1);
            if (number > 122) number -= 5 + counter * (i + 1);
            else number -= (3 + i) * (i + 1);
            counter++;
            delay(2);
        } 
        if ((i) % 2 == 0) idstr += char(number);
    }

    return idstr;                  
}

#endif