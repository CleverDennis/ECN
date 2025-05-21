#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "../../include/ecn_db.h"

// 全局数据库连接
static sqlite3 *db = NULL;

// 创建用户表的SQL语句
static const char *CREATE_USER_TABLE = 
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "username TEXT UNIQUE NOT NULL,"
    "password_hash BLOB NOT NULL,"
    "salt BLOB NOT NULL,"
    "created_at INTEGER NOT NULL,"
    "last_login INTEGER"
    ");";

// 创建笔记表的SQL语句
static const char *CREATE_NOTE_TABLE = 
    "CREATE TABLE IF NOT EXISTS notes ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id INTEGER NOT NULL,"
    "title TEXT NOT NULL,"
    "content BLOB,"
    "content_len INTEGER NOT NULL,"
    "created_at INTEGER NOT NULL,"
    "updated_at INTEGER NOT NULL,"
    "encryption_key BLOB NOT NULL,"
    "FOREIGN KEY(user_id) REFERENCES users(id)"
    ");";

// 创建会话表的SQL语句
static const char *CREATE_SESSION_TABLE = 
    "CREATE TABLE IF NOT EXISTS sessions ("
    "token BLOB PRIMARY KEY,"
    "user_id INTEGER NOT NULL,"
    "expires_at INTEGER NOT NULL,"
    "FOREIGN KEY(user_id) REFERENCES users(id)"
    ");";

// 数据库初始化
int ecn_db_init(const char *db_path) {
    int rc;
    char *err_msg = NULL;

    // 打开数据库
    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // 创建表
    rc = sqlite3_exec(db, CREATE_USER_TABLE, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    rc = sqlite3_exec(db, CREATE_NOTE_TABLE, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    rc = sqlite3_exec(db, CREATE_SESSION_TABLE, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}

// 数据库关闭
void ecn_db_close(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

// 用户相关操作
int ecn_db_user_create(ecn_user_t *user) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO users (username, password_hash, salt, created_at, last_login) "
                     "VALUES (?, ?, ?, ?, ?);";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, user->password_hash, 32, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 3, user->salt, 16, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, user->created_at);
    sqlite3_bind_int64(stmt, 5, user->last_login);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return -1;
    }

    user->id = sqlite3_last_insert_rowid(db);
    return 0;
}

int ecn_db_user_get(const char *username, ecn_user_t *user) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, username, password_hash, salt, created_at, last_login "
                     "FROM users WHERE username = ?;";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user->id = sqlite3_column_int(stmt, 0);
        strncpy(user->username, (const char *)sqlite3_column_text(stmt, 1), 31);
        user->username[31] = '\0';
        memcpy(user->password_hash, sqlite3_column_blob(stmt, 2), 32);
        memcpy(user->salt, sqlite3_column_blob(stmt, 3), 16);
        user->created_at = sqlite3_column_int64(stmt, 4);
        user->last_login = sqlite3_column_int64(stmt, 5);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int ecn_db_user_update(const ecn_user_t *user) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE users SET last_login = ? WHERE id = ?;";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, user->last_login);
    sqlite3_bind_int(stmt, 2, user->id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// 笔记相关操作
int ecn_db_note_create(ecn_note_t *note) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO notes (user_id, title, content, content_len, "
                     "created_at, updated_at, encryption_key) VALUES (?, ?, ?, ?, ?, ?, ?);";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, note->user_id);
    sqlite3_bind_text(stmt, 2, note->title, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 3, note->content, note->content_len, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, note->content_len);
    sqlite3_bind_int64(stmt, 5, note->created_at);
    sqlite3_bind_int64(stmt, 6, note->updated_at);
    sqlite3_bind_blob(stmt, 7, note->key, 16, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return -1;
    }

    note->id = sqlite3_last_insert_rowid(db);
    return 0;
}

int ecn_db_note_get(uint32_t note_id, ecn_note_t *note) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT user_id, title, content, content_len, created_at, "
                     "updated_at, encryption_key FROM notes WHERE id = ?;";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, note_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        note->id = note_id;
        note->user_id = sqlite3_column_int(stmt, 0);
        strncpy(note->title, (const char *)sqlite3_column_text(stmt, 1), 255);
        note->title[255] = '\0';
        
        note->content_len = sqlite3_column_int64(stmt, 3);
        note->content = malloc(note->content_len);
        if (!note->content) {
            sqlite3_finalize(stmt);
            return -1;
        }
        memcpy(note->content, sqlite3_column_blob(stmt, 2), note->content_len);
        
        note->created_at = sqlite3_column_int64(stmt, 4);
        note->updated_at = sqlite3_column_int64(stmt, 5);
        memcpy(note->key, sqlite3_column_blob(stmt, 6), 16);
        
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int ecn_db_note_update(const ecn_note_t *note) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE notes SET title = ?, content = ?, content_len = ?, "
                     "updated_at = ? WHERE id = ? AND user_id = ?;";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, note->title, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, note->content, note->content_len, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, note->content_len);
    sqlite3_bind_int64(stmt, 4, note->updated_at);
    sqlite3_bind_int(stmt, 5, note->id);
    sqlite3_bind_int(stmt, 6, note->user_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int ecn_db_note_delete(uint32_t note_id) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM notes WHERE id = ?;";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, note_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int ecn_db_note_list(uint32_t user_id, ecn_note_t **notes, size_t *count) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, title, created_at, updated_at FROM notes "
                     "WHERE user_id = ? ORDER BY updated_at DESC;";
    int rc;
    size_t capacity = 10;
    size_t index = 0;

    *notes = malloc(capacity * sizeof(ecn_note_t));
    if (!*notes) {
        return -1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(*notes);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (index >= capacity) {
            capacity *= 2;
            ecn_note_t *temp = realloc(*notes, capacity * sizeof(ecn_note_t));
            if (!temp) {
                free(*notes);
                sqlite3_finalize(stmt);
                return -1;
            }
            *notes = temp;
        }

        ecn_note_t *note = &(*notes)[index];
        note->id = sqlite3_column_int(stmt, 0);
        note->user_id = user_id;
        strncpy(note->title, (const char *)sqlite3_column_text(stmt, 1), 255);
        note->title[255] = '\0';
        note->created_at = sqlite3_column_int64(stmt, 2);
        note->updated_at = sqlite3_column_int64(stmt, 3);
        note->content = NULL;  // 列表视图不加载内容
        note->content_len = 0;
        index++;
    }

    sqlite3_finalize(stmt);
    *count = index;
    return 0;
}

// 会话相关操作
int ecn_db_session_create(ecn_session_t *session) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO sessions (token, user_id, expires_at) VALUES (?, ?, ?);";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_blob(stmt, 1, session->token, 64, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, session->user_id);
    sqlite3_bind_int64(stmt, 3, session->expires_at);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int ecn_db_session_get(const uint8_t token[64], ecn_session_t *session) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT user_id, expires_at FROM sessions WHERE token = ?;";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_blob(stmt, 1, token, 64, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        memcpy(session->token, token, 64);
        session->user_id = sqlite3_column_int(stmt, 0);
        session->expires_at = sqlite3_column_int64(stmt, 1);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int ecn_db_session_delete(const uint8_t token[64]) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM sessions WHERE token = ?;";
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_blob(stmt, 1, token, 64, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
} 