/*
    This module needs to be mcu agnostic. 
    This module uses ArduinoMqttClient library
*/

#ifndef GB_MQTT_h
#define GB_MQTT_h

#ifndef GB_h
    #include "../GB.h"
#endif

#ifndef _ARDUINO_MQTT_CLIENT_H_
    #include "../../../ArduinoMqttClient/src/ArduinoMqttClient.h"
#endif

class GB_MQTT
{
    public:
        GB_MQTT(GB &gb, GB_MCU &mcu);
        
        // Type definitions
        typedef void (*callback_t)(String, String);

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

        GB_MQTT& configure(String, int, String, callback_t);
        GB_MQTT& connect(String, String);
        GB_MQTT& update();

        bool connected();
        bool connected(bool);
        void publish(String, String);
        void subscribe(String);

    private:

        GB *_gb;
        GB_MCU *_mcu;
        
        // //! AG: Added following lines in the library
        // // MqttClient() {};

        MqttClient _mqttclient;
        callback_t _message_handler;
        String _subscription_list[10];
        int _subscription_count = 0;
};

GB_MQTT::GB_MQTT(GB &gb, GB_MCU &mcu) {
    _gb = &gb;
    _mcu = &mcu;
    _gb->includelibrary(this->device.id, this->device.name);
}

GB_MQTT& GB_MQTT::configure(String ip, int port, String client_id, callback_t ptr) {
    
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

void _mh(String data) {
    Serial.print("Received a message: ");
    Serial.println(data);
}

GB_MQTT& GB_MQTT::connect(String username, String password) {
    // Set instance variables
    this->USER = username;
    this->PASS = password;

    // Set broker, port, user, password, and client id
    _gb->log("Connecting to MQTT broker: " + String(this->BROKER_IP) + ", " + String(this->BROKER_PORT), false);
    this->_mqttclient = new MqttClient();
    this->_mqttclient.setClient(this->_mcu->client());
    this->_mqttclient.setUsernamePassword(this->USER, this->PASS);
    this->_mqttclient.setId(this->CLIENT_ID);
    this->_mqttclient.onMessage(_message_handler);

    bool success = false;
    int counter = 0;
    while(!success && counter++ < 5) { success = this->_mqttclient.connect(_gb->s2c(this->BROKER_IP), this->BROKER_PORT); delay(1000); }
    
    if (success) _gb->log(" -> Done");
    else {
        _gb->log(" -> Failed: ", false);
        _gb->log(this->_mqttclient.connectError());
    }
    return *this;
}

GB_MQTT& GB_MQTT::update() {
    if (!this->_mqttclient.connected()) {
        this->_mcu->reconnect();
        this->_mqttclient.stop();

        // Connect to the broker
        this->_mqttclient.connect(_gb->s2c(this->BROKER_IP), this->BROKER_PORT);
        
        // Resubscribe to topics if reconnected
        if(this->_mqttclient.connected()) {
            String tmp[sizeof(this->_subscription_list)/sizeof(this->_subscription_list[0])];
            memcpy(tmp, this->_subscription_list, sizeof(this->_subscription_list)/sizeof(this->_subscription_list[0]));

            _gb->log("Resubscribing to " + String(this->_subscription_count) + " topic(s).");
            for (int i = 0; i < this->_subscription_count; i++) {
                String topic = tmp[i];
                if(topic.length() > 0) {
                    this->_mqttclient.subscribe(_gb->s2c(topic));
                }
            }
        }
    }
    else this->_mqttclient.poll();
    return *this;
}

// Publish to a topic
void GB_MQTT::publish(String topic, String data) {
    _gb->log("Publishing to topic: " + topic, false);

    this->_mqttclient.beginMessage(topic);
    this->_mqttclient.print("Hello from GatorByte");
    this->_mqttclient.endMessage();

    // int start = millis();
    // bool success = _mqttclient.publish(_gb->s2c(topic), _gb->s2c(data));
    // if (success) _gb->log(" -> Done");
    // else _gb->log(" -> Failed");

    // int difference = millis() - start;
    // _gb->log("Time taken: " + String(difference / 1000) + " seconds");
}

// Subscribe to a topic
void GB_MQTT::subscribe(String topic) {
    _gb->log("Subscribing to topic: " + topic, false);
    this->_subscription_list[this->_subscription_count++] = topic;

    bool success = _mqttclient.subscribe(_gb->s2c(topic), 0);
    if (success) {
        _gb->log(" -> Done");
    }
    else _gb->log(" -> Failed");
}

// Get broker connection status
bool GB_MQTT::connected() {
    return this->connected(false);
}
bool GB_MQTT::connected(bool log) {
    if(log) _gb->log("Broker connection: ", false);
    bool result = _mqttclient.connected();
    if (result) if(log) _gb->log(" -> Done");
    else if(log) _gb->log(" -> Failed");
    return result;
}

#endif