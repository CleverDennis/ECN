#ifndef ECN_NOTE_H
#define ECN_NOTE_H

#include <stdint.h>
#include <time.h>
#include "ecn_user.h"

// 笔记结构体
typedef struct {
    uint32_t id;                 // 笔记ID
    uint32_t user_id;           // 所属用户ID
    char title[256];            // 笔记标题
    uint8_t *content;           // 加密后的内容
    size_t content_len;         // 内容长度
    time_t created_at;          // 创建时间
    time_t updated_at;          // 最后更新时间
    uint8_t key[16];           // SM4加密密钥
} ecn_note_t;

// 创建新笔记
int ecn_note_create(const ecn_session_t *session, const char *title, 
                   const char *content, ecn_note_t *note);

// 读取笔记
int ecn_note_read(const ecn_session_t *session, uint32_t note_id, 
                 ecn_note_t *note);

// 更新笔记
int ecn_note_update(const ecn_session_t *session, ecn_note_t *note, 
                   const char *new_content);

// 删除笔记
int ecn_note_delete(const ecn_session_t *session, uint32_t note_id);

// 列出用户的所有笔记
int ecn_note_list(const ecn_session_t *session, ecn_note_t **notes, 
                 size_t *count);

#endif // ECN_NOTE_H 