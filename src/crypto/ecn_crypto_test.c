#include <stdio.h>
#include <string.h>
#include "../../include/ecn_crypto.h"

// 打印十六进制数据
static void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

// 测试SM3哈希
static int test_sm3(void) {
    const char *test_data = "Hello, SM3!";
    uint8_t hash[32];

    printf("\n=== Testing SM3 Hash ===\n");
    if (ecn_sm3_hash((uint8_t *)test_data, strlen(test_data), hash) != 0) {
        printf("SM3 hash failed\n");
        return -1;
    }

    print_hex("Input", (uint8_t *)test_data, strlen(test_data));
    print_hex("Hash", hash, 32);
    return 0;
}

// 测试SM4加解密
static int test_sm4(void) {
    const char *test_data = "Hello, SM4-CTR!";
    uint8_t key[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    uint8_t ciphertext[1024];
    uint8_t decrypted[1024];
    size_t data_len = strlen(test_data);

    printf("\n=== Testing SM4-CTR ===\n");
    
    // 加密
    if (ecn_sm4_encrypt_ctr((uint8_t *)test_data, data_len, key, ciphertext) != 0) {
        printf("SM4 encryption failed\n");
        return -1;
    }

    // 解密
    if (ecn_sm4_decrypt_ctr(ciphertext, data_len + 16, key, decrypted) != 0) {
        printf("SM4 decryption failed\n");
        return -1;
    }

    print_hex("Original", (uint8_t *)test_data, data_len);
    print_hex("Key", key, 16);
    print_hex("Encrypted", ciphertext, data_len + 16);
    print_hex("Decrypted", decrypted, data_len);

    // 验证解密结果
    if (memcmp(test_data, decrypted, data_len) != 0) {
        printf("SM4 decryption result mismatch\n");
        return -1;
    }

    printf("SM4 test passed!\n");
    return 0;
}

// 测试SM2密钥生成和加解密
static int test_sm2(void) {
    const char *test_data = "SM2";
    uint8_t public_key[65];
    uint8_t private_key[32];
    uint8_t ciphertext[1024];
    uint8_t decrypted[1024];
    size_t data_len = strlen(test_data);

    printf("\n=== Testing SM2 ===\n");

    // 生成密钥对
    if (ecn_sm2_generate_keypair(public_key, private_key) != 0) {
        printf("SM2 key generation failed\n");
        return -1;
    }

    print_hex("Public Key", public_key, 65);
    print_hex("Private Key", private_key, 32);

    // SM2加密测试
    size_t ciphertext_len = sizeof(ciphertext);
    if (ecn_sm2_encrypt((uint8_t *)test_data, data_len, public_key, ciphertext, &ciphertext_len) != 0) {
        printf("SM2 encrypt failed\n");
        return -1;
    }
    size_t decrypted_len = sizeof(decrypted);
    if (ecn_sm2_decrypt(ciphertext, ciphertext_len, private_key, decrypted, &decrypted_len) != 0) {
        printf("SM2 decrypt failed\n");
        return -1;
    }

    print_hex("Original", (uint8_t *)test_data, data_len);
    print_hex("Encrypted", ciphertext, ciphertext_len);
    print_hex("Decrypted", decrypted, decrypted_len);

    // 验证解密结果
    if (memcmp(test_data, decrypted, data_len) != 0) {
        printf("SM2 decryption result mismatch\n");
        return -1;
    }

    printf("SM2 test passed!\n");
    return 0;
}

int main() {
    printf("Starting crypto module tests...\n");
    // 运行测试
    if (test_sm3() != 0) {
        printf("SM3 test failed\n");
        return 1;
    }
    if (test_sm4() != 0) {
        printf("SM4 test failed\n");
        return 1;
    }
    if (test_sm2() != 0) {
        printf("SM2 test failed\n");
        return 1;
    }
    printf("\nAll crypto tests passed!\n");
    return 0;
} 