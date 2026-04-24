#ifndef BME280_HPP
#define BME280_HPP

#include <cstdint>

class BME280 {
private:
    int i2c_fd;
    int address;     // 0x76
    int32_t t_fine;  // 온도 보정 변수 - 기압, 습도에 사용됨

    // 오차 보정 상수
    struct {
        uint16_t dig_T1; int16_t dig_T2; int16_t dig_T3;
        uint16_t dig_P1; int16_t dig_P2; int16_t dig_P3; int16_t dig_P4;
        int16_t  dig_P5; int16_t dig_P6; int16_t dig_P7; int16_t dig_P8; int16_t dig_P9;
        uint8_t  dig_H1; int16_t dig_H2; uint8_t dig_H3; int16_t dig_H4;
        int16_t  dig_H5; int8_t  dig_H6;
    } calib;

    bool writeReg(uint8_t reg, uint8_t value);
    bool readRegs(uint8_t reg, uint8_t* data, int length);
    bool readCalibrationData(); // 오차보정 상수 읽어오기

public:
    BME280(int addr = 0x76);
    ~BME280();

    bool init();              // I2C 파일 오픈 및 센서 초기화
    double readTemperature();
    double readPressure();
    double readHumidity();
};

#endif