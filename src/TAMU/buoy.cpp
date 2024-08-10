#include "../platform.h"

#if GB_PLATFORM_IS_TAMU_BUOY

    /*
        Buoy goals
        -------------`
        1. Short-term deployment
            a. Sampling frequency - 5 minutes.
            b. Upload strategies
                i.  Once every hour; Recovery mode implementation required
                ii. As soon as reading is taken; Recovery mode not required (This one)
    */
    
    /* 
        ! Gatorbyte library
        Import the GatorByte library (GBL).
    */
    #include "GB.h"

    /* 
        ! Gatorbyte core instancegb.. All the peripherals and sensors 
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
    // GB_HTTP http(gb);
    GB_74HC595 ioe(gb);
    GB_TCA9548A tca(gb);

    // Peripherals and devices
    GB_RGB rgb(gb);
    GB_AT24 mem(gb);
    GB_SD sd(gb);
    GB_DS3231 rtc(gb);
    // GB_BOOSTER booster(gb);
    GB_AT_09 bl(gb);
    GB_BUZZER buzzer(gb);
    GB_NEO_6M gps(gb);

    // Atlas Scientific sensors
    GB_AT_SCI_DO dox(gb);
    GB_AT_SCI_RTD rtd(gb);
    GB_AT_SCI_EC ec(gb);
    GB_AT_SCI_PH ph(gb);

    GB_AHT10 aht(gb);
    GB_SNTL sntl(gb);
    GB_DESKTOP gdc(gb);

    /*
        Control variables
    */
    bool REBOOT_FLAG = false;
    
    void set_control_variables(JSONary data) {

        REBOOT_FLAG = data.getboolean("REBOOT_FLAG");

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
        sd.updateallcontrol(str, set_control_variables);
        
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
        // gdc.send("highlight-yellow", "Device awake");
    }
    
    /* 
        ! Fetch latest control variable from the server
        The control variables can be updated using the web dashboard
            a. 'simplify: true' simplifies the response and returns a string.
            b. 'simplify: false' returns a json object.

    */
    // void get_control_variable () {
        
    //     sntl.watch(120, [] {

    //         // //! Disconnect from the network
    //         // mcu.disconnect("cellular");

    //         //! Connect to network
    //         mcu.connect("cellular");
    //     });

    //     sntl.watch(30, [] {
    //         JSONary data;
    //         data

    //             // The 'key' is the variable value requested
    //             .set("key", "RECOVERY_MODE")

    //             // 'reset' set the value to the provided value if the value is true.
    //             .set("reset", "false")

    //             // 'simplify: true' simplifies the response and returns a string.
    //             .set("simplify", "true")

    //             // Device SN is required
    //             .set("device-sn", gb.globals.DEVICE_SN);

    //         if(http.post("v3/gatorbyte/control/get/bykey", data.get()) ) {
    //             String response = http.httpresponse();
    //             gb.controls.set("RECOVERY_MODE", response);
    //             sd.updatecontrolbool("RECOVERY_MODE", response);
    //         }

    //         // //! Disconnect from the network
    //         // mcu.disconnect("cellular");
    //     });
    // }
    
    /*
        ! Prepare a queue file (with current iteration's data)
        The queue file will be read and data uploaded once the network is established.
        The file gets deleted if the upload is successful. 
    */
    void writetoqueue(CSVary csv) {
        // gb.log("Writing to queue file: ", false);
        String currentdataqueuefile = sd.getavailablequeuefilename();
        if (currentdataqueuefile.length() > 0) {
            gb.log(currentdataqueuefile, false);
            sd.writequeuefile(currentdataqueuefile, csv.get());
            // gb.arrow().color().log("Done");
        }
    }

    /*
        ! Upload all outstanding queue files to server
        Any files queued on the SD card will be sent one at a time.
    */
    void uploadqueuetoserver() {

        // if (true) return;

        // Send outstanding data and current data
        if (sd.getqueuecount() > 0) {
            gb.br().log("Found " + String(sd.getqueuecount()) + " outstanding queue files.");
            
            //! Connect to the network
            sntl.watch(120, [] {
            
                //! Connect to network
                mcu.connect();
                
                sntl.kick();

                //! Connect to MQTT servver
                mqtt.connect();
                
            });
                
            //! Publish first ten queued-data with MQTT
            if (CONNECTED_TO_MQTT_BROKER) {
                int counter = 50;
                
                while (!sd.isqueueempty() && counter-- > 0) {
                    
                    sntl.watch(30, [] {
                        
                        String queuefilename = sd.getfirstqueuefilename();
                        gb.log("Sending queue file: " + queuefilename);

                        // Attempt publishing queue-data
                        if (mqtt.publish("data/set", sd.readqueuefile(queuefilename))) {

                            // Remove queue file
                            sd.removequeuefile(queuefilename);
                        }

                        else {
                            gb.log("Queue file not deleted.");
                            mqtt.disconnect();
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
        ! Upload the current CSV file to server
        This will be executed if the SD card is not detected
    */
    void uploadcurrentdatatoserver(CSVary csv) {

        //! Connect to the network
        sntl.watch(120, [] {
        
            //! Connect to network
            mcu.connect();
            
            sntl.kick();

            //! Connect to MQTT servver
            mqtt.connect();
            
        });
            
        //! Publish first ten queued-data with MQTT
        if (CONNECTED_TO_MQTT_BROKER) {

            // Attempt publishing queue-data
            if (mqtt.publish("data/set", csv.get())) {
                // Do nothing
            }
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
        gb.configure();
        gb.globals.ENFORCE_CONFIG = true;
        
        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();
        tca.configure({A2}).initialize(); 

        //! Configure peripherals
        rgb.configure({0, 1, 2}).initialize(1).on("magenta");
        buzzer.configure({6}).initialize().play("...");

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        //! Initialize Sentinel
        sntl.configure({true, 4}, 9).initialize().setack(true).enablebeacon(0);

        while(false) {
            // sntl.configure({true, 4}, 9).initialize();
            sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter");
        }
        
        sntl.watch(120, []() {

            // Initialize SD first to read the config file
            // while (1) 
            sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter");

            // while (true) delay(100);
            
            // Initialize EEPROM
            mem.configure({true, SR0}).initialize();

            // mem.writecontrolvariables("REBOOT_FLAG:false");
            // mem.writecontrolvariables("REBOOT_FLAG:false\nRESET_VARIABLES_FLAG:false\nQUEUE_UPLOAD_INTERVAL:60000\nWLEV_SAMPLING_INTERVAL:30000");

            // // mem.getcontrolvariables();
            // gb.log(mem.getcontrolvariables());

            // Process device configuration
            gb.processconfig();

            // Read SD config and control files
            sd.readcontrol(set_control_variables);

            // Configure MQTT broker and connect 
            mqtt.configure(mqtt_message_handler, mqtt_on_connect);
            
            //! Initialize remaining peripherals
            rtc.configure({true, SR0}).initialize();
            gps.configure({true, SR2, SR10}).initialize();
            bl.configure({true, SR3, SR11}).initialize().on().persistent();
            aht.configure({true, SR0}).initialize();

            //! Configure sensors
            rtd.configure({true, SR6, true, 0}).initialize(true);
            dox.configure({true, SR8, true, 2}).initialize(true);
            ec.configure({true, SR9, true, 3}).initialize(true);
            ph.configure({true, SR7, true, 1}).initialize(true);

        });

        gb.log("Setup complete");
    }

    void preloop () {

        // Set sensor power modes
        // rtd.persistent().on();
        ph.persistent().on();
        // dox.persistent().on();
        // ec.persistent().on();

        /*
            This is a hack to eliminate the delay introduced by the DS3231 RTC.
        */
        gb.globals.INIT_SECONDS = rtc.timestamp().toDouble();

        gb.br().log("Environment: " + gb.env());
        gb.log("Battery: " + String(mcu.fuel("level")) + " %");
        gb.br().log("RTC time: " + rtc.date("MM/DD/YYYY") + ", " + rtc.time("hh:mm:ss"));
        gb.br().color("white").log("Error report").log(gb.globals.INIT_REPORT);

        // Detect GDC
        // gdc.detect(false);

        /*
            ! Blow the fuse
            This will enable the master sentinel timer (TIMER 3).

            When the fuse is set (in GDC), the GB device can have it's
            battery connected with the power relay turned off without
            the Sentinel turning on the power relay. This is acheived
            by disabling the master timer (TIMER 3).
        */
        bool fusewasset = !sntl.tell(39, 5);
        if (!gb.globals.GDC_CONNECTED && fusewasset) sntl.blowfuse();

        /*
            Stay in GPS read mode if the fuse was set. To get out 
            of this mode, reboot the GatorByte
        */
        if (!gb.globals.GDC_CONNECTED && fusewasset) {

            gb.color("cyan").log("Waiting in GPS read mode until reboot.");
            while(true) {
                gps.read();
                gps.fix() ? rgb.on("blue").wait(2000).revert() : rgb.on("red").wait(2000).revert(); 
            }
        }
        
        //! Connect to the network
        if (!gb.globals.GDC_CONNECTED) sntl.watch(120, [] {
            mcu.connect();
        });
    }

    void loop () {

        
        // GatorByte loop function
        gb.loop();


        // bl.listen([] (String command) {
        //     gb.log("Received command: " + command);
        //     if (command.contains("ping")) gb.log("pong");
        //     if (command.contains("fuse")) {
        //         sntl.setfuse();
        //         bl.print("ok");
        //     }
        // });

        sntl.watch(1200, [] {

            //! Turn GPS on
            gps.on();

            //! Get sensor readings
            float read_rtd_value = rtd.readsensor(), read_ph_value = ph.readsensor(), read_ec_value = ec.readsensor(), read_dox_value = dox.readsensor();

            /*
                ! Check the current state of the system and take actions accordingly
                Get latest GPS coordinates
            */
            GPS_DATA gpsdata = gps.read(true);

            String gps_lat = String(gpsdata.lat, 5), gps_lng = String(gpsdata.lng, 5);
            gps.fix() ? rgb.on("blue").wait(2000).revert() : rgb.on("red").wait(2000).revert(); 

            // Initialize CSVary object
            CSVary csv;
            String timestamp = rtc.timestamp();
            String date = rtc.date("MM/DD/YY");
            String time = rtc.time("hh:mm");

            // Construct CSV object
            csv
                .clear()
                .setheader("DEVICESN,TIMESTAMP,DATE,TIME,RTD,PH,DO,EC,TEMP,RH,LAT,LNG,BVOLT")
                .set(gb.globals.DEVICE_SN)
                .set(timestamp)
                .set(date)
                .set(time)
                .set(read_rtd_value)
                .set(read_ph_value)
                .set(read_dox_value)
                .set(read_ec_value)
                .set(aht.temperature())
                .set(aht.humidity())
                .set(gps_lat)
                .set(gps_lng)
                .set(mcu.fuel("voltage"))
            ;

            gb.br().log("Current data point: ");
            gb.log(csv.getheader());
            gb.log(csv.getrows());

            /*
                ! Prepare a queue file (with current iteration's data)
                The queue file will be read and data uploaded once the network is established.
                The file gets deleted if the upload is successful. 
            */
           
            // if (sd.device.detected) writetoqueue(csv);
            // else uploadcurrentdatatoserver(csv);

            if (sd.device.detected) sd.writeCSV("/readings/readings.csv", csv);
            uploadcurrentdatatoserver(csv);
        });

        // //! Upload data to server
        // if (sd.device.detected) uploadqueuetoserver();

        //// Set sleep configuration
        // mcu.set_sleep_callback(on_sleep);
        // mcu.set_wakeup_callback(on_wakeup);

        //! Sleep
        mcu.sleep();
    }

#endif