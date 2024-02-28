#include "../platform.h"

#if GB_PLATFORM_IS_LRY_DRIFTER_V2

    /*
        Drifter goals
        -------------`
        1. Long-term deployment
            a. Sampling frequency - 5 minutes.
            b. Upload strategies
                i.  Once every hour; Recovery mode implementation required
                ii. As soon as reading is taken; Recovery mode not required
        2. Only monitor GPS location
            a. GPS not always ON
                i. For sampling interval < 5 minutes, GPS can be turned off
                    and a cold start fix will be under a minute.
                ii. If sampling interval > 5 minutes, a cold start may requ-
                    -ire significant amount of time.
    */
    
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
    GB_NEO_6M gps(gb);

    GB_AHT10 aht(gb);
    GB_DESKTOP gdc(gb);
    GB_SNTL sntl(gb);

    // Pipers
    GB_PIPER breathpiper;
    GB_PIPER readpiper;
    GB_PIPER uploaderpiper;
    GB_PIPER recoverypiper;

    /*
        Control variables
    */
    bool REBOOT_FLAG = false;
    bool RECOVERY_MODE_FLAG = false;

    int UPLOAD_INTERVAL = 60 * 60 * 1000;
    int SAMPLING_INTERVAL = 5 * 60 * 1000;

    /*
        Timeouts
    */
    int ANTIFREEZE_REBOOT_DELAY = 5 * 86400 * 1000;
    
    /*
        Intervals
    */
    int BREATH_INTERVAL = 60 * 1000;


    void set_control_variables(JSONary data) {

        SAMPLING_INTERVAL = data.getint("SAMPLING_INTERVAL");
        REBOOT_FLAG = data.getboolean("REBOOT_FLAG");
        ANTIFREEZE_REBOOT_DELAY = data.getint("ANTIFREEZE_REBOOT_DELAY");
        RECOVERY_MODE_FLAG = data.getboolean("RECOVERY_MODE_FLAG");

        gb.log("Updating runtime variables -> Done");

        // Send the fresh list of control variables
        mqtt.publish("control/report", gb.controls.get());
    }

    void devicereboot () {

        gb.color("red").log("Rebooting device -> Done");

        // Reset flag
        REBOOT_FLAG = false;
        sd.updatecontrolbool("REBOOT_FLAG", REBOOT_FLAG, set_control_variables);

        buzzer.play(".....");
        sntl.reboot();
    }

    void parse_mqtt_message(String str){

        // Update the variables in SD card and in runtime memory
        sd.updatecontrol(str, set_control_variables);
        
        mqtt.publish("log/message", "Control variables updated.");

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
    }

    
    /* 
        ! Subscribe to topics
        This callback is executed the GB connects to the MQTT broker
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
    
    /* 
        ! Fetch latest control variable from the server
        The control variables can be updated using the web dashboard
            a. 'simplify: true' simplifies the response and returns a string.
            b. 'simplify: false' returns a json object.

    */
    void get_control_variable () {
        
        sntl.shield(120, [] {

            // //! Disconnect from the network
            // mcu.disconnect("cellular");

            //! Connect to network
            mcu.connect("cellular");
        });

        sntl.shield(30, [] {
            JSONary data;
            data

                // The 'key' is the variable value requested
                .set("key", "RECOVERY_MODE")

                // 'reset' set the value to the provided value if the value is true.
                .set("reset", "false")

                // 'simplify: true' simplifies the response and returns a string.
                .set("simplify", "true")

                // Device SN is required
                .set("device-sn", gb.globals.DEVICE_SN);

            if(http.post("v3/gatorbyte/control/get/bykey", data.get()) ) {
                String response = http.httpresponse();
                gb.controls.set("RECOVERY_MODE", response);
                sd.updatecontrolbool("RECOVERY_MODE", response);
                gb.log("Fetched RECOVERY_MODE flag: " + response);
            }

            // //! Disconnect from the network
            // mcu.disconnect("cellular");
        });
    }
    
    /*
        ! Prepare a queue file (with current iteration's data)
        The queue file will be read and data uploaded once the network is established.
        The file gets deleted if the upload is successful. 
    */
    void writetoqueue(CSVary csv) {
        gb.log("Writing to queue file: ", false);
        String currentdataqueuefile = sd.getavailablequeuefilename();
        gb.log(currentdataqueuefile, false);
        sd.writequeuefile(currentdataqueuefile, csv.get());
        gb.arrow().color().log("Done");
    }

    /*
        ! Upload all outstanding queue files to server
        Any files queued on the SD card will be sent one at a time.
    */
    void uploadtoserver() {

        // Send outstanding data and current data
        if (sd.getqueuecount() > 0) {
            gb.br().log("Found " + String(sd.getqueuecount()) + " outstanding queue files.");
            
            //! Connect to the network
            sntl.shield(120, [] {
            
                //! Connect to network
                mcu.connect("cellular");
                
                sntl.kick();

                //! Connect to MQTT servver
                mqtt.connect("pi", "abe-gb-mqtt");
                
            });
                
            //! Publish first ten queued-data with MQTT
            if (CONNECTED_TO_MQTT_BROKER) {
                int counter = 50;
                
                while (!sd.isqueueempty() && counter-- > 0) {
                    
                    sntl.shield(10, [] {

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
                    });
                }
            }

        }
        else {
            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files on SD. Skipping upload.");
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
        
        //! Mandatory GB setup function
        gb.setup();

        //! Initialize GatorByte and device configurator
        gb.configure(false, "gb-lry-test-drifter");
        gb.globals.ENFORCE_CONFIG = true;
        
        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();

        //! Configure peripherals
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        buzzer.configure({6}).initialize().play("...");
        mem.configure({true, SR0}).initialize();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        //! Detect GDC
        gdc.detect(false);
        
        //! Initialize Sentinel
        sntl.configure({false}, 9).initialize().ack(true).enablebeacon(0);

        // while(true) mcu.connect("cellular");

        sntl.shield(120, []() {
            
            // Check battery level
            gb.log("Current battery level: " + String(mcu.fuel("level")) + " %");

            // Initialize SD first to read the config file
            sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter");

            // Read SD config and control files
            sd.readconfig().readcontrol(set_control_variables);

            // Configure MQTT broker and connect 
            mqtt.configure(mqtt_message_handler, mqtt_on_connect);
            // http.configure("api.ezbean-lab.com", 80);
            
            // Initialize remaining peripherals
            gps.configure({true, SR2, SR10}).initialize();
            // bl.configure({true, SR3, SR11}).initialize().on().persistent();

            aht.configure({true, SR0}).initialize();
            rtc.configure({true, SR0}).initialize();
        });

        gb.log("Setup complete");
    }

    void preloop () {

        if (gb.env() == "development") gb.controls.set("RECOVERY_MODE", "false");

        /*
            This is a hack to eliminate the delay introduced by the DS3231 RTC.
        */
        gb.globals.INIT_SECONDS = rtc.timestamp().toDouble();

        gb.br().log("Environment: " + gb.env());
        gb.br().log("RTC time: " + rtc.date("MM/DD/YYYY") + ", " + rtc.time("hh:mm:ss"));
        gb.log("Init timestamp: " + String(gb.globals.INIT_SECONDS));
        gb.log("Sampling interval: " + String(SAMPLING_INTERVAL));

        gb.log("Recovery mode: " + String(gb.controls.getboolean("RECOVERY_MODE") ? "active" : "inactive"));
        gb.log("Recovery mode threshold: " + String(gb.controls.getint("LOW_BATT_THRESHOLD")) + " %");

        // Detect GDC
        gdc.detect(false);
    }

    void loop () {

        // GatorByte loop function
        gb.loop();
        
        bl.listen([] (String command) {
            Serial.println("Received command: " + command);

            if (command.contains("ping")) bl.print("pong");
        });

        // Check if the device needs to enter recovery mode
        if (gb.controls.getboolean("RECOVERY_MODE")) {
            gb.color("white").log("Recovery mode active");
            while (gb.controls.getboolean("RECOVERY_MODE")) {
                GPS_DATA gpsdata = gps.read(gb.env() == "development");
                String gps_lat = String(gpsdata.lat, 5), gps_lng = String(gpsdata.lng, 5);
                gps.fix() ? rgb.on("blue") : rgb.on("red"); 
                
                // Initialize CSVary object
                CSVary csv;
                int timestamp = rtc.timestamp().toInt();
                String date = rtc.date("MM/DD/YY");
                String time = rtc.time("hh:mm");

                // Construct CSV object
                csv
                    .clear()
                    .setheader("DEVICESN,TIMESTAMP,LAT,LNG,BVOLT,BLEV,MODE")
                    .set(gb.globals.DEVICE_SN)
                    .set(timestamp)
                    .set(gps_lat)
                    .set(gps_lng)
                    .set(mcu.fuel("voltage"))
                    .set(mcu.fuel("level"))
                    .set("recovery");

                gb.br().log("Recovery mode data point: ");
                gb.log(csv.getheader());
                gb.log(csv.getrows());
                
                // Write to queue
                writetoqueue(csv);

                // Upload queue files to server
                uploadtoserver();

                delay(gb.controls.getint("RECOVERY_MODE_INTERVAL"));
            };
        }

        if (uploaderpiper.ishot()) {
            gb.log("Uploader piper is hot");
        }

        if (readpiper.ishot()) {
            gb.log("Readings piper is hot");
            gps.on();
        }

        uploaderpiper.pipe(gb.controls.getint("UPLOAD_INTERVAL"), true, [] (int counter) {
            uploadtoserver();
        });


        // Take readings
        readpiper.pipe(SAMPLING_INTERVAL, true, [] (int counter) {

            sntl.shield(180, [] {

                /*
                    ! Check the current state of the system and take actions accordingly
                    Get latest GPS coordinates
                */
                GPS_DATA gpsdata = gps.read(gb.env() == "development");
                String gps_lat = String(gpsdata.lat, 5), gps_lng = String(gpsdata.lng, 5);
                gps.fix() ? rgb.on("blue") : rgb.on("red");

                // Initialize CSVary object
                CSVary csv;
                int timestamp = rtc.timestamp().toInt();
                String date = rtc.date("MM/DD/YY");
                String time = rtc.time("hh:mm");

                // Construct CSV object
                csv
                    .clear()
                    .setheader("DEVICESN,TIMESTAMP,DATE,TIME,TEMP,RH,FLTP,LAT,LNG,BVOLT,BLEV")
                    .set(gb.globals.DEVICE_SN)
                    .set(timestamp)
                    .set(date)
                    .set(time)
                    .set(aht.temperature())
                    .set(aht.humidity())
                    .set(gb.globals.FAULTS_PRIMARY)
                    .set(gps_lat)
                    .set(gps_lng)
                    .set(mcu.fuel("voltage"))
                    .set(mcu.fuel("level"));

                gb.br().log("Current data point: ");
                gb.log(csv.getheader());
                gb.log(csv.getrows());

                // //! Upload current data to desktop client
                // gdc.send("data", csv.getheader() + BR + csv.getrows());
                
                /*
                    ! Prepare a queue file (with current iteration's data)
                    The queue file will be read and data uploaded once the network is established.
                    The file gets deleted if the upload is successful. 
                */
                writetoqueue(csv);
            });
        });

        recoverypiper.pipe(30000, true, [] (int counter) {
            // get_control_variable();

            float battlevel = mcu.fuel("level");
            gb.log("Current battery level: " + String(battlevel), false);

            if (battlevel < gb.controls.getint("LOW_BATT_THRESHOLD")) {
                gb.arrow().color("yellow").log("Enabling recovery mode.");
                gb.controls.set("RECOVERY_MODE", "true");
                sd.updatecontrolbool("RECOVERY_MODE", true);
            }

        });

        //! Sleep
        mcu.sleep(on_sleep, on_wakeup);
    }
#endif