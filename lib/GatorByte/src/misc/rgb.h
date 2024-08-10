#ifndef GB_RGB_h
#define GB_RGB_h
#include <vector>

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
        GB_RGB& on(uint8_t identifer);
        GB_RGB& on(uint8_t r, uint8_t g, uint8_t b);
        GB_RGB& on(String color, float brightness);
        GB_RGB& on(String color);
        GB_RGB& off();
        GB_RGB& wait(int delay);
        
        bool testdevice();
        String status();

        GB_RGB& blink(String, uint8_t, uint16_t, uint16_t);
        GB_RGB& rainbow(uint32_t milliseconds);


        struct colorinfo  {
            int identifier;
            const char* name;
        };
        const std::vector<colorinfo> colormap = {
            {1, "red"},
            {2, "green"},
            {3, "blue"},

            {4, "cyan"},
            {5, "magenta"},
            {6, "yellow"},
            {7, "grass"},
            {8, "white"},

        };

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

/*
    Set color using the colors' identifiers
*/
GB_RGB& GB_RGB::on(uint8_t identifier) {
    for (const auto& color : this->colormap) {
        if (identifier == color.identifier) {
            this->on(color.name);
            break;
        }
    }
    return *this;
}

/*
    Set color using the individual RGB values (0-255)
*/
GB_RGB& GB_RGB::on(uint8_t red, uint8_t green, uint8_t blue) {
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
    this->on(1); delay(200);
    this->on(2); delay(200);
    this->on(3); delay(200);

    // Blink white 2 times
    this->on(8); delay(200);
    this->off();

    return *this;
}

// Test the device
bool GB_RGB::testdevice() { 
    
    _gb->log("Testing " + device.id + ": " + String(this->device.detected));

    // Cycle through primary colors
    this->on(1); delay(550);
    this->on(2); delay(550);
    this->on(3); delay(550);
    this->off();

    // Revert
    if (_gb->globals.GDC_CONNECTED) this->on(4);
    else this->on(5);

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

GB_RGB& GB_RGB::blink(String color, uint8_t count, uint16_t on_duration, uint16_t off_duration) {

    this->off(); delay(200);
    for (uint8_t i = 0; i < count; i++) {
        this->on(color); delay(on_duration);
        if (i < count - 1) this->off(); 
        delay(off_duration);
    }
    this->off(); delay(100);
    return *this;
}

GB_RGB& GB_RGB::rainbow(uint32_t milliseconds) {

    uint32_t now = millis();
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