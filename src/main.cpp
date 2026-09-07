#include <iostream>
#include <csignal>
#include <stdlib.h>
#include "../include/SubscriberManager.h"
#include "../include/CtpMdReceiver.h"
#include "TcpServer.h"

// 极简 Signal 处理：直接暴力退出，不挣扎。OS 会回收所有 Socket 和内存。
// CTP 的后台线程在被 signal 打断时如果去 join(Release)，100% 会引发 terminate 死锁。
void SignalHandler(int signum) {
    std::cout << "\n[System] 捕获中止信号 " << signum << "，直接安全退出以防线程死锁。\n";
    _exit(0); // _exit 不会调用 C++ 全局对象的析构函数，是彻底防死锁的最安全方式！
}

int main(int argc, char* argv[]) {
    std::cout << "--- HFT证券行情分发网关 (CTP 真实行情内核版) ---\n";

    SubscriberManager sub_mgr;
    TcpServer server;

    server.setMessageCallback([&sub_mgr](TcpConnection* conn, const char* data, size_t len) -> size_t {
        market::SubscribeRequest req;
        if (req.ParseFromArray(data, len)) {
            sub_mgr.HandleSubscribeRequest(conn, req);
            return len;
        }
        return 0;
    });

    CtpMdReceiver ctp_receiver(sub_mgr);
    
    // 改用 SimNow “第一套标准测试环境 (非7x24)” 的最新行情前置 IP
    // 因为 7x24 环境由于资源有限经常踢人 (Error 4097)
    std::string front_addr = "tcp://180.168.146.187:10110"; 
    std::string broker_id  = "9999"; 
    std::string investor_id = "在这里填你的六位数帐号"; 
    std::string password    = "在这里填你的密码";

    if (argc >= 3) {
        investor_id = argv[1];
        password = argv[2];
        if (argc >= 4) {
            front_addr = argv[3];
        }
    } else {
        std::cout << "\n⚠️ 提示: 可以使用: ./GatewayApp <帐号> <密码> [前置机IP地址]\n";
    }

    if (!ctp_receiver.StartListening(front_addr, broker_id, investor_id, password)) {
        return -1;
    }

    std::cout << "[Control-Plane] 运行于 8080 端口，等待 Protobuf 下游散户连线...\n";
    
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    server.start();

    return 0;
}
