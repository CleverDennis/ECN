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
#define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define ERROR_LOG(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

// 全局服务器实例，用于信号处理
static ecn_server_t *g_server = NULL;

// 函数声明
static int send_response(int sock, uint8_t error_code, const void *data, size_t data_len);
static int handle_client_message(ecn_server_t *server, int client_sock,
                               const ecn_msg_header_t *header,
                               const uint8_t *payload);
static void handle_client(ecn_server_t *server, int client_sock);

// 信号处理函数
static void handle_signal(int signo) {
    if (signo == SIGINT && g_server) {
        printf("\nReceived signal %d, shutting down...\n", signo);
        g_server->running = 0;
    }
}

// 发送响应
static int send_response(int sock, uint8_t error_code, const void *data, size_t data_len) {
    DEBUG_LOG("Sending response with error code: %d", error_code);
    
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

    DEBUG_LOG("Sending message: header size=%zu, response size=%zu, total size=%zu",
           sizeof(header), sizeof(response), total_len);
    DEBUG_LOG("Header: version=%d, type=%d, payload_len=%d",
           header.version, header.type, header.payload_len);
    
    // 发送消息
    ssize_t sent = send(sock, buffer, total_len, MSG_NOSIGNAL);
    if (sent != (ssize_t)total_len) {
        ERROR_LOG("Failed to send response: %s", strerror(errno));
        return -1;
    }
    
    DEBUG_LOG("Response sent successfully");
    return 0;
}

// 处理客户端连接
static void handle_client(ecn_server_t *server, int client_sock) {
    uint8_t buffer[MAX_BUFFER_SIZE];
    ssize_t received;
    
    DEBUG_LOG("Starting to handle client connection");
    
    while (server->running) {
        // 接收消息头
        DEBUG_LOG("Waiting for message header...");
        received = recv(client_sock, buffer, sizeof(ecn_msg_header_t), 0);
        if (received <= 0) {
            if (received == 0) {
                DEBUG_LOG("Client closed connection");
            } else {
                ERROR_LOG("Failed to receive header: %s", strerror(errno));
            }
            break;
        }
        
        DEBUG_LOG("Received %zd bytes for header", received);
        
        // 解析消息头
        ecn_msg_header_t *header = (ecn_msg_header_t *)buffer;
        DEBUG_LOG("Message version: %d, type: %d, payload length: %d",
               header->version, header->type, header->payload_len);
        
        if (header->version != ECN_PROTOCOL_VERSION) {
            ERROR_LOG("Invalid protocol version: %d", header->version);
            send_response(client_sock, ECN_ERR_VERSION, NULL, 0);
            break;
        }
        
        // 接收负载
        if (header->payload_len > 0) {
            if (header->payload_len > MAX_BUFFER_SIZE - sizeof(ecn_msg_header_t)) {
                ERROR_LOG("Payload too large: %d", header->payload_len);
                send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
                break;
            }
            
            DEBUG_LOG("Waiting for payload of %d bytes...", header->payload_len);
            received = recv(client_sock, buffer + sizeof(ecn_msg_header_t), 
                          header->payload_len, 0);
            if (received != header->payload_len) {
                ERROR_LOG("Failed to receive payload: expected %d bytes, got %zd bytes",
                       header->payload_len, received);
                break;
            }
            
            DEBUG_LOG("Received complete payload");
        }
        
        // 处理消息
        DEBUG_LOG("Processing message...");
        if (handle_client_message(server, client_sock, header, 
                                buffer + sizeof(ecn_msg_header_t)) != 0) {
            ERROR_LOG("Failed to handle client message");
            break;
        }
    }
    
    DEBUG_LOG("Client connection closed");
    close(client_sock);
}

// 处理注册请求
static int handle_register(ecn_server_t *server __attribute__((unused)), int client_sock, const uint8_t *payload, size_t len) {
    DEBUG_LOG("Processing registration request, payload size: %zu", len);
    
    if (len < sizeof(ecn_register_req_t)) {
        ERROR_LOG("Invalid request size: %zu (expected: %zu)", len, sizeof(ecn_register_req_t));
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    const ecn_register_req_t *req = (const ecn_register_req_t *)payload;
    ecn_user_t user;
    memset(&user, 0, sizeof(user));
    strncpy(user.username, req->username, sizeof(user.username) - 1);
    
    DEBUG_LOG("Registering user: %s", user.username);
    
    // 生成盐
    if (ecn_generate_random(user.salt, sizeof(user.salt)) != 0) {
        ERROR_LOG("Failed to generate salt");
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }
    
    DEBUG_LOG("Salt generated successfully");
    
    // 生成SM2密钥对
    if (ecn_sm2_generate_keypair(user.public_key, user.private_key) != 0) {
        ERROR_LOG("Failed to generate SM2 keypair");
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }
    
    DEBUG_LOG("SM2 keypair generated successfully");
    
    // 服务器端计算hash
    if (ecn_generate_password_hash(req->password, user.salt, user.password_hash) != 0) {
        ERROR_LOG("Failed to generate password hash");
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }
    
    DEBUG_LOG("Password hash generated successfully");
    
    user.created_at = time(NULL);
    user.last_login = 0;
    
    int rc = ecn_db_user_create(&user);
    if (rc != 0) {
        ERROR_LOG("Failed to create user in database: %d", rc);
        return send_response(client_sock, ECN_ERR_USER_EXISTS, NULL, 0);
    }
    
    DEBUG_LOG("User created successfully with ID: %d", user.id);
    return send_response(client_sock, ECN_ERR_NONE, NULL, 0);
}

// 处理登录请求
static int handle_login(ecn_server_t *server __attribute__((unused)), int client_sock, const uint8_t *payload, size_t len) {
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
static int handle_note_create(ecn_server_t *server __attribute__((unused)), int client_sock, 
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
static int handle_note_update(ecn_server_t *server __attribute__((unused)), int client_sock,
                            uint32_t user_id, const uint8_t *payload, size_t len) {
    if (len < sizeof(ecn_note_update_req_t)) {
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    const ecn_note_update_req_t *req = (const ecn_note_update_req_t *)payload;
    const uint8_t *content_data = payload + sizeof(ecn_note_update_req_t);
    size_t content_len = len - sizeof(ecn_note_update_req_t);

    // 验证内容长度
    if (content_len != req->content_len) {
        return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }

    // 获取原笔记
    ecn_note_t note;
    if (ecn_db_note_get(req->id, &note) != 0) {
        return send_response(client_sock, ECN_ERR_NOT_FOUND, NULL, 0);
    }

    // 验证所有权
    if (note.user_id != user_id) {
        return send_response(client_sock, ECN_ERR_AUTH_FAILED, NULL, 0);
    }

    // 获取用户SM2公钥
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

    // 更新笔记
    free(note.content);
    note.content = encrypted;
    note.content_len = encrypted_len;
    note.updated_at = time(NULL);

    if (ecn_db_note_update(&note) != 0) {
        free(encrypted);
        return send_response(client_sock, ECN_ERR_SERVER, NULL, 0);
    }

    return send_response(client_sock, ECN_ERR_NONE, NULL, 0);
}

// 处理删除笔记请求
static int handle_note_delete(ecn_server_t *server __attribute__((unused)), int client_sock,
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
static int handle_note_list(ecn_server_t *server __attribute__((unused)), int client_sock,
                          uint32_t user_id, const uint8_t *payload __attribute__((unused)), 
                          size_t len __attribute__((unused))) {
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
static int handle_note_get(ecn_server_t *server __attribute__((unused)), int client_sock,
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

// 处理客户端消息
static int handle_client_message(ecn_server_t *server, int client_sock,
                               const ecn_msg_header_t *header,
                               const uint8_t *payload) {
    uint32_t user_id = 0;

    // 检查会话（除了注册和登录请求）
    if (header->type != ECN_MSG_REGISTER && header->type != ECN_MSG_LOGIN) {
        if (verify_session(header->session_token, &user_id) != 0) {
            return send_response(client_sock, ECN_ERR_INVALID_SESSION, NULL, 0);
        }
    }

    // 根据消息类型处理
    switch (header->type) {
        case ECN_MSG_REGISTER:
            return handle_register(server, client_sock, payload, header->payload_len);
        case ECN_MSG_LOGIN:
            return handle_login(server, client_sock, payload, header->payload_len);
        case ECN_MSG_NOTE_CREATE:
            return handle_note_create(server, client_sock, user_id, payload, header->payload_len);
        case ECN_MSG_NOTE_UPDATE:
            return handle_note_update(server, client_sock, user_id, payload, header->payload_len);
        case ECN_MSG_NOTE_DELETE:
            return handle_note_delete(server, client_sock, user_id, payload, header->payload_len);
        case ECN_MSG_NOTE_LIST:
            return handle_note_list(server, client_sock, user_id, payload, header->payload_len);
        case ECN_MSG_NOTE_GET:
            return handle_note_get(server, client_sock, user_id, payload, header->payload_len);
        default:
            return send_response(client_sock, ECN_ERR_INVALID_REQ, NULL, 0);
    }
}

// 初始化服务器
int ecn_server_init(ecn_server_t *server, const ecn_server_config_t *config) {
    // 初始化服务器结构
    memset(server, 0, sizeof(ecn_server_t));
    server->config = *config;
    server->running = 0;
    server->listen_sock = -1;
    
    // 初始化数据库
    if (ecn_db_init(config->db_path) != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return -1;
    }
    
    return 0;
}

// 启动服务器
void ecn_server_start(ecn_server_t *server) {
    struct sockaddr_in server_addr;
    int opt = 1;

    // 保存服务器实例到全局变量
    g_server = server;

    // 创建套接字
    server->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_sock < 0) {
        perror("socket failed");
        return;
    }

    // 设置套接字选项
    if (setsockopt(server->listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server->listen_sock);
        return;
    }

    // 初始化服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server->config.port);

    // 绑定地址
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

    printf("Server listening on port %d\n", server->config.port);
    printf("Server running. Press Ctrl+C to stop.\n");

    // 设置信号处理
    signal(SIGINT, handle_signal);
    server->running = 1;

    // 主循环
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        // 接受新连接
        int client_sock = accept(server->listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept failed");
            break;
        }

        printf("New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));

        // 处理客户端连接
        handle_client(server, client_sock);
    }

    // 清理
    close(server->listen_sock);
    ecn_db_close();
    g_server = NULL;
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
    
    // 等待所有工作线程结束
    for (int i = 0; i < server->config.max_clients; i++) {
        if (server->clients[i].socket >= 0) {
            close(server->clients[i].socket);
            server->clients[i].socket = -1;
        }
    }
    
    printf("Server stopped\n");
}

// 清理服务器资源
void ecn_server_cleanup(ecn_server_t *server) {
    // 确保服务器已停止
    ecn_server_stop(server);
    
    // 清理资源
    ecn_db_close();
    
    memset(server, 0, sizeof(ecn_server_t));
} 