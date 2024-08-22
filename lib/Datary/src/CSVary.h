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

#endif