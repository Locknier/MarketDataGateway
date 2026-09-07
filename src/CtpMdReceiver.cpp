#include "../include/CtpMdReceiver.h"
#include <iostream>
#include <cstring>
#include <time.h>
#include <sys/time.h>

CtpMdReceiver::CtpMdReceiver(SubscriberManager& mgr) 
    : md_api_(nullptr), sub_mgr_(mgr), is_running_(false), req_id_(0) {
}

CtpMdReceiver::~CtpMdReceiver() {
    Stop();
}

bool CtpMdReceiver::StartListening(const std::string& front_addr, const std::string& broker, 
                                   const std::string& investor, const std::string& pwd) {
    front_address_ = front_addr;
    broker_id_ = broker;
    investor_id_ = investor;
    password_ = pwd;

    md_api_ = CThostFtdcMdApi::CreateFtdcMdApi("./", false, false);
    if (!md_api_) {
        std::cerr << "[Data-Plane] CTP Api 创建失败!\n";
        return false;
    }

    md_api_->RegisterSpi(this);
    md_api_->RegisterFront((char*)front_address_.c_str());

    is_running_ = true;
    md_api_->Init();
    
    std::cout << "[Data-Plane] CTP 引擎已启动，尝试连接交易所机房: " << front_addr << "\n";
    return true;
}

void CtpMdReceiver::Stop() {
    if (is_running_ && md_api_) {
        md_api_->RegisterSpi(nullptr);
        md_api_->Release(); 
        md_api_ = nullptr;
        is_running_ = false;
        std::cout << "[Data-Plane] CTP 引擎已安全释放。\n";
    }
}

void CtpMdReceiver::OnFrontConnected() {
    std::cout << "[Data-Plane] 与交易所机房建立 TCP 连接成功！准备发送登录请求...\n";

    CThostFtdcReqUserLoginField loginReq;
    memset(&loginReq, 0, sizeof(loginReq));
    
    strncpy(loginReq.BrokerID, broker_id_.c_str(), sizeof(loginReq.BrokerID) - 1);
    strncpy(loginReq.UserID, investor_id_.c_str(), sizeof(loginReq.UserID) - 1);
    strncpy(loginReq.Password, password_.c_str(), sizeof(loginReq.Password) - 1);

    md_api_->ReqUserLogin(&loginReq, ++req_id_);
}

void CtpMdReceiver::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, 
                                   CThostFtdcRspInfoField *pRspInfo, 
                                   int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID == 0) {
        std::cout << "[Data-Plane] CTP 登录成功! 交易日: " << md_api_->GetTradingDay() << "\n";
        
        char* instruments[] = { (char*)"ag2412", (char*)"au2412" }; 
        int count = 2;
        
        md_api_->SubscribeMarketData(instruments, count);
    } else {
        std::cerr << "[Data-Plane] CTP 登录失败，错误代码: " 
                  << (pRspInfo ? pRspInfo->ErrorID : -1) << "\n";
    }
}

void CtpMdReceiver::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
                                       CThostFtdcRspInfoField *pRspInfo, 
                                       int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID == 0 && pSpecificInstrument) {
        std::cout << "[Data-Plane] 成功订阅合约: " << pSpecificInstrument->InstrumentID << "\n";
    }
}

// 【HFT 级内联时间解析函数】：绝不使用 atoi 或 sscanf
inline uint64_t ParseCTPTimeFast(const char* updateTime, int updateMillisec) {
    if (!updateTime || updateTime[0] == '\0') return 0;
    
    // CTP 的 UpdateTime 格式为 "HH:MM:SS" (固定 8 字节)
    // 利用 ASCII 码原理进行极速 O(1) 解析，绕过一切函数调用
    uint32_t hours = (updateTime[0] - '0') * 10 + (updateTime[1] - '0');
    uint32_t mins  = (updateTime[3] - '0') * 10 + (updateTime[4] - '0');
    uint32_t secs  = (updateTime[6] - '0') * 10 + (updateTime[7] - '0');
    
    // 将当天经过的总毫秒数映射成唯一 timestamp 供下游策略对齐使用
    return (hours * 3600000ULL) + (mins * 60000ULL) + (secs * 1000ULL) + updateMillisec;
}

void CtpMdReceiver::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
    if (!pDepthMarketData) return;

    DepthMarketData ourData;
    
    strncpy(ourData.ticker, pDepthMarketData->InstrumentID, sizeof(ourData.ticker) - 1);
    ourData.ticker[sizeof(ourData.ticker) - 1] = '\0';
    
    // 翻译最后价格
    ourData.last_price = pDepthMarketData->LastPrice;
    
    // 采用自己编写的极限内联解析器，将 CTP 的两段式时间合成为 64位无符号整型纪元
    ourData.timestamp = ParseCTPTimeFast(pDepthMarketData->UpdateTime, pDepthMarketData->UpdateMillisec);

    // 翻译挂单薄 (Order Book)
    ourData.bid_price[0] = pDepthMarketData->BidPrice1;
    ourData.bid_volume[0] = pDepthMarketData->BidVolume1;
    ourData.ask_price[0] = pDepthMarketData->AskPrice1;
    ourData.ask_volume[0] = pDepthMarketData->AskVolume1;

    sub_mgr_.Broadcast(&ourData);
}
