#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "../../include/ecn_db.h"
#include "../../include/ecn_crypto.h"

// 测试用户操作
static int test_user_operations(void) {
    ecn_user_t user = {
        .username = "testuser",
        .password_hash = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
                         17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32},
        .salt = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
        .created_at = time(NULL),
        .last_login = time(NULL)
    };

    printf("\n=== Testing User Operations ===\n");

    // 生成SM2密钥对，补全user结构体
    if (ecn_sm2_generate_keypair(user.public_key, user.private_key) != 0) {
        printf("Failed to generate SM2 keypair\n");
        return -1;
    }

    // 创建用户
    if (ecn_db_user_create(&user) != 0) {
        printf("Failed to create user\n");
        return -1;
    }
    printf("Created user with ID: %d\n", user.id);

    // 获取用户
    ecn_user_t fetched_user;
    if (ecn_db_user_get("testuser", &fetched_user) != 0) {
        printf("Failed to get user\n");
        return -1;
    }
    printf("Retrieved user: %s\n", fetched_user.username);

    // 更新用户
    fetched_user.last_login = time(NULL);
    if (ecn_db_user_update(&fetched_user) != 0) {
        printf("Failed to update user\n");
        return -1;
    }
    printf("Updated user's last login time\n");

    return 0;
}

// 测试笔记操作
static int test_note_operations(void) {
    ecn_note_t note = {
        .user_id = 1,
        .title = "Test Note",
        .content = (uint8_t *)"This is a test note content",
        .content_len = 26,
        .created_at = time(NULL),
        .updated_at = time(NULL),
        .key = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
    };

    printf("\n=== Testing Note Operations ===\n");

    // 创建笔记
    if (ecn_db_note_create(&note) != 0) {
        printf("Failed to create note\n");
        return -1;
    }
    printf("Created note with ID: %d\n", note.id);

    // 获取笔记
    ecn_note_t fetched_note;
    if (ecn_db_note_get(note.id, &fetched_note) != 0) {
        printf("Failed to get note\n");
        return -1;
    }
    printf("Retrieved note: %s\n", fetched_note.title);
    free(fetched_note.content);  // 释放获取的笔记内容

    // 更新笔记
    strncpy(note.title, "Updated Test Note", 255);
    note.updated_at = time(NULL);
    if (ecn_db_note_update(&note) != 0) {
        printf("Failed to update note\n");
        return -1;
    }
    printf("Updated note title\n");

    // 列出笔记
    ecn_note_t *notes;
    size_t count;
    if (ecn_db_note_list(1, &notes, &count) != 0) {
        printf("Failed to list notes\n");
        return -1;
    }
    printf("Listed %zu notes\n", count);
    free(notes);

    // 删除笔记
    if (ecn_db_note_delete(note.id) != 0) {
        printf("Failed to delete note\n");
        return -1;
    }
    printf("Deleted note\n");

    return 0;
}

// 测试会话操作
static int test_session_operations(void) {
    ecn_session_t session = {
        .user_id = 1,
        .expires_at = time(NULL) + 3600  // 1小时后过期
    };
    // 生成随机会话令牌
    for (int i = 0; i < 64; i++) {
        session.token[i] = i;
    }

    printf("\n=== Testing Session Operations ===\n");

    // 创建会话
    if (ecn_db_session_create(&session) != 0) {
        printf("Failed to create session\n");
        return -1;
    }
    printf("Created session for user ID: %d\n", session.user_id);

    // 获取会话
    ecn_session_t fetched_session;
    if (ecn_db_session_get(session.token, &fetched_session) != 0) {
        printf("Failed to get session\n");
        return -1;
    }
    printf("Retrieved session for user ID: %d\n", fetched_session.user_id);

    // 删除会话
    if (ecn_db_session_delete(session.token) != 0) {
        printf("Failed to delete session\n");
        return -1;
    }
    printf("Deleted session\n");

    return 0;
}

int main() {
    printf("Starting database module tests...\n");

    // 初始化数据库
    if (ecn_db_init("test.db") != 0) {
        printf("Failed to initialize database\n");
        return 1;
    }

    // 运行测试
    if (test_user_operations() != 0) {
        printf("User operations test failed\n");
        ecn_db_close();
        return 1;
    }

    if (test_note_operations() != 0) {
        printf("Note operations test failed\n");
        ecn_db_close();
        return 1;
    }

    if (test_session_operations() != 0) {
        printf("Session operations test failed\n");
        ecn_db_close();
        return 1;
    }

    // 关闭数据库
    ecn_db_close();

    printf("\nAll database tests passed!\n");
    return 0;
} 