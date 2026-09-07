# MarketDataGateway: Ultra-Low Latency Market Data Pub/Sub Gateway

MarketDataGateway 是一个专为量化金融与高频交易（HFT）场景开发的纳秒级证券行情中继与分发网关。系统严格落实控制面与数据面分离（Control/Data Plane Separation）的微服务架构，深度整合上期技术官方（SFIT）的 CTP C++ API 接口与 Epoll 底层网络，旨在实现实盘期指行情的极限零延迟扇出（Fan-out）。

MarketDataGateway is an ultra-low latency market data relay and distribution gateway developed for financial and High-Frequency Trading (HFT) environments. The system strictly implements a Control/Data Plane Separation microservices architecture, deeply integrating the official CTP C++ API (by SFIT) and the Epoll event-driven network to achieve extreme zero-latency fan-out of live futures market data.

## 核心亮点 / Core Highlights

- **控制面与数据面双轨分离 (Control/Data Plane Separation)**
  摒弃传统网关将协议解析与底层业务混杂的设计。利用 Google Protocol Buffers (`Protobuf`) 管理动态的 TCP 下游订阅与退订链路（控制面）；并使用自研网络引擎在内核级别透传行情（数据面）。解析开销被完全隔离，不占用核心行情的 CPU 时间片。
  *Discards traditional gateway designs that mix protocol parsing with core business logic. Uses Google Protocol Buffers (`Protobuf`) to manage dynamic TCP subscriptions/unsubscriptions (Control Plane), while employing a custom network engine for kernel-level passthrough (Data Plane). Parsing overhead is completely isolated.*

- **上期技术 CTP 原生实盘对口 (Native CTP API Integration)**
  抛弃各种开源二次封装包装器与 UDP 回放轮询。网关直接引入 CTP 官方原生的纯 C++ 动态链接库 (`libthostmduserapi_se.so`)。底层通过继承 `CThostFtdcMdSpi` 实现完全无锁的异步事件回调（Asynchronous Event Callback），与上海期货交易所测试机房直接保持实时双工长连接。
  *Abandons third-party wrappers and UDP polling. The gateway natively embeds the official CTP C++ shared library. It implements completely lock-free asynchronous event callbacks by inheriting `CThostFtdcMdSpi`, establishing real-time duplex connections directly with the Shanghai Futures Exchange test servers.*

- **热点路径零拷贝转换 (Hot-Path Zero-Copy Translation)**
  为杜绝 CTP 那庞大的 40 字段大包体造成的内存挤兑，当 `OnRtnDepthMarketData` 被高频触发时，系统利用 `#pragma pack(1)` 的定长精简结构体，结合内存首地址偏移指针 (`Pointer Casting`)，原位提取核心挂单字段。彻底杜绝堆内存调用 (`new/malloc`)。
  *To prevent memory eviction caused by CTP's massive 40-field structs, upon high-frequency triggers of `OnRtnDepthMarketData`, the system uses `#pragma pack(1)` compact structures and pointer casting to extract core order book items in-place. Heap memory allocations (`new/malloc`) are strictly forbidden.*

- **O(1) 内联时间戳极限解析 (O(1) Inline Timestamp Parsing)**
  针对大厂底层普遍存在的字符串表示时间戳（如 `14:30:05`）反模式设计。系统坚决废弃极度拖慢堆栈的 `sscanf()` 与 `stoi()`。自行实现基于纯 ASCII 计算的位元位移算法，微秒级直接算归出 64位无符号纪元时间，榨干 CPU 流水线性能。
  *Countering the anti-pattern of string-based timestamps (e.g., `14:30:05`) common in legacy exchange APIs. The system adamantly forbids stack-heavy `sscanf()` formatting. Instead, it implements a pure ASCII offset-based bitwise algorithm to directly calculate a standard 64-bit unsigned epoch time within microseconds, maximizing CPU pipelining efficiency.*

- **绕过应用层极致并发 (Bypass User-Space Fan-out)**
  面对一对万的高并发行情扇出（Pub/Sub）挑战，网关数据面彻底抛弃了 `Reactor` 模型中因应用层 Buffer 排队引发的 `EPOLLOUT` 唤醒延迟。在 CTP 回调线程内，直接针对所有连接发出 OS 级别系统调用 `send(..., MSG_DONTWAIT)`，跨级压缩 CPU 延时。
  *To conquer the 1-to-N ultra-high concurrency fan-out challenge, the Data Plane thoroughly abandons context-switch delays induced by typical User-Space Buffer queuing in the Reactor pattern. Within the CTP callback thread, it directly issues OS-level `send(..., MSG_DONTWAIT)` system calls for all connected sockets.*

## 架构概览 / Architecture Overview

1. 下游客户连线 TCP (Port 8080)，发送 Protobuf 管理令进行 `Subscribe` 订阅。
2. 控制面板 `SubscriberManager` 以 O(1) 的哈希表更新权限连接池（支持动态扩缩）。
3. `CtpMdReceiver` 于背景启动上期技术双线程机制，并自动登入 SimNow `7x24` 测试机房。
4. 一旦机房行情跳动触发 SPI 事件，指针直转、Ascii 内联解算后，零延迟呼叫 OS Kernel API 向全网下发。

## 编译运行 / Build & Run

```bash
# 包含与官方 CTP .so 的动态链接操作
mkdir build && cd build
cmake ..
make -j4

# 启动服务器（需自带 SimNow 六位数 InvestorID 与密码，无需手机号）
# Front Node 默认使用电信1测试联机
./GatewayApp <帐号> <密码> [可选: TCP前置机地址]
```
