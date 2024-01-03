#include "platform.h"

#if GB_PLATFORM_IS_DRIFTER

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
    GB_NEO_6M gps(gb);
    GB_AT_09 bl(gb);
    GB_MPU6050 acc(gb);
    GB_BUZZER buzzer(gb);

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

    /* 
        ! Peripherals configurations and initializations
        Here, objects for the peripherals/components used are initialized and configured. This is where
        you can specify the enable pins (if used), any other pins that are being used, or whether an IO Expander is being used.

        Microcontroller configuration also includes initializing I2C, serial communications and specifying baud rates, specifying APN 
        for cellular communication or WiFi credentials if WiFi is being used.
    */
    void setup () {
        
        // .\bossac.exe --port COM19 --write --verify --reset firmware.bin

        // Mandatory GB setup function
        gb.setup();

        //! Initialize GatorByte and device configurator
        gb.configure(false, "gb-lab-buoy");
        
        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", ""); 

        //! Configure peripherals
        gps.configure({true, SR2}).initialize();
        bl.configure({true, SR3}).initialize();

        buzzer.configure({6}).initialize().play("x.x..");
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        booster.configure({true, SR1}).on();
        sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter").readconfig();
        mem.configure({true, SR0}).initialize();
        aht.configure({true, SR0}).initialize(); 
        rtc.configure({true, SR0}).initialize();
        acc.configure({true, SR5}).initialize();

        //! Configure MQTT broker and connect 
        mqtt.configure("mqtt.ezbean-lab.com", 1883, gb.DEVICE_ID, mqtt_message_handler, mqtt_on_connect);

        // Detect GDC
        gdc.detect();

        gb.log("Setup complete");
    }


    void loop () {

        gb.MODE = "read";
        
        //! Call GatorByte's loop procedure
        gb.loop();
        
        gb.log(gb.SLEEP_DURATION / 1000);
        gb.log(LOOPTIMESTAMP - LAST_READING_TIMESTAMP);
        gb.log(READINGS_DUE);

        // Take readings if due
        if (READINGS_DUE) {

            gb.log("Readings are due. Taking next set of readings");

            //! Get GPS data
            GPS_DATA gps_data = gps.read();
            String gps_lat = String(gps_data.lat, 5), gps_lng = String(gps_data.lng, 5), gps_attempts = String(gps_data.last_fix_attempts);
            gps.fix() ? rgb.on("blue") : rgb.on("red");
            gps.off();

            //! Log GPS data to console
            gb.log("GPS: " + String(gps.fix() ? "Located" : "Failed") + ": " + gps_lat + ", " + gps_lng);

            gb.log("Constructing CSV object", false);

            //! Construct CSV data object
            CSVary csv;
            csv
                .clear()
                .setheader("SURVEYID,LAT,LNG,TIMESTAMP,TEMP,RH,GPSATTEMPTS,CELLSIGSTR")
                .set(gb.MODE == "dummy" ? gb.MODE : gb.PROJECT_ID)
                .set(gps_lat)
                .set(gps_lng)
                .set(rtc.timestamp())
                .set(aht.temperature())
                .set(aht.humidity())
                .set(gps_attempts)
                .set(String(mcu.RSSI));
            
            gb.log(" -> Done");
            
            gb.br().log("Current data point: ");
            gb.log(csv.getheader());
            gb.log(csv.getrows());

            //! Write current data to SD card
            sd.writeCSV(csv);
            gb.log("Readings written to SD card");
        
            //! Upload current data to desktop client_test
            gdc.send("data", csv.getheader() + BR + csv.getrows());
            
            /*
                ! Prepare a queue file (with current iteration's data)
                The queue file will be read and data uploaded once the network is established.
                The file gets deleted if the upload is successful. 
            */

            String currentdataqueuefile = sd.getavailablequeuefilename();
            sd.writeString(currentdataqueuefile, csv.get());

            LAST_READING_TIMESTAMP = rtc.timestamp().toInt();
            mem.write(12, String(LAST_READING_TIMESTAMP));
        }
        else {
            gb.log("Readings not due this iteration.");
        }

        /*
            ! Send data to server
        */
        if (sd.getqueuecount() > 0) {

            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files");

            //! Connect to cellular 
            mcu.connect();

            //! Connect to MQTT broker
            mqtt.connect("pi", "abe-gb-mqtt");

            gb.log("MQTT connection attempted");
            
            mqtt.publish("log/message", "Iteration: " + String(gb.ITERATION));
            mqtt.publish("log/message", "Cell signal: " + String(mcu.RSSI) + " dB");
            mqtt.publish("log/message", "SD write complete");

            //! Publish first ten queued-data with MQTT
            if (CONNECTED_TO_MQTT_BROKER) {
                int counter = 10;
                while (!sd.isqueueempty() && counter-- > 0) {

                    String queuefilename = sd.getfirstqueuefilename();

                    // Attempt publishing queue-data
                    if (mqtt.publish("data/set", sd.readfile(queuefilename))) {

                        // Remove queue file
                        sd.removequeuefile(queuefilename);
                    }
                }
            }

            mqtt.publish("log/message", "Sleep initiated");
        }

        //! Sleep
        mcu.sleep(on_sleep, on_wakeup);
    }

    void breathe() {
        gb.breathe();
    }
    
#endif