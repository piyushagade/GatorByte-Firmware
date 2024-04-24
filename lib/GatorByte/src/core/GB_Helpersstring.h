#pragma once
#include <string.h>
#include <ctype.h>

/*
    ! Split string by delimiter and get value at index
    For example: 
        data: abc,def,ghi,jkl,mno
        delimiter: ,
        index: 3

        returns: jkl
*/
String GB::split(String data, char delimiter, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == delimiter || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/*
    ! Remove a substring from a string
*/
String GB::sreplace(String original, String target, String replacement) {
    String result = "";

    int index = 0;
    int lastIndex = 0;

    while ((index = original.indexOf(target, lastIndex)) != -1) {
        result += original.substring(lastIndex, index) + replacement;
        lastIndex = index + target.length();
    }

    result += original.substring(lastIndex);

    return result;
}

/*
    ! Remove a substring from a string
*/
String GB::sremove(String input, String startSubstring, String endSubstring) {
    int startIndex = input.indexOf(startSubstring);
    int endIndex = input.indexOf(endSubstring, startIndex);

    // Check if both start and end substrings are found
    if (startIndex != -1 && endIndex != -1) {
        // Remove the dynamic substring
        input.remove(startIndex, endIndex - startIndex + endSubstring.length());
    }

    return input;
}

String GB::uuid() {
    int randomNumber = random(10000, 99999);
    return String(randomNumber);
}
String GB::uuid(int index) {
    int randomNumber = random(10 * (index - 1), 10 * index - 1);
    return String(randomNumber);
}

/*
    ! Delay function with mqtt updates
*/
GB& GB::wait(unsigned long milliseconds){
    unsigned long now = millis();
    while (millis() - now < milliseconds) {
        now = millis();


        // Loop mqtt
        if (this->hasdevice("mqtt")) this->getdevice("mqtt")->update();
    }
    return *this;
}

/*
    ! Convert String to char*
*/
char* GB::s2c(String str){
    char *p = const_cast<char*>(str.c_str());
    return p;
}

/*
    ! Convert a char array to string
*/
String GB::ca2s(char char_array[]){
    String str((char*) char_array);
    return str;
}

/*
    ! Convert a byte array to float number
*/
float GB::ba2f(byte byte_array[]){
    return *((float*) byte_array);
}

/*
    ! Checks if a string is a valid number
*/
bool GB::isnumber (String str) {
    bool isvalidnumber = true;
    for (byte i=0; i < str.length (); i++) {
        if (!isDigit (str.charAt(i))) isvalidnumber = false;
    }
    return isvalidnumber;
}

GB& GB::logb(String message) {
    Serial.write(27);
    Serial.print("[2J");
    Serial.write(27);
    Serial.print("[H"); 
    return this->log(String(message), false);
}

GB& GB::loga(String message) {
    return this->log(String(message), false);
}

GB& GB::loge(String message) {
    return this->log(String(message), true);
}

GB& GB::color() { return this->color("transparent"); }
GB& GB::color(String color) { 
    if (color == "" || color == "transparent") this->globals.LOGCOLOR = "\033[0m";    
    else if (color == "black") this->globals.LOGCOLOR = "\033[1;37;40m"; 
    else if (color == "white") this->globals.LOGCOLOR = "\033[1;30;47m"; 
    else if (color == "red") this->globals.LOGCOLOR = "\033[1;37;41m";    
    else if (color == "green") this->globals.LOGCOLOR = "\033[1;37;42m";    
    else if (color == "yellow") this->globals.LOGCOLOR = "\033[1;30;43m";    
    else if (color == "blue") this->globals.LOGCOLOR = "\033[1;37;44m";    
    else if (color == "magenta") this->globals.LOGCOLOR = "\033[1;37;45m";    
    else if (color == "cyan") this->globals.LOGCOLOR = "\033[1;30;46m";    
    else if (color == "grey" || color == "gray") this->globals.LOGCOLOR = "\033[1;30;40m";    
    return *this;
}

/*
    ! Print String to console
*/
GB& GB::log(String message) { return this->log(String(message), true); }
GB& GB::log(String message, bool newline) {

    /*
        When MODEMDEBUG is enabled, only show MODEM debug messages
    */
    if (MODEM_DEBUG) return *this;

    if (this->_concat_print) {
        this->_concat_print = false;
        newline = false;
    }

    // if (!this->globals.GDC_CONNECTED) {
        if (message.contains("Done")) {
            message.replace("Done", "\033[1;37;42mDone");
        }
        else if (message.contains("Enabled")) {
            message.replace("Enabled", "\033[1;37;42mEnabled");
        }
        else if (message.contains("Failed")) {
            message.replace("Failed", "\033[1;37;41mFailed");
        }
        else if (message.contains("Not detected")) {
            message.replace("Not detected", "\033[1;37;41mNot detected");
        }
        else if (message.contains("Skipped")) {
            message.replace("Skipped", "\033[1;30;43mSkipped");
        }
        else if (message.contains(" NOT ")) {
            message.replace(" NOT ", " \033[1;30;47mNOT ");
        }
    // }

    if(newline) {
        
        int indexoflastslash = String(__FILE__).lastIndexOf("/");
        String filename = String(__FILE__).substring(indexoflastslash + 1, String(__FILE__).length());

        // Print file and line number
        // this->serial.debug->print("[" + filename + ":" + __func__ + ":" + String(__LINE__) + "] ");

        if (this->BLDEBUG && !this->globals.GDC_CONNECTED) {
            if (this->getdevice("bl")->initialized() && this->hasdevice("bl")) {
                this->getdevice("bl")->print(message, newline);
            }
        }

        if (this->SERIALDEBUG) {
            this->serial.debug->print((this->globals.SENTENCEENDED ? (this->globals.LOGPREFIX.length() > 0 ? this->globals.LOGPREFIX + " " : "  ") : (this->globals.NEWSENTENCE ? "   " : ""))  + this->globals.LOGCOLOR + message);
        
            // Reset color
            this->serial.debug->println("\033[0m");
            this->globals.LOGCOLOR = "";
            
            this->globals.NEWSENTENCE = true;
            this->globals.SENTENCEENDED = true;
        }
        
    }
    else {
        
        // Serial.write(27);                               // ESC
        // Serial.print("[2J");
        // Serial.write(27);                               // ESC
        // Serial.print("[H"); 
        
        if (!this->globals.GDC_CONNECTED) this->globals.NEWSENTENCE = this->globals.SENTENCEENDED ? true : false;
        
        if (this->BLDEBUG && !this->globals.GDC_CONNECTED) {
            if (this->getdevice("bl")->initialized() && this->hasdevice("bl")) {
                this->getdevice("bl")->print(message, false);
            }
        }

        if (this->SERIALDEBUG) {
            this->serial.debug->print((this->globals.NEWSENTENCE ? (this->globals.LOGPREFIX.length() > 0 ? this->globals.LOGPREFIX + " " : "  ") : "")  + this->globals.LOGCOLOR + message);
            
            // Reset color
            this->serial.debug->print("\033[0m");
            this->globals.SENTENCEENDED = false;
        }
    }

    return *this;

}

GB& GB::heading(String message) {

    this->log(message, true);
    String underline = "";
    for (int i = 0; i < message.length(); i++) underline += "-";
    this->log(underline, true);

    return *this;
}

GB& GB::arrow() {
    this->log(" -> ", false);
    return *this;
}

/*
    ! Print char to console
*/
GB& GB::log(char message) { return this->log(String(message), true); }
GB& GB::log(char message, bool new_line) {
    return this->log(String(message), new_line);
}

/*
    ! Print integer to console
*/
GB& GB::log(int message) { return this->log(String(message), true); }
GB& GB::log(int message, bool new_line) {
    return this->log(String(message), new_line);
}

/*
    ! Print float to console
*/
GB& GB::log(float message) { return this->log(String(message), true); }
GB& GB::log(float message, bool new_line) { return this->log(String(message), new_line); }

/*
    ! Print blank line in console
*/
GB& GB::log() {
    return this->log("", true);
}
GB& GB::bs(){
    this->_concat_print = true;
    return *this;
}

GB& GB::br(){
    return br(1);
}
GB& GB::br(int count){
    if (count == 1) {
        this->log("", true);
        return *this;
    }
    else {
        this->log("", true);
        this->br(--count);
    }
    return *this;
}
GB& GB::space(int count){
    for (int i = 0; i < count; i++) this->log(" ", false);
    return *this;
}

GB& GB::del() {
    Serial.write(27);
    return *this;
}
GB& GB::clr() {
    Serial.print("\e[2J");
    return *this;
}

/*
    ! Debug print to console
*/
GB& GB::logd(String message) { return this->logd(String(message), true); }
GB& GB::logd(String message, bool newline) {
    
    // Return if debug is off
    if (!this->SERIALDEBUG) return *this;


    if(newline) {
        this->serial.debug->println(F(message.c_str()));
    }
    else {
        // Serial.write(27);                               // ESC
        // Serial.print("[2J");
        // Serial.write(27);                               // ESC
        // Serial.print("[H"); 
        this->serial.debug->print(F(message.c_str()));
    }

    return *this;
}

int GB::s2hash(String data) {
    
    unsigned int hash = 0;
    for (int i = 0; i < data.length(); i++) {
        unsigned int charCode = static_cast<unsigned int>(data.charAt(i));
        hash = (hash << 5) - charCode;
    }

    return hash;
}
char* GB::trim(char str[]) {
    char* trimmed_str = str;

    // Trim leading spaces
    while (isspace(*trimmed_str)) {
        trimmed_str++;
    }

    if (*trimmed_str == '\0') // All spaces?
        return trimmed_str;

    // Trim trailing spaces
    char *end = trimmed_str + strlen(trimmed_str) - 1;
    while (end > trimmed_str && isspace(*end)) {
        end--;
    }

    // Write new null terminator
    *(end + 1) = '\0';

    return trimmed_str;
}

char* GB::strcat(char str[], char c) {
    // Create a temporary string with the character to concatenate
    char temp[2];
    temp[0] = c;
    temp[1] = '\0';  // Ensure the string is null-terminated

    // Concatenate temp to str
    if (strlen(str) + strlen(temp) < sizeof(str)) {
        strcat(str, temp); // Pass the address of temp to strcat
    }

    return str;
}