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

    // MAC 주소 읽어오기
    std::string getMacAddress(const std::string& interface_name) {
        std::string mac = "UNKNOWN_MAC";
        std::string path = "/sys/class/net/" + interface_name + "/address";
        std::ifstream file(path);
        
        if (file.is_open()) {
            std::getline(file, mac);
            file.close();
            
            // 소문자를 대문자로 변환
            std::transform(mac.begin(), mac.end(), mac.begin(), 
                           [](unsigned char c){ return std::toupper(c); });
        }
        return mac;
    }

    // IP 주소 읽어오기
    std::string getIpAddress(const std::string& interface_name) {
        std::string ip = "127.0.0.1"; // 디폴트 IP
        struct ifaddrs *interfaces = nullptr;
        struct ifaddrs *temp_addr = nullptr;

        // 시스템의 모든 네트워크 인터페이스 가져오기
        if (getifaddrs(&interfaces) == 0) {
            temp_addr = interfaces;
            while (temp_addr != nullptr) {
                if (temp_addr->ifa_addr->sa_family == AF_INET) { // IPv4만 확인
                    // wlan0 or eth0 찾음
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

    // 현재 활성화된 네트워크 어댑터 찾기
    std::string getActiveInterfaceName() {
        if (getIpAddress("wlan0") != "127.0.0.1") return "wlan0";
        if (getIpAddress("eth0") != "127.0.0.1") return "eth0";
        return "lo";
    }
}