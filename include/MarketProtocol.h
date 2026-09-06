#pragma once
#include <cstdint>

// ============================================================================
// [数据面协议：零拷贝二进制行情结构]
// 严禁存在任何虚函数、std::string 等堆内存字段。
// 使用 1 字节紧凑对齐，允许网络缓冲区进来后进行直接指针强转(Casting)。
// ============================================================================

#pragma pack(push, 1)
struct DepthMarketData {
    char ticker[8];       // 股票代号，例如 "AAPL\0"
    uint64_t timestamp;   // 交易所时间戳
    double last_price;    // 最新成交价
    double bid_price[5];  // 五档买价
    int bid_volume[5];    // 五档买量
    double ask_price[5];  // 五档卖价
    int ask_volume[5];    // 五档卖量
};
#pragma pack(pop)
