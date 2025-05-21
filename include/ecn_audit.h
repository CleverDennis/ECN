#ifndef ECN_AUDIT_H
#define ECN_AUDIT_H

#include <time.h>
#include "ecn_user.h"

// 审计事件类型
typedef enum {
    ECN_AUDIT_LOGIN,           // 用户登录
    ECN_AUDIT_LOGOUT,          // 用户登出
    ECN_AUDIT_NOTE_CREATE,     // 创建笔记
    ECN_AUDIT_NOTE_READ,       // 读取笔记
    ECN_AUDIT_NOTE_UPDATE,     // 更新笔记
    ECN_AUDIT_NOTE_DELETE      // 删除笔记
} ecn_audit_type_t;

// 审计日志结构
typedef struct {
    uint32_t id;              // 日志ID
    uint32_t user_id;         // 用户ID
    ecn_audit_type_t type;    // 事件类型
    time_t timestamp;         // 时间戳
    char ip_addr[16];         // IP地址
    char details[256];        // 详细信息
} ecn_audit_log_t;

// 记录审计日志
int ecn_audit_log(const ecn_session_t *session, 
                 ecn_audit_type_t type,
                 const char *details);

// 查询审计日志
int ecn_audit_query(const ecn_session_t *session,
                   time_t start_time,
                   time_t end_time,
                   ecn_audit_log_t **logs,
                   size_t *count);

#endif // ECN_AUDIT_H 