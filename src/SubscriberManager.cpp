#include "../include/SubscriberManager.h"
#include <iostream>
#include <sys/socket.h>

void SubscriberManager::HandleSubscribeRequest(TcpConnection *conn, const market::SubscribeRequest &req){
    std::lock_guard<std::mutex> lock(sub_mtx_);

    for (const auto& ticker : req.tickers()) {
        if (req.op_type() == market::SubscribeRequest::SUBSCRIBE) {
            ticker_subs_[ticker].insert(conn);
            std::cout << "[Control-Plane] FD " << conn->getFd() << " subscribed to: " << ticker << "\n";
        } else {
            ticker_subs_[ticker].erase(conn);
            std::cout << "[Control-Plane] FD " << conn->getFd() << " unsubscribed from: " << ticker << "\n";
        }
    }
}

void SubscriberManager::RemoveConnection(TcpConnection* conn) {
    std::lock_guard<std::mutex> lock(sub_mtx_);

    for (auto& pair : ticker_subs_) {
        pair.second.erase(conn);
    }
    std::cout << "[Control-Plane] FD " << conn->getFd() << " disconnected. State cleaned.\n";
}

void SubscriberManager::Broadcast(const DepthMarketData *data) {
    std::lock_guard<std::mutex> lock(sub_mtx_);
    std::string ticker(data->ticker);

    // [调试信息] 如果收到 UDP 数据，就在终端打印出来，确保 UDP 底层真的通了！
    std::cout << "[Data-Plane DEBUG] UDP 层成功收到股票 " << ticker << " 行情，现价 " << data->last_price << "，准备群发...\n";

    if (ticker_subs_.count(ticker) > 0) {
        for (TcpConnection *conn : ticker_subs_[ticker]) {
            send(conn->getFd(), data, sizeof(DepthMarketData), MSG_DONTWAIT);
        }
    }
}
