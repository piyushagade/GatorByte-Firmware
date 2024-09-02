#include "platform.h"

#if GB_PLATFORM_IS_SENTINEL_V8
/*
    The secondary uC listens for "breaths" from primary uC.
    The breaths can either be implemented as:
        1. A change in the state of pin A1 on the primary.
        2. An I2C message to turn the timer on the secondary on/off

    If the timer runs out, the secondary resets the primary uC.
    The timer interval can be set by sending an I2C message from primary to secondary. (Experimental) It may also be set using the voltage level of
    pin A1 (analog output on the primary) and read on pin PB1 (analog input on the secondary)

    I am first going to try I2C method. Using analog pins is a fallback.

    TODO:
    -----------------------------------------------
        1. Test 1 MHz operation (done)
        2. Test if 1 ms on ATTINY85 is actually 1 ms (done, at 8 MHz and 1 MHz)
        3. Send ACK to primary (done)
        4. Send 2 byte response (done)
        5. Adjustable PB1 pin mode
        6. Add sleep mode to save power

    Future design upgrades:
    -----------------------------------------------
        1. Add LED on pin ANA_IN (done)
        2. Add decoupling capacitor between VCC and GND (done)
        3. Ability to programatically reboot ATTINY85 using the RST pin
        4. 3 timers

    First flash instructions:
    -----------------------------------------------
        1. Select 1 MHz CPU frequency
        2. Burn bootloader
        3. 'Upload using programmer'

    Legendary advices:
    -----------------------------------------------
        1. The dynamic memory needs to be below 45% full. If not, the I2C communication doesn't work.

    EEPROM map:
    -----------------------------------------------
    The program is designed to store 2 bytes of data. Hence, all location indices should be an even number.
    Indices are double that of the corresponding IDs.

    Hex  |  Name                         |  Description
    -----------------------------------------------------------------------------------------
    0x0    |  EEPROM initialization flag     |  If 1, the EEPROM has values stored in it
    0x1    |  I2C address                    |  Stores the I2C address of the device
    0x2    |  SNTL Interval base             |  Stores the base interval value
    0x3    |  SNTL Interval multiplier       |  Stores the interval multiplier value
    0x4    |  PSM Interval base              |  Stores the base interval value
    0x5    |  PSM Interval multiplier        |  Stores the interval multiplier value
    0x6    |  Primary's fault counter        |  Stores the count of reboots of primary uC
    0x7    |  Secondary's fault counter      |  Stores the count of reboots of the device
    0x8    |  PSM state flag                 |  Is the device waking up from PSM sleep?
    0x9    |  Timer 0 start timestamp        |  The value of start_timestamp for Timer 0
    0xA    |  Timer 1 start timestamp        |  The value of start_timestamp for Timer 1
    0xB    |  Timer 2 start timestamp        |  The value of start_timestamp for Timer 2
    0xC    |  Timer 3 start timestamp        |  The value of start_timestamp for Timer 3
    0xD    |  Beacon enabled                 |  Indicates if the beacon feature is enabled
    0xE    |  Send ACK                       |  Controls whether an ACK should be sent after receiving data
    0xF    |  Beacon base interval           |  Stores the base interval value for the beacon
    0x10   |  Beacon multiplier              |  Stores the interval multiplier value for the beacon
    0x11   |  Beacon mode                    |  Determines the operation mode of the beacon
*/

#include <TinyWireS.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>

#define Wire TinyWireS

struct LOCATIONS
{
    uint16_t init = 0x00;
    uint16_t address = 0x01;
    

    uint16_t psm_base = 0x04;
    uint16_t psm_multiplier = 0x05;
    uint16_t primary_fault = 0x06;
    uint16_t secondary_fault = 0x07;
    uint16_t psm_state_flag = 0x08;

    uint16_t timer0_start_timestamp = 0x09;
    uint16_t timer1_start_timestamp = 0x0A;
    uint16_t timer2_start_timestamp = 0x0B;
    uint16_t timer3_start_timestamp = 0x0C;
    
    uint16_t beacon_enabled = 0x0D;
    uint16_t send_ack = 0x0E;
    uint16_t beacon_base = 0x0F;
    uint16_t beacon_multiplier = 0x10;
    uint16_t beacon_mode = 0x11;

    uint16_t enabled_t0 = 0x12;
    uint16_t sentinence_base_t0 = 0x13;
    uint16_t sentinence_multiplier_t0 = 0x14;
    
    uint16_t enabled_t1 = 0x15;
    uint16_t sentinence_base_t1 = 0x16;
    uint16_t sentinence_multiplier_t1 = 0x17;
    
    uint16_t enabled_t2 = 0x18;
    uint16_t sentinence_base_t2 = 0x19;
    uint16_t sentinence_multiplier_t2 = 0x1A;
    
    uint16_t enabled_t3 = 0x1B;
    uint16_t sentinence_base_t3 = 0x1C;
    uint16_t sentinence_multiplier_t3 = 0x1D;

    uint16_t last_ping_timestamp = 0x1E;
} location;

// Utility functions
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// Sentinel firmware meta data
#define FIRMWARE_MAJOR_VERSION 2
#define FIRMWARE_MINOR_VERSION 13
#define FIRMWARE_MONTH 9
#define FIRMWARE_DATE 24

// Define addresses and pins
#define MOSFET_ON PB3
#define MOSFET_OFF PB4
#define BEACON PB1
#define SDA PB0
#define SCL PB2

// Define error codes
#define SUCCESS 0
#define ERROR 1
#define INVALIDACTION 2
#define PINGRESPONSE 3
#define CONFIG_LOCKED_ERROR 4
#define INVALID_OPTION 5
#define INVALID_ACTION_SEN_ON 6
#define INVALID_ACTION_SEN_OFF 7

// State variables
bool USE_PSM = false;
bool SEND_ACK = true;
bool TIMER_ENABLED[] = {false, false, false, false};
bool TIMER_EXPIRED[] = {false, false, false, false};
bool BEACON_ENABLED = false;
bool BEACON_TRIGGER_PENDING = false;
bool PRIMARY_REBOOT_PENDING = false;
bool PRIMARY_OFF = false;
bool I2C_ENABLED = false;
bool CONFIG_UNLOCKED = false;
bool PSM_WAKE_FLAG = false;
bool PB1_MODE = INPUT;
uint8_t INTERVAL_SET_MODE = 0;

uint8_t BEACON_MODE = 1;

// Default timer is Timer 0
uint8_t SELECTED_TIMERID = 0;
uint32_t LAST_PING_TIMESTAMP = 0;

// Timer interval storage (Units: seconds)
uint8_t INTERVAL_BASE[4] = { 15, 15, 15, 144 };
uint8_t INTERVAL_MULTIPLIER[4] = { 8, 8, 8, 100 };

uint8_t PSM_INTERVAL_BASE = 15;
uint8_t PSM_INTERVAL_MULTIPLIER = 8;
uint8_t BCN_INTERVAL_BASE = 5;
uint8_t BCN_INTERVAL_MULTIPLIER = 2;

// Sentinel I2C address (0 - 127)
uint8_t I2C_ADDRESS = 9;

// Time trackers (unit: seconds)
uint32_t start_timestamp[4] = {millis() / 1000, millis() / 1000, millis() / 1000, millis() / 1000};

// Function declarations
void sleep(uint8_t);
void trigger(uint8_t);
void blinkmode(uint8_t, uint8_t);
void wdtduration(uint8_t);
void wdt(uint8_t);
void sendresponse(uint16_t);
void sendresponseforced(uint16_t);
void setsentinence(uint8_t);
void reboot(uint8_t);
void shutdown();
bool fuse(uint8_t);
void i2cstate(uint8_t);
void i2clistener(uint8_t);
void memfetch();
uint16_t memread(uint8_t);
void memwrite(uint8_t, uint16_t);
typedef void (*callback_t_func)();
void watch(uint8_t, callback_t_func);

/*
    System sleep
    The system wakes up when watchdog is timed out.
*/
void sleep(uint8_t duration)
{

    /*
        https://www.gammon.com.au/forum/?id=11497, Sketch H
        https://github.com/combs/Arduino-Watchdog-Handler
        https://www.hackster.io/Itverkx/sleep-at-tiny-371f04
    */

    // Write PSM state flag
    memwrite(location.psm_state_flag, 1);

    // Write time tracker value
    memwrite(location.timer0_start_timestamp, millis() - start_timestamp[0]);
    memwrite(location.timer1_start_timestamp, millis() - start_timestamp[1]);
    memwrite(location.timer2_start_timestamp, millis() - start_timestamp[2]);
    memwrite(location.timer3_start_timestamp, millis() - start_timestamp[3]);

    // Switch ADC off
    cbi(ADCSRA, ADEN);

    // Added: Clear various "reset" flags; needed?
    MCUSR = 0;

    // Turn on WDT; Set sleep duration to 2 second
    wdtduration(duration);

    // Added: Pat the dog, needed?
    wdt_reset();

    // Sleep mode is set here
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    // Enable the Interrupts so the wdt can wake the device up
    sei();

    // System sleeps here
    sleep_cpu();

    // // Added: Disable interrupts, Needed?
    // cli();

    // System continues execution here when watchdog timed out
    sleep_disable();

    // Turn ADC back on
    sbi(ADCSRA, ADEN);

    // If execution reaches here, the microcontroller has woken from sleep

    // Check if the wake-up was caused by the Watchdog Timer
    if (bit_is_set(MCUSR, WDRF))
    {
        // Clear the Watchdog Reset Flag
        MCUSR &= ~(1 << WDRF);

        // Execute function only if waking from sleep
    }
}

/*
    Change WDT state
*/
void wdtduration(uint8_t duration)
{
    if (duration >= WDTO_15MS)
        wdt_enable(duration);
    else if (duration <= -1)
        wdt(0x02);
}
void wdt(uint8_t state)
{

    // Enable WDT
    if (state == 0x01)
    {
        wdt_enable(WDTO_4S);
    }

    // Disable WDT
    else if (state == 0x02)
    {
        MCUSR = 0x0;
        wdt_disable();
    }

    // Initialize WDT
    else if (state == 0x03)
    {
    }

    // Reset WDT
    else if (state == 0x04)
    {
        wdt_reset();
    }
}

/*
    Send two response to the primary
    The function sends two bytes of data
*/
void sendresponse(uint16_t response)
{

    if (!SEND_ACK)
        return;

    // // Turn WDT on
    // wdt(0x01);

    // Send low byte
    Wire.write((byte)response);
    delay(50);

    // Send high byte
    Wire.write((byte)(response >>= 8));
    delay(50);

    // // Turn WDT off
    // wdt(0x02);
}

/*
    Send two response to the primary
    The function sends two bytes of data
*/
void sendresponseforced(uint16_t response)
{

    // // Turn WDT on
    // wdt(0x01);

    // Send low byte
    Wire.write((byte)response);
    delay(50);

    // Send high byte
    Wire.write((byte)(response >>= 8));
    delay(50);

    // // Turn WDT off
    // wdt(0x02);
}

/*
    Set sentinel's state (turn timer on/off)
*/
void setsentinence(uint8_t state)
{

    // Turn on Timer SELECTED_TIMERID if not already on
    if (state == 0x01 && !TIMER_ENABLED[SELECTED_TIMERID])
    {
        // if (state == 0x01) {
        TIMER_ENABLED[SELECTED_TIMERID] = true;
        start_timestamp[SELECTED_TIMERID] = millis() / 1000;

        // Write to EEPROM
        memwrite(location.enabled_t0 + (SELECTED_TIMERID * 3), TIMER_ENABLED[SELECTED_TIMERID]);
        memwrite(location.timer0_start_timestamp + SELECTED_TIMERID, start_timestamp[SELECTED_TIMERID]);

        // Send ack to primary
        sendresponse(SUCCESS);

        // Lock Sentinel configuration
        CONFIG_UNLOCKED = false;
    }

    // If already on, send error response
    else if (state == 0x01 && TIMER_ENABLED[SELECTED_TIMERID])
    {
        sendresponse(INVALID_ACTION_SEN_ON);
    }

    // If timer is on, turn it off
    else if (state == 0x02 && TIMER_ENABLED[SELECTED_TIMERID])
    {
        // else if (state == 0x02) {
        TIMER_ENABLED[SELECTED_TIMERID] = false;
        start_timestamp[SELECTED_TIMERID] = millis() / 1000;

        // Write to EEPROM
        memwrite(location.enabled_t0 + (SELECTED_TIMERID * 3), TIMER_ENABLED[SELECTED_TIMERID]);
        memwrite(location.timer0_start_timestamp + SELECTED_TIMERID, start_timestamp[SELECTED_TIMERID]);

        // Lock Sentinel configuration
        CONFIG_UNLOCKED = false;

        // Send ack to primary
        sendresponse(SUCCESS);
    }

    // If timer is already off, send an error response
    else if (state == 0x02 && !TIMER_ENABLED[SELECTED_TIMERID])
    {
        sendresponse(INVALID_ACTION_SEN_OFF);
    }

    // Kick a sentinence timer (reset a timer)
    else if (state == 0x03 && TIMER_ENABLED[SELECTED_TIMERID])
    {
        start_timestamp[SELECTED_TIMERID] = millis() / 1000;

        // Write to EEPROM
        memwrite(location.timer0_start_timestamp + SELECTED_TIMERID, start_timestamp[SELECTED_TIMERID]);

        // Send ack to primary
        sendresponse(SUCCESS);

        // Lock Sentinel configuration
        CONFIG_UNLOCKED = false;
    }

    // Send error response
    else sendresponse(INVALIDACTION);
}

/*
    Reboot the primary microcontroller
    Inverted logic due to P-channel MOSFETs
*/

void reboot(uint8_t device)
{

    // Primary
    if (device == 0x02)
    {

        // Wait for 1 seconds
        tws_delay(1000);

        // Turn off the primary
        trigger(0x02);

        // A 250 ms delay
        tws_delay(500);

        // Turn on the primary
        trigger(0x01);
    }

    // Secondary
    else if (device == 0x01)
    {

        // Force reset
        wdt_enable(WDTO_15MS);
        while (true)
            delay(1);
    }
}

/*
    Turn off the primary microcontroller
    Inverted logic due to P-channel MOSFETs
*/

void shutdown()
{

    // Turn off sentinence
    if (TIMER_EXPIRED[0])
    {
        TIMER_ENABLED[0] = false;
        start_timestamp[0] = millis() / 1000;
    }
    if (TIMER_EXPIRED[1])
    {
        TIMER_ENABLED[1] = false;
        start_timestamp[1] = millis() / 1000;
    }
    if (TIMER_EXPIRED[2])
    {
        TIMER_ENABLED[2] = false;
        start_timestamp[2] = millis() / 1000;
    }
    if (TIMER_EXPIRED[3])
    {
        start_timestamp[3] = millis() / 1000;
    }

    // Turn off the primary
    trigger(0x02);
}

/*
    Set I2C state and listener
*/

void i2cstate(uint8_t state)
{

    // Enable I2C and set up ISR
    if (state == 0x01)
    {
        tws_delay(100);
        Wire.begin(I2C_ADDRESS);
        I2C_ENABLED = true;
        tws_delay(10);
        Wire.onReceive(i2clistener);
    }

    // Disable I2C
    else if (state == 0x02)
    {
        pinMode(SDA, INPUT);
        digitalWrite(SDA, LOW);
        pinMode(SCL, INPUT);
        digitalWrite(SCL, LOW);
        I2C_ENABLED = false;
        tws_delay(100);
    }
}

/*
    Trigger On/Off MOSFETs
*/
void trigger(uint8_t state)
{

    if (state == 0x01)
    {
        PRIMARY_OFF = false;

        digitalWrite(MOSFET_ON, !LOW);

        digitalWrite(MOSFET_ON, !HIGH);
        tws_delay(50);
        digitalWrite(MOSFET_ON, !LOW);

        // Enable I2C and set up ISR
        i2cstate(0x01);
    }

    else if (state == 0x02)
    {
        PRIMARY_OFF = true;

        digitalWrite(MOSFET_OFF, !LOW);

        digitalWrite(MOSFET_OFF, !HIGH);
        tws_delay(50);
        digitalWrite(MOSFET_OFF, !LOW);

        // Disable I2C
        i2cstate(0x02);
    }
}

/*
    Blink function
*/
void blinkmode(uint8_t mode, uint8_t times)
{

    // Set pin as output
    pinMode(BEACON, OUTPUT);

    int on, off;

    // Slow
    if (mode == 0x01)
    {
        tws_delay(100);
        on = 1000;
        off = 1000;
    }

    // Fast
    else if (mode == 0x02)
    {
        on = 500;
        off = 500;
    }

    // Blink/beacon
    else if (mode == 0x03)
    {
        on = 100;
        off = 250;
    }

    // Long
    else if (mode == 0x04)
    {
        on = 3000;
        off = 1000;
    }

    else
    {
        on = 200;
        off = 2000;
    }

    tws_delay(100);
    int counter = 0;
    while (counter++ < times)
    {
        digitalWrite(BEACON, !HIGH);
        tws_delay(on);

        digitalWrite(BEACON, !LOW);
        if (times > 1)
            tws_delay(off);
    }
}

/*
    Format EEPROM on first run
*/
void memformat()
{

  blinkmode(0x04, 1);

  // Write init flag
  memwrite(location.init, 0x0F);

  // Write I2C address
  memwrite(location.address, I2C_ADDRESS);

  // Write base PSM interval
  memwrite(location.psm_base, PSM_INTERVAL_BASE);

  // Write interval PSM multiplier
  memwrite(location.psm_multiplier, PSM_INTERVAL_MULTIPLIER);

  // Write primary's fault counter
  memwrite(location.primary_fault, 0);

  // Write secondary's fault counter
  memwrite(location.secondary_fault, 0);

  // Write PSM state flag
  memwrite(location.psm_state_flag, 0);

  // Write time tracker value
  memwrite(location.timer0_start_timestamp, 0);
  memwrite(location.timer1_start_timestamp, 0);
  memwrite(location.timer2_start_timestamp, 0);
  memwrite(location.timer3_start_timestamp, 0);

  // Beacon enabled flag
  memwrite(location.beacon_enabled, BEACON_ENABLED);

  // Send acknowledgements flag
  memwrite(location.send_ack, SEND_ACK);

  // Beacon mode
  memwrite(location.beacon_mode, BEACON_MODE);

  // Write base interval for TIMER 0
  memwrite(location.sentinence_base_t0, INTERVAL_BASE[0]);

  // Write interval multiplier for TIMER 0
  memwrite(location.sentinence_multiplier_t0, INTERVAL_MULTIPLIER[0]);

  // Write base interval for TIMER 1
  memwrite(location.sentinence_base_t1, INTERVAL_BASE[1]);

  // Write interval multiplier for TIMER 1
  memwrite(location.sentinence_multiplier_t1, INTERVAL_MULTIPLIER[1]);

  // Write base interval for TIMER 2
  memwrite(location.sentinence_base_t2, INTERVAL_BASE[2]);

  // Write interval multiplier for TIMER 2
  memwrite(location.sentinence_multiplier_t2, INTERVAL_MULTIPLIER[2]);

  // Write base interval for TIMER 3
  memwrite(location.sentinence_base_t3, INTERVAL_BASE[3]);

  // Write interval multiplier for TIMER 3
  memwrite(location.sentinence_multiplier_t3, INTERVAL_MULTIPLIER[3]);

  // Timer enabled
  memwrite(location.enabled_t0, TIMER_ENABLED[0]);
  memwrite(location.enabled_t1, TIMER_ENABLED[1]);
  memwrite(location.enabled_t2, TIMER_ENABLED[2]);
  memwrite(location.enabled_t3, TIMER_ENABLED[3]);

  memwrite(location.last_ping_timestamp, 0);
}

/*
    Get data from EEPROM and assign state variables
*/
void memfetch()
{

    // Base interval for TIMER 0
    INTERVAL_BASE[0] = memread(location.sentinence_base_t0);

    // Interval multiplier for TIMER 0
    INTERVAL_MULTIPLIER[0] = memread(location.sentinence_multiplier_t0);

    // Base interval for TIMER 1
    INTERVAL_BASE[1] = memread(location.sentinence_base_t1);

    // Interval multiplier for TIMER 1
    INTERVAL_MULTIPLIER[1] = memread(location.sentinence_multiplier_t1);

    // Base interval for TIMER 2
    INTERVAL_BASE[2] = memread(location.sentinence_base_t2);

    // Interval multiplier for TIMER 2
    INTERVAL_MULTIPLIER[2] = memread(location.sentinence_multiplier_t2);

    // Base interval for TIMER 3
    INTERVAL_BASE[3] = memread(location.sentinence_base_t3);

    // Interval multiplier for TIMER 3
    INTERVAL_MULTIPLIER[3] = memread(location.sentinence_multiplier_t3);

    // Base PSM interval
    PSM_INTERVAL_BASE = memread(location.psm_base);

    // Interval PSM multiplier
    PSM_INTERVAL_MULTIPLIER = memread(location.psm_multiplier);

    // PSM state flag
    PSM_WAKE_FLAG = memread(location.psm_state_flag);

    // Beacon flag
    BEACON_ENABLED = memread(location.beacon_enabled);

    // Beacon flag
    SEND_ACK = memread(location.send_ack);

    // Beacon mode
    BEACON_MODE = memread(location.beacon_mode);

    // Timer enabled
    TIMER_ENABLED[0] = memread(location.enabled_t0);
    TIMER_ENABLED[1] = memread(location.enabled_t1);
    TIMER_ENABLED[2] = memread(location.enabled_t2);
    TIMER_ENABLED[3] = memread(location.enabled_t3);

    LAST_PING_TIMESTAMP = memread(location.last_ping_timestamp);
}

/*
    Write two bytes to EEPROM
*/
void memwrite(uint8_t location, uint16_t data)
{

    byte low = data & 0xFF;
    byte high = (data >> 8) & 0xFF;

    // Write low byte
    EEPROM.write(location * 2, low);

    // Write high byte
    EEPROM.write(location * 2 + 1, high);
}

/*
    Read two bytes from EEPROM
*/
uint16_t memread(uint8_t location)
{

    // Write low byte
    byte low = EEPROM.read(location * 2);

    // Write high byte
    byte high = EEPROM.read(location * 2 + 1);

    return (high << 8) | low;
}

/*
    Watchdog Interrupt Service
    This is executed when the watchdog timed out
*/
// ISR(WDT_vect) {
ISR(TIMER1_COMPA_vect)
{
    blinkmode(0x01, 3);
}

bool fuse(uint8_t state) {

  /*
    Get fuse status
    If TIMER3 is enabled -> Blown Fuse 
    If TIMER3 is disabled -> Fuse is set 
  */

  // Set the fuse
  if (state == 0x01) {
    TIMER_ENABLED[3] = false;

    start_timestamp[3] = millis() / 1000;

    // Write to EEPROM
    memwrite(location.enabled_t3, TIMER_ENABLED[3]);
    memwrite(location.timer3_start_timestamp, start_timestamp[3]);
  }

  // Blow the fuse
  else if (state == 0x02) {
    sendresponse(SUCCESS);
    TIMER_ENABLED[3] = true;

    // Write to EEPROM
    memwrite(location.enabled_t3, TIMER_ENABLED[3]);
    memwrite(location.timer3_start_timestamp, start_timestamp[3]);
  }

  else {
    // Do nothing, just return the state
  }

  return TIMER_ENABLED[3];
}

/*
    Read incoming I2C commands
*/
void i2clistener(uint8_t byteCount)
{

    // Do no accept new commands if a reboot is pending
    if (PRIMARY_REBOOT_PENDING) return;

    // // Do no accept new commands if a beacon is pending
    // if (BEACON_TRIGGER_PENDING) return;

    unsigned long elapsed = 0;
    while (elapsed++ <= 200 && !Wire.available()) delay(10);

    // Reset the Eye of Sauron everytime the primary sends a command
    if (Wire.available()) start_timestamp[3] = millis() / 1000;

    uint8_t selector = 0;
    while (Wire.available()) {

        // Convert received byte to decimal (range is 0 to 127)
        byte b = Wire.read();
        char s[4];
        itoa(b, s, 10);
        int x = atoi(s);

        // Ping
        if (x == 0)
        {
            sendresponseforced(PINGRESPONSE);

            // If two pings were sent within  15 minutes from each other
            if (LAST_PING_TIMESTAMP > 0 && (millis() / 1000) - LAST_PING_TIMESTAMP < 10) {

                // Disable all timers, including blowing the fuse
                for (uint8_t TIMER_ID = 0; TIMER_ID < 4; TIMER_ID++) {
                  TIMER_ENABLED[TIMER_ID] = false;

                  memwrite(location.enabled_t0 + (TIMER_ID * 3), TIMER_ENABLED[TIMER_ID]);
                  memwrite(location.timer0_start_timestamp + TIMER_ID, start_timestamp[TIMER_ID]);
                  delay(5);
                }

                // Turn off the primary
                trigger(0x02);

                // But don not turn off the I2C
                i2cstate(0x01);
            }

            // If two pings were sent within  15 minutes from each other
            else if (LAST_PING_TIMESTAMP > 0 && (millis() / 1000) - LAST_PING_TIMESTAMP < 30) {

              // If the fuse is blown
              if (fuse(0x00)) {
                
                // Set the fuse
                fuse(0x01);
              }
              else {
                
                // Blow the fuse
                fuse(0x02);
              }

              // Reset the timestamp
              memwrite(location.last_ping_timestamp, 0);
            }

            LAST_PING_TIMESTAMP = millis() / 1000;
            memwrite(location.last_ping_timestamp, LAST_PING_TIMESTAMP);
        };

        // Unlock configuration
        if (x == 1)
        {
            CONFIG_UNLOCKED = true;
            sendresponseforced(SUCCESS);
        }

        // Lock configuration
        if (x == 2)
        {
            CONFIG_UNLOCKED = false;
            sendresponseforced(SUCCESS);
        }

        // Firmware version
        if (x == 3)
        {
            sendresponseforced(FIRMWARE_MAJOR_VERSION * 100 + FIRMWARE_MINOR_VERSION);
        }

        // Firmware month and date
        if (x == 4)
        {
            sendresponseforced(FIRMWARE_MONTH + '-' + FIRMWARE_DATE);
        }

        // Power off the primary indefinitely (will restart when secondary self reboots from WDT in an hour)
        if (x == 5) { }

        // Reboot the primary
        if (x == 6)
        {
            sendresponse(SUCCESS);

            // Lock Sentinel configuration
            CONFIG_UNLOCKED = false;

            // Reboot the primary
            reboot(0x02);

            delay(500);

            // Reboot the secondary
            reboot(0x01);
        }
        
        // Reboot the secondary microcontroller
        if (x == 7) // 13
        {
            sendresponse(SUCCESS);
            delay(50);
            reboot(0x01);
            PRIMARY_REBOOT_PENDING = false;
        };

        // Reset fault counters
        if (x == 8) // 7
        {
            memwrite(location.primary_fault, 0);
            memwrite(location.secondary_fault, 0);
            sendresponse(SUCCESS);
        }

        // Primary's fault counter
        if (x == 9) // 8
        {
            sendresponseforced(memread(location.primary_fault));
        }

        // Secondary's fault counter
        if (x == 10) // 9
        {
            sendresponseforced(memread(location.secondary_fault));
        }

        // Put secondary to sleep
        if (x == 11) { }

        // Set interval setting mode to sentinence interval
        if (x == 12) // 11
        {
            if (!CONFIG_UNLOCKED)
                sendresponse(CONFIG_LOCKED_ERROR);
            else
                sendresponse(SUCCESS);
            INTERVAL_SET_MODE = 0;
        }

        // Set interval setting mode to power saving interval
        if (x == 13) // 12
        {
            if (!CONFIG_UNLOCKED)
                sendresponse(CONFIG_LOCKED_ERROR);
            else
                sendresponse(SUCCESS);
            INTERVAL_SET_MODE = 1;
        }

        // Set interval setting mode to beacon interval
        if (x == 14)
        {
            if (!CONFIG_UNLOCKED)
                sendresponse(CONFIG_LOCKED_ERROR);
            else
                sendresponse(SUCCESS);
            INTERVAL_SET_MODE = 2;
        }

        // Turn on periodic beacon
        if (x == 15)
        {
            if (BEACON_ENABLED)
            {
                sendresponse(SUCCESS);
                return;
            }

            sendresponse(SUCCESS);
            BEACON_ENABLED = true;
            memwrite(location.beacon_enabled, BEACON_ENABLED);
        }

        // Turn off periodic beacon
        if (x == 16)
        {
            if (!BEACON_ENABLED)
            {
                sendresponse(SUCCESS);
                return;
            }

            sendresponse(SUCCESS);
            BEACON_ENABLED = false;
            memwrite(location.beacon_enabled, BEACON_ENABLED);
        }

        // Trigger beacon
        if (x == 17)
        {

            sendresponse(SUCCESS);

            digitalWrite(BEACON, !HIGH);
            delay(100);
            digitalWrite(BEACON, !LOW);
            delay(50);

            digitalWrite(BEACON, !HIGH);
            delay(100);
            digitalWrite(BEACON, !LOW);
            delay(50);
        }

        // Turn off the primary indefinitely
        // TODO: Put secondary on PSM when this happens
        if (x == 18)
        {
            sendresponse(SUCCESS);

            // Turn off the primary
            shutdown();
        }

        // Enable sending acknowledgements
        if (x == 19)
        {
            if (SEND_ACK)
            {
                sendresponse(SUCCESS);
                return;
            }

            sendresponse(SUCCESS);

            SEND_ACK = true;
            memwrite(location.send_ack, SEND_ACK);
        }

        // Disable sending acknowledgements
        if (x == 20)
        {
            if (!SEND_ACK)
            {
                sendresponse(SUCCESS);
                return;
            }

            sendresponse(SUCCESS);
            SEND_ACK = false;
            memwrite(location.send_ack, SEND_ACK);
        }

        // Format EEPROM on the secondary microcontroller
        if (x == 21)
        {
            sendresponse(SUCCESS);
            memformat();
        }

        // Diagnostics test
        if (x == 22)
        {
            // Send acknowledgment
            sendresponseforced(SUCCESS);

            // Trigger OFF MOSFET
            trigger(0x02);

            tws_delay(500);

            // Trigger ON MOSFET
            trigger(0x01);

            // Test beacon/LED
            blinkmode(0x03, 4);
        }

        /*
            ! Sentinence control
            These functions apply on the SELECTED_TIMERID
        */

        // Sentinence on (Turn on Timer SELECTED_TIMERID)
        else if (x == 30)
        {
            setsentinence(0x01);
        }

        // Sentinence off (Turn off Timer SELECTED_TIMERID)
        else if (x == 31)
        {
            setsentinence(0x02);
        }

        // Heartbeat (kick the dog) (reset on Timer SELECTED_TIMERID)
        else if (x == 32)
        {
            setsentinence(0x03);
        }

        // Select timer (Set SELECTED_TIMERID)
        else if (x >= 33 && x <= 36)
        {
            sendresponse(SUCCESS);
            selector = x - 33;

            SELECTED_TIMERID = selector;
        }

        /*
        Set fuse
        This disables Timer 3 until the fuse is reset
        */
        else if (x == 37) 
        {
          fuse(0x01);
        }

        // Blow/reset fuse
        else if (x == 38) 
        {
          fuse(0x02);
        }

        // Fuse/Timer 3 status
        else if (x == 39) 
        {
          sendresponseforced(fuse(0x00));
        }
        
        // Interval base interval control (10 options)
        else if (x >= 40 && x <= 49)
        {
            selector = x - 40;

            if (!CONFIG_UNLOCKED)
                sendresponse(CONFIG_LOCKED_ERROR);

            /*
                Set the base interval.
                The range for selector index is 0 to 6.

                The options correspond to:
                1 sec, 5 sec, 10 sec, 15 sec, 30 sec, 1 min, 1 min 30 seconds
            */
            uint8_t options[] = {1, 5, 10, 15, 30, 60, 90};

            // Check if the selector is out of range (the length of the options array).
            if (selector < static_cast<uint8_t>(sizeof(options) / sizeof(options[0]))) {

                // Set sentinence interval base
                if (INTERVAL_SET_MODE == 0)
                {
                    sendresponse(SUCCESS);
                    INTERVAL_BASE[SELECTED_TIMERID] = options[selector];
                    memwrite(location.sentinence_base_t0 + (SELECTED_TIMERID * 3), INTERVAL_BASE[SELECTED_TIMERID]);
                    // sendresponse(memread(location.sentinence_base_t0));
                }

                // Set power-saving interval base
                else if (INTERVAL_SET_MODE == 1)
                {
                    sendresponse(SUCCESS);
                    PSM_INTERVAL_BASE = options[selector];
                    memwrite(location.psm_base, PSM_INTERVAL_BASE);
                    // sendresponse(memread(location.psm_base));
                }

                // Set beacon interval base
                else if (INTERVAL_SET_MODE == 2)
                {
                    sendresponse(SUCCESS);
                    BCN_INTERVAL_BASE = options[selector];
                }
            }
            else
                sendresponse(ERROR);
        }

        // Interval duration control (20 levels)
        else if (x >= 50 && x <= 69)
        {
            selector = x - 50;

            if (!CONFIG_UNLOCKED)
                sendresponse(CONFIG_LOCKED_ERROR);
            else
                sendresponse(SUCCESS);

            /*
                Set the interval multiplier.
                The  range for the multiplier is 1 to 20.
            */

            // Set sentinence interval multiplier
            if (INTERVAL_SET_MODE == 0)
            {
                sendresponse(SUCCESS);
                INTERVAL_MULTIPLIER[SELECTED_TIMERID] = (uint8_t) selector + 1;
                memwrite(location.sentinence_multiplier_t0 + (SELECTED_TIMERID * 3), INTERVAL_MULTIPLIER[SELECTED_TIMERID]);
            }

            // Set power-saving interval multiplier
            else if (INTERVAL_SET_MODE == 1)
            {
                sendresponse(SUCCESS);
                PSM_INTERVAL_MULTIPLIER = (uint8_t) selector + 1;
                memwrite(location.psm_multiplier, PSM_INTERVAL_MULTIPLIER);
            }

            // Set beacon interval multiplier
            else if (INTERVAL_SET_MODE == 2)
            {
                sendresponse(SUCCESS);
                BCN_INTERVAL_MULTIPLIER = (uint8_t)selector + 1;
            }
        }

        // Read EEPROM contents
        else if (x >= 70 && x <= 99)
        {
            selector = x - 70;

            // Compute location index
            uint16_t MEMLOC = selector;

            // Read data from EEPROM
            uint16_t data = memread(MEMLOC);

            // Send response
            sendresponseforced(data);
        }

        /*
            Set beacon mode
                0 - Periodic beacon based on BCN time values
                1 - Sentinence indicator for TIMER0
                2 - Blink with 1 second ON and 1 second OFF
                3 - One 150ms audio chirp
        */
        else if (x >= 100 && x <= 103)
        {
            sendresponse(SUCCESS);
            selector = x - 100;
            BEACON_MODE = selector;
        }
        else
        {
            sendresponse(INVALID_OPTION);
        }

        // TinyWire library needs this to detect the end of an incoming message
        TinyWireS_stop_check();
    }
}

/*
    WDT shield
*/
void watch(uint8_t duration, callback_t_func function) {

    // Enable WDT
    wdtduration(duration);

    // Call the function/scope/block
    function();

    // Disable WDT
    wdt(0x02);
}

uint32_t last_i2c_turnon_timestamp = 0;
uint32_t last_beacon_timestamp = 0;
int led_state = LOW;

void loop() {

    watch(WDTO_4S, []() { 

        /*
            Ensure I2C is working by resetting I2C every 30 minutes
            This ocassionally causes I2C communication failures.
        */
        if (millis() - last_i2c_turnon_timestamp >= 30UL * 60 * 1000) {
            i2cstate(0x01);
            last_i2c_turnon_timestamp = millis();
        }

        // TinyWire library needs this to detect end of an incoming message
        TinyWireS_stop_check();

        // Check if the reboot condition is met. The bytes need to be converted to integers before comparisions
        TIMER_EXPIRED[0] = TIMER_ENABLED[0] && uint16_t(millis() / 1000) - uint16_t(start_timestamp[0]) > uint16_t(INTERVAL_BASE[0]) * uint16_t(INTERVAL_MULTIPLIER[0]);
        TIMER_EXPIRED[1] = TIMER_ENABLED[1] && uint16_t(millis() / 1000) - uint16_t(start_timestamp[1]) > uint16_t(INTERVAL_BASE[1]) * uint16_t(INTERVAL_MULTIPLIER[1]);
        TIMER_EXPIRED[2] = TIMER_ENABLED[2] && uint16_t(millis() / 1000) - uint16_t(start_timestamp[2]) > uint16_t(INTERVAL_BASE[2]) * uint16_t(INTERVAL_MULTIPLIER[2]);
        TIMER_EXPIRED[3] = TIMER_ENABLED[3] && uint16_t(millis() / 1000) - uint16_t(start_timestamp[3]) > uint16_t(INTERVAL_BASE[3]) * uint16_t(INTERVAL_MULTIPLIER[3]);

        PRIMARY_REBOOT_PENDING = TIMER_EXPIRED[0] || TIMER_EXPIRED[1] || TIMER_EXPIRED[2] || TIMER_EXPIRED[3];
        
        // Mirror the state of Timer 0
        if (BEACON_MODE == 0) {
            digitalWrite(BEACON, !TIMER_ENABLED[0]);
        }

        // Flash like a beacon at a configurable interval
        else if (BEACON_MODE == 1) {
            BEACON_TRIGGER_PENDING = BEACON_ENABLED && (uint16_t(millis() / 1000) - uint16_t(last_beacon_timestamp) >= uint16_t(BCN_INTERVAL_BASE) * uint16_t(BCN_INTERVAL_MULTIPLIER));
        }

        // Blink LED with 1 second ON and 1 second OFF
        else if (BEACON_MODE == 2) {
            BEACON_TRIGGER_PENDING = uint16_t(millis() / 1000) - uint16_t(last_beacon_timestamp) >= uint16_t(1);
        }

        // Audio chirp
        else if (BEACON_MODE == 3) {
            BEACON_TRIGGER_PENDING = BEACON_ENABLED && uint16_t(millis() / 1000) - uint16_t(last_beacon_timestamp) >= uint16_t(BCN_INTERVAL_BASE) * uint16_t(BCN_INTERVAL_MULTIPLIER);
        }

        // Trigger beacon mode 1
        if (BEACON_MODE == 1 && BEACON_TRIGGER_PENDING) {
            last_beacon_timestamp = millis() / 1000;

            // Reset variable
            BEACON_TRIGGER_PENDING = false;

            digitalWrite(BEACON, !HIGH); 
            delay(80); 
            digitalWrite(BEACON, !LOW); 
            delay(50);
            
            digitalWrite(BEACON, !HIGH); 
            delay(40); 
            digitalWrite(BEACON, !LOW); 

            if (TIMER_ENABLED[3]) {
                delay(50);
                digitalWrite(BEACON, !HIGH);         
                delay(220);
                digitalWrite(BEACON, !LOW); 
            }
        }

        if (BEACON_MODE == 2 && BEACON_TRIGGER_PENDING) {
            last_beacon_timestamp = millis() / 1000;

            // Reset variable
            BEACON_TRIGGER_PENDING = false;

            led_state = !led_state;
            digitalWrite(BEACON, led_state); 
        }

        if (BEACON_MODE == 3 && BEACON_TRIGGER_PENDING) {
            last_beacon_timestamp = millis() / 1000;

            // Reset variable
            BEACON_TRIGGER_PENDING = false;
            
            digitalWrite(BEACON, !HIGH); 
            delay(150); 
            digitalWrite(BEACON, !LOW); 
        }

        // Check if a reboot is pending on the primary microcontroller
        if (PRIMARY_REBOOT_PENDING) {

            // Turn off sentinence
            if (TIMER_EXPIRED[0]) {
                TIMER_ENABLED[0] = false;
                start_timestamp[0] = millis() / 1000;
            }
            if (TIMER_EXPIRED[1]) {
                TIMER_ENABLED[1] = false;
                start_timestamp[1] = millis() / 1000;
            }
            if (TIMER_EXPIRED[2]) {
                TIMER_ENABLED[2] = false;
                start_timestamp[2] = millis() / 1000;
            }
            if (TIMER_EXPIRED[3]) {
                start_timestamp[3] = millis() / 1000;
            }

            // Lock Sentinel configuration
            CONFIG_UNLOCKED = false;

            // Increment the primary fault counter
            uint8_t faults = memread(location.primary_fault) + 1;
            memwrite(location.primary_fault, faults);
            
            // Reboot the primary
            reboot(0x02);
            
            // Reset state variable
            PRIMARY_REBOOT_PENDING = false;
            
        }

        // Put sentinel to sleep (current drops from 1 mA to 0 mA)
        if (USE_PSM) sleep(WDTO_2S); 
    });

    // 10 ms delay (Required; do not change)
    delay(10);
}

void setup() {

    watch(WDTO_4S, []() {
        // If the device is waking up from PSM sleep
        if (USE_PSM && memread(location.psm_state_flag) == 1) PSM_WAKE_FLAG = true;
        if (USE_PSM && PSM_WAKE_FLAG) {

            // Restore time tracker variables
            start_timestamp[0] = memread(location.timer0_start_timestamp);
            start_timestamp[1] = memread(location.timer1_start_timestamp);
            start_timestamp[2] = memread(location.timer2_start_timestamp);

            // Reset variables
            memwrite(location.psm_state_flag, 0);
            memwrite(location.timer0_start_timestamp, 0);
            memwrite(location.timer1_start_timestamp, 0);
            memwrite(location.timer2_start_timestamp, 0);
        }

        // Format EEPROM if first run
        if (memread(location.init) != 0x0F) memformat();

        // Fetch data from EEPROM
        memfetch();

        // Set pinModes
        pinMode(MOSFET_ON, OUTPUT);
        pinMode(MOSFET_OFF, OUTPUT);
        pinMode(BEACON, INPUT);

        // Set beacon pin to HIGH (connected to P-MOSFET)
        digitalWrite(BEACON, !LOW);

        // De-trigger MOSFETs (connected to P-MOSFETs)
        digitalWrite(MOSFET_ON, !LOW);
        digitalWrite(MOSFET_OFF, !LOW);

        // If the device is turning on from a cold state
        if (!USE_PSM || (USE_PSM && !PSM_WAKE_FLAG)) {

            // If the fuse is blown (i.e., TIMER3 is enabled)
            if (TIMER_ENABLED[3]) {
              trigger(0x01);
              blinkmode(0x03, 1);
            }

            // The fuse is set (i.e., TIMER3 is disabled)
            else {
              blinkmode(0x03, 2);
              trigger(0x02);

              // Enable I2C and set up ISR
              i2cstate(0x01);
            }
        }

        // If the device is waking up from a PSM sleep state
        else if (USE_PSM && PSM_WAKE_FLAG) {

            // blinkmode(0x03, 2);
            i2cstate(0x01);
            tws_delay(1000);
            blinkmode(0x03, 3);
        }

        // Check if the microcontroller was reset by the Watchdog Timer
        if (MCUSR & (1 << WDRF)) {

            // Clear the Watchdog Reset Flag
            MCUSR &= ~(1 << WDRF);

            blinkmode(0x03, 5);

            // // Increment the fault counter
            // uint8_t faults = memread(location.secondary_fault) + 1;
            // memwrite(location.secondary_fault, faults);
        }
        else if (MCUSR & (1 << PORF)) {

            // Clear the Watchdog Reset Flag
            MCUSR &= ~(1 << WDRF);

            blinkmode(0x03, 2);
        }
        // else if (MCUSR & (1 << BORF)) {

        //   // Clear the Watchdog Reset Flag
        //   MCUSR &= ~(1 << WDRF);

        //   blinkmode(0x03, 6);
        // }
    });

    // // Enable WDT
    // wdtduration(WDTO_2S);
    // delay(4000);
}
#endif