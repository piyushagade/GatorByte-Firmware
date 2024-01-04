# Setup Instructions

Please follow the steps below to configure VS Code and PlatformIO extension to enable updating and uploading the firmware to the GatorByte.

##  Visual Studio Code
VS Code is a code-editor (IDE) that developers use to write code. VS Code offers a range of developer-friendly features and extensions. The original developers of the GatorByte firmware used VS Code for managing and updating the codebase and libraries.

1. Download Visual Studio Code from [here](https://code.visualstudio.com/Download). Select the download for your operating system and follow on-screen instructions to install the software.
2. Open VS Code.
3. Open "Extensions" in the left sidebar by either clicking on the "Extensions" button or by pressing ``Ctrl+Shift+X`` on Windows/Linux and ``Cmnd+Shift+X`` on MacOS.

4. In the search box, enter ``PlatformIO``. Install the extension.

## PlatformIO IDE extension
  
PlatformIO is an open-source ecosystem for Internet of Things (IoT) development. It provides a unified development platform that supports multiple microcontroller architectures, frameworks, and development platforms. PlatformIO is designed to simplify and streamline the process of developing embedded systems.

The PlatformIO VSCode extension is an extension for Microsoft Visual Studio Code The extension integrates PlatformIO into VSCode, providing a powerful environment for embedded systems development. With this extension, developers can easily manage and build projects for various microcontroller platforms, upload firmware, and monitor serial output, among other things.

## Install PlatformIO Extension for VS Code 
---------------------------------------- 
1. **Open Visual Studio Code:** 
Launch Visual Studio Code after the installation is complete. 
2. **Open Extensions View:** 
Click on the Extensions icon in the Activity Bar on the side of the window or press `Ctrl+Shift+X` (Windows/Linux) or `Cmd+Shift+X` (macOS). 
3. **Search for PlatformIO:**
In the Extensions view search bar, type "PlatformIO" and press Enter. 
4. **Install PlatformIO Extension:**
Look for the PlatformIO IDE extension in the search results. Click the "Install" button next to the PlatformIO IDE extension to install it. 
5. **Restart VS Code:** 
After installation, click the "Reload" button or restart Visual Studio Code to activate the PlatformIO extension. 

Configure PlatformIO for Arduino MKR NB 1500 
-------------------------------------------- 
6. **Create a New Project:**
Open Visual Studio Code. Click on the "View" menu, then "Command Palette" (`Ctrl+Shift+P` or `Cmd+Shift+P`). Type and select "PlatformIO: New Project" in the command palette. Choose a directory for your project and select the board "Arduino MKR NB 1500." 
7. **Edit `platformio.ini` File:** 
* Open the `platformio.ini` file in your project directory. 
* Configure the `platform` and `board` settings for Arduino MKR NB 1500: ini
* Copy code 

        [env:mkrnb1500] 
        platform = atmelsam 
        board = mkrnb1500
        framework = arduino
        monitor_speed = 9600
        lib_deps =
	        arduino-libraries/ArduinoHttpClient@^0.4.0
	        xreef/PCF8575 library@^1.0.1 
	        arduino-libraries/Arduino Low Power@^1.2.2
	        mikalhart/TinyGPSPlus@^1.0.2
	        electroniccats/MPU6050@^0.2.1
	        arduino-libraries/ArduinoMqttClient@^0.1.5
	        khoih-prog/SAMD_TimerInterrupt@^1.9.0
	        wh1terabbithu/ADS1115-Driver@^1.0.2
	        cmaglie/FlashStorage@^1.0.0
	        khoih-prog/FlashStorage_SAMD@^1.3.2
	        marzogh/SPIMemory@^3.4.0
   
    

3. **Install Dependencies:** * Save the `platformio.ini` file. * Open the PlatformIO sidebar (`Ctrl+Alt+P` or `Cmd+Alt+P`) and click on "Platforms." * Click "Update" to ensure the platform and board packages are up to date. * PlatformIO will automatically download and install the necessary dependencies. 4. **Upload Firmware:** * Write your Arduino sketch in the `src` directory. * Connect your Arduino MKR NB 1500 to your computer. * Click on the "PlatformIO" icon in the Activity Bar and select "Upload" to compile and upload the firmware to your Arduino.
