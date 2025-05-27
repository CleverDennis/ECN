#ifndef ECN_PROTOCOL_H
#define ECN_PROTOCOL_H

#include <stdint.h>

// 协议版本
#define ECN_PROTOCOL_VERSION 1

// 消息类型
enum ecn_msg_type {
    // 认证相关
    ECN_MSG_REGISTER = 1,    // 注册请求
    ECN_MSG_LOGIN = 2,       // 登录请求
    ECN_MSG_LOGOUT = 3,      // 登出请求
    
    // 笔记相关
    ECN_MSG_NOTE_CREATE = 10,  // 创建笔记
    ECN_MSG_NOTE_UPDATE = 11,  // 更新笔记
    ECN_MSG_NOTE_DELETE = 12,  // 删除笔记
    ECN_MSG_NOTE_LIST = 13,    // 列出笔记
    ECN_MSG_NOTE_GET = 14,     // 获取笔记
    
    // 响应
    ECN_MSG_RESPONSE = 100,    // 通用响应
    ECN_MSG_ERROR = 101        // 错误响应
};

// 错误码
enum ecn_error_code {
    ECN_ERR_NONE = 0,          // 无错误
    ECN_ERR_AUTH_FAILED = 1,   // 认证失败
    ECN_ERR_USER_EXISTS = 2,   // 用户已存在
    ECN_ERR_INVALID_TOKEN = 3, // 无效的会话令牌
    ECN_ERR_NOT_FOUND = 4,     // 资源不存在
    ECN_ERR_SERVER = 5,        // 服务器内部错误
    ECN_ERR_INVALID_REQ = 6,   // 无效的请求
    ECN_ERR_VERSION = 7,       // 协议版本不匹配
    ECN_ERR_INVALID_SESSION = 8 // 无效的会话
};

// 消息头部
typedef struct {
    uint8_t version;          // 协议版本
    uint8_t type;            // 消息类型
    uint16_t payload_len;    // 负载长度
    uint8_t session_token[64]; // 会话令牌（登录后使用）
} __attribute__((packed)) ecn_msg_header_t;

// 注册请求
typedef struct {
    char username[32];       // 用户名
    char password[64];      // 明文密码
    uint8_t public_key[65];  // SM2公钥
} __attribute__((packed)) ecn_register_req_t;

// 登录请求
typedef struct {
    char username[32];       // 用户名
    char password[64];      // 明文密码
} __attribute__((packed)) ecn_login_req_t;

// 创建笔记请求
typedef struct {
    char title[256];        // 笔记标题
    uint32_t content_len;   // 内容长度
    // 后跟加密的内容数据
} __attribute__((packed)) ecn_note_create_req_t;

// 更新笔记请求
typedef struct {
    uint32_t id;           // 笔记ID
    uint32_t content_len;  // 内容长度
    // 后跟加密的内容数据
} __attribute__((packed)) ecn_note_update_req_t;

// 通用响应
typedef struct {
    uint8_t error_code;     // 错误码
    uint32_t data_len;      // 响应数据长度
    // 后跟响应数据
} __attribute__((packed)) ecn_response_t;

#endif // ECN_PROTOCOL_H