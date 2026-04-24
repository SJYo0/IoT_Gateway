#include "BME280.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

BME280::BME280(int addr) : i2c_fd(-1), address(addr), t_fine(0) {}

BME280::~BME280() {
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
}

bool BME280::writeReg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return write(i2c_fd, buf, 2) == 2;
}

bool BME280::readRegs(uint8_t reg, uint8_t* data, int length) {
    if (write(i2c_fd, &reg, 1) != 1) return false;
    return read(i2c_fd, data, length) == length;
}

bool BME280::readCalibrationData() {
    uint8_t data[24];
    
    // 온도 & 기압 보정 데이터
    if (!readRegs(0x88, data, 24)) return false;
    calib.dig_T1 = (data[1] << 8) | data[0]; // 8bit 짜리를 16bit 로 결합
    calib.dig_T2 = (data[3] << 8) | data[2];
    calib.dig_T3 = (data[5] << 8) | data[4];
    calib.dig_P1 = (data[7] << 8) | data[6];
    calib.dig_P2 = (data[9] << 8) | data[8];
    calib.dig_P3 = (data[11] << 8) | data[10];
    calib.dig_P4 = (data[13] << 8) | data[12];
    calib.dig_P5 = (data[15] << 8) | data[14];
    calib.dig_P6 = (data[17] << 8) | data[16];
    calib.dig_P7 = (data[19] << 8) | data[18];
    calib.dig_P8 = (data[21] << 8) | data[20];
    calib.dig_P9 = (data[23] << 8) | data[22];

    // 습도 보정 데이터
    uint8_t h1;
    readRegs(0xA1, &h1, 1);
    calib.dig_H1 = h1;

    uint8_t h_data[7];
    readRegs(0xE1, h_data, 7);
    calib.dig_H2 = (h_data[1] << 8) | h_data[0];
    calib.dig_H3 = h_data[2];

    calib.dig_H4 = (h_data[3] << 4) | (h_data[4] & 0x0F);
    calib.dig_H5 = (h_data[5] << 4) | (h_data[4] >> 4);
    calib.dig_H6 = (int8_t)h_data[6];

    return true;
}

bool BME280::init() {
const char* bus = "/dev/i2c-1";
    i2c_fd = open(bus, O_RDWR);
    if (i2c_fd < 0) {
        std::cerr << "[Sensor] I2C 버스 오픈 실패" << std::endl;
        return false;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, address) < 0) {
        std::cerr << "[Sensor] 센서 연결 실패" << std::endl;
        return false;
    }

    uint8_t id = 0;
    readRegs(0xD0, &id, 1);
    if (id != 0x60) {
        std::cerr << "[Sensor] BME280 센서가 아닙니다" << std::endl;
        return false;
    }

    // 보정 데이터 읽기
    if (!readCalibrationData()) return false;

    writeReg(0xF2, 0x01); // 습도 오버샘플링 1 설정
    writeReg(0xF4, 0x27); // 온도/기압 오버샘플링 1 설정, Normal mode 설정

    return true;
}

double BME280::readTemperature() {
    uint8_t data[3];
    
    // 온도 원시 데이터
    if (!readRegs(0xFA, data, 3)) {
        std::cerr << "[Sensor] 온도 데이터 읽기 실패" << std::endl;
        return -999.0;
    }

    // 온도 원시 데이터 조합
    int32_t adc_T = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);

    // 온도 오차 보정 상수 활용 값 계산 (데이터시트 참고)
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;

    // t_fine은 나중에 쓰임
    t_fine = var1 + var2;

    double temperature = (t_fine * 5 + 128) >> 8;
    return temperature / 100.0; // 소수점 변환
}

double BME280::readPressure() {
    uint8_t data[3];
    if (!readRegs(0xF7, data, 3)) return -999.0;

    int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);

    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;

    if (var1 == 0) return 0;

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);

    return (double)p / 25600.0; // hPa 변환
}

double BME280::readHumidity() {
    uint8_t data[2];
    if (!readRegs(0xFD, data, 2)) return -999.0;

    int32_t adc_H = (data[0] << 8) | data[1];

    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - (((int32_t)calib.dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) *
                   (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
                   ((int32_t)calib.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

    return (double)(v_x1_u32r >> 12) / 1024.0; // % 변환
}