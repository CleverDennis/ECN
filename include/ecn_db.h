#ifndef ECN_DB_H
#define ECN_DB_H

#include <sqlite3.h>
#include "ecn_user.h"
#include "ecn_note.h"

// 数据库初始化
int ecn_db_init(const char *db_path);

// 数据库关闭
void ecn_db_close(void);

// 用户相关数据库操作
int ecn_db_user_create(ecn_user_t *user);
int ecn_db_user_get(const char *username, ecn_user_t *user);
int ecn_db_user_update(const ecn_user_t *user);
int ecn_db_user_get_by_id(uint32_t id, ecn_user_t *user);

// 笔记相关数据库操作
int ecn_db_note_create(ecn_note_t *note);
int ecn_db_note_get(uint32_t note_id, ecn_note_t *note);
int ecn_db_note_update(const ecn_note_t *note);
int ecn_db_note_delete(uint32_t note_id);
int ecn_db_note_list(uint32_t user_id, ecn_note_t **notes, size_t *count);

// 会话相关数据库操作
int ecn_db_session_create(ecn_session_t *session);
int ecn_db_session_get(const uint8_t token[64], ecn_session_t *session);
int ecn_db_session_delete(const uint8_t token[64]);

#endif // ECN_DB_H 