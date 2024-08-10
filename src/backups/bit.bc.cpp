#include "platform.h"

#if GB_PLATFORM_IS_CHMD_OLD

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

    GB_AHT10 aht(gb);
    GB_DESKTOP gdc(gb);
    GB_SNTL sntl(gb);

    GB_RELAY vst(gb);
    GB_EADC wlev(gb);
    GB_TPBCK rain(gb);

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

    String STATE = "IDLE";
    int RAINID = 0;
    int HOURID = -1;
    int TIMETRACKER = 0;
    int TIME = 0;
    int RAININT = 0;
    int EEPROMLOCATION = 11112;
    
    CSVary chomp () {
        
        // Initialize CSVary object
        CSVary csv;
        
        // Trigger autosampler
        vst.trigger(100);

        // Log to SD
        csv
            .clear()
            .setheader("SURVEYID,DEVICEID,TIMESTAMP,TEMP,RH,FLTP,RAINID,HOURID")
            .set(gb.PROJECT_ID)
            .set(gb.DEVICE_ID)
            .set(aht.temperature())
            .set(aht.humidity())
            .set(FAULTS_PRIMARY)
            .set(RAINID)
            .set(HOURID);
        
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

        return csv;







        /*
            ! Send data to server
            Send the queue data first.
            Then send the current data to the server.
        */
        if (sd.getqueuecount() > 0) {

            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files");

            //! Connect to cellular 
            mcu.connect();

            //! Connect to MQTT broker
            mqtt.connect();

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

        return csv;
    }

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
        gb.configure(false, "gb-irrec-test-station");

        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        //! Configure peripherals
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        buzzer.configure({6}).initialize().play("...");

        sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter").readconfig();

        //! Configure MQTT broker and connect 
        mqtt.configure("mqtt.ezbean-lab.com", 1883, gb.DEVICE_ID, mqtt_message_handler, mqtt_on_connect);

        // Initialize Sentinel
        sntl.configure(9).initialize();
        sntl.interval("sentinence", 15).enable();
        
        mem.configure({true, SR0}).initialize();
        aht.configure({true, SR0}).initialize();
        rtc.configure({true, SR0}).initialize();
        
        vst.configure({true, SR5}).initialize();
        wlev.configure({true, SR2}).initialize();
        
        sntl.disable();

        // Detect GDC
        gdc.detect();

        gb.log("Setup complete");
    }

    void loop () {

        // wlev.getreading(ADS1115_MUX_AIN0_GND);

        //! Set device mode
        gb.MODE = "read";
        
        // //! Call GatorByte's loop procedure
        // gb.loop();
        
        /*
            ! Check the current state of the system and take actions accordingly
            Possible states:
                1. IDLE - No rain in the last 12 hours
                2. ACTIVE - Rain was detected within the last 12 hours

            State variables:
                1. RAINID - The ID of current rain event and corresponding samples (0, 1, 2, 3, ...)
                2. TIME - Time elapsed since the current rain event was first detected (in hours)
                3. RAININT - Rain intesity for the current rain event (in inches)

        */

        if (STATE == "IDLE") {
            
            // Check if a rain is happening right now
            bool detected = rain.listener();

            // If rain is detected, set up new state, start the times, take samples, etc.
            if (detected) {

                gb.log("Rain detected. Starting the timers and logging.");
                
                // Set states
                STATE = "ACTIVE";
                TIMETRACKER = rtc.timestamp().toInt();
                TIME = 0;
                RAININT = 0.1;

                // Update/set rain id variable and in EEPROM
                if (mem.get(EEPROMLOCATION) == "") mem.write(EEPROMLOCATION, 0);
                RAINID = mem.get(EEPROMLOCATION).toInt() + 1;
                mem.write(EEPROMLOCATION, String(RAINID));
            }
        }

        else  if (STATE == "ACTIVE") {
            
            // Check if a rain is happening right now
            bool detected = rain.listener();
            
            // Get rain id from EEPROM
            RAINID = mem.get(EEPROMLOCATION).toInt();

            // Calculate time (hours) since the current rain event was first detected
            TIME = (rtc.timestamp().toInt() - TIMETRACKER) / 3600 / 1000;

            gb.log("System active", false);
            gb.log(" -> RAINID: " + String(RAINID), false);
            gb.log(" -> TIMETRACKER: " + String((rtc.timestamp().toFloat() - TIMETRACKER) / 3600 / 1000));

            // Create a CSVary instance
            CSVary csv;

            // If 12 hours have passed since the rain event and all 5 samples have been taken.
            if (TIME >= 12 && HOURID == TIME) {

                // Reset variables
                STATE = "IDLE";
                HOURID = -1;
                TIMETRACKER = rtc.timestamp().toInt();
                TIME = 0;

            }

            // Take the 4th sample (at 12th hour)
            else if (TIME == 12 && HOURID != TIME) {
                HOURID = TIME;

                gb.log("Taking 4th sample");

                // Take action - trigger autosampler, log to SD, etc.
                csv = chomp();
            }

            // Take the 3rd sample (at 9th hour)
            else if (TIME == 9 && HOURID != TIME) {
                HOURID = TIME;

                gb.log("Taking 3rd sample");

                // Take action
                csv = chomp();
            }

            // Take the 2nd sample (at 6th hour)
            else if (TIME == 6 && HOURID != TIME) {
                HOURID = TIME;

                gb.log("Taking 2nd sample");

                // Take action
                csv = chomp();
            }

            // Take the 1st sample (at 3rd hour)
            else if (TIME == 3 && HOURID != TIME) {
                HOURID = TIME;

                gb.log("Taking 1st sample");

                // Take action
                csv = chomp();
            }

            // Take the 0th sample (at 0th hour)
            else if (TIME == 0 && HOURID != TIME) {
                HOURID = TIME;

                gb.log("Taking 0th sample");

                // Take action
                csv = chomp();
            }

        }


        // //! Sleep
        // mcu.sleep(on_sleep, on_wakeup);
    }

    void breathe() {
        gb.log("Breathing");
        gb.breathe();
    }

#endif