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
        std::cerr << "❌ SPI 디바이스 오픈 실패: " << device_path << std::endl;
        return false;
    }

    // 1. SPI 통신 모드 설정 (Mode 0: CPOL=0, CPHA=0)
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) return false;
    
    // 2. 한 번에 전송할 비트 수 설정 (8비트 = 1바이트)
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0) return false;
    
    // 3. 통신 속도 설정 (기본 1MHz)
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) return false;

    std::cout << "[MCP3008] ADC 초기화 및 SPI 버스 연결 완료" << std::endl;
    return true;
}

int MCP3008::readADC(int channel) {
    if (channel < 0 || channel > 7) {
        std::cerr << "❌ MCP3008 채널 오류 (0~7 사이여야 함)" << std::endl;
        return -1;
    }

    // 통신을 위한 송수신 버퍼 (3바이트)
    uint8_t tx_buf[3];
    uint8_t rx_buf[3] = {0,};

    // 🚨 핵심: MCP3008 데이터시트에 명시된 3바이트 송신 프로토콜
    tx_buf[0] = 0x01;  // 1바이트: Start Bit (나 통신 시작할게!)
    // 2바이트: Single-Ended 모드(1) + 채널 번호(3비트) + 나머지 패딩
    tx_buf[1] = (8 + channel) << 4; 
    tx_buf[2] = 0x00;  // 3바이트: 센서가 데이터를 보낼 수 있도록 빈 공간(클럭) 제공용

    // 리눅스 커널에게 SPI 통신을 요청하기 위한 구조체
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr)); // 구조체를 0으로 초기화 (매우 중요, 쓰레기값 방지)
    
    tr.tx_buf = (unsigned long)tx_buf; // 보낼 데이터
    tr.rx_buf = (unsigned long)rx_buf; // 받을 데이터
    tr.len = 3;                        // 총 3바이트 교환
    tr.speed_hz = speed;
    tr.bits_per_word = bits_per_word;

    // SPI_IOC_MESSAGE: 송신과 수신을 '동시에' 처리하는 시스템 콜
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        std::cerr << "❌ SPI 통신 에러" << std::endl;
        return -1;
    }

    // 🚨 수신된 3바이트 비트 조립 (10비트 분해능 복원)
    // rx_buf[1]의 하위 2비트와 rx_buf[2]의 전체 8비트를 합쳐서 10비트 결과값 생성
    int result = ((rx_buf[1] & 3) << 8) | rx_buf[2];
    
    return result; // 0 ~ 1023 사이의 아날로그 값 반환
}