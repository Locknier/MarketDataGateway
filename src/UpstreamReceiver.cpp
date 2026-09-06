#include "../include/UpstreamReceiver.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

UpstreamReceiver::UpstreamReceiver(SubscriberManager& mgr) 
    : udp_fd_(-1), sub_mgr_(mgr), is_running_(false) {
}

UpstreamReceiver::~UpstreamReceiver() {
    Stop();
}

bool UpstreamReceiver::StartListening(const std::string& multicast_ip, int port) {
    udp_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd_ < 0) {
        std::cerr << "[Data-Plane] UDP Socket 创建失败\n";
        return false;
    }

    int reuse = 1;
    setsockopt(udp_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(port);

    if (bind(udp_fd_, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        std::cerr << "[Data-Plane] UDP 端口绑定失败\n";
        close(udp_fd_);
        return false;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(udp_fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        std::cerr << "[Data-Plane] 加入多播组失败 (IP: " << multicast_ip << ")\n";
        close(udp_fd_);
        return false;
    }

    // 设置 100 毫秒超时，让 recv 不会永久阻塞，从而能优雅退出
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; 
    setsockopt(udp_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::cout << "[Data-Plane] 成功监听上游行情组播 IP: " << multicast_ip << " Port: " << port << "\n";
    is_running_ = true;
    
    receive_thread_ = std::thread(&UpstreamReceiver::ReceiveLoop, this);
    return true;
}

void UpstreamReceiver::Stop() {
    if (is_running_) {
        is_running_ = false;
        if (udp_fd_ >= 0) {
            close(udp_fd_); 
            udp_fd_ = -1;
        }
        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }
    }
}

void UpstreamReceiver::ReceiveLoop() {
    char buffer[1024]; 
    
    while (is_running_) {
        ssize_t bytes_read = recv(udp_fd_, buffer, sizeof(buffer), 0);
        
        if (bytes_read < 0) {
            // UDP 设置了超时，如果没有数据会返回 -1 且 errno 为 EAGAIN 或 EWOULDBLOCK
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // 只是这 100ms 没收到包，继续重新 recv
            }
            break; // 真正的 Socket 异常
        }
        
        if (bytes_read == 0) {
            break; // Socket 关闭
        }
        
        if (bytes_read == sizeof(DepthMarketData)) {
            const DepthMarketData* marketdata = reinterpret_cast<const DepthMarketData*>(buffer);
            sub_mgr_.Broadcast(marketdata);
        }
    }
    std::cout << "[Data-Plane] 行情组播监听线程已退出。\n";
}
