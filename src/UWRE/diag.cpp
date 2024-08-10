#include "../platform.h"

#if GB_PLATFORM_IS_DIAG
    
    /* 
        ! Gatorbyte library
        Import the GatorByte library (GBL).
    */
    #include "GB.h"


    /* 
        ! Gatorbyte core instance
        This is a virtual twin of the GatorByte datalogger. All the peripherals and sensors 
        will be "in" this virtual twin. Pass this object to the peripherals/sensors as the
        first parameter.
    */
    GB gb = GB();

    /* 
        ! Peripherals and sensors (devices) instances
        Include the instances for the devices that your GatorByte physically 
        has. Include the microcontroller, a communication protocol, and the other devices. 
    */

    // Microcontroller and communications
    GB_NB1500 mcu(gb);

    GB_MQTT mqtt(gb);
    GB_74HC595 ioe(gb);

    // Peripherals and devices
    GB_RGB rgb(gb);
    GB_PWRMTR pwr(gb);
    GB_AT24 mem(gb);
    GB_SD sd(gb);
    GB_DS3231 rtc(gb);
    GB_BOOSTER booster(gb);
    GB_AT_09 bl(gb);
    GB_BUZZER buzzer(gb);
    GB_NEO_6M gps(gb);

    GB_AHT10 aht(gb);
    GB_DESKTOP gdc(gb);
    GB_SNTL sntl(gb);

    /* 
        ! Procedure to handle incoming MQTT messages
        It is mandatory to define this function if GB_MQTT library is instantiated above
    */

    void mqtt_message_handler(String topic, String message) {

        gb.log("\n\nIncoming topic: " + topic + "\n\n");

        if (topic == "gb-lab/test") {
            mqtt.publish("gatorbyte::gb-lab::response", "message");
        }
    }

    /* 
        ! Actions to perform on MQTT connect/reconnect
        It is mandatory to define this function if GB_MQTT library is instantiated above
    */

    void mqtt_on_connect() {

        mqtt.subscribe("gb-lab/test");
    }

    /* 
        ! Actions to perform before going to sleep
        You can turn specific components on/off. By default all components are turned off before
        entering power-saving mode (sleep)
    */

    void on_sleep() {
        gb.log("\nPerforming user-defined pre-sleep tasks");

        // mqtt.disconnect();
        mcu.disconnect("cellular");

        // Disable timer
        mcu.stopbreathtimer();

        delay(1000);
    }

    /* 
        ! Actions to perform after the microcontroller wakes up
        You can turn specific components on/off.
    */

    void on_wakeup() {
        
        rgb.on("magenta");
        delay(5000);

        gb.log("The device is now awake.");
        gdc.send("highlight-yellow", "Device awake");
    }

    
    void nwdiagnostics () {
        
        sntl.watch(45, [] {
            //! Connect to network
            mcu.connect("cellular");

            sntl.kick();

            //! Connect to MQTT server
            mqtt.connect();
        });

        rgb.rainbow(5000);
    }

    /*
        ! Diagnostics function
    */
    bool flag = false;
    void diagnostics () {
        flag = true;

        // Test RGB & buzzer
        rgb.rainbow(5000).off(); buzzer.play(".-"); delay(2000);

        // Test SD
        rgb.on(sd.rwtest() ? "green" : "red"); delay(1000); rgb.off();

        // Test RTC
        rgb.on(rtc.test() ? "green" : "red"); delay(1000); rgb.off();

        // Test sentinel
        rgb.on(sntl.ping() ? "green" : "red"); delay(1000); rgb.off();

        // Test GPS
        gps.on(); delay(2000); gps.off(); delay(1000);

    }

    /* 
        ! Peripherals configurations and initializations
        Here, objects for the peripherals/components used are initialized and configured. This is where
        you can specify the enable pins (if used), any other pins that are being used, or whether an IO Expander is being used.

        Microcontroller configuration also includes initializing I2C, serial communications and specifying baud rates, specifying APN 
        for cellular communication or WiFi credentials if WiFi is being used.
    */
    void setup () {

        delay(2000);
        
        //! Mandatory GB setup function
        gb.setup();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");
        
        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        buzzer.configure({6}).initialize().play(".");

        mcu.checklist();
        gb.log(mcu.getimei());
        
        // //! Set Twilio APN
        mcu.apn("super");
        mcu.connect("cellular");
        
        // //! Initialize Sentinel
        // sntl.configure(9).initialize();

        // // Initialize SD first to read the config file
        // sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter").readconfig();
        
        // Initialize remaining peripherals
        gps.configure({true, SR2, SR10}).initialize();
        mem.configure({true, SR0}).initialize();
        aht.configure({true, SR0}).initialize();
        rtc.configure({true, SR0}).initialize();
        
        // Detect GDC
        gdc.detect();

        gb.log("Setup complete");
    }

    void loop () {

        gdc.detect();
        aht.on();
        mem.on();
        rtc.on();

        // gb.log(rtc.time());
        // gb.log(rtc.date("MM/DD/YYYY"));

        // return nwdiagnostics();

        // if (!flag) return diagnostics();

        delay(10000);
    }

    void breathe() {
        gb.log("Breathing");
        gb.breathe();
    }

#endif