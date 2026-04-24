#include "ENS160.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <iostream>

ENS160::ENS160(int addr) : i2c_fd(-1), address(addr) {}
ENS160::~ENS160() { if (i2c_fd >= 0) close(i2c_fd); }

bool ENS160::writeReg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return write(i2c_fd, buf, 2) == 2;
}

bool ENS160::readRegs(uint8_t reg, uint8_t* data, int length) {
    if (write(i2c_fd, &reg, 1) != 1) return false;
    return read(i2c_fd, data, length) == length;
}

bool ENS160::init() {
    const char* bus = "/dev/i2c-1";
    i2c_fd = open(bus, O_RDWR);
    if (i2c_fd < 0 || ioctl(i2c_fd, I2C_SLAVE, address) < 0) return false;

    // 센서 동작 모드 설정 STANDARD Gas Sensing Mode
    if (!writeReg(0x10, 0x02)) return false;
    
    std::cout << "[Sensor] ENS160 세팅 완료" << std::endl;
    return true;
}

uint16_t ENS160::readTVOC() {
    uint8_t data[2];
    if (!readRegs(0x22, data, 2)) return 0;
    return (data[1] << 8) | data[0];
}

uint16_t ENS160::readECO2() {
    uint8_t data[2];
    if (!readRegs(0x24, data, 2)) return 0;
    return (data[1] << 8) | data[0];
}

bool ENS160::writeReg16(uint8_t reg, uint16_t value) {
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = value & 0xFF;
    buf[2] = (value >> 8) & 0xFF;
    return write(i2c_fd, buf, 3) == 3;
}

// 온도 & 습도값 삽입
void ENS160::setEnvironmentData(double temp, double humidity) {
    // 온도값 Example: For 25°C the input value is calculated as follows: (25 + 273.15) * 64 = 0x4A8A
    uint16_t t_data = static_cast<uint16_t>((temp + 273.15) * 64.0);
    writeReg16(0x13, t_data);

    // 습도값 Example: For 50% rH the input value is calculated as follows: 50 * 512 = 0x6400.
    uint16_t h_data = static_cast<uint16_t>(humidity * 512.0);
    writeReg16(0x15, h_data);
}