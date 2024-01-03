/*
Eeprom_at24c256.h - Library for AT24C256 IIC EEPROM.
*/
#ifndef Eeprom_at24c256_h
#define Eeprom_at24c256_h

//! AG: Commented out following files for MKRNB
// #include "application.h"

class Eeprom_at24c256
{
public:
	Eeprom_at24c256(int address);
	void write(unsigned  int writeAddress, char* data, int len);
	void read(unsigned  int readAdress, char *buffer, int len);
	String EMPTY = "";
private:
	int _address;
};

#endif