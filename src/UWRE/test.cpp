#include "../platform.h"

#if GB_PLATFORM_IS_BUOY_TEST

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

    GB_HTTP http(gb);
    GB_MQTT mqtt(gb);
    GB_74HC595 ioe(gb);
    GB_TCA9548A tca(gb);
    GB_SERMUX sermux(gb);

    // Peripherals and devices
    GB_RGB rgb(gb);
    GB_PWRMTR pwr(gb);
    GB_AT24 mem(gb);
    GB_SD sd(gb);
    // GB_FRAM fram(gb);
    GB_DS3231 rtc(gb);
    GB_BOOSTER booster(gb);
    GB_NEO_6M gps(gb);
    GB_AT_09 bl(gb);
    // GB_MPU6050 acc(gb);
    GB_BUZZER buzzer(gb);

    GB_RELAY vst(gb);
    GB_EADC eadc(gb);
    GB_USS uss(gb);
    GB_TPBCK rain(gb);

    // Atlas Scientific sensors
    GB_AT_SCI_DO dox(gb);
    GB_AT_SCI_RTD rtd(gb);
    GB_AT_SCI_EC ec(gb);
    GB_AT_SCI_PH ph(gb);

    GB_AHT10 aht(gb);
    GB_CONSOLE cm(gb);
    GB_CONFIGURATOR cfg(gb);
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

        else if (topic == "gb-lab/calibration/perform") {

            /* Payload (Message) format
            [sensor]:[step]
            */

            gb.globals.MODE = "calibrate";
            String sensor = gb.split(message, ':', 0);
            int step = gb.split(message, ':', 1).toInt();

            gb.log("Calibration command received for: " + sensor);

            String response;
            // if (sensor == "ec") response = ec.calibrate(step);
            // else if (sensor == "ph") response = ph.calibrate(step);
            // else if (sensor == "dox") response = dox.calibrate(step);
            // else if (sensor == "rtd") response = rtd.calibrate(step);

            gb.log(message + "::" + response);

            mqtt.publish("gb-aws-server/calibration/response", message + "::" + response);
        }

        else if (topic == "gb-lab/calibration/abort") {
            gb.globals.MODE = "read";
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
        gb.log("The device is now awake.");
        gdc.send("highlight-yellow", "Device awake");

        // Connect to cellular network    
        mcu.connect();

        // Connect to MQTT broker
        mqtt.connect();

        // bl.on();
    }

    /*
        ! Set date and time for files in the SD card
    */
    void setsddatetime(uint16_t* date, uint16_t* time) {

        // Get datetime object
        DateTime now = rtc.now();

        // return date using FAT_DATE macro to format fields
        *date = FAT_DATE(now.year(), now.month(), now.day());

        // return time using FAT_TIME macro to format fields
        *time = FAT_TIME(now.hour(), now.minute(), now.second());
    }

    #include "RS485.h"
    RS485 Sensor;
    
    /* 
        ! Peripherals configurations and initializations
        Here, objects for the peripherals/components used are initialized and configured. This is where
        you can specify the enable pins (if used), any other pins that are being used, or whether an IO Expander is being used.

        Microcontroller configuration also includes initializing I2C, serial communications and specifying baud rates, specifying APN 
        for cellular communication or WiFi credentials if WiFi is being used.
    */
    void setup () {
        
        // Mandatory GB setup function
        gb.setup();

        //! Initialize GatorByte and device configurator
        gb.configure(false, "gb-lab-buoy");
        gb.globals.ENFORCE_CONFIG = false;
        
        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        delay(4000);
        gdc.detect(false);

        booster.configure({true, SR1}).on();

        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();
        tca.configure({A2}).initialize();
        // sermux.configure().initialize();
        
        //! Initialize essentials
        sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter").readconfig().on();
        // fram.configure({true, SR15, 7, SR4}).initialize("quarter");

        buzzer.configure({6}).initialize().play("...");
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");

        // // ussconfigure({true, SR7, A6}).initialize();
        
        // //! Initialize Sentinel
        // sntl.configure({true, 4}, 9).initialize().ack(false).enablebeacon();

        // // sntl.readmemory(0);
        // // sntl.readmemory(1);
        // // for (int i = 0; i < 15; i++) {
        // //     sntl.readmemory(i);
        // //     delay(250);
        // // }

        //! Initialize other peripherals
        bl.configure({true, SR3, SR11}).initialize();
        gps.configure({true, SR2, SR10}).initialize();

        //! Configure sensors
        rtd.configure({true, SR6, true, 0}).initialize(true);
        dox.configure({true, SR8, true, 2}).initialize(true);
        ec.configure({true, SR9, true, 3}).initialize(true);
        ph.configure({true, SR7, true, 1}).initialize(true);
        
        mem.configure({true, SR0}).initialize();
        aht.configure({true, SR0}).initialize();
        rtc.configure({true, SR0}).initialize();
        
        // vst.configure({true, SR5}).initialize();
        // eadc.configure({true, SR7}).initialize();
        // rain.configure({false, A2}).initialize();
        
        // sermux.createchannel({true, SR7}, 1);
        // sermux.selectexclusive(1);

        gb.log("Setup complete");

        // sntl.shutdown();
    }

    int count = 0;
    void loop () {

        // rtd.initialize(true).on(); dox.off(); delay(5000);
        // rtd.off(); dox.on(); delay(5000);

        // return;

        //! Call GatorByte's loop procedure
        gb.loop();

        // bl.listen([] (String command) {
        //     Serial.println("Received command: " + command);

        // });

    }

    void breathe() {
        gb.breathe();
    }

#endif