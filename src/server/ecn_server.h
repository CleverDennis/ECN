#ifndef ECN_SERVER_H
#define ECN_SERVER_H

#include <stdint.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

// 服务器配置结构
typedef struct {
    uint16_t port;           // 监听端口
    int max_clients;         // 最大客户端连接数
    const char *db_path;     // 数据库路径
} ecn_server_config_t;

// 客户端连接结构
typedef struct {
    int socket;              // 客户端socket
    struct sockaddr_in addr; // 客户端地址
    uint32_t user_id;       // 已认证的用户ID（0表示未认证）
    uint8_t session_token[64]; // 会话令牌
} ecn_client_t;

// 服务器上下文结构
typedef struct {
    int listen_sock;         // 监听socket
    pthread_t accept_thread; // 接受连接的线程
    pthread_t *worker_threads; // 工作线程池
    ecn_client_t *clients;   // 客户端连接数组
    int running;            // 服务器运行状态
    pthread_mutex_t clients_mutex; // 客户端数组互斥锁
} ecn_server_t;

// 服务器API
int ecn_server_init(ecn_server_t *server, const ecn_server_config_t *config);
void ecn_server_start(ecn_server_t *server);
void ecn_server_stop(ecn_server_t *server);
void ecn_server_cleanup(ecn_server_t *server);

#endif // ECN_SERVER_H 