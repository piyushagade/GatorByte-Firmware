#include "../platform.h"

#if GB_PLATFORM_IS_SASBJEP_BIT

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

    GB_SNTL sntl(gb);
    GB_USS uss(gb);

    GB_AHT10 aht(gb);
    GB_DESKTOP gdc(gb);

    GB_PIPER breathpiper;
    GB_PIPER wlevpiper;
    GB_PIPER resetvariablespiper;
    GB_PIPER uploaderpiper;
    GB_PIPER recoverypiper;
    GB_PIPER controlvariablespiper;
    GB_PIPER antifreezepiper;
    GB_PIPER fivedayantifreezepiper;
    GB_PIPER pingpiper;
    GB_PIPER statepiper;
    
    /*
        Control variables
    */
    bool REBOOT_FLAG = false;
    bool RESET_VARIABLES_FLAG = false;

    int CV_UPLOAD_INTERVAL = 30 * 60 * 1000;
    int STATE_UPLOAD_INTERVAL = 60 * 60 * 1000;
    int WLEV_SAMPLING_INTERVAL = 10 * 60 * 1000;

    /*
        Timeouts
    */
    int ANTIFREEZE_REBOOT_DELAY = 5 * 86400 * 1000;
    
    /*
        Intervals
    */
    int BREATH_INTERVAL = 60 * 1000;

    /*
        Globals
    */
    int WLEV = 0;

    void send_state () {

        sntl.watch(120, [] {
            gb.log("Uploading state");
            
            //! Connect to network
            mcu.connect("cellular");
            
            sntl.kick();

            //! Connect to MQTT servver
            mqtt.connect();
            
        });

        sntl.watch(60, [] {
            JSONary state; 
            state
                .set("SNTL", sntl.ping() ? "PONG" : "ERROR")
                .set("WLEV", uss.read());

            mqtt.publish("state/report", state.get());
        });
    }

    void send_control_variables () {
        
        sntl.watch(120, [] {
            gb.log("Uploading state");
            
            //! Connect to network
            mcu.connect("cellular");
            
            sntl.kick();

            //! Connect to MQTT servver
            mqtt.connect();
            
        });

        sntl.watch(60, [] {
            mqtt.publish("control/report", gb.controls.get());
        });
    }

    void get_control_variable () {
        
        sntl.watch(120, [] {
            
            //! Connect to network
            mcu.connect("cellular");
            
        });

        sntl.watch(60, [] {
            JSONary data;
            data
                .set("key", "RECOVERY_MODE")
                .set("simplify", "true")
                .set("device-sn", gb.globals.DEVICE_SN);

            if(http.post("v3/gatorbyte/control/get/bykey", data.get()) ) {
                String response = http.httpresponse();
                gb.controls.set("RECOVERY_MODE", response);
                sd.updatecontrolbool("RECOVERY_MODE", response);
            }
        });
    }

    /* 
        ! Set control variables
        This callback is executed when the control variables file is successfully read
    */

    void set_control_variables(JSONary data) {

        WLEV_SAMPLING_INTERVAL = data.getint("WLEV_SAMPLING_INTERVAL");
        REBOOT_FLAG = data.getboolean("REBOOT_FLAG");
        RESET_VARIABLES_FLAG = data.getboolean("RESET_VARIABLES_FLAG");
        CV_UPLOAD_INTERVAL = data.getint("CV_UPLOAD_INTERVAL");
        ANTIFREEZE_REBOOT_DELAY = data.getint("ANTIFREEZE_REBOOT_DELAY");

        gb.log("Updating runtime variables -> Done");

        // Send the fresh list of control variables
        mqtt.publish("control/report", gb.controls.get());
    }
    
    void resetvariables () {

        gb.br().log("Request to reset variables -> Processed");

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

    void parse_mqtt_message(String str){

        // Update the variables in SD card and in runtime memory
        sd.updateallcontrol(str, set_control_variables);
        
        mqtt.publish("log/message", "Control variables updated.");

        if (RESET_VARIABLES_FLAG) {
            resetvariables();
            RESET_VARIABLES_FLAG = 0;
        }
        if (REBOOT_FLAG) {
            devicereboot();
            REBOOT_FLAG = 0;
        }
    }

    /* 
        ! Procedure to handle incoming MQTT messages
        It is mandatory to define this function if GB_MQTT library is instantiated above
    */

    void mqtt_message_handler(String topic, String message) {

        gb.br(2).log("Incoming topic: " + topic);
        gb.log("Incoming message: " + message);
        gb.br(2);

        // If broker sends an acknowledgement
        if (topic == "gatorbyte/ack" && message == "success") {
            mqtt.ACK_RECEIVED = true;
            return;
        }

        // Parse message
        parse_mqtt_message(message);

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

        int sentinenceduration = (gb.globals.SLEEP_DURATION / 1000) + 300;
        sntl.interval("sentinence", sentinenceduration).enable().enablebeacon(1);
        buzzer.play("----");

        mcu.disconnect("cellular");
    }

    /* 
        ! Actions to perform after the microcontroller wakes up
        You can turn specific components on/off.
    */

    void on_wakeup() {
        
        rgb.on("magenta");
        buzzer.play("....");
        delay(2000);

        sntl.disable().enablebeacon(0);

        gb.log("The device is now awake.");
        gdc.send("highlight-yellow", "Device awake");

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

            sntl.watch(120, [] {
                gb.log("Found " + String(sd.getqueuecount()) + " queue files.");
                
                //! Connect to network
                mcu.connect("cellular");
                
                sntl.kick();

                //! Connect to MQTT servver
                mqtt.connect();
                
            });

            gb.log("MQTT connection attempted");
            
            //! Publish first ten queued-data with MQTT
            sntl.watch(120, [] {

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

        }
        else {
            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files. Skipping upload.");
        }
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
        gb.configure(false, "gb-sasbjep-test-station");
        gb.globals.ENFORCE_CONFIG = true;

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
        sntl.configure({false}, 9).initialize().setack(true).enablebeacon(0);

        // while(true) {
        //     // sntl.configure({true, 4}, 9).initialize();
        //     sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter");
        //     mem.configure({true, SR0}).initialize();
        // }

        sntl.watch(120, []() {

            // Configure SD
            sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter");
            
            mem.configure({true, SR0}).initialize();

            // mem.writeconfig(sd.readconfig());

            gb.processconfig(sd.readconfig());
            // gb.processconfig();

            // Read control variables
            sd.readcontrol(set_control_variables);
            
            // Detect GDC
            gdc.detect(false);
            
            //! Configure MQTT broker and connect 
            mqtt.configure(mqtt_message_handler, mqtt_on_connect);
            // http.configure("api.ezbean-lab.com", 80);

            if (false) {
                sntl.watch(90, [] {
                    
                    //! Connect to network
                    mcu.connect("cellular");
                    
                    sntl.kick();

                    //! Connect to MQTT servver
                    mqtt.connect();
                    
                });
            }
            
            // Configure other peripheralsm
            rtc.configure({true, SR0}).initialize();
            aht.configure({true, SR0}).initialize();

            // Configure BL
            bl.configure({true, SR3, SR11}).off();

            // Configure sensors and actuators
            uss.configure({true, SR7, A6}).persistent().initialize();


        });

        gb.log("Setup complete");
        
    }

    void preloop () {

        /*
            This is a hack to eliminate the delay introduced by the DS3231 RTC.
        */
        gb.globals.INIT_SECONDS = rtc.timestamp().toDouble();

        gb.br().log("RTC time: " + rtc.date("MM/DD/YYYY") + ", " + rtc.time("hh:mm:ss"));
        gb.log("Init timestamp: " + String(gb.globals.INIT_SECONDS));
        gb.log("Water level sensing timeout: " + String(gb.controls.getint("WLEV_SAMPLING_INTERVAL")) + " ms");
        
        // Detect GDC
        gdc.detect(false);
    }

    void loop () {

        // GatorByte loop function
        gb.loop();

        // If recovery mode has been activated
        if (false && gb.controls.getboolean("RECOVERY_MODE")) {

            while (gb.controls.getboolean("RECOVERY_MODE")) {
                // buzzer.play("..").wait(200).play("..");

                recoverypiper.pipe(0.5 * 60 * 1000, true, [] (int counter) {
                    
                    sntl.watch(90, [] {
                
                        //! Connect to network
                        gb.log("Connecting to cellular", false);
                        mcu.connect("cellular"); sntl.kick();
                        gb.log(" -> Completed");

                        //! Connect to MQTT servver
                        mqtt.connect();
                    });

                    sntl.watch(15, [] {

                        // Initialize CSVary object
                        CSVary csv;

                        int timestamp = rtc.timestamp().toInt(); 
                        String date = rtc.date("MM/DD/YY");
                        String time = rtc.time("hh:mm");

                        // Log to SD
                        csv
                            .clear()
                            .setheader("DEVICESN,TIMESTAMP,DATE,TIME,FLTP,BVOLT")
                            .set(gb.globals.DEVICE_SN)
                            .set(timestamp)
                            .set(date)
                            .set(time)
                            .set(gb.globals.FAULTS_PRIMARY)
                            .set(mcu.fuel("voltage"));

                        // Publsih to server
                        mqtt.publish("data/set", csv.getheader() + BR + csv.getrows());

                    });
                });
            }
        }
        
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
            
            gb.br().log("Since boot: ", false);
            gb.log(String(days) + " days, ", false);
            gb.log(String(hours) + " hours, ", false);
            gb.log(String(minutes) + " minutes, ", false);
            gb.log(String(seconds) + " seconds");

            gb.log("Sentinel state: " + String(sntl.ping() ? "PONG" : "ERROR"));
            
            rgb.on("white");
            buzzer.play(".");
            rgb.off();
        });

        //! Upload control variables state to the server
        controlvariablespiper.pipe(gb.controls.getint("CV_UPLOAD_INTERVAL"), false, [] (int counter) {
            // send_control_variables();
            get_control_variable();
        });

        //! Read water level data and send data to the server
        wlevpiper.pipe(gb.controls.getint("WLEV_SAMPLING_INTERVAL"), true, [] (int counter) {

            sntl.watch(90, [] {
                
                //! Connect to network
                mcu.connect("cellular");
                
                sntl.kick();

                //! Connect to MQTT servver
                mqtt.connect();
                
            });

            sntl.watch(30, [] {
                gb.br().log("Reading water level at " + rtc.time("hh:mm:ss"));

                WLEV = uss.read();
                gb.log("Water level: " + String(WLEV));

                // MQTT update
                mqtt.update();
            });
            
            sntl.watch(180, [] { 

                // Initialize CSVary object
                CSVary csv;

                int timestamp = rtc.timestamp().toInt(); 
                String date = rtc.date("MM/DD/YY");
                String time = rtc.time("hh:mm");

                // Log to SD
                csv
                    .clear()
                    .setheader("DEVICESN,TIMESTAMP,DATE,TIME,TEMP,RH,FLTP,FLTS,WLEV,BVOLT,BLEV")
                    .set(gb.globals.DEVICE_SN)
                    .set(timestamp)
                    .set(date)
                    .set(time)
                    .set(aht.temperature())
                    .set(aht.humidity())
                    .set(gb.globals.FAULTS_PRIMARY)
                    .set(gb.globals.FAULTS_SECONDARY)
                    .set(WLEV)
                    .set(mcu.fuel("voltage"))
                    .set(mcu.fuel("level"));

                //! Write current data to SD card
                sd.writeCSV(csv);
                gb.log("Readings written to SD card");

                gb.br().log("Current data point: ");
                gb.log(csv.getheader());
                gb.log(csv.getrows());
                
                // MQTT update 
                mqtt.update();

                //! Upload current data to desktop client
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

            // Send outstanding data and current data
            if (sd.getqueuecount() > 0) {
                gb.br().log("Found " + String(sd.getqueuecount()) + " outstanding queue files.");
                
                //! Publish first ten queued-data with MQTT
                sntl.watch(120, [] {

                    if (CONNECTED_TO_MQTT_BROKER) {
                        
                        int MAX_FILES_TO_UPLOAD = 50;
                        
                        while (!sd.isqueueempty() && MAX_FILES_TO_UPLOAD-- > 0) {

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

                            sntl.kick();
                        }
                    }
                });

            }
            else {
                gb.br().log("Found " + String(sd.getqueuecount()) + " queue files on SD. Skipping upload.");
            }

        });

        //! Upload state to the server
        statepiper.pipe(STATE_UPLOAD_INTERVAL, true, [] (int counter) {
            send_state();
        });
        
        // // Set sleep configuration
        // mcu.set_sleep_callback(on_sleep);
        // mcu.set_wakeup_callback(on_wakeup);
        // mcu.set_primary_piper(wlevpiper);

        // //! Sleep
        // mcu.sleep();
    }

#endif