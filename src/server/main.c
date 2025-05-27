#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../../include/ecn_server.h"

static ecn_server_t server;
static volatile int running = 1;

// 信号处理函数
static void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        printf("\nReceived signal %d, shutting down...\n", signo);
        running = 0;
    }
}

int main(int argc __attribute__((unused)), char *argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 服务器配置
    ecn_server_config_t config = {
        .port = 8443,           // 默认端口
        .max_clients = 100,     // 最大客户端数
        .db_path = "ecn.db"     // 数据库路径
    };
    
    // 解析命令行参数（可选）
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            config.port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            config.db_path = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            config.max_clients = atoi(argv[i + 1]);
            i++;
        } else {
            printf("Usage: %s [-p port] [-d db_path] [-c max_clients]\n", argv[0]);
            return 1;
        }
    }
    
    // 初始化服务器
    if (ecn_server_init(&server, &config) != 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    // 启动服务器
    ecn_server_start(&server);
    
    // 主循环
    printf("Server running. Press Ctrl+C to stop.\n");
    while (running) {
        sleep(1);
    }
    
    // 停止服务器
    ecn_server_stop(&server);
    ecn_server_cleanup(&server);
    
    return 0;
} 