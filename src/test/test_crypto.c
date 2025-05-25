#include <stdio.h>
#include <string.h>
#include "../../include/ecn_crypto.h"

// 测试SM3哈希函数
static int test_sm3_hash() {
    printf("Testing SM3 hash function... ");
    
    const char *test_data = "Hello, World!";
    uint8_t hash[32];
    
    if (ecn_calculate_sm3((uint8_t*)test_data, strlen(test_data), hash) != 0) {
        printf("Failed to calculate hash\n");
        return -1;
    }

    // 验证哈希值非空
    int all_zero = 1;
    for (int i = 0; i < 32; i++) {
        if (hash[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    
    if (all_zero) {
        printf("Hash is all zeros\n");
        return -1;
    }

    printf("OK\n");
    return 0;
}

// 测试SM4密钥生成
static int test_sm4_key_generation() {
    printf("Testing SM4 key generation... ");
    
    uint8_t key[16];
    if (ecn_generate_sm4_key(key) != 0) {
        printf("Failed to generate key\n");
        return -1;
    }

    // 验证密钥非空
    int all_zero = 1;
    for (int i = 0; i < 16; i++) {
        if (key[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    
    if (all_zero) {
        printf("Key is all zeros\n");
        return -1;
    }

    printf("OK\n");
    return 0;
}

// 测试SM4加密和解密
static int test_sm4_encryption_decryption() {
    printf("Testing SM4 encryption and decryption... ");
    
    const char *test_data = "This is a test message for encryption and decryption!";
    size_t data_len = strlen(test_data);
    uint8_t key[16];
    
    // 生成密钥
    if (ecn_generate_sm4_key(key) != 0) {
        printf("Failed to generate key\n");
        return -1;
    }
    
    // 加密数据
    uint8_t *encrypted;
    size_t encrypted_len;
    if (ecn_sm4_encrypt(key, (uint8_t*)test_data, data_len,
                       &encrypted, &encrypted_len) != 0) {
        printf("Encryption failed\n");
        return -1;
    }
    
    // 解密数据
    uint8_t *decrypted;
    size_t decrypted_len;
    if (ecn_sm4_decrypt(key, encrypted, encrypted_len,
                       &decrypted, &decrypted_len) != 0) {
        printf("Decryption failed\n");
        free(encrypted);
        return -1;
    }
    
    // 验证解密结果
    if (decrypted_len != data_len ||
        memcmp(test_data, decrypted, data_len) != 0) {
        printf("Decrypted data does not match original\n");
        free(encrypted);
        free(decrypted);
        return -1;
    }
    
    free(encrypted);
    free(decrypted);
    printf("OK\n");
    return 0;
}

// 测试密码哈希和验证
static int test_password_hash() {
    printf("Testing password hash and verification... ");
    
    const char *password = "MySecurePassword123!";
    uint8_t salt[16];
    uint8_t hash[32];
    
    // 生成密码哈希
    if (ecn_generate_password_hash(password, salt, hash) != 0) {
        printf("Failed to generate password hash\n");
        return -1;
    }
    
    // 验证正确密码
    if (ecn_verify_password(password, salt, hash) != 0) {
        printf("Failed to verify correct password\n");
        return -1;
    }
    
    // 验证错误密码
    if (ecn_verify_password("WrongPassword", salt, hash) == 0) {
        printf("Incorrectly verified wrong password\n");
        return -1;
    }
    
    printf("OK\n");
    return 0;
}

// 主测试函数
int test_crypto() {
    int failed = 0;
    
    // 测试SM3哈希
    if (test_sm3_hash() != 0) failed++;
    
    // 测试SM4密钥生成
    if (test_sm4_key_generation() != 0) failed++;
    
    // 测试SM4加密解密
    if (test_sm4_encryption_decryption() != 0) failed++;
    
    // 测试密码哈希
    if (test_password_hash() != 0) failed++;
    
    return failed;
} 