#include <stdio.h>
#include <stdlib.h>
#include "../include/ecn_db.h"
#include "../include/ecn_user.h"
#include "../include/ecn_note.h"
#include "../include/ecn_audit.h"

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    // 初始化数据库
    if (ecn_db_init("ecn.db") != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }

    printf("ECN (Encrypted Cloud Notes) v1.0\n");
    printf("================================\n");

    // TODO: 实现命令行参数解析
    // TODO: 实现用户交互界面
    // TODO: 实现主程序逻辑

    // 清理资源
    ecn_db_close();
    return 0;
}