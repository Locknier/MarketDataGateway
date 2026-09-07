#pragma once

#include <string>
#include <thread>
#include <atomic>
#include "SubscriberManager.h"

// 引入官方的 CTP 行情接口与数据结构
#include "../third_party/ctp_api/include/ThostFtdcMdApi.h"

// ============================================================================
// [上游数据面接收机：CTP API 回调拦截器]
// 继承自 CThostFtdcMdSpi，实现纯异步 (Asynchronous) 的事件响应
// ============================================================================
class CtpMdReceiver : public CThostFtdcMdSpi {
public:
    CtpMdReceiver(SubscriberManager& mgr);
    virtual ~CtpMdReceiver();

    // 暴露给外层的启动方法
    bool StartListening(const std::string& front_addr, const std::string& broker,
                        const std::string& investor, const std::string& pwd);
    void Stop();

    // -------------------------------------------------------------
    // 以下为 CThostFtdcMdSpi 的回调函数 (由 CTP 背景线程池触发)
    // -------------------------------------------------------------

    // 1. 网络连接成功的回调
    virtual void OnFrontConnected() override;

    // 2. 登陆结果的回调
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast) override;

    // 3. 订阅股票/期货成功的回调
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                    CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) override;

    // 4. 【极速核心】实际深度行情闪动时的回调
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;

private:
    CThostFtdcMdApi* md_api_;         // 事件发包器 (Producer)
    SubscriberManager& sub_mgr_;      // 下游网关连接池

    std::string broker_id_;
    std::string investor_id_;
    std::string password_;
    std::string front_address_;

    std::atomic<bool> is_running_;
    int req_id_;
};