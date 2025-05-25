#ifndef ECN_SERVER_H
#define ECN_SERVER_H

#include <stdint.h>
#include <pthread.h>
#include <netinet/in.h>

// 服务器配置结构
typedef struct {
    uint16_t port;           // 监听端口
    int max_clients;         // 最大客户端连接数
    const char *db_path;     // 数据库路径
} ecn_server_config_t;

// 客户端连接结构
typedef struct {
    int socket;                 // 客户端socket
    struct sockaddr_in addr;    // 客户端地址
    uint32_t user_id;          // 用户ID（如果已登录）
    uint8_t session_token[64]; // 会话令牌
} ecn_client_t;

// 服务器结构
typedef struct {
    int listen_sock;           // 监听socket
    int running;               // 运行状态标志
    ecn_server_config_t config; // 服务器配置
    ecn_client_t *clients;     // 客户端连接数组
    pthread_t accept_thread;    // 接受连接线程
    pthread_t *worker_threads;  // 工作线程池
    pthread_mutex_t clients_mutex; // 客户端数组互斥锁
} ecn_server_t;

// 初始化服务器
int ecn_server_init(ecn_server_t *server, const ecn_server_config_t *config);

// 启动服务器
void ecn_server_start(ecn_server_t *server);

// 停止服务器
void ecn_server_stop(ecn_server_t *server);

// 清理服务器资源
void ecn_server_cleanup(ecn_server_t *server);

#endif // ECN_SERVER_H 