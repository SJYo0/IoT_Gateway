#ifndef SENSORDATA_HPP
#define SENSORDATA_HPP

#include <cstdint>

struct SensorDTO {
    double temperature;
    double humidity;
    double pressure;
    uint16_t tvoc;
    uint16_t eco2;
    int flameValue;
    long long timestamp;
};

#endif