import socket
import struct
import sys
# 引入 Protobuf 生成的 Python 文件
sys.path.append('./proto')
import subscription_pb2

def main():
    print("--- [散户端模拟] 订阅 AAPL 行情 ---")
    
    # 建立 TCP 连线
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect(('127.0.0.1', 8080))
    except Exception as e:
        print("连线失败，伺服器可能还没启动: ", e)
        return
        
    print("连线成功，正在发送 Protobuf 订阅包...")
    
    # 构造 Protobuf 订阅请求
    req = subscription_pb2.SubscribeRequest()
    req.op_type = subscription_pb2.SubscribeRequest.SUBSCRIBE
    req.tickers.append("ag2412")
    req.client_id = "User_XYZ_999"
    
    # 获取序列化字节流
    pb_data = req.SerializeToString()
    
    # 发送给网关
    sock.sendall(pb_data)
    
    print("订阅指令请求已送出，等待服务器群发二进制 UDP 行情...")
    
    # C++ struct 接收格式 (强制 1 字节对齐 =)
    fmt = '=8sQd5d5i5d5i'
    struct_size = struct.calcsize(fmt)
    
    while True:
        data = sock.recv(struct_size)
        if not data:
            print("伺服器关闭了连线。")
            break
            
        if len(data) == struct_size:
            # 零拷贝还原
            unpacked = struct.unpack(fmt, data)
            ticker = unpacked[0].decode('utf-8').strip('\x00')
            timestamp = unpacked[1]
            last_price = unpacked[2]
            print(f"[TCP Client] 收到二进制直通行情 => 股票: {ticker}, 最新价: $ {last_price:.2f}")

if __name__ == '__main__':
    main()
