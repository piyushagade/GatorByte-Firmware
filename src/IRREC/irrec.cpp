#include "../platform.h"

#if GB_PLATFORM_IS_IRREC

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
    GB_AT24 mem(gb);
    GB_SD sd(gb);
    GB_DS3231 rtc(gb);
    GB_BUZZER buzzer(gb);

    // Atlas Scientific sensors
    GB_AT_SCI_DO dox(gb);
    GB_AT_SCI_RTD rtd(gb);
    GB_AT_SCI_PH ph(gb);

    GB_AHT10 aht(gb);
    GB_DESKTOP gdc(gb);
    GB_SNTL sntl(gb);

    /* 
        ! Procedure to handle incoming MQTT messages
        It is mandatory to define this function if GB_MQTT library is instantiated above
    */

    void mqtt_message_handler(String topic, String message) {

        gb.log("\n\nIncoming topic: " + topic + "\n\n");
    }

    /* 
        ! Actions to perform on MQTT connect/reconnect
        It is mandatory to define this function if GB_MQTT library is instantiated above
    */

    void mqtt_on_connect() {

        mqtt.subscribe("gb-lab/test");

    }
    

    void setsensors(String state) {
        // return;
        if (state == "on") {
            dox.on();
            ph.on();
            rtd.on();
        }
        else if (state == "off") {
            dox.off();ph.off();rtd.off();
        }
        delay(500);
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
        
        // Disable sentinel
        setsensors("on");
        sntl.disable();

        delay(1000);
    }

    /* 
        ! Actions to perform after the microcontroller wakes up
        You can turn specific components on/off.
    */

    void on_wakeup() {
        
        rgb.on("magenta");
        delay(2000);

        gb.log("The device is now awake.");
        gdc.send("highlight-yellow", "Device awake");

        // // Connect to cellular network    
        // mcu.connect();

        // // Connect to MQTT broker
        // mqtt.connect("pi", "abe-gb-mqtt");
    }

    void nwdiagnostics () {
        
        sntl.watch(45, [] {
            
            //! Connect to network
            mcu.connect("cellular");

            sntl.kick();

            //! Connect to MQTT server
            mqtt.connect("pi", "abe-gb-mqtt");
        
        });

        rgb.rainbow(5000);
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
        gb.configure(false);

        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        //! Configure peripherals
        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        buzzer.configure({6}).initialize().play("...");

        sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter").readconfig();

        //! Configure MQTT broker and connect 
        mqtt.configure("mqtt.ezbean-lab.com", 1883, gb.DEVICE_SN, mqtt_message_handler, mqtt_on_connect);

        //! Configure sensors
        dox.configure({true, SR8}, {0x67}).initialize().persistent().off();
        ph.configure({true, SR7}, {0x65}).initialize().persistent().off();
        rtd.configure({true, SR6}, {0x68}).initialize(true).off();

        //! Initialize Sentinel
        setsensors("on");
        sntl.configure(9).initialize();

        setsensors("on");
        sntl.interval("sentinence", 15).enable();
        
        setsensors("on");
        mem.configure({true, SR0}).initialize();
        aht.configure({true, SR0}).initialize();
        
        setsensors("off");dox.on();ph.on();
        rtc.configure({true, SR0}).initialize();
        
        setsensors("on");
        sntl.disable();

        // Detect GDC
        setsensors("off");
        gdc.detect();
        setsensors("on");

        gb.log("Setup complete");
    }

    void test_sensor_isolation() {
        // dox.on();
        ph.quickreadsensor(50);
    }

    /*
        Test SNTL
    */
    void test_sntl() {
        
        // Do nothing
        rgb.rainbow(5000);
        delay(1000);

        // Turn on sentinence
        rgb.on("cyan");
        delay(1000);
        rgb.off();
        delay(500);
        sntl.enable();
        buzzer.play("xx");

        // Wait 10 seconds
        rgb.on("blue");
        int timer = 0;
        while (timer++ < 15) {
            delay(1000);
            int response = sntl.ask();
            // if (response > -1 && response < 65535) gb.log(String(response));
        }

        // Turn off sentinence
        rgb.on("magenta");
        delay(1000);
        rgb.off();
        delay(500);
        sntl.disable();
        buzzer.play("x");
    }

    void loop () {
  
        // return nwdiagnostics();

        // return test_sensor_isolation();

        // test_sntl();

        // gb.MODE = "dummy";
        
        //! Call GatorByte's loop procedure
        setsensors("off"); dox.on();ph.on();
        gb.loop();

        // Take readings if due
        if (READINGS_DUE) {

            gb.log("Readings are due. Taking next set of readings");

            // Enable sentinel
            setsensors("on");
            sntl.interval("sentinence", 150).enable();

            //! Get sensor readings
            setsensors("off");
            float read_rtd_value = rtd.readsensor(), read_ph_value = ph.readsensor(), read_dox_value = dox.readsensor();
            
            // Get timestamp
            setsensors("off");
            dox.on();ph.on();
            String timestamp = rtc.timestamp();

            gb.log("Constructing CSV object", false);

            //! Construct CSV data object
            setsensors("on");
            CSVary csv;
            csv
                .clear()
                .setheader("SURVEYID,RTD,PH,DO,TIMESTAMP,TEMP,RH,FLTP")
                .set(gb.MODE == "dummy" ? gb.MODE : gb.PROJECT_ID)
                .set(read_rtd_value)
                .set(read_ph_value)
                .set(read_dox_value) 
                .set(timestamp)
                .set(aht.temperature())
                .set(aht.humidity())
                .set(FAULTS_PRIMARY);
            
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

            setsensors("off"); dox.on(); ph.on();
            LAST_READING_TIMESTAMP = rtc.timestamp().toInt();
            mem.write(12, String(LAST_READING_TIMESTAMP));

            // Disable sentinel
            setsensors("on");
            sntl.disable();
        }
        else {
            gb.log("Readings not due this iteration.");
        }

        // Enable sentinel
        setsensors("on");
        sntl.interval("sentinence", 60).enable();

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
            // mqtt.publish("log/message", "Cell signal: " + String(mcu.RSSI) + " dB");
            // mqtt.publish("log/message", "SD write complete");

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

        // TODO: Turn sensor persistence off

        setsensors("off"); ph.on(); dox.on();
        SECONDS_SINCE_LAST_READING = rtc.timestamp().toInt() - LAST_READING_TIMESTAMP;
        
        setsensors("on");
        sntl.disable();

        //! Sleep
        mcu.sleep(on_sleep, on_wakeup);
    }

    void breathe() {
        gb.breathe();
    }

#endif