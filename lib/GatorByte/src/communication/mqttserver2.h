/*
    This module needs to be mcu agnostic. 
    This module uses MQTT library
*/

#ifndef GB_MQTT_h
#define GB_MQTT_h

#ifndef GB_h
    #include "../GB.h"
#endif

#include "../../MQTT/src/MQTT.h"

class GB_MQTT
{
    public:
        GB_MQTT(GB &gb, GB_MCU &mcu);
        
        // Type definitions
        typedef void (*callback)(String &topic, String &payload);
        
        DEVICE device = {
            "mqtt",
            "MQTT broker interface",
            "Module to connect to MQTT broker",
            "N/A"
        };

        String BROKER_IP = "";
        int BROKER_PORT = -1;
        String CLIENT_ID = "";
        String USER = "";
        String PASS = "";

        GB_MQTT& configure(String, int, String, MQTTClientCallbackSimple);
        GB_MQTT& connect(String, String);
        GB_MQTT& update();

        void publish(String, String);
        void subscribe(String);

    private:

        GB *_gb;
        GB_MCU *_mcu;
        MQTTClient _mqttclient;
        callback _message_handler;
        String _subscription_list[10];
        int _subscription_count = 0;
        int _client_id_count = 0;
};

GB_MQTT::GB_MQTT(GB &gb, GB_MCU &mcu) {
    _gb = &gb;
    _mcu = &mcu;
    _gb->includelibrary(this->device.id, this->device.name);
}

GB_MQTT& GB_MQTT::configure(String ip, int port, String client_id, callback ptr) {
    
    _gb->log("Configuring MQTT client", false);
    this->BROKER_IP = ip;
    this->BROKER_PORT = port;
    this->CLIENT_ID = client_id;
    this->_message_handler = ptr;

    if(ip.length() == 0) _gb->log(" -> Failed. Invalid Broker IP");
    else if(port <= 0) _gb->log(" -> Failed. Invalid Broker port");
    else _gb->log(" -> Done");

    return *this;
}

// void _message_handler(String &topic, String &payload) {
//     Serial.println("Incoming: " + topic + " - " + payload);
// }

// TODO Implement reconnection on disconnection (DONE)
// TODO: Resubscribe on  reconnection
GB_MQTT& GB_MQTT::connect(String username, String password) {
    // Set instance variables
    this->USER = username;
    this->PASS = password;

    // Connect to the broker
    _gb->log("Connecting to MQTT broker", false);
    _mqttclient.begin(_gb->s2c(this->BROKER_IP), this->BROKER_PORT, _mcu->client());
    bool success = false;
    int counter = 0;
    while(!success && counter++ < 5) { success = _mqttclient.connect(_gb->s2c(this->CLIENT_ID), _gb->s2c(this->USER), _gb->s2c(this->PASS)); delay(1000); };
    if (success) {
        _gb->log(" -> Done");
          
        // Start listener for incoming messages
        _mqttclient.onMessage(_message_handler);
    }
    else _gb->log(" -> Failed");
    return *this;
}

GB_MQTT& GB_MQTT::update() {
    if (!_mqttclient.connected()) {
        _gb->log("Reconnecting to broker", false);
        
        // Reconnect to the broker
        int counter = 0;
        while(!this->_mqttclient.connected() && counter++ < 5) {
            _gb->log(" .", false);
            this->_mqttclient = MQTTClient();
            this->_mqttclient.begin(_gb->s2c(this->BROKER_IP), this->BROKER_PORT, _mcu->client());
            this->_mqttclient.connect(_gb->s2c(this->CLIENT_ID), _gb->s2c(this->USER), _gb->s2c(this->PASS));
            if(!this->_mqttclient.connected()) delay(2000);
        }

        if (this->_mqttclient.connected()) {
            _gb->log(" -> Done");

            // Duplicate subscriptions list to tmp
            String tmp[sizeof(this->_subscription_list)/sizeof(this->_subscription_list[0])];
            memcpy(tmp, this->_subscription_list, sizeof(this->_subscription_list)/sizeof(this->_subscription_list[0]));

            // Resubscribe to topics
            _gb->log("Resubscribing to " + String(this->_subscription_count) + " topic(s).");
            for (int i = 0; i < this->_subscription_count; i++) {
                String topic = tmp[i];
                if(topic.length() > 0) {
                    this->_mqttclient.unsubscribe(_gb->s2c(topic));
                    delay(10);
                    this->_mqttclient.subscribe(_gb->s2c(topic));
                }
            }

            // Reset list
            for (int i = 0; i < this->_subscription_count; i++) 
                this->_subscription_list[i] = "";

            this->_subscription_count = 0;
        }
        else _gb->log(" -> Failed", true);
    }
    else this->_mqttclient.loop();
    
    return *this;
}

// Publish to a topic
void GB_MQTT::publish(String topic, String data) {
    _gb->log("Publishing to topic: " + topic, false);

    if(this->_mqttclient.connected()) {
        int start = millis();
        bool success = _mqttclient.publish(_gb->s2c(topic), _gb->s2c(data));
        if (success) _gb->log(" -> Done");
        else _gb->log(" -> Failed");

        int difference = millis() - start;
        _gb->log("Time taken: " + String(difference / 1000) + " seconds");
    }
    else _gb->log(" -> Broker disconnected");
}

// Subscribe to a topic
void GB_MQTT::subscribe(String topic) {
    _gb->log("Subscribing to topic: " + topic, false);

    if(this->_mqttclient.connected()) {
        bool success = _mqttclient.subscribe(_gb->s2c(topic), 0);
        if (success) {
            _gb->log(" -> Done");
            this->_subscription_list[this->_subscription_count++] = topic;
        }
        else _gb->log(" -> Failed");
    }
    else _gb->log(" -> Broker disconnected");
}

#endif