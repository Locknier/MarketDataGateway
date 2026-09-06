# MarketDataGateway: Ultra-Low Latency Market Data Pub/Sub Gateway

MarketDataGateway 是一个专为量化金融与高频交易（HFT）场景开发的纳秒级证券行情中继与分发网关。系统严格落实控制面与数据面分离（Control/Data Plane Separation）的微服务架构，深度整合 Epoll 底层网络并直接操作系统 Socket 层，旨在实现行情接收与扇出（Fan-out）的极限零延迟。

MarketDataGateway is an ultra-low latency market data relay and distribution gateway developed for financial and High-Frequency Trading (HFT) environments. The system strictly implements a Control/Data Plane Separation microservices architecture, deeply integrates with the Epoll event-driven network, and manipulates the OS Socket layer directly to achieve extreme zero-latency in market data reception and fan-out.

## 核心亮点 / Core Highlights

- **控制面与数据面双轨分离 (Control/Data Plane Separation)**
  摒弃传统网关将协议解析与底层业务混杂的设计。利用 Google Protocol Buffers (`Protobuf`) 管理动态的 TCP 下游订阅与退订链路（控制面）；并使用自研网络引擎在内核级别拦截与透传（数据面）。解析开销被完全隔离，不再占用核心行情的 CPU 时间片。
  *Discards traditional gateway designs that mix protocol parsing with core business logic. Uses Google Protocol Buffers (`Protobuf`) to manage dynamic TCP subscriptions/unsubscriptions (Control Plane), while employing a custom network engine for kernel-level interception and passthrough (Data Plane). Parsing overhead is completely isolated.*

- **多播极速内核拦截 (UDP Multicast Interception)**
  真实模拟纳斯达克等一线交易所的以太网核心组播机制。接收端调用 `IP_ADD_MEMBERSHIP` 的 IGMP 协议桥接内网网段。并结合阻塞式监听配合 `SO_RCVTIMEO`，使得工作线程既具备极速响应，又规避了挂起死锁陷阱。
  *Realistically simulates ethernet core multicast mechanisms of top-tier exchanges like NASDAQ. The receiver invokes `IP_ADD_MEMBERSHIP` via IGMP protocol to bridge the internal subnet, combining blocking listeners with `SO_RCVTIMEO` to ensure instantaneous response while evading hung deadlock traps.*

- **零内存分配流式解析 (Zero Heap Allocation)**
  废弃所有 JSON 以及高开销的序列化操作。利用 C++ 的 `#pragma pack(1)` 字节对齐，强制防堵内存空洞。当 UDP 数据报文到来时，直接利用栈内存缓冲区配合指针强制转型（`Pointer Casting`）解包，全程杜绝堆内存调用 (`new/malloc`)。
  *Abandons all JSON and high-overhead serialization. Utilizes C++ `#pragma pack(1)` byte alignment to strictly prevent memory padding. Upon UDP datagram arrival, it decodes utilizing stack memory buffers combined with pointer casting, completely eliminating heap memory allocations (`new/malloc`).*

- **绕过应用层极致并发 (Bypass User-Space Fan-out)**
  面对一对万的高并发行情扇出（Pub/Sub）挑战，网关数据面彻底抛弃了 `Reactor` 模型中传统由于应用层 Buffer 排队引发的 `EPOLLOUT` 唤醒延迟 (Context Switch)。异步线程直接针对所有连接发出 OS 级别系统调用 `send(..., MSG_DONTWAIT)`，跨级将 CPU 延时压缩至物理极限。
  *To conquer the 1-to-N ultra-high concurrency fan-out (Pub/Sub) challenge, the Data Plane thoroughly abandons the context-switch delays induced by traditional User-Space Buffer queuing and `EPOLLOUT` wake-ups in the Reactor pattern. Asynchronous threads directly execute OS-level `send(..., MSG_DONTWAIT)` system calls against all socket FDs, compressing CPU latency to theoretical physical limits.*

## 架构概览 / Architecture Overview

1. 下游客户连线 TCP (Port 8080)，发送 Protobuf 管理令进行 `Subscribe` 订阅。
2. 控制面板 `SubscriberManager` 以 O(1) 的哈希表 (Hashmap) 更新权限连接池。
3. `UpstreamReceiver` 在主线程堵塞监听 `239.0.0.1` 的 Multicast UDP 内网行情轰炸。
4. 一旦内核收到行情数据，指针直转，零拷贝调用系统 Kernel TCP 栈向所有匹配权限的用户下发。

## 编译运行 / Build & Run

```bash
# 由于整合了 Protobuf 与 Submodule 网络核心，建议使用 CMake 进行编译
# Requires CMake to properly link Protobuf libraries and ZeroNet resources
mkdir build && cd build
cmake ..
make -j4

# 启动服务器 / Run Gateway
./GatewayApp
```
