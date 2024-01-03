#ifndef GB_DS3231_h
#define GB_DS3231_h

/* Uses RTClib. THe library is compatible with DS3231 and DS1307 */
#include "RTClib.h"
#include <vector>

/* Import the core GB library */
#ifndef GB_h
    #include "../GB.h"
#endif

/* Import I/O expander */
#ifndef GB_74HC595_h
    #include "../misc/74hc595.h"
#endif

class GB_DS3231 : public GB_DEVICE {
    public:
        /* Constructor */
        GB_DS3231(GB &gb);

        /* Device meta information */
        DEVICE device = {
            "rtc",
            "Mini DS3231 Realtime clock"
        };

        /* Device pin information */
        struct PINS {
            bool mux;
            int enable;
        } pins;

        /* Public interface functions */
        GB_DS3231& initialize();
        GB_DS3231& initialize(bool testdevice);
        GB_DS3231& configure();
        GB_DS3231& configure(PINS);
        GB_DS3231& sync();
        GB_DS3231& sync(String);
        GB_DS3231& sync(char[], char[]);
        GB_DS3231& on();
        GB_DS3231& off();
        GB_DS3231& persistent();
        DateTime now();   // add this to other RTC libraries
        bool valid();
        String timestamp();
        String date();
        String date(String format);
        String time();
        String time(String format);
        String converttogmt(String, String);
        int converttogmt(String, int);
        String converttotz(String, String, String);
        int converttotz(String, String, int);
        GB_DS3231& settimezone(String);
        
        bool testdevice();
        String status();

        struct tzinfo  {
            const char* timezone;
            int sign;
            int offsetHours;
            int offsetMinutes;
        };

        const std::vector<tzinfo> tzoffsets = {
            {"GMT", 1, 0, 0},    // No specific country

            {"CET", 1, 1, 0},    // Central Europe
            {"CEST", 1, 2, 0},   // Central Europe (Daylight Saving Time)
            {"EET", 1, 2, 0},    // Eastern Europe
            {"EEST", 1, 3, 0},   // Eastern Europe (Daylight Saving Time)
            {"ART", -1, 3, 0},   // Argentina
            {"BRT", -1, 3, 0},   // Brazil (Brasília Time)
            {"CLT", -1, 4, 0},   // Chile
            {"UYT", -1, 3, 0},   // Uruguay
            {"VET", -1, 4, 0},   // Venezuela
            
            {"AST", -1, 4, 0},   // USA (Atlantic Time)
            {"EDT", -1, 4, 0},   // USA (Eastern Time, Daylight Saving Time)
            {"CDT", -1, 5, 0},   // USA (Central Time, Daylight Saving Time)
            {"EST", -1, 5, 0},   // USA (Eastern Time)
            {"MDT", -1, 6, 0},   // USA (Mountain Time, Daylight Saving Time)
            {"CST", -1, 6, 0},   // USA (Central Time)
            {"PDT", -1, 7, 0},   // USA (Pacific Time, Daylight Saving Time)
            {"MST", -1, 7, 0},   // USA (Mountain Time)
            {"PST", -1, 8, 0},   // USA (Pacific Time)
            {"HST", -1, 10, 0},  // USA (Hawaii)

            {"NZST", 1, 12, 0},  // New Zealand
            {"NZDT", 1, 13, 0},  // New Zealand (Daylight Saving Time)
            {"ACST", 1, 9, 30},  // Australia (Adelaide)
            {"AEST", 1, 10, 0},  // Australia (Sydney)
            {"AWST", 1, 8, 0},   // Australia (Perth)
            {"ICT", 1, 7, 0},    // Thailand, Vietnam, Cambodia, Laos
            {"SGT", 1, 8, 0},    // Singapore
            {"HKT", 1, 8, 0},    // Hong Kong
            {"CST", 1, 8, 0},    // China
            {"IST", 1, 5, 30},   // India
            {"PKT", 1, 5, 0},    // Pakistan
            {"KST", 1, 9, 0},    // North Korea
            {"WIB", 1, 7, 0},    // Indonesia (Java, Sumatra, West and Central Kalimantan)
            {"MYT", 1, 8, 0},    // Malaysia
            {"PHT", 1, 8, 0},    // Philippines
            {"WITA", 1, 8, 0},   // Indonesia (Bali, Nusa Tenggara, South and East Kalimantan)
            {"JST", 1, 9, 0}     // Japan
        };

        String timezone = "EST";
    private:
        byte _ADDRESS = 0x68;
        RTC_DS3231 _rtc;
        GB *_gb;
        bool _persistent = false;
        long _timezone_offset = 0;
};

/* 
    Constructor 
*/
GB_DS3231::GB_DS3231(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.rtc = this;
}

// Test the device
bool GB_DS3231::testdevice() { 
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));
    return this->device.detected;
}
String GB_DS3231::status() { 
    return this->device.detected ? this->date() + "::" + this->time() : (String("not-detected") + String(":") + device.id);
}

/* 
    Configure the device 
*/
GB_DS3231& GB_DS3231::configure() {
    return *this;
}
GB_DS3231& GB_DS3231::configure(PINS pins) {
    this->on();
    this->pins = pins;
    
    // Set pin modes of the device
    if(!this->pins.mux) pinMode(this->pins.enable, OUTPUT);

    this->off();
    return *this;
}

/* 
    Initialize the device 
*/
GB_DS3231& GB_DS3231::initialize() { this->initialize(true); }
GB_DS3231& GB_DS3231::initialize(bool testdevice) { 
    _gb->init();
    
    this->on();
    _gb->log("Initializing RTC module", false);
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    // Begin I2C communication
    bool success = this->_rtc.begin();
    
    #if not defined (LOW_MEMORY_MODE)
        if (testdevice) {
            bool success = this->valid();
            
            // int counter = 3;
            // while((!success || this->date("YYYY") == "2000") && counter-- >= 0) { delay(500); success = this->valid(); }
            
            if (success) {
                this->device.detected = true;
                
                if (_gb->globals.GDC_CONNECTED) {
                    this->off();
                    _gb->log(" -> Done");
                    this->settimezone("EST");
                    return *this;
                }
                
                _gb->log(" -> " + this->date("MMMM DDth, YYYY") + " at " + this->time("hh:mm:ss"), false);
                
                if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("--.").wait(250).play("...");
                if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("green").wait(250).revert(); 
                _gb->log(" -> Done", true);
                this->settimezone("EST");

                // Sync if the RTC is booted for the first time
                if (this->date("YYYY") == "2000") this->sync("EST");
            }
            else {
                this->device.detected = false;
                _gb->log(" -> Not detected", true);
                
                if (_gb->globals.GDC_CONNECTED) {
                    this->off();
                    return *this;
                }

                if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("--.").wait(250).play("---");
                if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red").wait(250).revert(); 
            }
        }
        else {
            this->device.detected = true;
            _gb->log(" -> Done", true);
            this->settimezone("EST");
            
            if (_gb->globals.GDC_CONNECTED) {
                this->off();
                return *this;
            }
            
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("--.").wait(250).play("...");
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("green").wait(250).revert(); 
        }
    #else
        if(this->valid()) {
            this->device.detected = true;
            _gb->log(" -> Done", true);
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("--.").wait(500).play("...");
        }
        else {
            this->device.detected = false;
            _gb->log(" -> Failed", true);
            _gb->log(" -> ", false);
            if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("--.").wait(500).play("---");
            // this->sync();
        }
    #endif
    
    // // Disable watchdog timer
    // this->_gb->getmcu().watchdog("disable");

    this->off();
    return *this;
}

/*
    Set RTC to a date and time.
    Date format: Nov 07 20
    Time format: 15:23:59
*/
GB_DS3231& GB_DS3231::sync(char date[], char time[]) {
    this->on();delay(50);

    _gb->log("Syncing RTC to the provided time", false);
    _rtc.adjust(DateTime(date, time));

    long timestamp = _rtc.now().unixtime();
    _gb->log(" -> RTC set to " + String(_rtc.now().month()) + "/" + String(_rtc.now().day()) + "/" + String(_rtc.now().year()) + ", " + String(_rtc.now().hour()) + ":" + String(_rtc.now().minute()) + ":" + String(_rtc.now().second()), false);

    _gb->log(" -> Done");
    this->off();
    return *this;
}

/*
    ! Sets timezone
    Provide a 5-character string representing the timezone in
    the following format: [+/-]HH:MM. For example, +05:30, -4:00
*/
GB_DS3231& GB_DS3231::settimezone(String timezone) { 
    timezone.trim();
    timezone.toUpperCase();
    this->timezone = timezone;

    return *this;
}

/* 
    Sync the RTC to the date and time of code compilation
*/
GB_DS3231& GB_DS3231::sync() { this->sync(this->timezone); }
GB_DS3231& GB_DS3231::sync(String timezone) {
    timezone.trim();
    timezone.toUpperCase();

    // Parse the input time string
    int hours, minutes, seconds;
    sscanf(String(__TIME__).c_str(), "%d:%d:%d", &hours, &minutes, &seconds);

    // Adjust for the timezone
    String __GMTTIME__ = this->converttogmt(timezone, __TIME__);
    _gb->log("Adjusted for local timezone: " + timezone);

    // return sync("May 10 2023", "01:11:00");
    return sync(__DATE__, _gb->s2c(__GMTTIME__));

}

/* 
    Convert a time string to GMT time string
    May not work on the last day of the month
*/
String GB_DS3231:: converttogmt(String localtimezone, String time) {
    int offsethour = 0, offsetminutes = 0, offsetsign = 1;
    localtimezone.trim();
    localtimezone.toUpperCase();
    
    for (const auto& info : this->tzoffsets) {
        if (strcmp(localtimezone.c_str(), info.timezone) == 0) {
            offsethour = info.offsetHours;
            offsetminutes = info.offsetMinutes;
            offsetsign = info.sign;
        }
    }

    // Parse the input time string
    int hours, minutes, seconds;
    int date, month;
    sscanf(String(__TIME__).c_str(), "%d:%d:%d", &hours, &minutes, &seconds);
    hours -= offsetsign * offsethour;
    minutes -= offsetsign * offsetminutes;

    if (minutes >= 60) {
        hours += 1;
        minutes -= 60;
    }
    else if (minutes < 0) {
        hours -= 1;
        minutes = 60 - abs(minutes);
    }

    if (hours >= 24) {
        date += 1;
        hours -= 24;
    }
    else if (hours < 0) {
        date -= 1;
        hours = 24 - abs(hours);
    } 

    return (abs(hours) < 9 ? "0" : "") + String(hours) + ":" + (abs(minutes) < 9 ? "0" : "") + String(minutes) + ":" + (abs(seconds) < 9 ? "0" : "") + String(seconds);
}
int GB_DS3231:: converttogmt(String localtimezone, int timestamp) {
    int offsethour = 0, offsetminutes = 0, offsetsign = 1;
    localtimezone.trim();
    localtimezone.toUpperCase();
    
    for (const auto& info : this->tzoffsets) {
        if (strcmp(localtimezone.c_str(), info.timezone) == 0) {
            offsethour = info.offsetHours;
            offsetminutes = info.offsetMinutes;
            offsetsign = info.sign;
        }
    }

    timestamp = timestamp - offsetsign * offsethour * 3600 - offsetsign * offsetminutes * 60;
    return timestamp;
}


/* 
    Convert a time string to GMT time string
    May not work on the last day of the month
*/
String GB_DS3231:: converttotz(String localtimezone, String targettimezone, String time) {
    int localtzhour = 0, localtzminutes = 0, localtzsign = 1;
    int targettzhour = 0, targettzminutes = 0, targettzsign = 1;
    localtimezone.trim();
    localtimezone.toUpperCase();
    targettimezone.trim();
    targettimezone.toUpperCase();
    
    for (const auto& info : this->tzoffsets) {
        if (strcmp(localtimezone.c_str(), info.timezone) == 0) {
            localtzhour = info.offsetHours;
            localtzminutes = info.offsetMinutes;
            localtzsign = info.sign;
        }
        if (strcmp(targettimezone.c_str(), info.timezone) == 0) {
            targettzhour = info.offsetHours;
            targettzminutes = info.offsetMinutes;
            targettzsign = info.sign;
        }
    }

    // Parse the input time string
    int hours, minutes, seconds;
    int date, month;
    sscanf(String(__TIME__).c_str(), "%d:%d:%d", &hours, &minutes, &seconds);
    hours = hours - localtzsign * localtzhour + targettzsign * targettzhour;
    minutes = minutes - localtzsign * localtzminutes + targettzsign * targettzminutes;

    if (minutes >= 60) {
        hours += 1;
        minutes -= 60;
    }
    else if (minutes < 0) {
        hours -= 1;
        minutes = 60 - abs(minutes);
    }

    if (hours >= 24) {
        date += 1;
        hours -= 24;
    }
    else if (hours < 0) {
        date -= 1;
        hours = 24 - abs(hours);
    } 

    return (abs(hours) < 9 ? "0" : "") + String(hours) + ":" + (abs(minutes) < 9 ? "0" : "") + String(minutes) + ":" + (abs(seconds) < 9 ? "0" : "") + String(seconds);
}
int GB_DS3231:: converttotz(String localtimezone, String targettimezone, int timestamp) {
    int localtzhour = 0, localtzminutes = 0, localtzsign = 1;
    int targettzhour = 0, targettzminutes = 0, targettzsign = 1;
    localtimezone.trim();
    localtimezone.toUpperCase();
    targettimezone.trim();
    targettimezone.toUpperCase();
    
    for (const auto& info : this->tzoffsets) {
        if (strcmp(localtimezone.c_str(), info.timezone) == 0) {
            localtzhour = info.offsetHours;
            localtzminutes = info.offsetMinutes;
            localtzsign = info.sign;
        }
        if (strcmp(targettimezone.c_str(), info.timezone) == 0) {
            targettzhour = info.offsetHours;
            targettzminutes = info.offsetMinutes;
            targettzsign = info.sign;
        }
    }

    timestamp = timestamp - localtzsign * localtzhour * 3600 - localtzsign * localtzminutes * 60 + targettzsign * targettzhour * 3600 + targettzsign * targettzminutes * 60;
    return timestamp;
}
GB_DS3231& GB_DS3231::on() {
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, HIGH);
    else digitalWrite(this->pins.enable, HIGH);
    delay(50);
    return *this;
}

GB_DS3231& GB_DS3231::off() {
    
    if (this->_persistent) return *this;
    
    if(this->pins.mux) _gb->getdevice("ioe").writepin(this->pins.enable, LOW);
    else digitalWrite(this->pins.enable, LOW);
    return *this;
}

GB_DS3231& GB_DS3231::persistent() {
    
    this->_persistent = true;
    return *this;
}

/*
    Returns 'now' Datetime object
*/
DateTime GB_DS3231::now() {
    return this->_rtc.now();
}

/*
    Check if the device is running/communicating properly
*/
bool GB_DS3231::valid() {
    this->on();
    delay(50);
    
    bool error = false;
    if (!this->_rtc.begin()) error = true;

    DateTime time = _rtc.now();
    uint32_t unixtime = time.unixtime();
    uint16_t month = time.month();

    if (unixtime >= 2000000000) error = true;
    else if (unixtime <= 946684800) error = true;
    else if (month == 165) error = true;
    else if (unixtime > 946684800) error = false;
    else error = true;

    return !error;
}


/*
    Returns unix timestamp
*/
String GB_DS3231::timestamp() {
    if (!this->device.detected) return "-1";

    // Check if the time is invalid
    DateTime time;
    bool invalid = true;
    int counter = 0;
    
    // Gt time from the RTC
    uint32_t unixtime;
    while (invalid && counter++ <= 4) {
        this->on(); time = _rtc.now(); this->off();
        unixtime = time.unixtime();
        invalid = !this->valid();
        if (invalid) _gb->log("Invalid timestamp detected: " + String(unixtime));
        delay(250 + (counter * 250));
    }

    // Get timestamp from the time
    if (!invalid) unixtime = time.unixtime();
    else unixtime = _gb->globals.INIT_SECONDS + (millis() / 1000);

    return String(unixtime);
}

/* 
    Returns date in default format.
    Format: MM/DD/YY
*/
String GB_DS3231::date() {
    return this->date("MM/DD/YY");
}

/* 
    Returns date in specified format.
*/
String GB_DS3231::date(String format) {
    if (!this->device.detected) return "-1";

    this->on();
    delay(50);

    DateTime time = _rtc.now();
    
    #if not defined (LOW_MEMORY_MODE)

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
    #else
        format = String(time.month()) + "/" + String(time.day()) + "/" + String(time.year());
    #endif

    this->off();
    return String(format);
}

/* 
    Returns time in default format.
    Format: hh:mm:ss
*/
String GB_DS3231::time() {
    return this->time("hh:mm:ss");
}

/* 
    Returns time in specified format.
*/
String GB_DS3231::time(String format) {
    if (!this->device.detected) return "-1";

    this->on();
    delay(50);
    DateTime time = _rtc.now(); 

    bool hr12 = format.indexOf("a") > -1; int hour;
    if (hr12) hour = time.hour() < 12 ? time.hour() : time.hour() - 12; 
    else hour = time.hour();

    format.replace("hh", hour < 10 ? "0" + String(hour) : String(hour));
    format.replace("mm", (time.minute() < 10 ? "0" : "") + String(time.minute()));
    format.replace("ss", (time.second() < 10 ? "0" : "") + String(time.second()));
    format.replace("a", (time.hour() > 12 ? "PM" : "AM"));

    this->off();
    return String(format);
}

#endif