#include <iostream>
#include <csignal>
#include "../include/SubscriberManager.h"
#include "../include/UpstreamReceiver.h"
#include "TcpServer.h"

TcpServer* g_server = nullptr;
UpstreamReceiver* g_receiver = nullptr;

void SignalHandler(int signum) {
    std::cout << "\n[System] 捕获中止信号 " << signum << "，触发系统优雅关闭...\n";
    if (g_receiver) g_receiver->Stop();
    if (g_server) g_server->stop();
}

int main() {

    std::cout << "--- HFT证券行情分发网关 (Control/Data分离架构) ---\n";
    
    // 初始化发布/订阅管理器
    SubscriberManager sub_mgr;
    
    // 启动基于 ZeroNet 的 TCP 下游分发服务器 (监听 8080)
    TcpServer server;
    g_server = &server;
    // 在 TcpServer 初始化后覆盖它的 SignalHandler
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // [控制面：断线清理机制]
    server.setConnectionCallback([&sub_mgr](TcpConnection* conn) {
        // ZeroNet 会在连线和断线时呼叫此函数。真实专案可在此读取连线状态进行踢除。
        // 为展现解耦，我们在外层逻辑不作过多修改。
    });

    // [控制面：处理 Protobuf 请求]
    server.setMessageCallback([&sub_mgr](TcpConnection* conn, const char* data, size_t len) -> size_t {
        market::SubscribeRequest req;
        // 尝试用 Protobuf 从字节流中反序列化
        if (req.ParseFromArray(data, len)) {
            // 如果解析成功，则是控制面指令！
            sub_mgr.HandleSubscribeRequest(conn, req);
            return len; // 告知底层已消化全部封包
        }
        return 0; // 封包不完整或非合法 Protobuf
    });

    // 初始化并启动内部上游网络组播监听接收器
    UpstreamReceiver upstream_receiver(sub_mgr);
    g_receiver = &upstream_receiver;
    
    // 假设交易所的核心上游发射端是 239.0.0.1:30000
    if (!upstream_receiver.StartListening("239.0.0.1", 30000)) {
        return -1;
    }

    std::cout << "[Control-Plane] 运行于 8080 端口，等待 Protobuf 下游订阅连线...\n";
    
    // 阻塞主线程运行外部网路网络引擎
    server.start();

    std::cout << "[System] 优雅关闭完成。\n";
    return 0;
}
