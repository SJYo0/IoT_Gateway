#ifndef ENS160_HPP
#define ENS160_HPP

#include <cstdint>

class ENS160 {
private:
    int i2c_fd;
    int address;

    bool writeReg(uint8_t reg, uint8_t value);
    bool writeReg16(uint8_t reg, uint16_t value); // 온도, 습도 쓰기 용도
    bool readRegs(uint8_t reg, uint8_t* data, int length);

public:
    ENS160(int addr = 0x53);
    ~ENS160();

    bool init();             // I2C 파일 오픈 및 센서 초기화
    void setEnvironmentData(double temp, double humidity); // 온도, 습도 쓰기
    uint16_t readTVOC();
    uint16_t readECO2();
};

#endif