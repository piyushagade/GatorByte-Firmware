/*
	CSVary.h - Library for manipulating CSV strings
*/
#ifndef CSVary_h
#define CSVary_h

#include "Datary.h"

class CSVary
{
	public:
		CSVary();
		CSVary& clear();
		CSVary& setheader(String);
		String get();
		String getrows();
		String getheader();
		CSVary& set(int);
		CSVary& set(float);
		CSVary& set(double);
		CSVary& set(String);
		CSVary& crlf();
	private:
		String _header;
		String _data;
		String _line;
        char* s2c(String str);
};

CSVary::CSVary() {	
	this->_data = "";
	this->_line = "";
	this->_header = "";
}

String CSVary::get() {
	return this->getheader() + "\n" + this->getrows();
}

String CSVary::getrows() {
	this->_data = this->_data + this->_line;
	String response = this->_data;
	this->_line = "";
	return response;
}

String CSVary::getheader() {
	return this->_header;
}

CSVary& CSVary::clear() {
	this->_data = "";
	this->_line = "";
	this->_header = "";
    return *this;
}

CSVary& CSVary::setheader(String header) {
	if(this->_header.length() == 0) {
		this->_header = header;
	}
    return *this;
}

CSVary& CSVary::set(int value) {
    return set(String(value));
}

CSVary& CSVary::set(float value) {
    return set(String(value));
}

CSVary& CSVary::set(double value) {
    return set(String(value));
}

CSVary& CSVary::set(String value) {
    this->_line = this->_line + (this->_line.length() == 0 ?  "" : ",") + value;
    return *this;
}

CSVary& CSVary::crlf() {
	this->_data = this->_data + this->_line;
	this->_line = "";
    return *this;
}

// Convert String to char*
char* CSVary::s2c(String str){
    if(str.length()!=0){
        char *p = const_cast<char*>(str.c_str());
        return p;
    }
}

#endif