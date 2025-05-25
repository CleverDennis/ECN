#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../include/ecn_server.h"
#include "../../include/ecn_protocol.h"

// 模拟客户端结构
typedef struct {
    int sock;
    uint8_t session_token[64];
    uint32_t user_id;
} test_client_t;

// 测试配置
static const char *TEST_USERNAME = "testuser";
static const char *TEST_PASSWORD = "testpass123";
static const int TEST_PORT = 8899;

// 创建测试客户端连接
static test_client_t* create_test_client() {
    test_client_t *client = malloc(sizeof(test_client_t));
    if (!client) return NULL;

    client->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client->sock < 0) {
        free(client);
        return NULL;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client->sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(client->sock);
        free(client);
        return NULL;
    }

    memset(client->session_token, 0, sizeof(client->session_token));
    client->user_id = 0;

    return client;
}

// 清理测试客户端
static void cleanup_test_client(test_client_t *client) {
    if (client) {
        close(client->sock);
        free(client);
    }
}

// 测试用户注册
static int test_user_registration(test_client_t *client) {
    printf("Testing user registration... ");

    // 构造注册请求
    uint8_t buffer[1024];
    ecn_msg_header_t *header = (ecn_msg_header_t*)buffer;
    ecn_register_req_t *req = (ecn_register_req_t*)(buffer + sizeof(ecn_msg_header_t));

    header->version = ECN_PROTOCOL_VERSION;
    header->type = ECN_MSG_REGISTER;
    header->payload_len = sizeof(ecn_register_req_t);
    memset(header->session_token, 0, sizeof(header->session_token));

    strncpy(req->username, TEST_USERNAME, sizeof(req->username) - 1);
    ecn_generate_password_hash(TEST_PASSWORD, req->salt, req->password_hash);

    // 发送请求
    if (send(client->sock, buffer, sizeof(ecn_msg_header_t) + sizeof(ecn_register_req_t), 0) < 0) {
        printf("Failed to send registration request\n");
        return -1;
    }

    // 接收响应
    memset(buffer, 0, sizeof(buffer));
    if (recv(client->sock, buffer, sizeof(buffer), 0) < sizeof(ecn_msg_header_t)) {
        printf("Failed to receive response\n");
        return -1;
    }

    header = (ecn_msg_header_t*)buffer;
    ecn_response_t *resp = (ecn_response_t*)(buffer + sizeof(ecn_msg_header_t));

    if (resp->error_code != ECN_ERR_NONE) {
        printf("Registration failed with error code %d\n", resp->error_code);
        return -1;
    }

    printf("OK\n");
    return 0;
}

// 测试用户登录
static int test_user_login(test_client_t *client) {
    printf("Testing user login... ");

    // 构造登录请求
    uint8_t buffer[1024];
    ecn_msg_header_t *header = (ecn_msg_header_t*)buffer;
    ecn_login_req_t *req = (ecn_login_req_t*)(buffer + sizeof(ecn_msg_header_t));

    header->version = ECN_PROTOCOL_VERSION;
    header->type = ECN_MSG_LOGIN;
    header->payload_len = sizeof(ecn_login_req_t);
    memset(header->session_token, 0, sizeof(header->session_token));

    strncpy(req->username, TEST_USERNAME, sizeof(req->username) - 1);
    ecn_generate_password_hash(TEST_PASSWORD, req->salt, req->password_hash);

    // 发送请求
    if (send(client->sock, buffer, sizeof(ecn_msg_header_t) + sizeof(ecn_login_req_t), 0) < 0) {
        printf("Failed to send login request\n");
        return -1;
    }

    // 接收响应
    memset(buffer, 0, sizeof(buffer));
    if (recv(client->sock, buffer, sizeof(buffer), 0) < sizeof(ecn_msg_header_t)) {
        printf("Failed to receive response\n");
        return -1;
    }

    header = (ecn_msg_header_t*)buffer;
    ecn_response_t *resp = (ecn_response_t*)(buffer + sizeof(ecn_msg_header_t));

    if (resp->error_code != ECN_ERR_NONE) {
        printf("Login failed with error code %d\n", resp->error_code);
        return -1;
    }

    // 保存会话令牌
    memcpy(client->session_token, resp->data, sizeof(client->session_token));

    printf("OK\n");
    return 0;
}

// 测试创建笔记
static int test_create_note(test_client_t *client) {
    printf("Testing note creation... ");

    const char *test_title = "Test Note";
    const char *test_content = "This is a test note content.";

    // 构造创建笔记请求
    size_t content_len = strlen(test_content);
    size_t total_len = sizeof(ecn_msg_header_t) + sizeof(ecn_note_create_req_t) + content_len;
    uint8_t *buffer = malloc(total_len);
    if (!buffer) {
        printf("Failed to allocate memory\n");
        return -1;
    }

    ecn_msg_header_t *header = (ecn_msg_header_t*)buffer;
    ecn_note_create_req_t *req = (ecn_note_create_req_t*)(buffer + sizeof(ecn_msg_header_t));

    header->version = ECN_PROTOCOL_VERSION;
    header->type = ECN_MSG_NOTE_CREATE;
    header->payload_len = sizeof(ecn_note_create_req_t) + content_len;
    memcpy(header->session_token, client->session_token, sizeof(header->session_token));

    strncpy(req->title, test_title, sizeof(req->title) - 1);
    req->content_len = content_len;
    memcpy((uint8_t*)(req + 1), test_content, content_len);

    // 发送请求
    if (send(client->sock, buffer, total_len, 0) < 0) {
        printf("Failed to send create note request\n");
        free(buffer);
        return -1;
    }

    // 接收响应
    memset(buffer, 0, total_len);
    if (recv(client->sock, buffer, total_len, 0) < sizeof(ecn_msg_header_t)) {
        printf("Failed to receive response\n");
        free(buffer);
        return -1;
    }

    header = (ecn_msg_header_t*)buffer;
    ecn_response_t *resp = (ecn_response_t*)(buffer + sizeof(ecn_msg_header_t));

    if (resp->error_code != ECN_ERR_NONE) {
        printf("Note creation failed with error code %d\n", resp->error_code);
        free(buffer);
        return -1;
    }

    // 保存笔记ID和密钥
    struct {
        uint32_t note_id;
        uint8_t key[16];
    } *response_data = (void*)resp->data;
    client->user_id = response_data->note_id;  // 用于后续测试

    free(buffer);
    printf("OK\n");
    return 0;
}

// 测试获取笔记列表
static int test_list_notes(test_client_t *client) {
    printf("Testing note listing... ");

    // 构造获取笔记列表请求
    uint8_t buffer[1024];
    ecn_msg_header_t *header = (ecn_msg_header_t*)buffer;

    header->version = ECN_PROTOCOL_VERSION;
    header->type = ECN_MSG_NOTE_LIST;
    header->payload_len = 0;
    memcpy(header->session_token, client->session_token, sizeof(header->session_token));

    // 发送请求
    if (send(client->sock, buffer, sizeof(ecn_msg_header_t), 0) < 0) {
        printf("Failed to send list notes request\n");
        return -1;
    }

    // 接收响应
    memset(buffer, 0, sizeof(buffer));
    if (recv(client->sock, buffer, sizeof(buffer), 0) < sizeof(ecn_msg_header_t)) {
        printf("Failed to receive response\n");
        return -1;
    }

    header = (ecn_msg_header_t*)buffer;
    ecn_response_t *resp = (ecn_response_t*)(buffer + sizeof(ecn_msg_header_t));

    if (resp->error_code != ECN_ERR_NONE) {
        printf("List notes failed with error code %d\n", resp->error_code);
        return -1;
    }

    printf("OK\n");
    return 0;
}

// 测试服务器启动和停止
static int test_server_lifecycle() {
    printf("Testing server lifecycle... ");

    // 创建服务器配置
    ecn_server_config_t config = {
        .port = TEST_PORT,
        .max_clients = 10,
        .db_path = "test.db"
    };

    // 创建服务器实例
    ecn_server_t server;
    if (ecn_server_init(&server, &config) != 0) {
        printf("Failed to initialize server\n");
        return -1;
    }

    // 启动服务器
    ecn_server_start(&server);

    // 等待服务器启动
    sleep(1);

    // 创建测试客户端
    test_client_t *client = create_test_client();
    if (!client) {
        printf("Failed to create test client\n");
        ecn_server_stop(&server);
        ecn_server_cleanup(&server);
        return -1;
    }

    // 运行功能测试
    int failed = 0;
    if (test_user_registration(client) != 0) failed++;
    if (test_user_login(client) != 0) failed++;
    if (test_create_note(client) != 0) failed++;
    if (test_list_notes(client) != 0) failed++;

    // 清理客户端
    cleanup_test_client(client);

    // 停止服务器
    ecn_server_stop(&server);
    ecn_server_cleanup(&server);

    if (failed > 0) {
        printf("Failed (%d tests failed)\n", failed);
        return -1;
    }

    printf("OK\n");
    return 0;
}

// 主测试函数
int test_server() {
    return test_server_lifecycle();
} 