#include "../platform.h"

#if GB_PLATFORM_IS_LRY_DRIFTER_V1
    
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
        Control variables
    */
    bool REBOOT_FLAG = false;
    bool RESET_VARIABLES_FLAG = false;

    int STATE_UPLOAD_INTERVAL = 60 * 60 * 1000;
    int SAMPLING_INTERVAL = 10 * 60 * 1000;

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
        gb.configure(false, "gb-sasbjep-test-station");
        gb.globals.ENFORCE_CONFIG = true;
        
        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        //! Configure peripherals
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        buzzer.configure({6}).initialize().play("...");
        
        //! Initialize Sentinel
        sntl.configure({false}, 9).initialize().ack(true).enablebeacon(0);

        sntl.shield(120, []() {

            // Initialize SD first to read the config file
            sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter");

            // Read SD config and control files
            sd.readconfig().readcontrol(set_control_variables);

            // Configure MQTT broker and connect 
            mqtt.configure(mqtt_message_handler, mqtt_on_connect);
            
            // Initialize remaining peripherals
            bl.configure({true, SR3, SR11}).initialize();
            gps.configure({true, SR2, SR10}).initialize();

            mem.configure({true, SR0}).initialize();
            aht.configure({true, SR0}).initialize();
            rtc.configure({true, SR0}).initialize();
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
        gb.log("Sampling interval: " + String(SAMPLING_INTERVAL));
    }
    void loop () {

        // GatorByte loop function
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
            String date = rtc.date("MM/DD/YY");
            String time = rtc.time("hh:mm");

            // Construct CSV object
            csv
                .clear()
                .setheader("PROJECTID,DEVICESN,TIMESTAMP,DATE,TIME,TEMP,RH,FLTP,LAT,LNG,BVOLT,BLEV")
                .set(gb.globals.PROJECT_ID)
                .set(gb.globals.DEVICE_ID)
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

            //! Write current data to SD card
            sd.writeCSV(csv);
            gb.log("Readings written to SD card");

            gb.br().log("Current data point: ");
            gb.log(csv.getheader());
            gb.log(csv.getrows());

            //! Upload current data to desktop client
            gdc.send("data", csv.getheader() + BR + csv.getrows());
            
            /*
                ! Prepare a queue file (with current iteration's data)
                The queue file will be read and data uploaded once the network is established.
                The file gets deleted if the upload is successful. 
            */

            String currentdataqueuefile = sd.getavailablequeuefilename();
            gb.log("Wrote to queue file: " + currentdataqueuefile);
            sd.writeString(currentdataqueuefile, csv.get());
        });

        // Send outstanding data and current data
        if (sd.getqueuecount() > 0) {
            gb.br().log("Found " + String(sd.getqueuecount()) + " outstanding queue files.");
            
            //! Publish first ten queued-data with MQTT
            sntl.shield(120, [] {

                sntl.shield(120, [] {
                
                //! Connect to network
                mcu.connect("cellular");
                
                sntl.kick();

                //! Connect to MQTT servver
                mqtt.connect("pi", "abe-gb-mqtt");
                
            });

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
            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files on SD. Skipping upload.");
        }

        //! Sleep
        mcu.sleep(on_sleep, on_wakeup);
    }

#endif