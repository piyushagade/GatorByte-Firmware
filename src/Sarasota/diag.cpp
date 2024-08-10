#include "../platform.h"

#if GB_PLATFORM_IS_CHMD_DIAG

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
    GB_EADC eadc(gb);
    GB_TPBCK rain(gb);
    
    String STATE = "IDLE";
    int RAINID = 0;
    int HOURID = -1;
    int WLEV = 0;
    int TIMETRACKER = 0;
    int TIME = 0;
    int RAININT = 0;

    /*
        Time trackers
    */
    int CV_LAST_UPLOAD_AT = 0;
    int CV_UPLOAD_INTERVAL = 5 * 60 * 1000;
    int QUEUE_UPLOAD_INTERVAL = 60 * 60 * 1000;
    int QUEUE_LAST_UPLOAD_AT = 0;
    int WLEV_SAMPLING_INTERVAL = 10 * 60 * 1000;
    int WLEV_LAST_SAMPLE_AT = 0;
    int32_t INITSEC = 0;

    /*
        Timestamps
    */
    int FIRST_TIP_AT_TIMESTAMP = 0;
    int LAST_TIP_AT_TIMESTAMP = 0;
    int TREATMENT_STARTED_AT_TIMESTAMP = 0;

    /*
        Counts
    */
    int CUMULATIVE_TIP_COUNT = 0;
    
    /*
        Thresholds
    */
    int END_TIP_CONT_THRESHOLD = 3;
    int MIN_TIP_CONT_THRESHOLD = 10;
    
    /*
        Timeouts
    */
    int INTER_TIP_TIMEOUT = 6 * 60 * 60 * 1000;
    int HOMOGENIZATION_DELAY = 6 * 60 * 60 * 1000;
    int INTER_SAMPLE_DURATION = 3 * 60 * 60 * 1000;

    /*
        Miscellaneous
    */
    int MEMLOC_RAINID = 32;
    int MEMLOC_LASTSTATE = 33;
    int MEMLOC_CUMMTIPCOUNT = 34;
    int MEMLOC_TRTSTRTTMSTP = 35;

    void resetvariables () {

        gb.log("Resetting variables -> Done");

        mem.write(MEMLOC_LASTSTATE, "");
        mem.write(MEMLOC_CUMMTIPCOUNT, "0");
        mem.write(MEMLOC_RAINID, "0");
        mem.write(MEMLOC_TRTSTRTTMSTP, "0");

        String filename = "/readings/" + (gb.DEVICE_NAME.length() > 0 ? gb.DEVICE_NAME + "_" : "") + "readings.csv";
        sd.rm(filename);
    }

    void testvariables () {
        /*
            Time trackers
        */
        CV_LAST_UPLOAD_AT = 0;
        CV_UPLOAD_INTERVAL = 1 * 60 * 1000;
        QUEUE_UPLOAD_INTERVAL = 1 * 60 * 1000;
        QUEUE_LAST_UPLOAD_AT = 0;
        WLEV_SAMPLING_INTERVAL = 2 * 60 * 1000;
        WLEV_LAST_SAMPLE_AT = 0;

        /*
            Timestamps
        */
        FIRST_TIP_AT_TIMESTAMP = 0;
        LAST_TIP_AT_TIMESTAMP = 0;
        TREATMENT_STARTED_AT_TIMESTAMP = 0;

        /*
            Counts
        */
        CUMULATIVE_TIP_COUNT = 0;
        
        /*
            Thresholds
        */
        END_TIP_CONT_THRESHOLD = 2;
        MIN_TIP_CONT_THRESHOLD = 5;
        
        /*
            Timeouts
            Units: hour
            TODO: Change it to milliseconds
        */
        INTER_TIP_TIMEOUT = 60 * 1000;
        HOMOGENIZATION_DELAY = 45 * 1000;
        INTER_SAMPLE_DURATION = 1 * 60 * 1000;

        /*
            Miscellaneous
        */
        MEMLOC_RAINID = 32;
        MEMLOC_LASTSTATE = 33;
        MEMLOC_CUMMTIPCOUNT = 34;
        MEMLOC_TRTSTRTTMSTP = 35;
        
    }

    void parse(String str){
        if(str.length() == 0) return;
        
        int MAX_KEYS = 10;
        String keys[MAX_KEYS];
        String values[MAX_KEYS];
        int count = 0;
        int cursor = 0;
        String state = "key";

        String key = "";
        String value = "";
        do { 
            String newchar = String(str.charAt(cursor));

            if (state == "key") {
                if (newchar == "=") {
                    state = "value";
                    keys[count] = key;
                    value = "";
                }
                else {
                    key += newchar;
                }
            }

            if (state == "value") {
                if (newchar != "=") { 
                    if (newchar == "," || newchar == "") {
                        state = "key";
                        values[count] = value;
                        key = "";
                        count++;
                    }
                    else {
                        value += newchar;
                    }
                }
            }

        } while (String(str.charAt(cursor++)).length() > 0);

        for (int i = 0; i < 10; i++) {
            String key = keys[i];
            
            if (key != "") {

                if (key == "QUEUE_UPLOAD_INTERVAL") QUEUE_UPLOAD_INTERVAL = values[i].toInt();
                if (key == "WLEV_SAMPLING_INTERVAL") WLEV_SAMPLING_INTERVAL = values[i].toInt();
                if (key == "END_TIP_CONT_THRESHOLD") END_TIP_CONT_THRESHOLD = values[i].toInt();
                if (key == "MIN_TIP_CONT_THRESHOLD") MIN_TIP_CONT_THRESHOLD = values[i].toInt();

                Serial.println(key + " = " + values[i]);
            }
        }
    }

    /* 
        ! Procedure to handle incoming MQTT messages
        It is mandatory to define this function if GB_MQTT library is instantiated above
    */

    void mqtt_message_handler(String topic, String message) {

        gb.log("\n\nIncoming topic: " + topic);
        gb.log("Incoming message: " + message + "\n\n");

        parse(message);

        // String targetdevice = topic.substring(0, topic.indexOf("::"));
        // if (targetdevice != gb.DEVICE_SN) return;

        // topic = topic.substring(topic.indexOf("::") + 1, topic.length());

        // if (topic == "gb-lab/test") {
        //     mqtt.publish("gatorbyte::gb-lab::response", "message");
        // }

    }

    /* 
        ! Actions to perform on MQTT connect/reconnect
        It is mandatory to define this function if GB_MQTT library is instantiated above
    */

    void mqtt_on_connect() {

        mqtt.subscribe("gb-lab/test");
        mqtt.subscribe("ping");
        mqtt.subscribe("control/update");
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
        // mcu.stopbreathtimer();

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

            //! Connect to network
            sntl.interval("sentinence", 45).enable();
            mcu.connect("cellular");
            sntl.disable();

            //! Connect to MQTT servver
            sntl.interval("sentinence", 30).enable();
            mqtt.connect();
            sntl.disable();

            gb.log("MQTT connection attempted");
            
            // mqtt.publish("log/message", "Iteration: " + String(gb.ITERATION));
            // mqtt.publish("log/message", "Cell signal: " + String(mcu.RSSI) + " dB");
            // mqtt.publish("log/message", "SD write complete");

            //! Publish first ten queued-data with MQTT
            sntl.interval("sentinence", 45).enable();
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
            sntl.disable();

        }
    }

    void write_data_to_sd_and_upload () {

        sntl.interval("sentinence", 15).disable().enable();
        
        // Initialize CSVary object
        CSVary csv;

        int timestamp = rtc.timestamp().toInt(); 

        // Log to SD
        csv
            .clear()
            .setheader("SURVEYID,DEVICESN,TIMESTAMP,TEMP,RH,FLTP,RAINID,HOURID,WLEV")
            .set(gb.PROJECT_ID)
            .set(gb.DEVICE_SN)
            .set(timestamp)
            .set(aht.temperature())
            .set(aht.humidity())
            .set(FAULTS_PRIMARY)
            .set(RAINID)
            .set(HOURID)
            .set(WLEV);

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

        sntl.disable();

        send_queue_files_to_server();
        
    }

    void diagnostics() {
        float wlev = eadc.getdepth(0);
        bool raindetected = rain.listener();

        gb.br().log("Water level: " + String(wlev));
        if (raindetected) {
            gb.log("Rain detected");

            vst.trigger(400);
        }
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

    void triggervst () {
        gb.log("Triggering auto-sampler");
        vst.trigger(400);
    }
    
    /* 
        ! Peripherals configurations and initializations
        Here, objects for the peripherals/components used are initialized and configured. This is where
        you can specify the enable pins (if used), any other pins that are being used, or whether an IO Expander is being used.

        Microcontroller configuration also includes initializing I2C, serial communications and specifying baud rates, specifying APN 
        for cellular communication or WiFi credentials if WiFi is being used.
    */
    void setup () {

        delay(3000);

        gb.log("Diagnosing GatorBit");
        
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
        
        //! Initialize Sentinel
        sntl.configure(9).initialize();
        sntl.interval("sentinence", 60).disable().enable();

        sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter").readconfig();

        //! Configure MQTT broker and connect 
        mqtt.configure("mqtt.ezbean-lab.com", 1883, gb.DEVICE_SN, mqtt_message_handler, mqtt_on_connect);
        
        mem.configure({true, SR0}).initialize();
        aht.configure({true, SR0}).initialize();
        rtc.configure({true, SR0}).initialize();
        
        vst.configure({true, SR5}).initialize();
        eadc.configure({true, SR2}).initialize();
        rain.configure({false, A2}).initialize();
        
        sntl.disable();

        // //! Set APN
        // mcu.apn("super");
        
        // Detect GDC
        gdc.detect();
        
        /*
            This is a hack to eliminate the delay introduced by the DS3231 RTC.
        */
        INITSEC = rtc.timestamp().toDouble();

        gb.log("Setup complete");
    }

    bool flag = false;
    int i = 0;
    void loop () {

        gdc.detect();

    }

    int count = 0;
    void breathe() {
        gb.log("Breathing counter: " + String(count++));
        
        mqtt.update();
        
        gb.breathe();
    }

#endif