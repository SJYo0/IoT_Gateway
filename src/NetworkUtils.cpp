#include "NetworkUtils.hpp"
#include <fstream>
#include <iostream>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace NetworkUtils {

    // 1. MAC 주소 읽어오기 (리눅스 파일 시스템 활용)
    std::string getMacAddress(const std::string& interface_name) {
        std::string mac = "UNKNOWN_MAC";
        std::string path = "/sys/class/net/" + interface_name + "/address";
        std::ifstream file(path);
        
        if (file.is_open()) {
            std::getline(file, mac);
            file.close();
            
            // 소문자를 대문자로 변환 (보기 좋게 AA:BB:CC 형식으로)
            std::transform(mac.begin(), mac.end(), mac.begin(), 
                           [](unsigned char c){ return std::toupper(c); });
        }
        return mac;
    }

    // 2. IP 주소 읽어오기 (getifaddrs 시스템 콜 활용)
    std::string getIpAddress(const std::string& interface_name) {
        std::string ip = "127.0.0.1";
        struct ifaddrs *interfaces = nullptr;
        struct ifaddrs *temp_addr = nullptr;

        // 시스템의 모든 네트워크 인터페이스 가져오기
        if (getifaddrs(&interfaces) == 0) {
            temp_addr = interfaces;
            while (temp_addr != nullptr) {
                if (temp_addr->ifa_addr->sa_family == AF_INET) { // IPv4만 확인
                    // 이름이 일치하는 인터페이스(예: wlan0)를 찾음
                    if (std::strcmp(temp_addr->ifa_name, interface_name.c_str()) == 0) {
                        ip = inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
                        break;
                    }
                }
                temp_addr = temp_addr->ifa_next;
            }
            freeifaddrs(interfaces); // 메모리 해제
        }
        return ip;
    }

    // 3. 현재 활성화된 네트워크 어댑터 찾기 (Wi-Fi 우선, 그 다음 유선 LAN)
    std::string getActiveInterfaceName() {
        // 라즈베리파이에서 Wi-Fi는 wlan0, 유선랜은 eth0입니다.
        if (getIpAddress("wlan0") != "127.0.0.1") return "wlan0";
        if (getIpAddress("eth0") != "127.0.0.1") return "eth0";
        return "lo"; // 아무것도 연결 안 된 로컬 루프백
    }
}