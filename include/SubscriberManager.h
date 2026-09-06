#pragma once
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <string>
#include "../src/subscription.pb.h"
#include "TcpConnection.h"
#include "MarketProtocol.h"

class SubscriberManager {
public:
    SubscriberManager() = default;
    ~SubscriberManager() = default;

    void HandleSubscribeRequest(TcpConnection *conn, const market::SubscribeRequest &req);
    void RemoveConnection(TcpConnection *conn);
    void Broadcast(const DepthMarketData *data);
private:
    std::unordered_map<std::string, std::unordered_set<TcpConnection*>> ticker_subs_;
    std::mutex sub_mtx_;
};