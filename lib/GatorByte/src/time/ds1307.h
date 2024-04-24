#ifndef GB_DS1307_h
#define GB_DS1307_h

/* Uses RTClib. THe library is compatible with DS3231 and DS1307 */
#include "RTClib.h"

/* Import the core GB library */
#ifndef GB_h
    #include "../GB.h"
#endif

/* Import I/O expander */


class GB_DS1307 : public GB_DEVICE {
    public:
        /* Constructor */
        GB_DS1307(GB &gb);

        /* Device meta information */
        DEVICE device = {
            "rtc",
            "DS1307 Realtime clock"
        };

        /* Device pin information */
        struct PINS {
            bool mux;
            int enable;
        } pins;

        /* Public interface functions */
        GB_DS1307& initialize();
        GB_DS1307& configure(PINS);
        GB_DS1307& sync();
        GB_DS1307& sync(char[], char[]);
        GB_DS1307& on();
        GB_DS1307& off();
        bool running();
        String timestamp();
        String date();
        String date(String format);
        String time();
        String time(String format);

    private:
        byte _ADDRESS = 0x68;
        RTC_DS1307 _rtc;
        GB *_gb;
};

/* Constructor */
GB_DS1307::GB_DS1307(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.rtc = this;
}

/* Configure the device */
GB_DS1307& GB_DS1307::configure(PINS pins) {
    this->on();
    this->pins = pins;
    
    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    this->off();
    return *this;
}

/* Initialize the device */
GB_DS1307& GB_DS1307::initialize() {
    this->on();
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    // Begin
    Wire.begin();
    _rtc.begin();   // Comment Wire.begin in the RTC's library if error occurs
    
    _gb->log("Initializing RTC module", false);
    if(this->running()) {
        if(_rtc.now().unixtime() > 1000000000) _gb->log(" -> Done at " + this->date() + " " + this->time("hh:mm:ss a"), true);
        else {
            _gb->log(" -> Timestamp: " + String(_rtc.now().unixtime()), false);
            _gb->log(" -> RTC running. Incorrect time. ", false);
        }

    }
    else {
        _gb->log(" -> Failed at " + String(_rtc.now().unixtime()), true);
    }

    this->off();
    return *this;
}

/*
    Set RTC to a date and time.
    Date format: Nov 07 20
    Time format: 15:23:59
*/
GB_DS1307& GB_DS1307::sync(char date[], char time[]) {
    this->on();delay(50);

    _gb->log("Syncing RTC to the time of compilation", false);
    _rtc.adjust(DateTime(date, time));
    _gb->log(" -> RTC set to " + String(_rtc.now().month()) + "/" + String(_rtc.now().day()) + "/" + String(_rtc.now().year()) + ", " + String(_rtc.now().hour()) + ":" + String(_rtc.now().minute()) + ":" + String(_rtc.now().second()));

    this->off();
    return *this;
}

/* Sync the RTC to the date and time of code compilation */
GB_DS1307& GB_DS1307::sync() {
    return sync(__DATE__, __TIME__);
}

GB_DS1307& GB_DS1307::on() {
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    
    // Begin I2C communication
    delay(50); Wire.begin();
    
    return *this;
}

GB_DS1307& GB_DS1307::off() {
    if(this->pins.mux) _gb->getdevice("ioe")->writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    return *this;
}

/*
    Returns unix timestamp
*/
String GB_DS1307::timestamp() {
    this->on();
    delay(200);
    DateTime time = _rtc.now();
    uint32_t unixtime = time.unixtime();
    this->off();
    return String(unixtime);
}

/* 
    Returns date in default format.
    Format: MM/DD/YY
*/
String GB_DS1307::date() {
    return this->date("MM/DD/YY");
}

/* 
    Returns date in specified format.
*/
String GB_DS1307::date(String format) {
    this->on();
    delay(50);

    DateTime time = _rtc.now();
    
    // Format month
    String monthsshort[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    String monthsfull[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    format.replace("MMMM", monthsfull[time.month() - 1]);
    format.replace("MMM", monthsshort[time.month() - 1]);
    format.replace("MM", String(time.month()));
    
    // Format date
    String dateprefix[] = {"st", "nd", "rd", "th" };
    format.replace("DD", String(time.day()));
    format.replace("th", time.day() > 0 && time.day() < 3 ? dateprefix[time.day() - 1] : dateprefix[3]);

    // Format year
    format.replace("YYYY", String(time.year()));
    format.replace("YY", String(time.year()).substring(String(time.year()).length() - 2, String(time.year()).length()));

    this->off();
    return String(format);
}

/* 
    Returns time in default format.
    Format: hh:mm:ss
*/
String GB_DS1307::time() {
    return this->time("hh:mm:ss");
}

/* 
    Returns time in specified format.
*/
String GB_DS1307::time(String format) {
    this->on();
    delay(50);
    DateTime time = _rtc.now(); 
    
    format.replace("hh", format.indexOf("a") > -1 ? (time.hour() - 12 < 10 ? "0" : "") + String(time.hour() - 12) : (time.hour() < 10 ? "0" : "") + String(time.hour()));
    format.replace("mm", (time.minute() < 10 ? "0" : "") + String(time.minute()));
    format.replace("ss", (time.second() < 10 ? "0" : "") + String(time.second()));
    format.replace("a", (time.hour() > 12 ? "PM" : "AM"));

    this->off();
    return String(format);
}

/*
    Check if the device is running/communicating properly
*/
bool GB_DS1307::running() {
    this->on();
    delay(100);
    DateTime time = _rtc.now();
    uint32_t unixtime = time.unixtime();
    uint16_t month = time.month();

    if (unixtime > 2000000000) return false;
    else if (unixtime == 946684800) return false;
    else if (month == 165) return false;
    else if (unixtime > 946684800) return true;
    else return false;
}

#endif