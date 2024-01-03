/* 
    ! Enable low memory mode 
    This mode can be turned on if you want to use a low-end/less powerful 
    microcontroller like Arduino Uno, Arduino Pro-mini. These microcontrollers
    have small flash capacity (~32Kb) and small SRAM capacity (~2Kb)
        - Flash memory, is where the Arduino sketch is stored.
        - SRAM is where the sketch creates and manipulates variables when it runs.

    However, GatorByte library (GBL) requires >60Kb SRAM and >10Kb Flash memory by default.
    This is because, by default all modules of the GBL are imported even if they are not required.
    This behaviour can be changed by enabling "LOW_MEMORY_MODE". 
    
    #define LOW_MEMORY_MODE

    If the LOW_MEMORY_MODE is not specified
    enabled, the user will have to explicitly specify which module should be imported.

    #define INCLUDE_NB1500
    #define INCLUDE_RGB

    More information about this mode can be found in the GBL documentation.
*/

// #define LOW_MEMORY_MODE
// #define INCLUDE_NB1500
// #define INCLUDE_IOE
// #define INCLUDE_MQTT
// #define INCLUDE_SD
// #define INCLUDE_AT24
// #define INCLUDE_GPS
// #define INCLUDE_DS1307
// #define INCLUDE_MPU6050
// #define INCLUDE_AHT10
// #define INCLUDE_5V_BOOSTER
// #define INCLUDE_RGB
// #define INCLUDE_AT_SCI_RTD
// #define INCLUDE_AT_SCI_PH
// #define INCLUDE_AT_SCI_DO
// #define INCLUDE_AT_SCI_EC

/*
    Import the primary GBL sublibrary
*/
#include "./core/GB_Primary.h"

/*
    Import the GBL sublibraries for supported devices
*/
#include "./core/GB_Devices.h"