#include "ProbeMeasurementStore.h"

ProbeMeasurement::ProbeMeasurement(String DEVICESN,
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
    float BVOLT)
    : DEVICESN(DEVICESN), TIMESTAMP(TIMESTAMP), DATE(DATE), TIME(TIME),
      RTD(RTD), PH(PH), DO(DO), EC(EC), TEMP(TEMP), RH(RH),
      LAT(LAT), LONG(LONG), BVOLT(BVOLT) {}

CSVary ProbeMeasurement::toCSVary() const {
    CSVary csv;
    csv.clear()
       .setheader("DEVICESN,TIMESTAMP,DATE,TIME,RTD,PH,DO,EC,TEMP,RH,LAT,LNG,BVOLT")
       .set(DEVICESN)
       .set(TIMESTAMP)
       .set(DATE)
       .set(TIME)
       .set(RTD)
       .set(PH)
       .set(DO)
       .set(EC)
       .set(TEMP)
       .set(RH)
       .set(LAT)
       .set(LONG)
       .set(BVOLT);
    return csv;
}

ProbeMeasurementStore::ProbeMeasurementStore(size_t maxSize)
    : maxSize(maxSize) {}

void ProbeMeasurementStore::addMeasurement(ProbeMeasurement measurement) {
    if (measurements.size() >= maxSize) {
        measurements.erase(measurements.begin());
    }
    measurements.push_back(measurement);
}

bool ProbeMeasurementStore::isEmpty() const {
    return measurements.empty();
}

size_t ProbeMeasurementStore::size() const {
    return measurements.size();
}

ProbeMeasurement ProbeMeasurementStore::getOldest() {
    if (isEmpty()) {
        // Return a default ProbeMeasurement object if the store is empty
        return ProbeMeasurement("", "", "", "", 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, "", "", 0.0f);
    }
    return measurements.front();
}

void ProbeMeasurementStore::removeOldest() {
    if (!isEmpty()) {
        measurements.erase(measurements.begin());
    }
}

void ProbeMeasurementStore::clear() {
    measurements.clear();
}