#pragma once
#include <string>
#include <thread>
#include <atomic>
#include "SubscriberManager.h"

// ============================================================================
// [上游数据面接收机：UDP Multicast Receiver]
// ============================================================================
class UpstreamReceiver {
private:
    int udp_fd_;
    SubscriberManager& sub_mgr_;
    std::atomic<bool> is_running_;
    std::thread receive_thread_;

    void ReceiveLoop();

public:
    UpstreamReceiver(SubscriberManager& mgr);
    ~UpstreamReceiver();

    // 初始化并启动组播监听 (例如监听纳斯达克/内网核心组播 239.0.0.1:30000)
    bool StartListening(const std::string& multicast_ip, int port);
    
    void Stop();
};
