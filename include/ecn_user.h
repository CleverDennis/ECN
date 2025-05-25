#ifndef ECN_USER_H
#define ECN_USER_H

#include <stdint.h>
#include <time.h>

// 用户结构体
typedef struct {
    uint32_t id;                 // 用户ID
    char username[32];           // 用户名
    uint8_t password_hash[32];   // SM3密码哈希
    uint8_t salt[16];           // 密码盐值
    uint8_t public_key[65];     // SM2公钥
    uint8_t private_key[32];    // SM2私钥（实际项目应安全存储）
    time_t created_at;          // 创建时间
    time_t last_login;          // 最后登录时间
} ecn_user_t;

// 用户会话结构体
typedef struct {
    uint32_t user_id;           // 用户ID
    uint8_t token[64];          // 会话令牌
    time_t expires_at;          // 过期时间
} ecn_session_t;

// 用户注册
int ecn_user_register(const char *username, const char *password);

// 用户登录
int ecn_user_login(const char *username, const char *password, ecn_session_t *session);

// 验证会话
int ecn_verify_session(const ecn_session_t *session);

// 用户登出
int ecn_user_logout(ecn_session_t *session);

#endif // ECN_USER_H 