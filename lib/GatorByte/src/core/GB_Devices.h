#pragma once

#ifndef GB_Devices_h
#define GB_Devices_h

//! Datary library
#include "CSVary.h"
#include "JSONary.h"

//! Microcontrollers
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_NB1500)
    #include "../microcontrollers/nb1500.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_BORON)
    #include "../microcontrollers/boron.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_ESP32)
    #include "../microcontrollers/esp32.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_PROMINI)
    #include "../microcontrollers/promini.h"
#endif

//! IO Expander
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_IOE) 
    #include "../misc/74hc595.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_TCA) 
    #include "../misc/tca9548a.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_SERMUX) 
    #include "../misc/serialmux.h"
#endif


//! Communications
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_HTTP) 
    #include "../communication/httpserver.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_MQTT) 
    #include "../communication/mqttserver.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_HC05) 
    #include "../communication/hc_05.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_AT09) 
    #include "../communication/at_09.h"
#endif


//! Local storage
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_SD) 
    #include "../storage/sd.h"
#endif
// #if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_FRAM) 
//     #include "../storage/fram.h"
// #endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_AT24) 
    #include "../storage/at24.h"
#endif

//! GPS
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_GPS) 
    #include "../location/neo6m.h"
#endif

//! Time keeping
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_DS3231) 
    #include "../time/ds3231.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_DS1307) 
    #include "../time/ds1307.h"
#endif

//! Actuators
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_RELAY) 
    #include "../actuators/relay.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_BUZZER) 
    #include "../misc/buzz.h"
#endif

// // Analog sensors
//     #include "../sensors/analog/mb7374.h"
//     #include "../sensors/analog/sr04.h"
//     #include "../sensors/analog/wdet.h"
//     #include "../sensors/analog/wlev.h"

//! Digital sensors
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_RG11) 
    #include "../sensors/digital/rg11.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_TPBCK) 
    #include "../sensors/digital/tpbck.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_EADC) 
    #include "../misc/ads1115.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_USS) 
    #include "../sensors/modbus/seeduss.h"
#endif

#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_AT_SCI_DO) 
    #include "../sensors/i2c/atlas_sci_do.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_AT_SCI_RTD) 
    #include "../sensors/i2c/atlas_sci_rtd.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_AT_SCI_EC) 
    #include "../sensors/i2c/atlas_sci_ec.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_AT_SCI_PH) 
    #include "../sensors/i2c/atlas_sci_ph.h"
#endif

// I2C sensors
// #if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_MPU6050) 
//     #include "../sensors/i2c/mpu6050.h"
// #endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_AHT10) 
    #include "../sensors/i2c/aht10.h"
#endif

// 5V booster
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_5V_BOOSTER) 
    #include "../misc/booster.h"
#endif

// RGB LED
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_RGB)
    #include "../misc/rgb.h"
#endif

// Power Meter
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_PWRMTR)
    #include "../misc/powermeter.h"
#endif

// Sentinel
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_SNTL)
    #include "../misc/sentinel.h"
#endif

// Console mode
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_CONSOLE) 
    #include "./GB_CONSOLE.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_CONFIGURATOR) 
    #include "./GB_CONFIGURATOR.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_DESKTOP_CLIENT) 
    #include "./GB_DESKTOP.h"
#endif
#if not defined (LOW_MEMORY_MODE) || defined (INCLUDE_PIPER) 
    #include "./GB_PIPER.h"
#endif

// __AVR__ for any board with AVR architecture.
// ARDUINO_AVR_PRO for the Arduino Pro or Pro Mini
// ESP8266 for any ESP8266 board.
// ESP32 for any ESP32 board.
// ARDUINO_ESP8266_NODEMCU for the NodeMCU 0.9 (ESP-12 Module) or NodeMCU 1.0 (ESP-12E Module)
// __AVR_ATmega328P__, __AVR_ATmega168__, __AVR_ATmega2560__ for controller specific

#endif