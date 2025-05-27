#!/usr/bin/env python3
import socket
import struct

# 协议常量
ECN_PROTOCOL_VERSION = 1
ECN_MSG_REGISTER = 1

# 连接服务器
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8443))

# 构造注册请求
username = "Dennis"
password = "123456"
public_key = b'\x01' * 65  # 测试用假公钥

# 构造负载
payload = struct.pack('!32s64s65s',
    username.encode().ljust(32, b'\x00'),
    password.encode().ljust(64, b'\x00'),
    public_key
)
payload_len = len(payload)

# 构造消息头（68字节，payload_len为uint16_t小端）
header = struct.pack('<BBH64s',
    ECN_PROTOCOL_VERSION,  # 版本
    ECN_MSG_REGISTER,     # 消息类型
    payload_len,          # 负载长度 (161)
    b'\x00' * 64          # 会话令牌
)

print("Sending registration request...")
print(f"Header: version={ECN_PROTOCOL_VERSION}, type={ECN_MSG_REGISTER}, payload_len={payload_len}")

# 发送请求
sock.sendall(header + payload)

# 接收响应头
header_size = struct.calcsize('<BBH64s')
print(f"Waiting for response header... (expect {header_size} bytes)")
header_data = sock.recv(header_size)
if len(header_data) < header_size:
    print(f"Incomplete header received: {len(header_data)} bytes")
    print("Raw response:", header_data.hex())
    sock.close()
    exit(1)

# 解析响应头（payload_len为uint16_t小端）
version, msg_type, payload_len, session_token = struct.unpack('<BBH64s', header_data)
print(f"Response header: version={version}, type={msg_type}, payload_len={payload_len}")

# 接收响应负载
if payload_len > 0:
    print(f"Waiting for response payload ({payload_len} bytes)...")
    payload_data = sock.recv(payload_len)
    if len(payload_data) < payload_len:
        print(f"Incomplete payload received: {len(payload_data)} bytes")
        print("Raw payload:", payload_data.hex())
        sock.close()
        exit(1)
    
    # 解析响应负载
    if len(payload_data) >= 5:  # sizeof(ecn_response_t)
        error_code = payload_data[0]
        data_len = int.from_bytes(payload_data[1:5], 'big')
        print(f"Response: error_code={error_code}, data_len={data_len}")
        if error_code == 0:
            print("Registration successful!")
        else:
            print(f"Registration failed with error code: {error_code}")
        if data_len > 0 and len(payload_data) > 5:
            print("Additional data:", payload_data[5:5+data_len].hex())
    else:
        print("Invalid response payload format")
else:
    print("No response payload")

sock.close() 