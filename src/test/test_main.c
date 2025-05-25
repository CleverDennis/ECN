#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/ecn_crypto.h"
#include "../../include/ecn_db.h"
#include "../../include/ecn_server.h"

// 声明测试函数
extern int test_crypto();
extern int test_database();
extern int test_server();

int main(int argc, char *argv[]) {
    int failed = 0;
    printf("Starting ECN system tests...\n\n");

    // 测试加密模块
    printf("=== Testing Crypto Module ===\n");
    if (test_crypto() != 0) {
        printf("❌ Crypto module tests failed!\n\n");
        failed++;
    } else {
        printf("✅ Crypto module tests passed!\n\n");
    }

    // 测试数据库模块
    printf("=== Testing Database Module ===\n");
    if (test_database() != 0) {
        printf("❌ Database module tests failed!\n\n");
        failed++;
    } else {
        printf("✅ Database module tests passed!\n\n");
    }

    // 测试服务器模块
    printf("=== Testing Server Module ===\n");
    if (test_server() != 0) {
        printf("❌ Server module tests failed!\n\n");
        failed++;
    } else {
        printf("✅ Server module tests passed!\n\n");
    }

    // 输出总结
    printf("=== Test Summary ===\n");
    if (failed > 0) {
        printf("❌ %d test modules failed!\n", failed);
        return 1;
    } else {
        printf("✅ All tests passed successfully!\n");
        return 0;
    }
} 