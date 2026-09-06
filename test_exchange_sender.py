import socket
import struct
import time

def main():
    multicast_group = ('239.0.0.1', 30000)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    # TTL 控制，只在本地子网组播
    ttl = struct.pack('b', 1)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)

    print("--- [交易所模拟] 核心 UDP 行情发射器已启动 ---")
    mock_price = 150.0
    
    while True:
        mock_price += 0.1
        ticker = b"AAPL\0\0\0\0" # 8 字节股票代码
        timestamp = int(time.time() * 1000)
        
        # 将二进制打包成 C++ struct: 8sQ11d10i
        # 由于我们数据结构中有 1 个 char[8], 1 个 uint64, 11 个 double, 10 个 int
        # 对应格式：8s (char[8]), Q (uint64_t), d (double last_price), 
        # 5d (bid_price), 5i (bid_volume), 5d (ask_price), 5i (ask_volume)
        fmt = '=8sQd5d5i5d5i'
        
        # 构造空盘口数据
        bid_px = [mock_price - i for i in range(1, 6)]
        bid_vol = [100 * i for i in range(1, 6)]
        ask_px = [mock_price + i for i in range(1, 6)]
        ask_vol = [50 * i for i in range(1, 6)]
        
        message = struct.pack(fmt, ticker, timestamp, mock_price,
                              *bid_px, *bid_vol, *ask_px, *ask_vol)
                              
        sock.sendto(message, multicast_group)
        print(f"[UDP Sender] 群播发送 AAPL 行情: $ {mock_price:.2f}")
        time.sleep(1.0)

if __name__ == '__main__':
    main()
