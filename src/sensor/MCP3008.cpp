#include "sensor/MCP3008.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstring>

MCP3008::MCP3008(const std::string& path, uint32_t speed_hz) 
    : spi_fd(-1), device_path(path), speed(speed_hz), mode(SPI_MODE_0), bits_per_word(8) {}

MCP3008::~MCP3008() {
    if (spi_fd >= 0) {
        close(spi_fd);
    }
}

bool MCP3008::init() {
    spi_fd = open(device_path.c_str(), O_RDWR);
    if (spi_fd < 0) {
        std::cerr << "[Sensor] SPI 버스 응답 실패: " << device_path << std::endl;
        return false;
    }

    // SPI 통신 모드 설정 Mode 0
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) return false;
    
    // 통신 비트수 8bit
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0) return false;
    
    // 클럭 설정
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) return false;

    std::cout << "[Sensor] SPI 버스 연결 완료" << std::endl;
    return true;
}

int MCP3008::readADC(int channel) {
    if (channel < 0 || channel > 7) {
        std::cerr << "[Sensor] 채널은 0~7 사이의 값입니다." << std::endl;
        return -1;
    }

    // 송수신 버퍼
    uint8_t tx_buf[3]; // 자료형으로 원소당 8bit 고정
    uint8_t rx_buf[3] = {0,};

    tx_buf[0] = 0x01;  // 동작시작 알림
    tx_buf[1] = (8 + channel) << 4; // 읽어올 채널 + 더미 클럭킹
    tx_buf[2] = 0x00;  // 더미 클럭킹

    // SPI 통신 구조체
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr)); // 구조체를 0으로 초기화
    
    tr.tx_buf = (unsigned long)tx_buf; // 보낼 데이터
    tr.rx_buf = (unsigned long)rx_buf; // 받을 데이터
    tr.len = 3;                        // 총 3바이트 교환
    tr.speed_hz = speed;
    tr.bits_per_word = bits_per_word;

    // 동시 송수신 SPI
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        std::cerr << "[Sensor] SPI 통신 에러" << std::endl;
        return -1;
    }

    // 아날로그 10bit 값 복원
    int result = ((rx_buf[1] & 3) << 8) | rx_buf[2];
    
    return result;
}