/*
    This module needs to be mcu agnostic. 
    This module uses PubSubClient library

*/

#ifndef GB_MQTT_h
#define GB_MQTT_h

#ifndef GB_h
    #include "../GB.h"
#endif

#ifndef PubSubClient_h
    #include "../../../PubSubClient/src/PubSubClient.h"
#endif

#if defined(SPARK) || defined(PARTICLE)
    #include "spark_wiring_string.h"
    #include "spark_wiring_tcpclient.h"
    #include "spark_wiring_usbserial.h"
    #include "application.h"
#elif defined(ARDUINO)
    #include <Arduino.h>
    #include "IPAddress.h"
    #include "Client.h"
    #include "Stream.h"
#endif

class GB_MQTT : public GB_DEVICE {
    public:
        GB_MQTT(GB &gb);
        
        // Type definitions
        typedef void (*callback_t_on_message)(String, String);
        typedef void (*callback_t_on_connect)();

        DEVICE device = {
            "mqtt",
            "MQTT broker interface"
        };

        String BROKER_IP = "";
        int BROKER_PORT = -1;
        String CLIENT_ID = "";
        String USER = "";
        String PASS = "";
        int PUBQOS = 1;
        int SUBQOS = 1;
        bool ACK_RECEIVED = false;

        GB_MQTT& configure(callback_t_on_message, callback_t_on_connect);
        GB_MQTT& configure(String, int, String, callback_t_on_message, callback_t_on_connect);
        GB_MQTT& connect();
        GB_MQTT& disconnect();
        GB_MQTT& update();
        GB_MQTT& ack(String id);
        GB_MQTT& noack();

        bool connected();
        bool connected(bool);
        bool waituntilresponse(String, unsigned long, bool);
        bool publish(String, String, String);
        bool publish(String, String);
        void subscribe(String);

    private:

        GB *_gb;
        GB_MCU *_mcu;
        PubSubClient _mqttclient;
        callback_t_on_message _on_message_handler;
        callback_t_on_connect _on_connect_handler;
        int _reconnection_attempt_count = 0;
        int _last_pinged_at = 0;
        bool _wait_for_ack = false;
        String _waiting_for_response_topic = "";
        bool _waiting_for_response_flag = false;
        String _ack_id = "";
};

GB_MQTT::GB_MQTT(GB &gb) {
    _gb = &gb;
    // _mcu = &_gb->getmcu();
    _gb->includelibrary(this->device.id, this->device.name);
    _gb->devices.mqtt = this;
}

GB_MQTT& GB_MQTT::configure(callback_t_on_message on_message_ptr, callback_t_on_connect on_connect_ptr) {
    return this->configure(_gb->globals.SERVER_URL, _gb->globals.SERVER_PORT, _gb->globals.DEVICE_SN, on_message_ptr, on_connect_ptr);
}
GB_MQTT& GB_MQTT::configure(String ip, int port, String client_id, callback_t_on_message on_message_ptr, callback_t_on_connect on_connect_ptr) {
    _gb->includedevice(this->device.id, this->device.name);

    _gb->log("Configuring MQTT client", false);
    this->BROKER_IP = ip;
    this->BROKER_PORT = port;
    this->CLIENT_ID = client_id;
    this->_on_message_handler = on_message_ptr;
    this->_on_connect_handler = on_connect_ptr;

    if(ip.length() == 0) _gb->arrow().log("Failed. Invalid Broker IP");
    else if(port <= 0) _gb->arrow().log("Failed. Invalid Broker port");
    else _gb->arrow().log("Done");

    return *this;
}

GB_MQTT& GB_MQTT::disconnect() {

    _gb->log("Disconnecting MQTT broker", false);

    if (this->_mqttclient.connected()) {
        this->_mqttclient.disconnect();
        CONNECTED_TO_MQTT_BROKER = false;
        _gb->arrow().log("Done");
    }
    else _gb->arrow().log("Already disconnected");
    return *this;
}

GB_MQTT& GB_MQTT::connect() {
    
    if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->on(8);

    // Set instance variables
    this->USER = _gb->globals.MQTTUSER;
    this->PASS = _gb->globals.MQTTPASS;

    /*
        ! Disconnect MQTT client
        This should be done only when the MODEM and cellular network had been
        reset, e.g. after a sleep session.

        If the GatorByte is being operated in a non-sleep mode, the MQTT client should/need
        not be disconnected.
    */
    // this->disconnect();

    _gb->log("Connecting to MQTT broker: " + String(this->USER) + "@" + String(this->BROKER_IP) + ":" + String(this->BROKER_PORT), false);

    // MQTT loop (update '_state')
    for (int i = 0; i < 6; i++) { this->_mqttclient.loop(); delay(1); }

    // Get connection state
    int8_t state = this->_mqttclient.state();

    if (state == 0 && this->_mqttclient.connected()) {
        _gb->arrow().log("Already connected");
        
        // Blink green 2 times
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->blink("green", 2, 300, 200);
        _gb->getdevice("buzzer")->play("..");

        return *this;
    }

    // If MQTT connection lost/error
    else if (state < 0) {
        if (state == -1) _gb->arrow().color("red").log("Not connected", false).color();
        else _gb->arrow().color("red").log("Connection " + String(state == -2 ? "failed" : (state == -3 ? "lost" : (state == -4 ? "timeout" : "error"))), false).color();
    }

    #ifdef ESP32
        if(!CONNECTED_TO_NETWORK || !CONNECTED_TO_INTERNET || !MODEM_INITIALIZED) {
            _gb->arrow().log("Skipped. Not connected to the Internet.");
            
            // Blink red 2 times
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->blink("red", 2, 300, 200);
            _gb->getdevice("buzzer")->play("..-");
            
            return *this;
        }
    #else
        if(!CONNECTED_TO_NETWORK || !CONNECTED_TO_INTERNET || !MODEM_INITIALIZED) {
            if (!CONNECTED_TO_NETWORK) _gb->arrow().log("Skipped");
            else if (!CONNECTED_TO_INTERNET) _gb->arrow().log("Skipped");
            else _gb->arrow().log("Skipped");
            
            // Blink red 2 times
            if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->blink("red", 2, 300, 200);
            _gb->getdevice("buzzer")->play("..-");
            
            return *this;
        }
    #endif

    /*
        ! Close cellular socket

        Error solution:
        https://ubidots.com/community/t/mqtt-pubsubclient-problem-with-timers-reconnect/2460/7

        Findings:
            1. 12/2021 - Initial (short-term) inspection seems to work.
            2. 11/2022 - Stopping client causes failure in reconnection? Needs testing
    */
    // _gb->getmcu()->getclient().stop();
    // _gb->getmcu()->getsslclient().stop();

    // Set broker
    IPAddress ip(3,13,100,232);
    this->_mqttclient.setClient(_gb->getmcu()->getclient());
    this->_mqttclient.setServer(ip, this->BROKER_PORT);
    this->_mqttclient.setCallback(_on_message_handler);

    // // Enable watchdog
    // _gb->getmcu()->watchdog("enable");
    
    //! Attempt connection
    bool success = false; int counter = 0;
    while (counter++ < 3 && !success) {
        

        if (!_gb->getmcu()->getclient().available()) {
            _gb->getmcu()->getclient().flush();
            _gb->getmcu()->getclient().readString();
            _gb->getmcu()->deleteclient();
        }

        success = this->_mqttclient.connect(_gb->s2c(this->CLIENT_ID), _gb->s2c(this->USER), _gb->s2c(this->PASS)); 

        // Reset watchdog
        _gb->getmcu()->watchdog("reset");
        
        if (!success) {
            _gb->log(" .", false);
            delay(1000);
        }
    }

    // Reset watchdog
    _gb->getmcu()->watchdog("reset");

    if (success) {
        CONNECTED_TO_MQTT_BROKER = true;
        this->_on_connect_handler();

        // Blink green 2 times
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->blink("green", 2, 300, 200);
        _gb->getdevice("buzzer")->play("..");
        
        _gb->arrow().log("Done");
    }
    else {
        _gb->getdevice("rgb")->on(1);
        CONNECTED_TO_MQTT_BROKER = false;
        int errorcode = this->_mqttclient.state();
        String errormessage = errorcode == -2 ? "Couldn't connect to the broker" : "Unknown error";
        
        // Blink red 2 times
        if (_gb->hasdevice("rgb")) _gb->getdevice("rgb")->blink("red", 2, 300, 200);
        _gb->getdevice("buzzer")->play("..-");
                
        _gb->arrow().log("Failed: " + errormessage + " (" + String(errorcode) + ")");
    }

    // // Disable watchdog
    // _gb->getmcu()->watchdog("disable");
    
    return *this;
}

GB_MQTT& GB_MQTT::update() {

    if (!_gb->getmcu()->connected()) {
        CONNECTED_TO_MQTT_BROKER = 0;
        return *this;
    }

    #ifdef ESP32
        if(!CONNECTED_TO_NETWORK || !CONNECTED_TO_INTERNET || !MODEM_INITIALIZED) {
            CONNECTED_TO_MQTT_BROKER = 0;
            return *this;
        }
    #else
        if(!CONNECTED_TO_NETWORK || !CONNECTED_TO_INTERNET || !MODEM_INITIALIZED) {
            CONNECTED_TO_MQTT_BROKER = 0;
            return *this;
        }
    #endif

    CONNECTED_TO_MQTT_BROKER = this->_mqttclient.connected();
    
    if (!CONNECTED_TO_MQTT_BROKER) {
        
        _gb->arrow().color("yellow").log("MQTT disconnected", false).arrow();
        
        //! Attempt reconnection with the broker
        this->connect();
        
        //! Resubscribe to topics if reconnected
        if(this->_mqttclient.connected()) {
            CONNECTED_TO_MQTT_BROKER = 1;
            this->_reconnection_attempt_count = 0;
            // _gb->color("green").log("Reconnected.", false);
        }
        else {
            CONNECTED_TO_MQTT_BROKER = 0;
            _gb->color("red").log("Reconnection failed. Attempt: " + String(this->_reconnection_attempt_count), false);

            /* 
                ! Reset MCU if max reattempts of connection to broker
                It appears that the issue where the MQTT fails to reconnect (Even after an mcu reset) can be solved (a hack) by
                first resetting the modem, and then resetting the mcu. 
                
                A better solution would obliviate the need to reset the microcontroller. 

                * Good resource
                https://forum.arduino.cc/t/small-project-reliable-and-fault-tolerant-modem-start-sequence/704318/5
                https://1ot.mobi/resources/blog/finding-patterns-with-terminal
            */
            if (this->_reconnection_attempt_count++ == 5) {
                
                // Reset mcu
                _gb->getmcu()->reset("mcu");
            }

        }
    }
    else {
        this->_mqttclient.loop();
    }

    return *this;
}

// Publish to a topic
bool GB_MQTT::publish(String topic, String header, String data) { return this->publish(topic, header + "\n" + data); }
bool GB_MQTT::publish(String topic, String data) {

    bool log = topic != "log/message";
    
    if (log) _gb->log("Publishing to topic: gb-server::" + topic, false);

    #ifdef ESP32
        if(!CONNECTED_TO_NETWORK || !CONNECTED_TO_INTERNET || !CONNECTED_TO_MQTT_BROKER) {
            if (log) this->_gb->arrow().log("Skipped");
            return false;
        }
    #elif __AVR_ATmega328P__ || __AVR_ATmega168__
        if (log) this->_gb->arrow().log("Skipped");
        return false;
    #else
        if(!CONNECTED_TO_NETWORK || !CONNECTED_TO_INTERNET || !CONNECTED_TO_MQTT_BROKER) {
            if (log) this->_gb->arrow().log("Skipped");
            return false;
        }
    #endif

    // MODEM.debug(Serial);
    
    // MQTT update
    for (int i = 0; i < 5; i++) { _gb->bs(); this->update(); delay(1); }

    bool success = false;
    int attempts = 0;
    int maxattempts = 3;

    while (!success && attempts++ <= maxattempts)  {

        // if (this->_wait_for_ack && _gb->getdevice("sd")->exists("/queue/sent/" + this->_ack_id)) {
        //     _gb->arrow().log("Already sent");
        //     return false;
        // }
        
        success = this->_mqttclient.publish(
            _gb->s2c("gb-server::" + topic),
            _gb->s2c(_gb->globals.DEVICE_SN + "::" + "null" + "::" + _gb->uuid() + ":::" + data + ":::" + this->_wait_for_ack)
        );

        if (!success) {
            _gb->log(" .", false);
            delay(100);
            
            // MQTT update
            this->update();

            // if (this->_wait_for_ack) {
            //     _gb->getdevice("sd")->writeCSV("/queue/sent/" + this->_ack_id, "", "");
            //     this->_wait_for_ack = false;
            // }

        }
        else {

            // MQTT update
            _gb->bs(); this->update();

        }
    }


    // // If waiting for acknowledgment
    // if (this->_wait_for_ack) {
        
    // };

    // Report the result of the action
    if (log && success) _gb->arrow().color("green").log("Done");
    else if (log) _gb->arrow().color("red").log("Failed");
    delay(5);

    // Return false if waiting for acknowledgment, else return success booelan
    return success;
}

// Subscribe to a topic
void GB_MQTT::subscribe(String topic) {
    // _gb->log("Subscribing to topic: " + String(_gb->s2c(_gb->globals.DEVICE_SN + "::" + topic)));
    bool success = _mqttclient.subscribe(_gb->s2c(_gb->globals.DEVICE_SN + "::" + topic), this->SUBQOS);
    delay(20);
}

bool GB_MQTT::waituntilresponse(String topic, unsigned long timeout_ms, bool isBroadcast) {
    
    // Raise the flag
    _mqttclient.waiting_for_response_flag = true;
    _mqttclient.waiting_for_response_topic = isBroadcast ? "" : (_gb->globals.DEVICE_SN + "::") + topic;

    // Wait for the flag to dispense
    unsigned long start = millis();
    while (_mqttclient.waiting_for_response_flag && millis() - start <= timeout_ms) {
        this->update();
        delay(500);
    }

    // Check if a response was received
    return !_mqttclient.waiting_for_response_flag;
}

// Get broker connection status
bool GB_MQTT::connected() {
    return this->connected(false);
}

bool GB_MQTT::connected(bool log) {
    if(log) _gb->log("Broker connection: ", false);
    bool result = _mqttclient.connected();
    if (log) {
        if(result) _gb->arrow().log("Done");
        else _gb->arrow().log("Failed");
    }
    return result;
}

GB_MQTT& GB_MQTT::ack(String filename) {
    this->_wait_for_ack = true;

    String id = filename.substring(filename.lastIndexOf(".") - 5, filename.lastIndexOf("."));
    this->_ack_id = id;
    return *this;
}

GB_MQTT& GB_MQTT::noack() {
    this->_wait_for_ack = false;
    return *this;
}

void mqtt_response_handler(GB gb, String topic, String payload) { }

#endif