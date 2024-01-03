#include "../platform.h"

#if GB_PLATFORM_IS_LRY_DRIFTER
    
    bool nwflag = true;
    bool gpsdummyflag = false;

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
        ! Send data to server
        Send the queue data first.
        Then send the current data to the server.
    */
    void send_queue_files_to_server() {

        if (sd.getqueuecount() > 0) {

            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files");

            sntl.shield(45, [] {
                
                //! Connect to network
                mcu.connect("cellular");
                
                sntl.kick();

                //! Connect to MQTT servver
                mqtt.connect("pi", "abe-gb-mqtt");
            });

            // mqtt.publish("log/message", "Iteration: " + String(gb.ITERATION));
            // mqtt.publish("log/message", "Cell signal: " + String(mcu.RSSI) + " dB");
            // mqtt.publish("log/message", "SD write complete");

            //! Publish first ten queued-data with MQTT
            sntl.shield(10, [] {
                if (CONNECTED_TO_MQTT_BROKER) {
                    int counter = 10;
                    while (!sd.isqueueempty() && counter-- > 0) {

                        String queuefilename = sd.getfirstqueuefilename();

                        // Attempt publishing queue-data
                        if (mqtt.publish("data/set", sd.readfile(queuefilename))) {

                            // Remove queue file
                            sd.removequeuefile(queuefilename);
                        }

                        sntl.kick();
                    }
                }
            });

        }
        else {
            gb.log("No files files to upload.");
        }
    }
    
    void nwdiagnostics () {
        
        sntl.shield(45, [] {
            //! Connect to network
            mcu.connect("cellular");

            sntl.kick();

            //! Connect to MQTT server
            mqtt.connect("pi", "abe-gb-mqtt");
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
        
        //! Mandatory GB setup function
        gb.setup();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        buzzer.configure({6}).initialize().play("...");
        
        //! Initialize Sentinel
        sntl.configure({false}, 9).initialize();

        sntl.shield(60, [] {

            // Initialize SD first to read the config file
            sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter").readconfig();

            // Configure MQTT broker and connect 
            mqtt.configure("mqtt.ezbean-lab.com", 1883, gb.globals.DEVICE_SN, mqtt_message_handler, mqtt_on_connect);
            
            // Initialize remaining peripherals
            bl.configure({true, SR3, SR11}).initialize();
            gps.configure({true, SR2, SR10}).initialize();

            mem.configure({true, SR0}).initialize();
            aht.configure({true, SR0}).initialize();
            rtc.configure({true, SR0}).initialize();
        });
        
        // Detect GDC
        gdc.detect();

        gb.log("Setup complete");
    }

    void loop () {

        String read;
        int counter = 0;

        // // BL test
        // counter = 0;
        // read = "EMPTY";
        // bl.on();
        // while(!Serial1.available() && counter++ < 5) delay(1000);
        // if (Serial1.available()) read = Serial1.readString();
        // Serial.println("BL: " + read);
        // bl.off("comm");

        // // GPS test
        // counter = 0;
        // read = "EMPTY";
        // gps.initialize();

        // if (Serial1.available()) read = Serial1.readString();
        // Serial.println("GPS: " + read);
        // gps.off("comm");

        // delay(5000);

        // return;

        // return nwdiagnostics();

        // if (!flag) return diagnostics();

        // return send_queue_files_to_server();
        
        //! Call GatorByte's loop procedure
        gb.loop();

        sntl.shield(180, [] {
        
            /*
                ! Check the current state of the system and take actions accordingly
                Get latest GPS coordinates
            */
            GPS_DATA gpsdata = gps.read(gpsdummyflag);
            String gps_lat = String(gpsdata.lat, 5), gps_lng = String(gpsdata.lng, 5);
            gps.fix() ? rgb.on("blue") : rgb.on("red");

            // Initialize CSVary object
            CSVary csv;
            int timestamp = rtc.timestamp().toInt();

            // Construct CSV object
            csv
                .clear()
                .setheader("SURVEYID,DEVICEID,TEMP,RH,FLTP,TIMESTAMP,LAT,LNG")
                .set(gb.globals.PROJECT_ID)
                .set(gb.globals.DEVICE_ID)
                .set(aht.temperature())
                .set(aht.humidity())
                .set(gb.globals.FAULTS_PRIMARY)
                .set(timestamp)
                .set(gps_lat)
                .set(gps_lng);

            //! Write current data to SD card
            sd.writeCSV(csv);
            gb.log("Readings written to SD card");

            gb.br().log("Current data point: ");
            gb.log(csv.getheader());
            gb.log(csv.getrows());

            //! Upload current data to desktop client_test
            gdc.send("data", csv.getheader() + BR + csv.getrows());
            
            /*
                ! Prepare a queue file (with current iteration's data)
                The queue file will be read and data uploaded once the network is established.
                The file gets deleted if the upload is successful. 
            */

            String currentdataqueuefile = sd.getavailablequeuefilename();
            sd.writeString(currentdataqueuefile, csv.get());
        });

        // Send data to the server
        if (nwflag) send_queue_files_to_server();

        // //! Sleep
        // mcu.sleep(on_sleep, on_wakeup);
        delay(nwflag ? 1000 : 10000);
    }

    void breathe() {
        gb.log("Breathing");
        gb.breathe();
    }

#endif