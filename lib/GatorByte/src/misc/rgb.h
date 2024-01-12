#ifndef GB_RGB_h
#define GB_RGB_h

#ifndef GB_h
    #include "../GB.h"
#endif

class GB_RGB : public GB_DEVICE {
    public:
        GB_RGB(GB &gb);

        DEVICE device = {
            "rgb",
            "RGB LED"
        };

        struct PINS {
            int red;
            int green;
            int blue;
        } pins;

        GB_RGB& initialize();
        GB_RGB& initialize(float);
        GB_RGB& configure(PINS);
        GB_RGB& revert();
        GB_RGB& on(int r, int g, int b);
        GB_RGB& on(String color, float brightness);
        GB_RGB& on(String color);
        GB_RGB& off();
        GB_RGB& wait(int delay);
        
        bool testdevice();
        String status();

        GB_RGB& blink(String, int, int, int);
        GB_RGB& rainbow(int milliseconds);

    private:
        GB *_gb;
        struct STATE {
            float red;
            float green;
            float blue;
            float brightness;
        };
        GB_RGB& _set(STATE);

        STATE _state = {0,0,0,0};
        STATE _prevstate = {0,0,0,0};
};

/* Constructor */ 
GB_RGB::GB_RGB(GB &gb) {
    _gb = &gb;
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.rgb = this;
}

GB_RGB& GB_RGB::configure(PINS pins) {
    this->pins = pins;
    
    // Set pin modes
    pinMode(this->pins.red, OUTPUT);
    pinMode(this->pins.green, OUTPUT);
    pinMode(this->pins.blue, OUTPUT);

    return *this;
}

GB_RGB& GB_RGB::_set(STATE state) {
    analogWrite(this->pins.red, state.red * 255 * state.brightness);
    analogWrite(this->pins.green, state.green * 255 * state.brightness);
    analogWrite(this->pins.blue, state.blue * 255 * state.brightness);
    this->_state = state;
    
    return *this;
}

GB_RGB& GB_RGB::revert() {
    this->_set(this->_prevstate);
    return *this;
}

GB_RGB& GB_RGB::on(int red, int green, int blue) {
    analogWrite(this->pins.red, red);
    analogWrite(this->pins.green, green);
    analogWrite(this->pins.blue, blue);
    return *this;
}

GB_RGB& GB_RGB::initialize() { this->initialize(1); }
GB_RGB& GB_RGB::initialize(float brightness) { 
    _gb->init();
    
    this->device.detected = true;
    
    // Add the device to included devices list
    _gb->includedevice(this->device.id, this->device.name);

    brightness = brightness * 255;
    if (brightness > 255) brightness = 255;
    if (brightness < 0) brightness = 0;

    // Cycle through primary colors
    this->on("red", brightness); delay(300);
    this->on("green", brightness); delay(300);
    this->on("blue", brightness); delay(300);

    // Blink white 2 times
    this->on("white", brightness); delay(200);
    this->off(); delay(100);
    this->on("white", brightness); delay(200);
    this->off(); delay(200);

    return *this;
}

// Test the device
bool GB_RGB::testdevice() { 
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));

    // Cycle through primary colors
    this->on("red", 255); delay(550);
    this->on("green", 255); delay(550);
    this->on("blue", 255); delay(550);
    this->off();

    // Revert
    if (_gb->globals.GDC_CONNECTED) this->on("cyan", 255);
    else this->on("magenta", 255);

    return this->device.detected;
}

String GB_RGB::status() { 
    return this->device.detected ? "detected" : "not-detected" + String(":") + device.id;
}

GB_RGB& GB_RGB::on(String color) { this->on(color, 80); return *this;}
GB_RGB& GB_RGB::on(String color, float brightness) {

    this->_prevstate = this->_state;

    // Calculate brightness
    if (brightness > 255) brightness = 255;
    if (brightness < 0) brightness = 0;
    brightness = brightness / 255.0;

    this->off();
    if (color == "red") {
        this->_set({1, 0, 0, brightness});
    }
    else if (color == "green") {
        this->_set({0, 1, 0, brightness});
    }
    else if (color == "blue") {
        this->_set({0, 0, 1, brightness});
    }
    else if (color == "white") {
        this->_set({1, 1, 1, brightness});
    }

    // The following colors are not available in low power mode
    #if not defined (LOW_MEMORY_MODE)
        else if (color == "cyan") {
            this->_set({0, 1, 1, brightness});
        }
        else if (color == "magenta") {
            this->_set({1, 0, 1, brightness});
        }
        else if (color == "grass") {
            this->_set({0, 1, 20/255, brightness});
        }
        else if (color == "yellow") {
            this->_set({1, 1, 0, brightness});
        }
    #endif
    return *this;
}

GB_RGB& GB_RGB::off() {
    this->_set({this->_state.red, this->_state.green, this->_state.blue, 0});
    return *this;
}

GB_RGB& GB_RGB::blink(String color, int count, int on_duration, int off_duration) {

    _gb->getdevice("rgb").off(); delay(200);
    for (int i = 0; i < count; i++) {
        _gb->getdevice("rgb").on(color); delay(on_duration);
       if (i < count - 1) _gb->getdevice("rgb").off(); delay(off_duration);
    }
    _gb->getdevice("rgb").off(); delay(100);
    return *this;
}

GB_RGB& GB_RGB::rainbow(int milliseconds) {

    int now = millis();
    while (true) {
        
        // milliseconds = 0 means infinite duration
        if (milliseconds > 0 && millis() - now >= milliseconds) break;
        else {

            // // Loop mqtt     
            // if (this->_gb->hasdevice("mqtt")) this->_gb->getdevice("mqtt").update();
        }

        //! Fade RGB light
        unsigned int rgbColour[3];

        // Start off with red.
        rgbColour[0] = 255;
        rgbColour[1] = 0;
        rgbColour[2] = 0;  

        // Choose the colours to increment and decrement.
        for (int decColour = 0; decColour < 3; decColour += 1) {
            int incColour = decColour == 2 ? 0 : decColour + 1;

            // cross-fade the two colours.
            for(int i = 0; i < 255; i += 1) {
                rgbColour[decColour] -= 1;
                rgbColour[incColour] += 1;
                
                this->on(rgbColour[0], rgbColour[1], rgbColour[2]);
                delay(5);
            }
        }
    }
    
    this->off(); delay(100);
    return *this;
}

GB_RGB& GB_RGB::wait(int milliseconds) {
    delay(milliseconds);
    return *this;
}

#endif

/* HEX to r,g,b
    (rgb>>16)&0xFF, (rgb>>8)&0xFF, (rgb)&0xFF
*/