#include "SDFat.h"
#include "SDU.h"


#include "FlashStorage_SAMD.h"
#include "HC595.h"
FlashClass flash;

#define SDU_START    0x2000
#define SDU_SIZE     0x4000

#define SKETCH_START (uint32_t*)(SDU_START + SDU_SIZE)

class GB_FW {
    public:
        GB_FW();
        bool check();
        char* s2c(String);

    private:
        String _FOLDER = "/update/";
        String _FILE = "firmware.bin";
        bool _update();
        SdFat _sd;
        HC595 _sr;
        int _CS_PIN = 7;
        int _SD_ENABLE_PIN = 4;
};

GB_FW::GB_FW() { }

// Check if the sd card has a firmware file
bool GB_FW::check() {

    this->_sr.begin(2, 3, 4, 5);

    // Turn on SD card module
    this->_sr.set(_SD_ENABLE_PIN, HIGH);

    // Test SD connection
    if(!_sd.begin(_CS_PIN, SPI_HALF_SPEED)) return;

    // Check if the file exists
    if (!_sd.exists(this->_FOLDER + this->_FILE)) return false;

    // Process update if all checks pass
    return this->_update();
}

// Update the firmware
bool GB_FW::_update() {

    File updateFile;
    bool success = updateFile.open(s2c(this->_FOLDER + this->_FILE), FILE_WRITE);

    uint32_t updateSize = updateFile.size();
    bool updateFlashed = false;

    if (updateSize > SDU_SIZE) {

        // Skip the SDU section
        updateFile.seek(SDU_SIZE);
        updateSize -= SDU_SIZE;

        uint32_t flashAddress = (uint32_t) SKETCH_START;

        // Erase the pages
        flash.erase((void*) flashAddress, updateSize);

        uint8_t buffer[512];

        // Write the pages
        for (uint32_t i = 0; i < updateSize; i += sizeof(buffer)) {
            updateFile.read(buffer, sizeof(buffer));
            flash.write((void*) flashAddress, buffer);
            flashAddress += sizeof(buffer);
        }

        updateFlashed = true;
    }

    updateFile.close();

    if (updateFlashed) {
      _sd.remove(this->_FOLDER + this->_FILE);
    }

    // Jump to the sketch
    __set_MSP(*SKETCH_START);

    // Reset vector table address
    SCB->VTOR = ((uint32_t)(SKETCH_START) & SCB_VTOR_TBLOFF_Msk);

    // Address of Reset_Handler is written by the linker at the beginning of the .text section (see linker script)
    uint32_t resetHandlerAddress = (uint32_t) * (SKETCH_START + 1);

    // Jump to reset handler
    asm("bx %0"::"r"(resetHandlerAddress));

    // Turn off SD card module
    this->_sr.set(_SD_ENABLE_PIN, LOW);

    return false;
}

char* GB_FW::s2c(String str){
    char *p = const_cast<char*>(str.c_str());
    return p;
}

// Initialize C library
extern "C" void __libc_init_array(void);

// int main() {
//     init();

//     __libc_init_array();
//     delay(1);

//     GB_FW fw;

//     // Check for and update the firmware
//     fw.check();
// }