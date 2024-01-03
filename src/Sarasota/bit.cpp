#include "../platform.h"

#if GB_PLATFORM_IS_CHMD_BIT

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
    GB_HTTP http(gb);
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

    GB_PIPER breathpiper(gb);
    GB_PIPER wlevpiper(gb);
    GB_PIPER resetvariablespiper(gb);
    GB_PIPER uploaderpiper(gb);
    GB_PIPER controlvariablespiper(gb);
    GB_PIPER antifreezepiper(gb);
    GB_PIPER fivedayantifreezepiper(gb);
    GB_PIPER pingpiper(gb);
    GB_PIPER statepiper(gb);
    
    string STATE = "IDLE";
    int RAINID = 0;
    int HOURID = -1;
    int WLEV = 0;
    int TIMETRACKER = 0;
    int TIME = 0;
    int RAININT = 0;
    bool REBOOT_FLAG = false;
    bool RESET_VARIABLES_FLAG = false;

    /*
        Time trackers
    */
    int CV_UPLOAD_INTERVAL = 15 * 60 * 1000;
    int STATE_UPLOAD_INTERVAL = 60 * 60 * 1000;
    int QUEUE_UPLOAD_INTERVAL = 10 * 60 * 1000;
    int QUEUE_LAST_UPLOAD_AT = 0;
    int WLEV_SAMPLING_INTERVAL = 10 * 60 * 1000;
    int WLEV_LAST_SAMPLE_AT = 0;

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
    int RAIN_INCHES = 0;
    
    /*
        Thresholds
    */
    int END_TIP_CONT_THRESHOLD = 3;
    int MIN_TIP_CONT_THRESHOLD = 10;
    
    /*
        Timeouts
    */
    int INTER_TIP_TIMEOUT = 6 * 60 * 60;
    int HOMOGENIZATION_DELAY = 6 * 60 * 60 * 1000;
    int INTER_SAMPLE_DURATION = 3 * 60 * 60 * 1000;
    int ANTIFREEZE_REBOOT_DELAY = 5 * 86400 * 1000;
    // int ANTIFREEZE_REBOOT_DELAY = 2 * 60 * 1000;
    
    /*
        Intervals
    */
    int BREATH_INTERVAL = 60 * 1000;
    int REMOTE_RESET_CHECK_INTERVAL = 1 * 60 * 1000;
    int ANTIFREEZE_CHECK_INTERVAL = 15 * 60 * 1000;
    int SERVER_PING_INTERVAL = 30 * 60 * 1000;

    /*
        Miscellaneous
    */
    int MEMLOC_RAINID = 32;
    int MEMLOC_LASTSTATE = 33;
    int MEMLOC_CUMMTIPCOUNT = 34;
    int MEMLOC_TRTSTRTTMSTP = 35;
    int MEMLOC_LASTTIPTMSTP = 36;
    int MEMLOC_RAININTENSITY = 37;
    int MEMLOC_LASTSAMPLETMSTP = 38;

    int MEMLOC_BOOT_COUNTER = 41;

    void send_state () {

        // if (!STATE.contains("new-trt-begin")) return;

        // Calculate time to next sample
        int LAST_SAMPLE_AT_TIMESTAMP = mem.get(MEMLOC_LASTSAMPLETMSTP).toInt();
        int NEXT_SAMPLE_IN = 0;
        
        // If the state is IDLE or dry
        if (STATE == "IDLE" || STATE == "dry") {
            NEXT_SAMPLE_IN = 0;
        }
        
        // If the treatment cycle has begun but homogenization is pending
        else if (STATE.endsWith("new-trt-begin")) {
            int now = gb.globals.INIT_SECONDS + (millis() / 1000);
            int MS_SINCE_LAST_SAMPLE = (now - LAST_SAMPLE_AT_TIMESTAMP) * 1000;
            NEXT_SAMPLE_IN = (HOMOGENIZATION_DELAY - MS_SINCE_LAST_SAMPLE) / 1000 / 60;
        }
        
        // If the treatment cycle is ongoing and homogenization has ended
        else if (STATE.contains("-hr-sample")) {
            int now = gb.globals.INIT_SECONDS + (millis() / 1000);
            int MS_SINCE_LAST_SAMPLE = (now - LAST_SAMPLE_AT_TIMESTAMP) * 1000;
            NEXT_SAMPLE_IN = (INTER_SAMPLE_DURATION - MS_SINCE_LAST_SAMPLE) / 1000 / 60;
        }
        
        // If the previous treatment cycle has ended but a new one hasn't started yet
        else if (STATE.endsWith("prev-trt-end")) {
            NEXT_SAMPLE_IN = 0;
        }

        else {
            NEXT_SAMPLE_IN = 0;
        }

        JSONary state; 
        state
            .set("SNTL", sntl.ping() ? "PONG" : "ERROR")
            .set("STATE", mem.get(MEMLOC_LASTSTATE))
            .set("WLEV", WLEV) 
            .set("TIPS", mem.get(MEMLOC_CUMMTIPCOUNT))
            .set("TIPTMSTP", mem.get(MEMLOC_LASTTIPTMSTP))
            .set("TRTTMSTP", mem.get(MEMLOC_TRTSTRTTMSTP))
            .set("NEXTSAMPLECTDN", NEXT_SAMPLE_IN)
            .set("HOURID", HOURID)
            .set("RAINID", RAINID);


        gb.log("Uploading state");
        gb.log(state.get());
        
        mqtt.publish("state/report", state.get());
    }

    void send_control_variables () {
        
        mqtt.publish("control/report", gb.CONTROLVARIABLES.get());
    }


    /* 
        ! Set control variables
        This callback is executed when the control variables file is successfully read
    */

    void set_control_variables(JSONary data) {

        QUEUE_UPLOAD_INTERVAL = data.parseInt("CV_UPLOAD_INTERVAL");
        WLEV_SAMPLING_INTERVAL = data.parseInt("WLEV_SAMPLING_INTERVAL");
        END_TIP_CONT_THRESHOLD = data.parseInt("END_TIP_CONT_THRESHOLD");
        MIN_TIP_CONT_THRESHOLD = data.parseInt("MIN_TIP_CONT_THRESHOLD");
        REBOOT_FLAG = data.parseBoolean("REBOOT_FLAG");
        RESET_VARIABLES_FLAG = data.parseBoolean("RESET_VARIABLES_FLAG");
        INTER_TIP_TIMEOUT = data.parseInt("INTER_TIP_TIMEOUT");
        HOMOGENIZATION_DELAY = data.parseInt("HOMOGENIZATION_DELAY");
        INTER_SAMPLE_DURATION = data.parseInt("INTER_SAMPLE_DURATION");
        CV_UPLOAD_INTERVAL = data.parseInt("CV_UPLOAD_INTERVAL");
        ANTIFREEZE_REBOOT_DELAY = data.parseInt("ANTIFREEZE_REBOOT_DELAY");

        gb.log("Updating runtime variables -> Done");

        // Send the fresh list of control variables
        mqtt.publish("control/report", gb.CONTROLVARIABLES.get());
    }
    
    void resetvariables () {

        gb.br().log("Request to reset variables -> Processed");
        gb.br();

        mem.write(MEMLOC_LASTSTATE, "");
        mem.write(MEMLOC_CUMMTIPCOUNT, "0");
        mem.write(MEMLOC_RAINID, "0");
        mem.write(MEMLOC_TRTSTRTTMSTP, "0");
        mem.write(MEMLOC_RAININTENSITY, "0");
        mem.write(MEMLOC_LASTSAMPLETMSTP, "0");

        STATE = "dry";
        CUMULATIVE_TIP_COUNT = 0;
        RAINID = 0;

        // String filename = "/readings/" + (gb.DEVICE_NAME.length() > 0 ? gb.DEVICE_NAME + "_" : "") + "readings.csv";
        // sd.rm(filename);

        // Reset flag
        RESET_VARIABLES_FLAG = 0;
        sd.updatecontrolbool("RESET_VARIABLES_FLAG", RESET_VARIABLES_FLAG, set_control_variables);

        buzzer.play("-----");
    }

    void devicereboot () {

        gb.log("Rebooting device -> Done");

        // Reset flag
        REBOOT_FLAG = false;
        sd.updatecontrolbool("REBOOT_FLAG", REBOOT_FLAG, set_control_variables);

        buzzer.play(".....");
        sntl.reboot();
    }

    void parse(String str){

        // Update the variables in SD card and in runtime memory
        sd.updatecontrol(str, set_control_variables);
        
        mqtt.publish("log/message", "Control variables updated.");
        // send_control_variables();

        // if (RESET_VARIABLES_FLAG) {
        //     resetvariables();
        //     RESET_VARIABLES_FLAG = 0;
        // }
        // if (REBOOT_FLAG) {
            // devicereboot();
        //     REBOOT_FLAG = 0;
        // }
    }

    /* 
        ! Procedure to handle incoming MQTT messages
        It is mandatory to define this function if GB_MQTT library is instantiated above
    */

    void mqtt_message_handler(String topic, String message) {

        gb.br().br().log("Incoming topic: " + topic);
        gb.log("Incoming message: " + message);
        gb.br().br();

        // If broker sends an acknowledgement
        if (topic == "gatorbyte/ack" && message == "success") {
            mqtt.ACK_RECEIVED = true;
            return;
        }

        parse(message);

        // String targetdevice = topic.substring(0, topic.indexOf("::"));
        // if (targetdevice != gb.DEVICE_SN) return;

        // topic = topic.substring(topic.indexOf("::") + 1, topic.length());

        // if (topic == "gb-lab/test") {
        //     mqtt.publish("gatorbyte::gb-lab::response", "message");
        // }

    }

    /* 
        ! Subscribe to topics
        This callback is executed the GB connects to the MQTT broker
    */

    void mqtt_on_connect() {
        mqtt.subscribe("gb-lab/test");
        mqtt.subscribe("ping");
        mqtt.subscribe("control/update");
        mqtt.subscribe("gatorbyte/ack");
    }

    /* 
        ! Actions to perform before going to sleep
        You can turn specific components on/off. By default all components are turned off before
        entering power-saving mode (sleep)
    */

    void on_sleep() {
        gb.br().log("Performing user-defined pre-sleep tasks");

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

    // Start antifreeze sentinence monitor    
    void antifreeze_monitor () {
        sntl.disable().interval("sentinence", 30 * 60).enable();
    }

    /*
        ! Send data to server
        Send the queue data first.
        Then send the current data to the server.
    */
    void send_queue_files_to_server() {

        if (sd.getqueuecount() > 0) {

            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files.");

            sntl.shield(45, [] {
                
                //! Connect to network
                mcu.connect("cellular");
                
                sntl.kick();

                //! Connect to MQTT servver
                mqtt.connect("pi", "abe-gb-mqtt");
                
            });

            gb.log("MQTT connection attempted");
            
            //! Publish first ten queued-data with MQTT
            sntl.shield(60, [] {

                if (CONNECTED_TO_MQTT_BROKER) {
                    int counter = 10;
                    
                    while (!sd.isqueueempty() && counter-- > 0) {

                        String queuefilename = sd.getfirstqueuefilename();

                        gb.log("Sending queue file: " + queuefilename);

                        // Attempt publishing queue-data
                        if (mqtt.publish("data/set", sd.readqueuefile(queuefilename))) {

                            // Remove queue file
                            sd.removequeuefile(queuefilename);
                        }

                        else {
                            gb.log("Queue file not deleted.");
                        }
                    }
                }
            });

            // Turn on antifreeze monitor
            antifreeze_monitor();

        }
        else {
            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files. Skipping upload.");
        }
    }

    void write_data_to_sd_and_upload () {

        sntl.shield(15, [] {
        
            // Initialize CSVary object
            CSVary csv;

            int timestamp = rtc.timestamp().toInt(); 
            String date = rtc.date("MM/DD/YY");
            String time = rtc.time("hh:mm");

            // Log to SD
            csv
                .clear()
                .setheader("SURVEYID,DEVICESN,TIMESTAMP,DATE,TIME,TEMP,RH,FLTP,RAINID,HOURID,WLEV,INT")
                .set(gb.globals.PROJECT_ID)
                .set(gb.globals.DEVICE_SN)
                .set(timestamp)
                .set(date)
                .set(time)
                .set(aht.temperature())
                .set(aht.humidity())
                .set(gb.globals.FAULTS_PRIMARY)
                .set(RAINID)
                .set(HOURID + 6 - 6)
                .set(WLEV)
                .set(RAIN_INCHES);

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

            gb.log("Wrote to queue file: " + currentdataqueuefile);
            sd.writequeuefile(currentdataqueuefile, csv);
            
        });

        // Upload to server
        send_queue_files_to_server();
        
    }

    // void diagnostics() {
    //     float wlev;
    //     wlev = eadc.getdepth(0);
    //     gb.log("Water level: " + String(wlev));
    //     bool raindetected = rain.listener();

    //     gb.br().log("Water level: " + String(wlev));
    //     if (raindetected) {
    //         gb.log("Rain detected");

    //         vst.trigger(400);
    //     }
    // }

    void nwdiagnostics () {
        
        sntl.shield(45, [] {
            
            //! Connect to network
            mcu.connect("cellular");

            sntl.kick();

            //! Connect to MQTT server
            mqtt.connect("pi", "abe-gb-mqtt");
        
        });
        
        // Turn on antifreeze monitor
        antifreeze_monitor();

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
        
        // Mandatory GB setup function
        gb.setup();
        
        //! Initialize GatorByte and device configurator
        gb.configure(false, "gb-irrec-test-station");
        gb.globals.ENFORCE_CONFIG = false;

        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        //! Configure peripherals
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        buzzer.configure({6}).initialize().play("...");
        
        // Detect GDC
        gdc.detect(false);
        
        //! Initialize Sentinel
        sntl.configure({false}, 9).initialize().ack(true).disablebeacon();
        
        sntl.shield(60, []() { 

            // Detect GDC
            gdc.detect(false);

            // Configure SD
            sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter");
            
            // Configure BL
            bl.configure({true, SR3, SR11}).initialize().persistent().on();

            // Read SD config and control files
            sd.readconfig().readcontrol(set_control_variables);

            //! Configure MQTT broker and connect 
            mqtt.configure("mqtt.ezbean-lab.com", 1883, gb.globals.DEVICE_SN, mqtt_message_handler, mqtt_on_connect);

            // Configure other peripherals
            mem.configure({true, SR0}).initialize();
            aht.configure({true, SR0}).initialize();
            rtc.configure({true, SR0}).initialize();

            // rtc.sync();

            // Configure sensors and actuators
            vst.configure({true, SR5}).initialize();
            eadc.configure({true, SR2}).initialize();
            rain.configure({false, A2}).initialize();

        });

        // mcu.checklist();

        // gb.log("ICCID: ", false);
        // gb.log(mcu.geticcid());

        // // //! Set Twilio APN
        // // mcu.apn("super");
        // mcu.connect("cellular");
        
        // Detect GDC
        gdc.detect(false);

        /*
            ! Network stuff
        */
        sntl.shield(90, [] {
            
            //! Connect to network
            mcu.connect("cellular");

            sntl.kick();

            //! Connect to MQTT server
            mqtt.connect("pi", "abe-gb-mqtt");
        
        });

        
        /*
            This is a hack to eliminate the delay introduced by the DS3231 RTC.
        */
        gb.globals.INIT_SECONDS = rtc.timestamp().toDouble();

        gb.log("Init timestamp: " + String(gb.globals.INIT_SECONDS));
        gb.log("Setup complete");
        
        gb.globals.GDC_SETUP_READY = true;
        if (gb.globals.GDC_CONNECTED) {
            Serial.println("##CL-GB-READY##");
            buzzer.play("-").wait(100).play("-");
        }
    }

    bool diagflag = false;
    bool restoreflag = false;
    bool blraintrigger = false;

    void loop () {

        // return diagnostics();
        
        // MQTT update
        mqtt.update();

        //! Protection against microcontroller freeze
        antifreezepiper.pipe(ANTIFREEZE_CHECK_INTERVAL, true, [] (int counter) {
            antifreeze_monitor();
        });

        bl.listen([] (String command) {
            Serial.println("Received command: " + command);

            if (command == "help") {
                bl.print("Bluetooth commands list");
                bl.print("--------------------------------------------");
                bl.print("1. reset - Resets the variables (HOURID, RAINID).");
                bl.print("2. reboot - Reboots the microcontroller.");
                bl.print("3. trigger - Trigger the autosampler.");
                bl.print("4. rain - Emulate a rain sensor tip.");
                bl.print("5. water - Get water level reading.");
                bl.print("6. data - Print SD card data.");
                bl.print("7. upload - Attempt sending " + String(sd.getqueuecount()) + " unsent data to server.");
                bl.print("8. state - Prints state variables.");
                bl.print("9. clear - Clears the data on SD card.");
                bl.print("10. rtc sync Nov 10 2023 20:43 - Set RTC date and time.");
                bl.print("11. rtc check - Check if RTC can hold time.");
                bl.print("11. bl name gb-chmd-ten - Set BL name.");
                bl.print("");
            }
            if (command == "reset") resetvariables();
            if (command == "reboot") devicereboot();
            if (command == "upload") send_queue_files_to_server();
            if (command == "trigger") triggervst();
            if (command == "beep") {
                
                rgb.on("white");
                buzzer.play(".");
                rgb.off();
                
                // Convert milliseconds to days, hours, minutes, and seconds
                float ms = millis();
                int seconds = ms / 1000;
                int minutes = seconds / 60;
                int hours = minutes / 60;
                int days = hours / 24;
                
                // Calculate the remaining hours, minutes, and seconds
                seconds %= 60;
                minutes %= 60;
                hours %= 24;
                
                bl.print("Since boot: ", false);
                bl.print(String(days) + " days, ", false);
                bl.print(String(hours) + " hours, ", false);
                bl.print(String(minutes) + " minutes, ", false);
                bl.print(String(seconds) + " seconds");
                
                bl.print("Machine state: " + String(STATE));
                bl.print("Sentinel state: " + String(sntl.ping() ? "PONG" : "ERROR"));
            }
            if (command == "sntl") {
                rgb.on("white");
                buzzer.play(".");
                rgb.off();
                
                bl.print("Sentinel state: " + String(sntl.ping() ? "PONG" : "ERROR"));
            }
            if (command == "ping") {
                bl.print("Connection test");
                mqtt.publish("log/message", "Connection test");
            }
            if (command == "control") {
                bl.print("Control sent to server");
                bl.print("--------------------------------------------");
                bl.print(gb.CONTROLVARIABLES.get());
                send_control_variables();

            }
            if (command == "cell") {
                sntl.shield(45, [] {
                
                    //! Connect to network
                    mcu.connect("cellular");
                    
                    sntl.kick();

                    //! Connect to MQTT servver
                    mqtt.connect("pi", "abe-gb-mqtt");
                    
                });

            }
            if (command == "rain") {
                blraintrigger = true;

                rgb.on("white");
                buzzer.play("...");
                rgb.off();
            }
            if (command == "water") {
                sntl.shield(4, [] {
                    WLEV = eadc.getdepth(0);
                    bl.print("Water level: " + String(WLEV));
                });
                
                // Turn on antifreeze monitor
                antifreeze_monitor();
            }
            if (command == "demo") {
                String data = "SURVEYID,DEVICEID,TIMESTAMP,DATE,TIME,TEMP,RH,FLTP,RAINID,HOURID,WLEV\ngb-demo-data,gb-demo-data,1687781674,26,42,15,1,12,0";
                mqtt.ack("demo_12345").publish("data/set", data);
            }
            if (command == "data") {
                bl.print("Sending data");
                String filename = "/readings/" + (gb.globals.DEVICE_NAME.length() > 0 ? gb.globals.DEVICE_NAME + "_" : "") + "readings.csv";
                bl.print(sd.readfile(filename));
            }
            if (command == "state") {

                bl.print("Current state information:");
                bl.print("--------------------------------------------");
            
                // Convert milliseconds to days, hours, minutes, and seconds
                float ms = millis();
                int seconds = ms / 1000;
                int minutes = seconds / 60;
                int hours = minutes / 60;
                int days = hours / 24;
                
                // Calculate the remaining hours, minutes, and seconds
                seconds %= 60;
                minutes %= 60;
                hours %= 24;
                
                bl.print("Since boot: ", false);
                bl.print(String(days) + " days, ", false);
                bl.print(String(hours) + " hours, ", false);
                bl.print(String(minutes) + " minutes, ", false);
                bl.print(String(seconds) + " seconds");
                
                bl.print("Machine state: " + String(STATE));
                bl.print("Last state: " + String(mem.get(MEMLOC_LASTSTATE)));
                bl.print("Sentinel state: " + String(sntl.ping() ? "PONG" : "ERROR"));
                bl.print("Date and time: " + String(rtc.date()) + " " + String(rtc.time()));
                bl.print("Boot counter: " + mem.get(MEMLOC_BOOT_COUNTER));
                bl.print("Sentinel induced reset: " + String(gb.globals.FAULTS_PRIMARY));
                bl.br();
                bl.print("Rain ID: " + String(RAINID));
                bl.print("Stored rain ID: " + String(mem.get(MEMLOC_RAINID).toInt()));
                bl.print("Current hour: " + String(HOURID));
                bl.print("Rain intensity: " + String(RAIN_INCHES));
                bl.print("Tip count: " + String(CUMULATIVE_TIP_COUNT));
                bl.print("Last tip: " + String(LAST_TIP_AT_TIMESTAMP));
                bl.print("Treatment started: " + String(TREATMENT_STARTED_AT_TIMESTAMP));

                // Calculate time to next sample
                int LAST_SAMPLE_AT_TIMESTAMP = mem.get(MEMLOC_LASTSAMPLETMSTP).toInt();
                int NEXT_SAMPLE_IN = 0;
                
                // If the state is IDLE or dry
                if (STATE == "IDLE" || STATE == "dry") {
                    NEXT_SAMPLE_IN = 0;
                }
                
                // If the treatment cycle has begun but homogenization is pending
                else if (STATE.endsWith("new-trt-begin")) {
                    int now = gb.globals.INIT_SECONDS + (millis() / 1000);
                    int MS_SINCE_LAST_SAMPLE = (now - LAST_SAMPLE_AT_TIMESTAMP) * 1000;
                    NEXT_SAMPLE_IN = (HOMOGENIZATION_DELAY - MS_SINCE_LAST_SAMPLE) / 1000 / 60;
                }
                
                // If the treatment cycle is ongoing and homogenization has ended
                else if (STATE.contains("-hr-sample")) {
                    int now = gb.globals.INIT_SECONDS + (millis() / 1000);
                    int MS_SINCE_LAST_SAMPLE = (now - LAST_SAMPLE_AT_TIMESTAMP) * 1000;
                    NEXT_SAMPLE_IN = (INTER_SAMPLE_DURATION - MS_SINCE_LAST_SAMPLE) / 1000 / 60;
                }
                
                // If the previous treatment cycle has ended but a new one hasn't started yet
                else if (STATE.endsWith("prev-trt-end")) {
                    NEXT_SAMPLE_IN = 0;
                }

                else {
                    NEXT_SAMPLE_IN = 0;
                }
                
                bl.print("Next sample in: " + (NEXT_SAMPLE_IN > 0 ? String(NEXT_SAMPLE_IN) + " minute(s)." : "Inf"));
            }
            if (command == "memory") {
                bool uninitialized = false;
                if (mem.get(MEMLOC_LASTSTATE).length() == 0) uninitialized = true;
                if (mem.get(MEMLOC_CUMMTIPCOUNT).length() == 0) uninitialized = true;

                if (uninitialized) {
                    bl.print("The EEPROM variables have not been initialized.");
                    return;
                }
                else {
                    String laststate = mem.get(MEMLOC_LASTSTATE);
                    int lasttipcount = mem.get(MEMLOC_CUMMTIPCOUNT).toInt();
                    int lasttipattimestamp = mem.get(MEMLOC_LASTTIPTMSTP).toInt();
                    int rainid = mem.get(MEMLOC_RAINID).toInt();
                    int rainintensity = mem.get(MEMLOC_RAININTENSITY).toInt();
                    int treatmentstarttimestamp = mem.get(MEMLOC_TRTSTRTTMSTP).toInt();
                    int lastsampletimestamp = mem.get(MEMLOC_LASTSAMPLETMSTP).toInt();
                    
                    bl.print("Reading memory variables:");
                    bl.print("--------------------------------------------");
                    bl.print("Last state: " + laststate); 
                    bl.print("Last tip count: " + String(lasttipcount)); 
                    bl.print("Last treatment started at: " + String(treatmentstarttimestamp)); 
                    bl.print("Last rain ID: " + String(rainid)); 
                    bl.print("Last rain intensity: " + String(rainintensity)); 
                    bl.print("Last sample at: " + String(lasttipattimestamp)); 
                }
            }
            if (command == "modem") {
                bl.print("Resetting MODEM", false);
                MODEM.reset();
                bl.print(" -> Done");
                delay(5000);

                sntl.shield(45, [] {
            
                    //! Connect to network
                    mcu.connect("cellular");

                    sntl.kick();

                    //! Connect to MQTT server
                    mqtt.connect("pi", "abe-gb-mqtt");
                
                });
                
                // Turn on antifreeze monitor
                antifreeze_monitor();
            }
            if (command.indexOf("rtc") == 0) {

                if (command.indexOf("rtc sync") == 0) {
                    /*
                        Command: rtc sync Nov 07 2023 15:23
                        Date format: Nov 07 2023
                        Time format: 15:23
                    */
                    string date = command.substring(9, 20);
                    string time = command.substring(21, 26) + ":" + "00";

                    if (date.length() == 0) {
                        bl.print("RTC date not provided.");
                    }
                    // else if (date.length() != 11) {
                    //     bl.print("Date format is incorrect. Please format the date as: Nov 07 2023");
                    // }
                    else if (time.length() == 0) {
                        bl.print("RTC time not provided.");
                    }
                    // else if (time.length() != 5) {
                    //     bl.print("Time format is incorrect. Please format the time as: 15:23");
                    // }

                    // Sync RTC
                    else {
                        rtc.sync(gb.s2c(date), gb.s2c(time));
                    }
                }
                if (command.indexOf("rtc check") == 0) {
                    /*
                        Command: rtc check
                    */

                    // Restart RTC to check if RTC holds the time
                    rtc.off(); rtc.on(); delay(50);

                    bl.print("Checking RTC", false);
                    if (rtc.valid()) {
                        bl.print(" -> Valid");
                        bl.print("Date: " + String(rtc.date("MMM DD YYYY")));
                        bl.print("Time: " + String(rtc.time("hh:mm:ss")));
                    }
                    else {
                        bl.print(" -> Invalid RTC timestamp: " + String(rtc.timestamp()));
                    }
                }
            }
            if (command.indexOf("bl") == 0) {
                /*
                    Command: bl name gb-chmd-venice
                */

                if(command.indexOf("bl name") == 0) {

                    string name = command.substring(8, command.length());

                    if (name.length() == 0) {
                        bl.print("BL name not provided.");
                    }
                    else {
                        bl.print("Setting name to: " + name);
                        bl.print("Restarting BL module. Please reconnect only after you hear 2 long beeps."); delay(500);
                        bl.off(); bl.on(); delay(500);

                        // Set BL name
                        bl.setname("GB/" + name); 
                        buzzer.play("-").wait(100).play("-");
                    }
                }
                else if(command.indexOf("bl info") == 0) {
                    bl.print("BL name: " + bl.getname());   
                }
            }
        });
        
        //! Five-day anti-freeze piper
        fivedayantifreezepiper.pipe(ANTIFREEZE_REBOOT_DELAY, false, [] (int counter) {

            gb.log("Resetting system to prevent millis() rollover/freeze");
            mqtt.publish("log/message", "Resetting system to prevent freeze: "  + String(millis()));
            delay(5000);
            sntl.reboot();
        });
        
        //! Publish ping
        pingpiper.pipe(SERVER_PING_INTERVAL, true, [] (int counter) {
            gb.log("Publishing ping count: "  + String(counter));
            mqtt.publish("log/message", "Device ping counter: "  + String(counter));
        });
        
        //! Remote variables reset/reboot listener
        resetvariablespiper.pipe(REMOTE_RESET_CHECK_INTERVAL, true, [] (int counter) {

            if (REBOOT_FLAG) {
                mqtt.publish("log/message", "Reboot request processed");
                delay(5000);
                devicereboot();
            }

            if (RESET_VARIABLES_FLAG) {
                delay(2000);
                mqtt.publish("log/message", "Variables reset completed");
                delay(2000);
                resetvariables();
            }
        });
        
        //! Breathing indicator
        breathpiper.pipe(BREATH_INTERVAL, true, [] (int counter) {
            float ms = millis();
            
            // Convert milliseconds to days, hours, minutes, and seconds
            int seconds = ms / 1000;
            int minutes = seconds / 60;
            int hours = minutes / 60;
            int days = hours / 24;
            
            // Calculate the remaining hours, minutes, and seconds
            seconds %= 60;
            minutes %= 60;
            hours %= 24;
            
            gb.log("Since boot: ", false);
            gb.log(String(days) + " days, ", false);
            gb.log(String(hours) + " hours, ", false);
            gb.log(String(minutes) + " minutes, ", false);
            gb.log(String(seconds) + " seconds");

            gb.log("Sentinel state: " + String(sntl.ping() ? "PONG" : "ERROR"));
            buzzer.play(".");
        });

        //! Upload control variables state to the server
        controlvariablespiper.pipe(CV_UPLOAD_INTERVAL, true, [] (int counter) {
            send_control_variables();
        });
        
        //! Upload data to server
        uploaderpiper.pipe(QUEUE_UPLOAD_INTERVAL, true, [] (int counter) {
            if (sd.getqueuecount() > 0) send_queue_files_to_server();
        });

        //! Read water level data
        wlevpiper.pipe(WLEV_SAMPLING_INTERVAL, true, [] (int counter) {

            sntl.shield(4, [] {
                WLEV = eadc.getdepth(0);
                gb.log("Water level: " + String(WLEV));
            });

            // Turn on antifreeze monitor
            antifreeze_monitor();
        });

        //! Upload state to the server
        statepiper.pipe(STATE_UPLOAD_INTERVAL, true, [] (int counter) {
            send_state();
        });

        //! Restore action/state after a reboot
        if (!restoreflag || STATE == "IDLE") { 
            restoreflag = true; 

            if (mem.get(MEMLOC_LASTSTATE).length() == 0) return;
            if (mem.get(MEMLOC_CUMMTIPCOUNT).length() == 0) return;

            String laststate = mem.get(MEMLOC_LASTSTATE);
            int lasttipcount = mem.get(MEMLOC_CUMMTIPCOUNT).toInt();
            int lasttipattimestamp = mem.get(MEMLOC_LASTTIPTMSTP).toInt();
            int rainid = mem.get(MEMLOC_RAINID).toInt();
            int rainintensity = mem.get(MEMLOC_RAININTENSITY).toInt();
            int treatmentstarttimestamp = mem.get(MEMLOC_TRTSTRTTMSTP).toInt();
            int lastsampletimestamp = mem.get(MEMLOC_LASTSAMPLETMSTP).toInt();
            
            gb.log("\nRestoring state.");
            if (STATE == "IDLE") gb.log("IDLE state detected."); 
            gb.log("Last state: " + laststate); 
            gb.log("Last tip count: " + String(lasttipcount));
            gb.log("Last treatment started at: " + String(treatmentstarttimestamp)); 
            gb.log("Last rain ID: " + String(rainid)); 
            gb.log("Last rain intensity: " + String(rainintensity));
            gb.log("Last sample at: " + String(lastsampletimestamp)); 
            
            if (laststate == "raining") {
                STATE = "raining";
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                RAIN_INCHES = rainintensity;
                RAINID = rainid;
            }
            
            if (laststate == "dry") {
                STATE = "dry";
                RAINID = rainid;
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                RAIN_INCHES = rainintensity;
                
                gb.log("Restoring state dry state. Last rain ID: " + String(RAINID));
            }

            // Previous treatment cycle had ended
            else if (laststate == "prev-trt-end") {
                STATE = "raining|prev-trt-end";
                RAINID = rainid;
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                RAIN_INCHES = rainintensity;
                
                gb.log("Restoring state to after the end of previous treatment cycle. Last rain ID: " + String(RAINID));
            }

            // New treatment cycle had bugun
            else if (laststate == "new-trt-begin") {
                STATE = "raining|prev-trt-end|new-trt-begin";
                RAINID = rainid;
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                TREATMENT_STARTED_AT_TIMESTAMP = treatmentstarttimestamp;
                RAIN_INCHES = rainintensity;

                gb.log("Restoring ongoing treatment cycle. Current rain ID: " + String(RAINID));
            }

            // First sample of the new treatment cycle was already taken (6-hr sample)
            else if (laststate == "0-hr-sample") {
                STATE = "raining|prev-trt-end|new-trt-begin|0-hr-sample";
                RAINID = rainid;
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                TREATMENT_STARTED_AT_TIMESTAMP = treatmentstarttimestamp;
                HOURID = 0 + 6;
                RAIN_INCHES = rainintensity;

                gb.log("Restoring state. 0-hour sample already taken.");
            }

            // Second sample of the new treatment cycle was already taken (9-hr sample)
            else if (laststate == "3-hr-sample") {
                STATE = "raining|prev-trt-end|new-trt-begin|0-hr-sample|3-hr-sample";
                RAINID = rainid;
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                TREATMENT_STARTED_AT_TIMESTAMP = treatmentstarttimestamp;
                HOURID = 3 + 6;
                RAIN_INCHES = rainintensity;

                gb.log("Restoring state. 3-hour sample already taken.");
            }

            // Third sample of the new treatment cycle was already taken (12-hr sample)
            else if (laststate == "6-hr-sample") {
                STATE = "raining|prev-trt-end|new-trt-begin|0-hr-sample|3-hr-sample|6-hr-sample";
                RAINID = rainid;
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                TREATMENT_STARTED_AT_TIMESTAMP = treatmentstarttimestamp;
                HOURID = 6 + 6;
                RAIN_INCHES = rainintensity;

                gb.log("Restoring state. 6-hour sample already taken.");
            }

            // Fourth sample of the new treatment cycle was already taken (15-hr sample)
            else if (laststate == "9-hr-sample") {
                STATE = "raining|prev-trt-end|new-trt-begin|0-hr-sample|3-hr-sample|6-hr-sample|9-hr-sample";
                RAINID = rainid;
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                TREATMENT_STARTED_AT_TIMESTAMP = treatmentstarttimestamp;
                HOURID = 9 + 6;
                RAIN_INCHES = rainintensity;

                gb.log("Restoring state. 9-hour sample already taken.");
            }

            // Fifth sample of the new treatment cycle was already taken (18-hr sample)
            else if (laststate == "12-hr-sample") {
                STATE = "dry";
                RAINID = rainid;
                CUMULATIVE_TIP_COUNT = lasttipcount;
                LAST_TIP_AT_TIMESTAMP = lasttipattimestamp;
                TREATMENT_STARTED_AT_TIMESTAMP = treatmentstarttimestamp;
                HOURID = 12 + 6;
                
                // Reset rain intensity variable
                RAIN_INCHES = 0;

                gb.log("Restoring state. 12-hour sample already taken. Waiting for the end of the current treatment cycle.");
            }

            gb.br().log("Ready to receive Bluetooth commands.");
            gb.br();
        }

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
       
        // bool raindetected = rain.listener() || blraintrigger;
        // if (blraintrigger) blraintrigger = false;
        
        //! Check if a rain is happening right now (check if the bucket has tipped)
        bool raindetected = (blraintrigger) ? !(blraintrigger = false) : rain.listener();

        if (raindetected) {

            gb.br().log("Current state: " + STATE);
            
            RAIN_INCHES += 1; 
            mem.write(MEMLOC_RAININTENSITY, String(RAIN_INCHES));

            gb.br().log("Rain detected.");
        }

        /*
            Hierarchy: 1

            ! If rain detected and a new rain event has not started yet

            STATE: { IDLE, dry, raining }
            This contains of following scenarios/states
                1. Blank slate 
                    The GatorBit booted for the first time
                2. Previous treatment cycle hasn't ended yet
                3. Bucket tips after a timeout delay
                    This resets the tip counter
        */
        if (raindetected && !STATE.contains("new-trt-begin")) {

            /*
                This state includes
                    1. Before the old treatment hasn't ended yet (3 tips haven't occured yet)
                    2. Old treatment has ended, but the new treatment hasn't begun yet (tip count is between 3 and 10)
            */

            int now = gb.globals.INIT_SECONDS + (millis() / 1000);

            /*
                Hierarchy: 1.1.1
            
                Detect if this bucket tip marks the first tip of the rain event
            */
            if 
            (
                !STATE.contains("raining") ||
                STATE.contains("dry")
            )  { 
            
                FIRST_TIP_AT_TIMESTAMP = now;
                LAST_TIP_AT_TIMESTAMP = now;
                STATE = "raining";
                CUMULATIVE_TIP_COUNT = 1;

                mem.write(MEMLOC_LASTSTATE, STATE);
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                
                gb.br().log("New rain event.");
                gb.log("Cumulative tip count: " + String(CUMULATIVE_TIP_COUNT));
                gb.log("Rain intensity: " + String(RAIN_INCHES));
            }
            
            /*
                Hierarchy: 1.1.2
            
                If this bucket tip is a continuation of the rain event
                    - and the tip happens before the timeout interval
            */
            else if 
            (
                STATE.contains("raining") && 
                (now - LAST_TIP_AT_TIMESTAMP < INTER_TIP_TIMEOUT)
            ) { 
            
                LAST_TIP_AT_TIMESTAMP = now;
                CUMULATIVE_TIP_COUNT += 1;
                
                mem.write(MEMLOC_LASTSTATE, String(STATE.contains("prev-trt-end") ? "prev-trt-end" : "raining"));
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                
                gb.log("Continuing rain event.");
                gb.log("Cumulative tip count: " + String(CUMULATIVE_TIP_COUNT));
                gb.log("Rain intensity: " + String(RAIN_INCHES));
            }
            
            /*
                Hierarchy: 1.1.3
            
                If this bucket tip happens after the time out
                    - before the end of previous treatment cycle 
                    - after the end of previous treatment cycle and before a new treatment cycle begins
            */
            else if 
            (
                STATE.contains("raining") &&
                (now - LAST_TIP_AT_TIMESTAMP >= INTER_TIP_TIMEOUT)
            ) {

                FIRST_TIP_AT_TIMESTAMP = now;
                LAST_TIP_AT_TIMESTAMP = now;
                STATE = "raining" + string(STATE.contains("prev-trt-end") ? "|prev-trt-end" : "");
                CUMULATIVE_TIP_COUNT = 1;

                mem.write(MEMLOC_LASTSTATE, String(STATE.contains("prev-trt-end") ? "prev-trt-end" : ""));
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                
                gb.log("Tip bucket timeout. New rain event.");
                gb.log("Cumulative tip count: " + String(CUMULATIVE_TIP_COUNT)); 
                gb.log("Rain intensity: " + String(RAIN_INCHES));
            }

            /*
                Hierarchy: 1.2.1
            
                Check if the tip count is more than END_TIP_CONT_THRESHOLD (HOURID 100)
                Marks the end of the treatment cycle with all samples collected for the treatment cycle
            */
            if 
            (
                !STATE.contains("prev-trt-end") &&
                CUMULATIVE_TIP_COUNT >= END_TIP_CONT_THRESHOLD
            ) {

                gb.log("Previous treatment cycle has ended.");
                gb.log("Taking sample t = inf hr");
                STATE += "|prev-trt-end";

                if (mem.get(MEMLOC_RAINID) == "") mem.write(MEMLOC_RAINID, "0");
                RAINID = mem.get(MEMLOC_RAINID).toInt();
                gb.log("All samples taken for Rain ID: " + String(RAINID) + ". Pending pickup.");
                RAIN_INCHES = 0;

                // Save state to EEPROM
                mem.write(MEMLOC_LASTSTATE, "prev-trt-end");
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                mem.write(MEMLOC_RAININTENSITY, String(RAIN_INCHES));
                mem.write(MEMLOC_LASTSAMPLETMSTP, String(now));

                HOURID = 94 + 6;
                triggervst();
                write_data_to_sd_and_upload();
                gb.log("Finished taking sample and reporting data");
                mcu.watchdog("disable");
            }

            /*
                Hierarchy: 1.2.2
            
                Check if the tip count is more than MIN_TIP_CONT_THRESHOLD
                Marks the start of a new treatment cycle
            */
            if (
                !STATE.contains("new-trt-begin") && 
                CUMULATIVE_TIP_COUNT >= MIN_TIP_CONT_THRESHOLD
            ) {
                STATE += "|new-trt-begin";
                CUMULATIVE_TIP_COUNT = 0;

                TREATMENT_STARTED_AT_TIMESTAMP = rtc.timestamp().toInt();

                // Save state to EEPROM
                if (mem.get(MEMLOC_RAINID) == "") mem.write(MEMLOC_RAINID, "0");
                RAINID = mem.get(MEMLOC_RAINID).toInt() + 1;
                mem.write(MEMLOC_RAINID, String(RAINID));

                mem.write(MEMLOC_LASTSTATE, "new-trt-begin");
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_TRTSTRTTMSTP, String(TREATMENT_STARTED_AT_TIMESTAMP));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));

                gb.log("Starting new treatment cycle. Rain ID: " + String(RAINID));
            }
        }

        /*
            Hierarchy: 2
        
            ! If the bucket tips after a new treatment cycle bagan and water homogenization is underway

            This also contains the following scenarios/states
                1. Bucket tips within a timeout; this will reset the 6-hr timer.
        */
        else if (raindetected && STATE.contains("new-trt-begin") && !STATE.contains("-hr-sample")) {
            
            gb.log("Continuing treatment cycle.");
            gb.log("A tip was detected. Resetting 6 hr timer to 0.");
            gb.log("Rain intensity: " + String(RAIN_INCHES));

            // Reset treatment start timestamp
            TREATMENT_STARTED_AT_TIMESTAMP = rtc.timestamp().toInt();

            mem.write(MEMLOC_TRTSTRTTMSTP, String(TREATMENT_STARTED_AT_TIMESTAMP));
            mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
        }

        /*
            Hierarchy: 3
        
            If no rain detected and water homogenization has concluded. Samples collection timer has begun.

            This contains of following scenarios/states
                1. 3-hr samples on a timer
                    a. 0-hr  (HOURID = 6)
                    b. 3-hr  (HOURID = 9)
                    c. 6-hr  (HOURID = 12)
                    d. 9-hr  (HOURID = 15)
                    e. 12-hr (HOURID = 18)
        */
        else if (!raindetected && STATE.contains("new-trt-begin")) {
            
            /*
                ! Known issue - Fixed
                The DS3231 library adds a delay that prevents rain pulse detection most of the time.
                A solution would be to use millis() function, but that will prevent resetting the state after an unexpected system reset.
            */
            int now = gb.globals.INIT_SECONDS + (millis() / 1000);

            HOURID = (now - TREATMENT_STARTED_AT_TIMESTAMP) / 3600;
            LAST_TIP_AT_TIMESTAMP = now;

            /*
                Hierarchy: 3.1

                Check if 0-hour (HOURID 6) sample is due and has not yet been taken.
            */
            if (
                HOURID * 60 * 60 * 1000 >= (HOMOGENIZATION_DELAY + INTER_SAMPLE_DURATION * 0) && 
                !STATE.contains("0-hr-sample")
            ) {
                gb.log("Water homogenization complete");
                gb.log("Taking sample t = 0 hr");
                STATE += "|0-hr-sample";
                
                // Save state to EEPROM
                mem.write(MEMLOC_LASTSTATE, "0-hr-sample");
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                mem.write(MEMLOC_LASTSAMPLETMSTP, String(now));

                HOURID = 0 + 6;
                triggervst();
                write_data_to_sd_and_upload();
                gb.log("Finished taking sample and reporting data");
                mcu.watchdog("disable");
            }
            
            /*
                Hierarchy: 3.2

                Check if 3-hour (HOURID 9) sample is due and has not yet been taken.
            */
            else if (
                HOURID * 60 * 60 * 1000 >= (HOMOGENIZATION_DELAY + INTER_SAMPLE_DURATION * 1) && 
                !STATE.contains("3-hr-sample")
            ) {
                gb.log("Taking sample t = 3 hr");
                STATE += "|3-hr-sample";
                
                // Save state to EEPROM
                mem.write(MEMLOC_LASTSTATE, "3-hr-sample");
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                mem.write(MEMLOC_LASTSAMPLETMSTP, String(now));

                HOURID = 3 + 6;
                triggervst();
                write_data_to_sd_and_upload();
                gb.log("Finished taking sample and reporting data");
                mcu.watchdog("disable");
            }

            /*
                Hierarchy: 3.3

                Check if 6-hour (HOURID 12) sample is due and has not yet been taken.
            */
            else if (
                HOURID * 60 * 60 * 1000 >= (HOMOGENIZATION_DELAY + INTER_SAMPLE_DURATION * 2) && 
                !STATE.contains("6-hr-sample")
            ) {
                gb.log("Taking sample t = 6 hr");
                STATE += "|6-hr-sample";
                
                // Save state to EEPROM
                mem.write(MEMLOC_LASTSTATE, "6-hr-sample");
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                mem.write(MEMLOC_LASTSAMPLETMSTP, String(now));

                HOURID = 6 + 6;
                triggervst();
                write_data_to_sd_and_upload();
                gb.log("Finished taking sample and reporting data");
                mcu.watchdog("disable");
            }

            /*
                Hierarchy: 3.4

                Check if 9-hour (HOURID 15) sample is due and has not yet been taken.
            */
            else if (
                HOURID * 60 * 60 * 1000 >= (HOMOGENIZATION_DELAY + INTER_SAMPLE_DURATION * 3) && 
                !STATE.contains("9-hr-sample")
            ) {
                gb.log("Taking sample t = 9 hr");
                STATE += "|9-hr-sample";

                // Save state to EEPROM
                mem.write(MEMLOC_LASTSTATE, "9-hr-sample");
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                mem.write(MEMLOC_LASTSAMPLETMSTP, String(now));

                HOURID = 9 + 6;
                triggervst();
                write_data_to_sd_and_upload();
                gb.log("Finished taking sample and reporting data");
                mcu.watchdog("disable");
            }
            
            /*
                Hierarchy: 3.5

                Check if 12-hour (HOURID 18) sample is due and has not yet been taken.
            */
            else if (
                HOURID * 60 * 60 * 1000 >= (HOMOGENIZATION_DELAY + INTER_SAMPLE_DURATION * 4) && 
                !STATE.contains("12-hr-sample")
            ) {
                gb.log("Taking sample t = 12 hr");
                STATE += "|12-hr-sample";

                // Reset state
                STATE = "dry";

                HOURID = 12 + 6;
                triggervst();
                write_data_to_sd_and_upload();
                gb.log("Finished taking sample and reporting data");
                mcu.watchdog("disable");

                // Reset rain intensity variable after the data is sent to server
                RAIN_INCHES = 0;

                // Save state to EEPROM
                mem.write(MEMLOC_LASTSTATE, "dry");
                mem.write(MEMLOC_CUMMTIPCOUNT, String(0));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                mem.write(MEMLOC_RAININTENSITY, String(RAIN_INCHES));
                mem.write(MEMLOC_LASTSAMPLETMSTP, String(now));
            }
        }


        /*
            Hierarchy: 4

            If rain IS detected (mid treatment rain) after the water homogenization has concluded and sample collection is underway
        */
        else if (raindetected && STATE.contains("new-trt-begin") && STATE.contains("-hr-sample")) {
            
            int now = gb.globals.INIT_SECONDS + (millis() / 1000);
            // int now = millis(); 

            if (!STATE.contains("new-trt-rain")) STATE += "new-trt-rain";
            CUMULATIVE_TIP_COUNT += 1;
            LAST_TIP_AT_TIMESTAMP = now;
            gb.br().log("Mid-treatment rain detected.");
            gb.log("Cumulative tip count: " + String(CUMULATIVE_TIP_COUNT));
            gb.log("Rain intensity: " + String(RAIN_INCHES));

            /*
                Hierarchy: 4.1

                Check rain threshold is surpassed
            */
            if (
                CUMULATIVE_TIP_COUNT >= END_TIP_CONT_THRESHOLD
            ) {

                gb.log("Mid-treatment tip threshold has been surpassed.");
                gb.log("The treatment cycle has been prematurely ended.");
                gb.log("Taking sample t = inf hr (HOURID 99)");
                
                STATE = "raining|prev-trt-end";
                RAIN_INCHES = 0;

                gb.log("Some samples taken for Rain ID: " + String(RAINID) + ". Pending pickup.");
            
                // Save state to EEPROM
                mem.write(MEMLOC_LASTSTATE, "prev-trt-end");
                mem.write(MEMLOC_CUMMTIPCOUNT, String(CUMULATIVE_TIP_COUNT));
                mem.write(MEMLOC_LASTTIPTMSTP, String(LAST_TIP_AT_TIMESTAMP));
                mem.write(MEMLOC_LASTSAMPLETMSTP, String(now));
                mem.write(RAIN_INCHES, String(RAIN_INCHES));
                
                HOURID = 93 + 6;
                triggervst();
                write_data_to_sd_and_upload();
                gb.log("Finished taking sample and reporting data");
                mcu.watchdog("disable");
            }
        }
    }

    int count = 0;
    void breathe() {
        gb.log("Breathing counter: " + String(count++));
        
        mqtt.update();
        
        gb.breathe();
    }

#endif