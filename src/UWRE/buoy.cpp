#include "../platform.h"

#if GB_PLATFORM_IS_BUOY

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

    GB_HTTP http(gb);
    GB_MQTT mqtt(gb);
    GB_74HC595 ioe(gb);
    GB_TCA9548A tca(gb);

    // Peripherals and devices
    GB_RGB rgb(gb);
    GB_PWRMTR pwr(gb);
    GB_AT24 mem(gb);
    GB_SD sd(gb);
    GB_DS3231 rtc(gb);
    GB_BOOSTER booster(gb);
    GB_NEO_6M gps(gb);
    GB_AT_09 bl(gb);
    // GB_MPU6050 acc(gb);
    GB_BUZZER buzzer(gb);

    // Atlas Scientific sensors
    GB_AT_SCI_DO dox(gb);
    GB_AT_SCI_RTD rtd(gb);
    GB_AT_SCI_EC ec(gb);
    GB_AT_SCI_PH ph(gb);

    GB_AHT10 aht(gb);
    GB_CONSOLE cm(gb);
    GB_CONFIGURATOR cfg(gb);
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

        else if (topic == "gb-lab/calibration/perform") {

            /* Payload (Message) format
            [sensor]:[step]
            */

            gb.globals.MODE = "calibrate";
            String sensor = gb.split(message, ':', 0);
            int step = gb.split(message, ':', 1).toInt();

            gb.log("Calibration command received for: " + sensor);

            String response;
            // if (sensor == "ec") response = ec.calibrate(step);
            // else if (sensor == "ph") response = ph.calibrate(step);
            // else if (sensor == "dox") response = dox.calibrate(step);
            // else if (sensor == "rtd") response = rtd.calibrate(step);

            gb.log(message + "::" + response);

            mqtt.publish("gb-aws-server/calibration/response", message + "::" + response);
        }

        else if (topic == "gb-lab/calibration/abort") {
            gb.globals.MODE = "read";
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

        // Connect to cellular network    
        mcu.connect();

        // Connect to MQTT broker
        mqtt.connect("pi", "abe-gb-mqtt");

        // bl.on();
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

    // Select I2C BUS
    void TCA9548A(uint8_t bus) {
        Wire.beginTransmission(0x70);
        Wire.write(1 << bus);
        Wire.endTransmission();
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
        gb.configure(false, "gb-lab-buoy");
        
        //! Configure peripherals
        ioe.configure({3, 4, 5}, 2).initialize();
        tca.configure({A2}).initialize();

        //! Configure microcontroller
        mcu.i2c().debug(Serial, 9600).serial(Serial1, 9600).configure("", "");

        rgb.configure({0, 1, 2}).initialize(0.2).on("magenta");
        buzzer.configure({6}).initialize().play("...");
        
        // Detect GDC
        gdc.detect(false);
        
        sntl.configure({true, 4}, 9).initialize().ack(true).disablebeacon();

        sntl.shield(75, []() { 

            //! Initialize other peripherals
            bl.configure({true, SR3, SR11}).initialize();
            gps.configure({true, SR2, SR10}).initialize();
            
            booster.configure({true, SR1}).on();
            sd.configure({true, SR15, 7, SR4}).state("SKIP_CHIP_DETECT", true).initialize("quarter").readconfig();
            mem.configure({true, SR0}).initialize();
            aht.configure({true, SR0}).initialize();
            rtc.configure({true, SR0}).initialize().sync();
            // acc.configure({true, SR5}).initialize();

            //! Configure MQTT broker and connect 
            mqtt.configure("mqtt.ezbean-lab.com", 1883, gb.globals.DEVICE_SN, mqtt_message_handler, mqtt_on_connect);

            //! Configure sensors
            rtd.configure({true, SR6, true, 0}).initialize(true);
            dox.configure({true, SR8, true, 2}).initialize(true);
            ec.configure({true, SR9, true, 3}).initialize(true);
            ph.configure({true, SR7, true, 1}).initialize(true);
        });

        // Detect GDC
        gdc.detect();

        gb.log("Setup complete");
    }

    void turbidity () {
        float value = analogRead(A1);
        float voltage = value * (3.3 / 1024.0);
        float ntu = -1120.4 * voltage * voltage + 5742.3 * voltage - 4352.9;
        gb.log(voltage, false);
        gb.log(", ", false);
        gb.log(ntu);
        delay(1000);
    }

    void loop () {

        bool DUMMY_GPS = false;
        // gb.MODE = "dummy";
        
        //! Call GatorByte's loop procedure
        gb.loop();
        
        gb.log(gb.globals.SLEEP_DURATION / 1000);
        gb.log(gb.globals.LOOPTIMESTAMP - gb.globals.LAST_READING_TIMESTAMP);
        gb.log(gb.globals.READINGS_DUE);

        // Take readings if due
        if (gb.globals.READINGS_DUE) {

            gb.log("Readings are due. Taking next set of readings");

            //! Get sensor readings
            float read_rtd_value = rtd.readsensor(), read_ph_value = ph.readsensor(), read_ec_value = ec.readsensor(), read_dox_value = dox.readsensor();

            //! Get GPS data
            GPS_DATA gps_data = gps.read(DUMMY_GPS);
            String gps_lat = String(gps_data.lat, 5), gps_lng = String(gps_data.lng, 5), gps_attempts = String(gps_data.last_fix_attempts);
            gps.fix() ? rgb.on("blue") : rgb.on("red");
            gps.off();
            bl.on();

            //! Log GPS data to console
            gb.log("GPS: " + String(gps.fix() ? "Located" : "Failed") + ": " + gps_lat + ", " + gps_lng);

            gb.log("Constructing CSV object", false);

            //! Construct CSV data object
            CSVary csv;
            csv
                .clear()
                .setheader("SURVEYID,RTD,PH,DO,EC,LAT,LNG,TIMESTAMP,TEMP,RH,GPSATTEMPTS,CELLSIGSTR")
                .set(gb.globals.MODE == "dummy" ? gb.globals.MODE : gb.globals.PROJECT_ID)
                .set(read_rtd_value)
                .set(read_ph_value)
                .set(read_dox_value)
                .set(read_ec_value)
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

            gb.globals.LAST_READING_TIMESTAMP = rtc.timestamp().toInt();
            mem.write(12, String(gb.globals.LAST_READING_TIMESTAMP));
        }
        else {
            gb.log("Readings not due this iteration.");
        }

        /*
            ! Send data to server
        */
        if (sd.getqueuecount() > 0) {

            gps.off();

            gb.br().log("Found " + String(sd.getqueuecount()) + " queue files");

            //! Connect to cellular 
            mcu.connect();

            //! Connect to MQTT broker
            mqtt.connect("pi", "abe-gb-mqtt");

            gb.log("MQTT connection attempted");
            
            mqtt.publish("log/message", "Iteration: " + String(gb.globals.ITERATION));
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