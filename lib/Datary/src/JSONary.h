/*
JSONary.h - Library for JSON
*/
#ifndef JSONary_h
#define JSONary_h

#include "Datary.h"

class JSONary
{
	public:
		JSONary();
		String get();
		JSONary& set(String);
		JSONary& set(String, double);
		JSONary& set(String, String);
		String unset(String);
		JSONary& reset();
		
		bool getboolean(String key);
		float getfloat(String key);
		int getint(String key);
		String getstring(String key);
		String toKeyValue();
	private:
		String _data;
        char* s2c(String str);
		bool isnumber (String str);
		JSONary& _set(String, String);
};

JSONary::JSONary()
{	
	this->_data = "{}";
}

String JSONary::get() {
	return this->_data;
}

JSONary& JSONary::set(String data) {
	if (data.indexOf(":") > -1) this->set(strtok(this->s2c(data), ":"), strtok(NULL, ":"));
	return *this;
}

JSONary& JSONary::set(String key, String value) {
	if (value == "true" || value == "false") return this->_set(key, String(value));
	else if (this->isnumber(value)) return this->_set(key, String(value));
    else return this->_set(key, "\"" + String(value) + "\"");
}

JSONary& JSONary::set(String key, double value) {
    return this->_set(key, String(value));
}

JSONary& JSONary::reset() {
	this->_data = "{}";
    return *this;
}

String JSONary::unset(String key) {
	
    // Delete the last '}'
    this->_data.remove(this->_data.length() - 1, 1);

	// Check if the key exists
	if (this->_data.indexOf("\"" + key + "\"" + ":") > -1) {
		// Modify value of the key in the json
		int key_index = this->_data.indexOf("\"" + key + "\"" + ":");
		int value_quote_start_index = this->_data.indexOf("\"" + key + "\"" + ":") + String("\"" + key + "\"" + ":").length();
		int value_quote_end_index = value_quote_start_index + 1;
		
		int new_value_index = 0;
		do { value_quote_end_index++; } while (String(this->_data.charAt(value_quote_end_index)).length() > 0 && String(this->_data.charAt(value_quote_end_index)) != "\"");

		String prefix = this->_data.substring(0, key_index);
		String suffix = this->_data.substring(value_quote_end_index + 1, this->_data.length() + 1) + "}";
		if (this->_data.charAt(value_quote_end_index + 1) == ',') suffix = this->_data.substring(value_quote_end_index + 2, this->_data.length() + 1) + "}";
		this->_data = prefix + suffix;
		if (this->_data.charAt(this->_data.length() - 2) == ',') this->_data = this->_data.substring(0, this->_data.length() - 2) + "}";
	}

    return this->_data;
}

JSONary& JSONary::_set(String key, String value) {

	
    bool first_pair = false;
    if(this->_data.length() == 2) { first_pair = true; }

    // Delete the last '}'
    this->_data.remove(this->_data.length() - 1, 1);

	// Check if the key is already in the json
	if (this->_data.indexOf("\"" + key + "\"" + ":") == -1) {

		// Add the key and value to the string
		this->_data += String(first_pair ? "" : ",") + "\"" + String(key) + "\":" + String(value);
		this->_data += "}";

	}
	
	// Modify value of the key in the json
	else {

		int key_index = this->_data.indexOf("\"" + key + "\"" + ":");
		int value_quote_start_index = this->_data.indexOf("\"" + key + "\"" + ":") + String("\"" + key + "\"" + ":").length();
		int value_quote_end_index = value_quote_start_index + 0;
		
		int new_value_index = 0;
		do { value_quote_end_index++; } while (String(this->_data.charAt(value_quote_end_index)).length() > 0 && String(this->_data.charAt(value_quote_end_index)) != ",");

		String prefix = this->_data.substring(0, value_quote_start_index + 0);
		String suffix = this->_data.substring(value_quote_end_index, this->_data.length()) + "}";

		this->_data = prefix + value + suffix;
	}
	
    return *this;
}

// Convert String to char*
char* JSONary::s2c(String str){
    if(str.length()!=0){
        char *p = const_cast<char*>(str.c_str());
        return p;
    }
}

/*
    ! Checks if a string is a valid number
*/
bool JSONary::isnumber (String str) {
    bool isvalidnumber = true;
    for (byte i=0; i < str.length (); i++) {
        if (!isDigit (str.charAt(i))) isvalidnumber = false;
    }
    return isvalidnumber;
}

String JSONary::getstring(String key){

	String state = "key";
	String jsonstr = this->_data;
	jsonstr.replace("{", "");
	jsonstr.replace("}", "");

	String currkey = "";
	String currvalue = "";
	for (int i = 0; i < this->_data.length(); i++) {
		char c = this->_data.charAt(i);

		if (c == ':') i++;

		if (state == "key") {
			while (c != ':') {
				c = this->_data.charAt(i);
				currkey += String(c);
				currkey.replace(":", "");
				currkey.replace("{", "");
				currkey.replace("}", "");
				currkey.replace("\"", "");
				i++;
			}
			currvalue = "";
			state = "value";
		}
		
		if (state == "value") {
			while (c != ',' && i < this->_data.length()) {
				c = this->_data.charAt(i);
				currvalue += String(c);
				currvalue.replace(",", "");
				currvalue.replace("}", "");
				currvalue.replace("\"", ""); // Only required for string values in parsestring()
				i++;
			}
			
			state = "key";
		}
		
		if (currkey == key) return currvalue;
		currkey = "";
	} 
	return "-1";
}

bool JSONary::getboolean(String key){
	return this->getstring(key) == "true" || this->getstring(key) == "1" ? true : false;
}

float JSONary::getfloat(String key){
	return this->getstring(key).toFloat();
}

int JSONary::getint(String key){
	return this->getstring(key).toInt();
}

// Parse a JSON string and return the number of keys in the object
// TODO Move this to KeyValueary
String JSONary::toKeyValue(){

	String data = this->_data;

	data.replace("{", "");
	data.replace("}", "");
	data.replace(",", "\n");

	// Remove the quotes between keys
	String state = "key";
	String result = "";
	for (int i = 0; i < data.length(); i++) {
		char c = data.charAt(i);

		if (state == "key" && c == '"') {}
		else if (state == "key" && c != '"') {
			result += String(c);
		}
		else if (state == "value" && c != '"') {
			result += String(c);
		}
		else  {

			if (c == ':') {
				state = "value";
				result += String(c);
			}
			if (c == '\n') {
				state = "key";
				result += String(c);
			}
		}
	}

	return result;

}

#endif