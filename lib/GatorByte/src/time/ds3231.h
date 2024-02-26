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
        GB_DS3231& sync(uint32_t);
        GB_DS3231& sync(DateTime);
        GB_DS3231& on();
        GB_DS3231& off();
        GB_DS3231& persistent();
        DateTime now();   // add this to other RTC libraries
        bool valid();
        bool valid(uint32_t);
        bool valid(DateTime);
        String timestamp();
        String date();
        String date(String format);
        String time();
        String time(String format);
        DateTime converttogmt(String, DateTime);
        String converttogmt(String, String);
        int converttogmt(String, int);
        String converttotz(String, String, String);
        int converttotz(String, String, int);
        DateTime converttotz(String, String, DateTime);
        GB_DS3231& settimezone(String);
        String getsource();
        
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
        String _source = "rtc";
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
    return this->device.detected ? this->date() + "::" + this->time() + "::" + String(this->_source) : (String("not-detected") + String(":") + device.id);
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
            
            if (success) {
                this->device.detected = true;
                
                if (_gb->globals.GDC_CONNECTED) {
                    this->off();
                    _gb->arrow().log("Done");
                    this->settimezone(_gb->globals.TIMEZONE);
                    return *this;
                }
                
                _gb->arrow().log(this->date("MMMM DDth, YYYY") + " at " + this->time("hh:mm:ss") + ", source: " + this->getsource(), false);
                
                if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("--.").wait(250).play("...");
                if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("green").wait(250).revert(); 
                _gb->arrow().log("Done", true);
                this->settimezone(_gb->globals.TIMEZONE);

                // Sync if the RTC is booted for the first time
                if (this->date("YYYY") == "2000") this->sync(_gb->globals.TIMEZONE);
            }
            else {
                this->device.detected = false;
                _gb->arrow().log("Not detected. Falling back to MODEM for time", false);
                
                if (_gb->globals.GDC_CONNECTED) {
                    this->off();
                    return *this;
                }
                _gb->arrow().log(this->date("MMMM DDth, YYYY") + " at " + this->time("hh:mm:ss"), false);
                _gb->arrow().log("Done", true);

                if (_gb->hasdevice("buzzer")) _gb->getdevice("buzzer").play("--.").wait(250).play("---");
                if (_gb->hasdevice("rgb")) _gb->getdevice("rgb").on("red").wait(250).revert(); 
            }
        }
        else {
            this->device.detected = true;
            _gb->arrow().log("Done", true);
            this->settimezone(_gb->globals.TIMEZONE);
            
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
    Sync the RTC to the date and time of code compilation and convert to GMT
*/
GB_DS3231& GB_DS3231::sync() { this->sync(this->timezone); }
GB_DS3231& GB_DS3231::sync(String timezone) {

    // Create DateTime object
    DateTime dt = DateTime(__DATE__, __TIME__);

    // Convert to GMT
    dt = this->converttogmt(timezone, dt);

    return sync(dt);
}

/*
    Set RTC to a date and time.
    The provided date and time should be GMT

    Date format: Nov 07 20
    Time format: 15:23:59
*/
GB_DS3231& GB_DS3231::sync(char date[], char time[]) {

    DateTime dt = DateTime(date, time);
    
    // Sync MODEM's clock (Skipped. MODEM resets to local time after every boot.)
    // TODO: Move to GB_NB1500. 
    // The dt should be in local timezone
    #ifndef false
        if (!MODEM_INITIALIZED) MODEM_INITIALIZED = MODEM.begin() == 1;
        if(MODEM_INITIALIZED) {
            
            _gb->log("Syncing MODEM to the provided time", false);
            String month = String(dt.month());
            String day = String(dt.day());
            String hour = String(dt.hour());
            String minute = String(dt.minute());
            String second = String(dt.second());

            String datestr = String(dt.year()).substring(2, 4) + "/" + (month.length() == 1 ? "0" : "") + month + "/" + (day.length() == 1 ? "0" : "") + day;
            String timestr = (hour.length() == 1 ? "0" : "") + hour + ":" + (minute.length() == 1 ? "0" : "") + minute + ":" + (second.length() == 1 ? "0" : "") + second + "+00";
            String atcommand = "AT+CCLK=\"" + datestr + "," + timestr + "\"";

            String res = _gb->getmcu().send_at_command(atcommand);
            if (res.endsWith("OK")) _gb->log(" -> Done");
            else _gb->log(" -> Failed");

        }
    #endif
    
    return this->sync(dt);
}

/*
    Set RTC to a timestamp.
    The provided timestamp should be GMT

    Timestamp units: seconds
*/
GB_DS3231& GB_DS3231::sync(uint32_t timestamp) {
    DateTime dt = DateTime(timestamp);
    return this->sync(dt);
}

/*
    Set RTC to a DateTime.
    The provided DateTime should be GMT
*/
GB_DS3231& GB_DS3231::sync(DateTime dt) {

    _gb->log("Syncing RTC to the provided time", false);
    
    if (this->device.detected) {
        this->on();delay(50);
        _rtc.adjust(dt); delay(50);
        _gb->log(" -> RTC set to " + String(_rtc.now().month()) + "/" + String(_rtc.now().day()) + "/" + String(_rtc.now().year()) + ", " + String(_rtc.now().hour()) + ":" + String(_rtc.now().minute()) + ":" + String(_rtc.now().second()), false);
        _gb->log(" -> Done");
        this->off();
    }
    else _gb->arrow().color("red").log("Not detected. Skipping.").color();

    return *this;
}

/* 
    Convert a time string to GMT time string
    * May not work on the last day of the month
    * Unreliable on the last day of the month and the year
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
            break;
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

/* 
    Convert a timestamp to GMT timestamp
*/
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
    Convert a DateTime to GMT DateTime
*/
DateTime GB_DS3231:: converttogmt(String localtimezone, DateTime dt) {
    int offsethour = 0, offsetminutes = 0, offsetsign = 1;
    localtimezone.trim();
    localtimezone.toUpperCase();
    
    for (const auto& info : this->tzoffsets) {
        if (strcmp(localtimezone.c_str(), info.timezone) == 0) {
            offsethour = info.offsetHours;
            offsetminutes = info.offsetMinutes;
            offsetsign = info.sign;

            TimeSpan span(offsetsign * (offsethour * 3600 + offsetminutes * 60));
            dt = dt.operator-(span);
            break;
        }
    }

    return dt;
}

/* 
    Convert timezones for a time string
    * May not work on the last day of the month
    * Unreliable on the last day of the month and the year
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

/* 
    Convert timezones for a timestamp
*/
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

/* 
    Convert timezones for a DateTime
*/
DateTime GB_DS3231:: converttotz(String localtimezone, String targettimezone, DateTime dt) {
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
            
            TimeSpan span(localtzsign * (localtzhour * 3600 + localtzminutes * 60));
            dt = dt.operator-(span);
        }
        if (strcmp(targettimezone.c_str(), info.timezone) == 0) {
            targettzhour = info.offsetHours;
            targettzminutes = info.offsetMinutes;
            targettzsign = info.sign;

            TimeSpan span(targettzsign * (targettzhour * 3600 + targettzminutes * 60));
            dt = dt.operator+(span);
        }
    }

    return dt;
}

/*
    Returns 'now' Datetime object
*/
DateTime GB_DS3231::now() {

    DateTime dt;

    // Get timestamp from RTC
    bool valid = false;
    if (this->device.detected) {
                
        this->_source = "rtc";
        this->on();
        delay(50);
        dt = _rtc.now(); 
        this->off();

        valid = this->valid(dt);
    }

    // If RTC was not detected, not initialized or provides an invalid timestamp, fallback to  MODEM
    if (!this->device.detected || !valid) {
                
        this->_source = "modem";
        int counter = 50;
        while (!MODEM_INITIALIZED && counter-- >= 0) { MODEM_INITIALIZED = MODEM.begin() == 1; delay(250); }
        if(MODEM_INITIALIZED) {
            String nwtimestr = _gb->getmcu().gettime();

            int year, month, day, hour, minute, second, offset;
            sscanf(_gb->s2c(nwtimestr), "\"%d/%d/%d,%d:%d:%d-%d\"", &year, &month, &day, &hour, &minute, &second, &offset);
            dt = DateTime(year, month, day, hour, minute, second);
        }
        else {
            return DateTime(0);
        }
    }

    return dt;
}

/*
    Check if the device is running/communicating properly
*/
bool GB_DS3231::valid() {
    this->on(); delay(50);
    if (!this->_rtc.begin()) return false;
    DateTime dt = _rtc.now();
    return this->valid(dt);
}

/*
    Check if the timestamp provided is likely accurate
*/
bool GB_DS3231::valid(uint32_t timestamp) {
    DateTime dt = DateTime(timestamp);
    return this->valid(dt);
}

/*
    Check if the DateTime provided is likely accurate
*/
bool GB_DS3231::valid(DateTime dt) {
    
    bool error = false;
    uint32_t unixtime = dt.unixtime();
    uint16_t month = dt.month();
    uint16_t year = dt.year();

    if (unixtime >= 2000000000) error = true;
    else if (unixtime <= 946684800) error = true;
    else if (month == 165) error = true;
    else if (unixtime > 1700000000) error = false;
    else error = true;

    return !error;
}

/*
    Returns unix timestamp from either RTC or MODEM
*/
String GB_DS3231::timestamp() {
    
    DateTime time = this->now();
    uint32_t unixtime = time.unixtime();

    // // Convert the MODEM's timestamp to GMT
    // if (this->_source == "modem") {
    //     unixtime = this->converttogmt(this->timezone, unixtime);
    // }

    #ifdef false // Keep for documentation
        // Check if the time is invalid
        DateTime time;
        bool invalid = true;
        int counter = 0;
        
        // Get time from the RTC
        while (invalid && counter++ <= 4) {
            this->on(); time = _rtc.now(); this->off();
            unixtime = time.unixtime();
            // invalid = !this->valid();
            if (invalid) _gb->log("Invalid timestamp detected: " + String(unixtime));
            delay(250 + (counter * 250));
        }

        // Get timestamp from the time
        if (!invalid) unixtime = time.unixtime();
        else unixtime = _gb->globals.INIT_SECONDS + (millis() / 1000);
    #endif

    return String(unixtime);
}

/* 
    Returns date string in default format.
    Format: MM/DD/YY
*/
String GB_DS3231::date() {
    return this->date("MM/DD/YY");
}

/* 
    Returns date string in specified format.
*/
String GB_DS3231::date(String format) {

    DateTime dt = this->now();
    
    // // Adjust MODEM time to GMT
    // if (this->_source == "modem") {
    //     for (const auto& info : this->tzoffsets) {
    //         if (strcmp(this->timezone.c_str(), info.timezone) == 0) {
    //             int offsethour = info.offsetHours;
    //             int offsetminutes = info.offsetMinutes;
    //             int offsetsign = info.sign;
    //             TimeSpan span(offsetsign * (offsethour * 3600 + offsetminutes * 60));
    //             dt = dt.operator-(span);
    //             break;
    //         }
    //     }
    // }

    #if not defined (LOW_MEMORY_MODE)

        // Format month
        String monthsshort[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        String monthsfull[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
        format.replace("MMMM", monthsfull[dt.month() - 1]);
        format.replace("MMM", monthsshort[dt.month() - 1]);
        format.replace("MM", String(dt.month()));
        
        // Format date
        String dateprefix[] = {"st", "nd", "rd", "th" };
        format.replace("DD", String(dt.day()));
        format.replace("th", dt.day() > 0 && dt.day() < 3 ? dateprefix[dt.day() - 1] : dateprefix[3]);

        // Format year
        format.replace("YYYY", String(dt.year()));
        format.replace("YY", String(dt.year()).substring(String(dt.year()).length() - 2, String(dt.year()).length()));
    #else
        format = String(dt.month()) + "/" + String(dt.day()) + "/" + String(dt.year());
    #endif

    return String(format);
}

/* 
    Return the source of time (RTC or MODEM)
*/
String GB_DS3231::getsource() {
    return this->_source;
}

/* 
    Returns time string in default format.
    Default: hh:mm:ss
*/
String GB_DS3231::time() {
    return this->time("hh:mm:ss");
}

/* 
    Returns time string in specified format.
*/
String GB_DS3231::time(String format) {
    
    DateTime time = this->now();

    // // Adjust MODEM time to GMT
    // if (this->_source == "modem") {
    //     for (const auto& info : this->tzoffsets) {
    //         if (strcmp(this->timezone.c_str(), info.timezone) == 0) {
    //             int offsethour = info.offsetHours;
    //             int offsetminutes = info.offsetMinutes;
    //             int offsetsign = info.sign;
    //             TimeSpan span(offsetsign * (offsethour * 3600 + offsetminutes * 60));
    //             time = time.operator-(span);
    //             break;
    //         }
    //     }
    // }

    // Convert to requested string format
    bool hr12 = format.indexOf("a") > -1; int hour;
    if (hr12) hour = time.hour() < 12 ? time.hour() : time.hour() - 12; 
    else hour = time.hour();

    format.replace("hh", hour < 10 ? "0" + String(hour) : String(hour));
    format.replace("mm", (time.minute() < 10 ? "0" : "") + String(time.minute()));
    format.replace("ss", (time.second() < 10 ? "0" : "") + String(time.second()));
    format.replace("a", (time.hour() > 12 ? "PM" : "AM"));
    
    return String(format);
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

#endif