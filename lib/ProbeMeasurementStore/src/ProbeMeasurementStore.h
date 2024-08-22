#ifndef PROBE_MEASUREMENT_STORE_H
#define PROBE_MEASUREMENT_STORE_H

#include <Arduino.h>
#include <vector>
#ifndef CSVary_h
    #include "CSVary.h"
#endif

class ProbeMeasurement {
public:
    ProbeMeasurement(String DEVICESN,
    String TIMESTAMP,
    String DATE,
    String TIME,
    float RTD,
    float PH,
    float DO,
    float EC,
    float TEMP,
    float RH,
    String LAT,
    String LONG,
    float BVOLT);
    
    // Getter methods for all properties
    String getDEVICESN() const { return DEVICESN; }
    String getTIMESTAMP() const { return TIMESTAMP; }
    String getDATE() const { return DATE; }
    String getTIME() const { return TIME; }
    float getRTD() const { return RTD; }
    float getPH() const { return PH; }
    float getDO() const { return DO; }
    float getEC() const { return EC; }
    float getTEMP() const { return TEMP; }
    float getRH() const { return RH; }
    String getLAT() const { return LAT; }
    String getLONG() const { return LONG; }
    float getBVOLT() const { return BVOLT; }

    // Method to get CSV representation of the measurement
    CSVary toCSVary() const;

private:
    String DEVICESN;
    String TIMESTAMP;
    String DATE;
    String TIME;
    float RTD;
    float PH;
    float DO;
    float EC;
    float TEMP;
    float RH;
    String LAT;
    String LONG;
    float BVOLT;
};

class ProbeMeasurementStore {
public:
    ProbeMeasurementStore(size_t maxSize = 100);
    void addMeasurement(ProbeMeasurement measurement);
    bool isEmpty() const;
    size_t size() const;
    ProbeMeasurement getOldest();
    void removeOldest();
    void clear();

private:
    std::vector<ProbeMeasurement> measurements;
    size_t maxSize;
};

#endif // PROBE_MEASUREMENT_STORE_H