#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../include/ecn_server.h"
#include "../../include/ecn_db.h"
#include "../../include/ecn_crypto.h"
#include "../../include/ecn_protocol.h"
#include <gmssl/rand.h>

#define MAX_BUFFER_SIZE 4096
#define THREAD_POOL_SIZE 4

// 发送响应
static int send_response(int sock, uint8_t error_code, const void *data, size_t data_len) {
    ecn_msg_header_t header;
    ecn_response_t response;
    uint8_t buffer[MAX_BUFFER_SIZE];
    size_t total_len = 0;

    // 构造消息头
    header.version = ECN_PROTOCOL_VERSION;
    header.type = (error_code == ECN_ERR_NONE) ? ECN_MSG_RESPONSE : ECN_MSG_ERROR;
    header.payload_len = sizeof(response) + data_len;
    memset(header.session_token, 0, sizeof(header.session_token));

    // 构造响应
    response.error_code = error_code;
    response.data_len = data_len;

    // 组装完整消息
    memcpy(buffer, &header, sizeof(header));
    total_len += sizeof(header);
    memcpy(buffer + total_len, &response, sizeof(response));
    total_len += sizeof(response);
    if (data && data_len > 0) {
        memcpy(buffer + total_len, data, data_len);
        total_len += data_len;
    }

    // 发送消息
    return send(sock, buffer, total_len, 0) == total_len ? 0 : -1;
}

// 处理注册请求
static int handle_register(ecn_server_t *server, int client_sock, const uint8_t *payload, size_t len) {
    if (len < sizeof(ecn_register_req_t)) {
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    const ecn_register_req_t *req = (const ecn_register_req_t *)payload;
    ecn_user_t user;
    memset(&user, 0, sizeof(user));
    strncpy(user.username, req->username, sizeof(user.username) - 1);
    // 生成盐
    if (ecn_generate_random(user.salt, sizeof(user.salt)) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }
    // 生成SM2密钥对
    if (ecn_sm2_generate_keypair(user.public_key, user.private_key) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }
    // 服务器端计算hash
    if (ecn_generate_password_hash(req->password, user.salt, user.password_hash) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }
    user.created_at = time(NULL);
    user.last_login = 0;
    if (ecn_db_user_create(&user) != 0) {
        return send_response(client_sock, ECN_ERR_USER_EXISTS, NULL, 0);
    }
    return send_response(client_sock, ECN_ERR_NONE, NULL, 0);
}

// 处理登录请求
static int handle_login(ecn_server_t *server, int client_sock, const uint8_t *payload, size_t len) {
    if (len < sizeof(ecn_login_req_t)) {
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }
    const ecn_login_req_t *req = (const ecn_login_req_t *)payload;
    ecn_user_t user;
    if (ecn_db_user_get(req->username, &user) != 0) {
        return send_response(client_sock, ECN_ERR_AUTH_FAILED, NULL, 0);
    }
    uint8_t hash[32];
    if (ecn_generate_password_hash(req->password, user.salt, hash) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }
    if (memcmp(user.password_hash, hash, 32) != 0) {
        return send_response(client_sock, ECN_ERR_AUTH_FAILED, NULL, 0);
    }

    // 生成会话令牌
    uint8_t session_token[64];
    if (ecn_generate_random(session_token, sizeof(session_token)) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    // 创建会话
    ecn_session_t session;
    memcpy(session.token, session_token, sizeof(session.token));
    session.user_id = user.id;
    session.expires_at = time(NULL) + 3600; // 1小时过期

    if (ecn_db_session_create(&session) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    // 更新最后登录时间
    user.last_login = time(NULL);
    ecn_db_user_update(&user);

    // 返回会话令牌
    return send_response(client_sock, ECN_ERR_NONE, session_token, sizeof(session_token));
}

// 验证会话令牌
static int verify_session(const uint8_t token[64], uint32_t *user_id) {
    ecn_session_t session;
    
    if (ecn_db_session_get(token, &session) != 0) {
        return -1;
    }

    if (session.expires_at < time(NULL)) {
        ecn_db_session_delete(token);
        return -1;
    }

    if (user_id) {
        *user_id = session.user_id;
    }

    return 0;
}

// 处理创建笔记请求
static int handle_note_create(ecn_server_t *server, int client_sock, 
                            uint32_t user_id, const uint8_t *payload, size_t len) {
    if (len < sizeof(ecn_note_create_req_t)) {
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    const ecn_note_create_req_t *req = (const ecn_note_create_req_t *)payload;
    const uint8_t *content_data = payload + sizeof(ecn_note_create_req_t);
    size_t content_len = len - sizeof(ecn_note_create_req_t);

    // 验证内容长度
    if (content_len != req->content_len) {
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    // 获取用户SM2公钥（假设数据库有存储，或注册时传入）
    ecn_user_t user;
    if (ecn_db_user_get_by_id(user_id, &user) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    // 混合加密
    uint8_t *encrypted = NULL;
    size_t encrypted_len = 0;
    if (ecn_hybrid_encrypt(content_data, content_len, user.public_key, &encrypted, &encrypted_len) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    // 创建笔记结构
    ecn_note_t note;
    note.user_id = user_id;
    strncpy(note.title, req->title, sizeof(note.title) - 1);
    note.title[sizeof(note.title) - 1] = '\0';
    note.content = encrypted;
    note.content_len = encrypted_len;
    note.created_at = time(NULL);
    note.updated_at = note.created_at;

    // 保存笔记
    if (ecn_db_note_create(&note) != 0) {
        free(encrypted);
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    free(encrypted);
    return send_response(client_sock, ECN_ERR_NONE, NULL, 0);
}

// 处理更新笔记请求
static int handle_note_update(const ecn_session_t *session, const ecn_note_update_req_t *req,
                      const uint8_t *content_data, size_t content_len,
                      ecn_response_t *resp) {
    ecn_note_t note;
    
    // 获取原笔记
    if (ecn_db_note_get(req->id, &note) != 0) {
        resp->error_code = ECN_ERR_NOT_FOUND;
        return -1;
    }
    
    // 验证所有权
    if (note.user_id != session->user_id) {
        resp->error_code = ECN_ERR_AUTH_FAILED;
        return -1;
    }
    
    // 加密新内容
    uint8_t *new_content = malloc(content_len + 16); // +16 for IV
    if (!new_content) {
        resp->error_code = ECN_ERR_SERVER;
        return -1;
    }
    
    if (ecn_sm4_encrypt_ctr(content_data, content_len,
                           note.key, new_content) != 0) {
        free(new_content);
        resp->error_code = ECN_ERR_SERVER;
        return -1;
    }
    
    // 更新笔记
    free(note.content);
    note.content = new_content;
    note.content_len = content_len + 16;
    note.updated_at = time(NULL);
    
    if (ecn_db_note_update(&note) != 0) {
        free(note.content);
        resp->error_code = ECN_ERR_SERVER;
        return -1;
    }
    
    resp->error_code = ECN_ERR_NONE;
    free(note.content);
    return 0;
}

// 处理删除笔记请求
static int handle_note_delete(ecn_server_t *server, int client_sock,
                            uint32_t user_id, const uint8_t *payload, size_t len) {
    if (len != sizeof(uint32_t)) {
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    uint32_t note_id = *(const uint32_t *)payload;

    // 获取笔记信息
    ecn_note_t note;
    if (ecn_db_note_get(note_id, &note) != 0) {
        return send_response(client_sock, ECN_ERR_NOT_FOUND, NULL, 0);
    }

    // 验证所有权
    if (note.user_id != user_id) {
        free(note.content);
        return send_response(client_sock, ECN_ERR_AUTH_FAILED, NULL, 0);
    }

    free(note.content);

    // 删除笔记
    if (ecn_db_note_delete(note_id) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    return send_response(client_sock, ECN_ERR_NONE, NULL, 0);
}

// 处理获取笔记列表请求
static int handle_note_list(ecn_server_t *server, int client_sock,
                          uint32_t user_id, const uint8_t *payload, size_t len) {
    ecn_note_t *notes;
    size_t count;

    // 获取用户的笔记列表
    if (ecn_db_note_list(user_id, &notes, &count) != 0) {
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    // 构造响应数据
    size_t response_size = count * sizeof(struct {
        uint32_t id;
        char title[256];
        uint64_t created_at;
        uint64_t updated_at;
    });
    uint8_t *response_data = malloc(response_size);
    if (!response_data) {
        free(notes);
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    // 填充响应数据
    uint8_t *ptr = response_data;
    for (size_t i = 0; i < count; i++) {
        *(uint32_t *)ptr = notes[i].id;
        ptr += sizeof(uint32_t);
        memcpy(ptr, notes[i].title, 256);
        ptr += 256;
        *(uint64_t *)ptr = notes[i].created_at;
        ptr += sizeof(uint64_t);
        *(uint64_t *)ptr = notes[i].updated_at;
        ptr += sizeof(uint64_t);
    }

    free(notes);
    int ret = send_response(client_sock, ECN_ERR_NONE, response_data, response_size);
    free(response_data);
    return ret;
}

// 处理获取笔记内容请求
static int handle_note_get(ecn_server_t *server, int client_sock,
                         uint32_t user_id, const uint8_t *payload, size_t len) {
    if (len != sizeof(uint32_t)) {
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    uint32_t note_id = *(const uint32_t *)payload;

    // 获取笔记
    ecn_note_t note;
    if (ecn_db_note_get(note_id, &note) != 0) {
        return send_response(client_sock, ECN_ERR_NOT_FOUND, NULL, 0);
    }

    // 验证所有权
    if (note.user_id != user_id) {
        free(note.content);
        return send_response(client_sock, ECN_ERR_AUTH_FAILED, NULL, 0);
    }

    // 获取用户SM2私钥（实际项目应安全存储和管理，这里假设有接口获取）
    ecn_user_t user;
    if (ecn_db_user_get_by_id(user_id, &user) != 0) {
        free(note.content);
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }
    uint8_t *decrypted = NULL;
    size_t decrypted_len = 0;
    if (ecn_hybrid_decrypt(note.content, note.content_len, user.private_key, &decrypted, &decrypted_len) != 0) {
        free(note.content);
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    // 构造响应数据
    size_t response_size = sizeof(ecn_note_create_req_t) + decrypted_len;
    uint8_t *response_data = malloc(response_size);
    if (!response_data) {
        free(note.content);
        free(decrypted);
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    // 填充响应数据
    ecn_note_create_req_t *resp = (ecn_note_create_req_t *)response_data;
    strncpy(resp->title, note.title, sizeof(resp->title) - 1);
    resp->title[sizeof(resp->title) - 1] = '\0';
    resp->content_len = decrypted_len;
    memcpy(response_data + sizeof(ecn_note_create_req_t), decrypted, decrypted_len);

    free(note.content);
    free(decrypted);
    int ret = send_response(client_sock, ECN_ERR_NONE, response_data, response_size);
    free(response_data);
    return ret;
}

// 更新handle_client_message函数中的switch语句
static int handle_client_message(ecn_server_t *server, int client_sock,
                               const ecn_msg_header_t *header,
                               const uint8_t *payload) {
    // 检查会话（除了注册和登录请求）
    ecn_session_t session;
    uint32_t user_id = 0;
    
    if (header->type != ECN_MSG_REGISTER && header->type != ECN_MSG_LOGIN) {
        if (ecn_db_session_get(header->session_token, &session) != 0) {
            return send_response(client_sock, ECN_ERR_INVALID_TOKEN, NULL, 0);
        }
        user_id = session.user_id;
    }
    
    // 处理不同类型的消息
    switch (header->type) {
        case ECN_MSG_REGISTER:
            return handle_register(server, client_sock, payload, header->payload_len);
            
        case ECN_MSG_LOGIN: {
            ecn_response_t *resp = (ecn_response_t *)(header + 1);
            return handle_login(server, client_sock, payload, header->payload_len);
        }
            
        case ECN_MSG_LOGOUT:
            if (header->payload_len == 0) {
                ecn_db_session_delete(header->session_token);
                return send_response(client_sock, ECN_ERR_NONE, NULL, 0);
            }
            break;

        case ECN_MSG_NOTE_CREATE:
            if (header->payload_len < sizeof(ecn_note_create_req_t)) {
                return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
            }
            return handle_note_create(server, client_sock, user_id, payload, header->payload_len);
            
        case ECN_MSG_NOTE_UPDATE:
            if (header->payload_len < sizeof(ecn_note_update_req_t)) {
                return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
            }
            return handle_note_update(&session,
                                    (const ecn_note_update_req_t *)payload,
                                    payload + sizeof(ecn_note_update_req_t),
                                    header->payload_len - sizeof(ecn_note_update_req_t),
                                    (ecn_response_t *)(header + 1));
            
        case ECN_MSG_NOTE_DELETE:
            return handle_note_delete(server, client_sock, user_id, payload, header->payload_len);
            
        case ECN_MSG_NOTE_LIST:
            return handle_note_list(server, client_sock, user_id, payload, header->payload_len);
            
        case ECN_MSG_NOTE_GET:
            return handle_note_get(server, client_sock, user_id, payload, header->payload_len);
            
        default:
            return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    return -1;
}

// 处理客户端连接的工作线程函数
static void *worker_thread(void *arg) {
    ecn_server_t *server = (ecn_server_t *)arg;
    uint8_t buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read;
    
    while (server->running) {
        // 检查所有客户端连接
        pthread_mutex_lock(&server->clients_mutex);
        for (int i = 0; i < server->config.max_clients; i++) {
            if (server->clients[i].socket < 0) {
                continue;
            }

            // 尝试读取消息头
            bytes_read = recv(server->clients[i].socket, buffer,
                            sizeof(ecn_msg_header_t), MSG_DONTWAIT);
            
            if (bytes_read < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    // 连接错误，关闭它
                    close(server->clients[i].socket);
                    server->clients[i].socket = -1;
                }
                continue;
            }
            
            if (bytes_read == 0) {
                // 客户端断开连接
                close(server->clients[i].socket);
                server->clients[i].socket = -1;
                continue;
            }

            if (bytes_read < sizeof(ecn_msg_header_t)) {
                // 消息不完整，等待更多数据
                continue;
            }

            // 解析消息头
            ecn_msg_header_t *header = (ecn_msg_header_t *)buffer;
            
            // 检查版本号
            if (header->version != ECN_PROTOCOL_VERSION) {
                send_response(server->clients[i].socket, ECN_ERR_INVALID_REQ, NULL, 0);
                continue;
            }

            // 读取负载
            if (header->payload_len > 0) {
                if (header->payload_len > MAX_BUFFER_SIZE - sizeof(ecn_msg_header_t)) {
                    send_response(server->clients[i].socket, ECN_ERR_INVALID_REQ, NULL, 0);
                    continue;
                }

                bytes_read = recv(server->clients[i].socket,
                                buffer + sizeof(ecn_msg_header_t),
                                header->payload_len, 0);
                
                if (bytes_read < 0 || bytes_read != header->payload_len) {
                    // 读取负载失败
                    continue;
                }
            }

            // 处理消息
            handle_client_message(server, server->clients[i].socket,
                                header, buffer + sizeof(ecn_msg_header_t));
        }
        pthread_mutex_unlock(&server->clients_mutex);

        // 避免CPU占用过高
        usleep(10000);  // 10ms
    }
    
    return NULL;
}

// 接受新客户端连接的线程函数
static void *accept_thread(void *arg) {
    ecn_server_t *server = (ecn_server_t *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (server->running) {
        // 接受新的客户端连接
        int client_sock = accept(server->listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (errno != EINTR) {
                perror("accept failed");
            }
            continue;
        }
        
        // 查找空闲的客户端槽位
        pthread_mutex_lock(&server->clients_mutex);
        int slot = -1;
        for (int i = 0; i < server->config.max_clients; i++) {
            if (server->clients[i].socket == -1) {
                slot = i;
                break;
            }
        }
        
        if (slot >= 0) {
            // 初始化新的客户端连接
            server->clients[slot].socket = client_sock;
            server->clients[slot].addr = client_addr;
            server->clients[slot].user_id = 0;
            memset(server->clients[slot].session_token, 0, sizeof(server->clients[slot].session_token));
            printf("New client connected from %s:%d\n", 
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        } else {
            // 没有空闲槽位，关闭连接
            close(client_sock);
            printf("Rejected client from %s:%d (server full)\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }
        pthread_mutex_unlock(&server->clients_mutex);
    }
    
    return NULL;
}

// 初始化服务器
int ecn_server_init(ecn_server_t *server, const ecn_server_config_t *config) {
    // 初始化服务器结构
    memset(server, 0, sizeof(ecn_server_t));
    server->config = *config;
    server->running = 0;
    server->listen_sock = -1;
    
    // 初始化客户端数组
    server->clients = calloc(config->max_clients, sizeof(ecn_client_t));
    if (!server->clients) {
        return -1;
    }
    for (int i = 0; i < config->max_clients; i++) {
        server->clients[i].socket = -1;
    }
    
    // 初始化互斥锁
    if (pthread_mutex_init(&server->clients_mutex, NULL) != 0) {
        free(server->clients);
        return -1;
    }
    
    // 创建工作线程池
    server->worker_threads = calloc(THREAD_POOL_SIZE, sizeof(pthread_t));
    if (!server->worker_threads) {
        pthread_mutex_destroy(&server->clients_mutex);
        free(server->clients);
        return -1;
    }
    
    // 初始化数据库
    if (ecn_db_init(config->db_path) != 0) {
        pthread_mutex_destroy(&server->clients_mutex);
        free(server->worker_threads);
        free(server->clients);
        return -1;
    }
    
    return 0;
}

// 启动服务器
void ecn_server_start(ecn_server_t *server) {
    struct sockaddr_in server_addr;
    
    // 创建监听socket
    server->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_sock < 0) {
        perror("socket failed");
        return;
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server->listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server->listen_sock);
        return;
    }
    
    // 绑定地址和端口
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server->config.port);
    
    if (bind(server->listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server->listen_sock);
        return;
    }
    
    // 开始监听
    if (listen(server->listen_sock, 5) < 0) {
        perror("listen failed");
        close(server->listen_sock);
        return;
    }
    
    // 设置服务器状态为运行中
    server->running = 1;
    
    // 启动工作线程
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&server->worker_threads[i], NULL, worker_thread, server) != 0) {
            fprintf(stderr, "Failed to create worker thread %d\n", i);
            server->running = 0;
            return;
        }
    }
    
    // 启动接受连接线程
    if (pthread_create(&server->accept_thread, NULL, accept_thread, server) != 0) {
        fprintf(stderr, "Failed to create accept thread\n");
        server->running = 0;
        return;
    }
    
    printf("Server started on port %d\n", server->config.port);
}

// 停止服务器
void ecn_server_stop(ecn_server_t *server) {
    if (!server->running) {
        return;
    }
    
    // 设置停止标志
    server->running = 0;
    
    // 关闭监听socket，中断accept
    if (server->listen_sock >= 0) {
        close(server->listen_sock);
        server->listen_sock = -1;
    }
    
    // 等待接受连接线程结束
    pthread_join(server->accept_thread, NULL);
    
    // 等待所有工作线程结束
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(server->worker_threads[i], NULL);
    }
    
    // 关闭所有客户端连接
    pthread_mutex_lock(&server->clients_mutex);
    for (int i = 0; i < server->config.max_clients; i++) {
        if (server->clients[i].socket >= 0) {
            close(server->clients[i].socket);
            server->clients[i].socket = -1;
        }
    }
    pthread_mutex_unlock(&server->clients_mutex);
    
    printf("Server stopped\n");
}

// 清理服务器资源
void ecn_server_cleanup(ecn_server_t *server) {
    // 确保服务器已停止
    ecn_server_stop(server);
    
    // 清理资源
    pthread_mutex_destroy(&server->clients_mutex);
    free(server->worker_threads);
    free(server->clients);
    ecn_db_close();
    
    memset(server, 0, sizeof(ecn_server_t));
} 