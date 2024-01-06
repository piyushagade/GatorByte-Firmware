#pragma once

//! GB_Primary.h also has globals; Figure out which ones to use.

// TODO: Deprecate the commented variables
// Global parameters/variables
// String BR = "\n";
// String DEVICE_TYPE, DEVICE_NAME;
// int ITERATION = 0;
// int SLEEP_DURATION = 5 * 60 * 1000;
// int OFFLINE_MODE = 0;
// int SEND_DATA_TO_SERVER = 1;
// int LOG_DATA_TO_SERIAL = 1;
// int WRITE_DATA_TO_SD = 1;
// int WAIT_FOR_GPS_FIX = 0;
// int MAX_NUMBER_OF_CHECKS_FOR_SAT_FIX = 20;
// int LAST_READING_TIMESTAMP = 0;
// int SECONDS_SINCE_LAST_READING = 0;
// int FAULTS_PRIMARY = 0;
// int LOOPTIMESTAMP = 0;
// bool READINGS_DUE = true;

// Debug states variables
bool MODEM_DEBUG = 0;
bool SERIAL_DEBUG = 0;

// States
bool MODEM_INITIALIZED = 0;
bool WATCHDOG_ENABLED = 0;
bool SIGNAL_VIABLE = 0;
bool SIM_DETECTED = 0;
bool CONNECTED_TO_NETWORK = 0;
bool CONNECTED_TO_INTERNET = 0;
bool CONNECTED_TO_MQTT_BROKER = 0;
bool CONNECTED_TO_API_SERVER = 0;


/* 
    ! Error state counters
    These reset on microcontroller reset
*/
struct ERROR_STATES_COUNTER {
    int no_cell_signal = 0;
    int low_cell_signal = 0;
    int modem_init_failure = 0;
    int invalid_rtc_timestamp = 0;
} esc;