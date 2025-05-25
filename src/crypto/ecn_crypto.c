#include <string.h>
#include <stdlib.h>
#include <gmssl/sm2.h>
#include <gmssl/sm3.h>
#include <gmssl/sm4.h>
#include <gmssl/rand.h>
#include <gmssl/error.h>
#include <gmssl/sm2_z256.h>
#include "../../include/ecn_crypto.h"

// 生成随机字节
int ecn_generate_random(uint8_t *buffer, size_t len) {
    return rand_bytes(buffer, len) == 1 ? 0 : -1;
}

// SM2密钥对生成
int ecn_sm2_generate_keypair(uint8_t public_key[65], uint8_t private_key[32]) {
    SM2_KEY key;
    if (sm2_key_generate(&key) != 1) {
        return -1;
    }
    // 导出私钥
    memcpy(private_key, key.private_key, 32);
    // 导出公钥（未压缩格式：0x04 || x || y）
    public_key[0] = 0x04;
    sm2_z256_point_to_bytes(&key.public_key, public_key + 1);
    return 0;
}

// SM2加密
int ecn_sm2_encrypt(const uint8_t *plaintext, size_t len,
                   const uint8_t public_key[65], uint8_t *ciphertext,
                   size_t *ciphertext_len) {
    SM2_KEY key;
    SM2_Z256_POINT pub;
    // 跳过0x04前缀，导入公钥
    if (sm2_z256_point_from_bytes(&pub, public_key + 1) != 1) {
        return -1;
    }
    if (sm2_key_set_public_key(&key, &pub) != 1) {
        return -1;
    }
    if (sm2_encrypt(&key, plaintext, len, ciphertext, ciphertext_len) != 1) {
        return -1;
    }
    return 0;
}

// SM2解密
int ecn_sm2_decrypt(const uint8_t *ciphertext, size_t len,
                   const uint8_t private_key[32], uint8_t *plaintext,
                   size_t *plaintext_len) {
    SM2_KEY key;
    sm2_z256_t priv;
    memcpy(priv, private_key, 32);
    if (sm2_key_set_private_key(&key, priv) != 1) {
        return -1;
    }
    if (sm2_decrypt(&key, ciphertext, len, plaintext, plaintext_len) != 1) {
        return -1;
    }
    return 0;
}

// SM3哈希计算
int ecn_sm3_hash(const uint8_t *data, size_t len, uint8_t hash[32]) {
    SM3_CTX ctx;
    
    sm3_init(&ctx);
    sm3_update(&ctx, data, len);
    sm3_finish(&ctx, hash);

    return 0;
}

// SM4密钥生成
int ecn_sm4_generate_key(uint8_t key[16]) {
    return ecn_generate_random(key, 16);
}

// SM4-CTR加密
int ecn_sm4_encrypt_ctr(const uint8_t *plaintext, size_t len,
                       const uint8_t key[16], uint8_t *ciphertext) {
    SM4_KEY sm4_key;
    uint8_t ctr[16] = {0};
    size_t blocks = (len + 15) / 16;
    
    // 生成随机IV（计数器初值）
    if (ecn_generate_random(ctr, 16) != 0) {
        return -1;
    }

    // 设置密钥
    sm4_set_encrypt_key(&sm4_key, key);

    // 复制IV到密文开头
    memcpy(ciphertext, ctr, 16);

    // CTR模式加密
    for (size_t i = 0; i < blocks; i++) {
        uint8_t block[16] = {0};
        uint8_t keystream[16];
        size_t block_len = (i == blocks - 1 && len % 16) ? len % 16 : 16;

        // 生成密钥流
        sm4_encrypt(&sm4_key, ctr, keystream);

        // 更新计数器
        for (int j = 15; j >= 0; j--) {
            if (++ctr[j]) break;
        }

        // 复制当前块
        memcpy(block, plaintext + i * 16, block_len);

        // 异或运算
        for (size_t j = 0; j < block_len; j++) {
            ciphertext[16 + i * 16 + j] = block[j] ^ keystream[j];
        }
    }

    return 0;
}

// SM4-CTR解密
int ecn_sm4_decrypt_ctr(const uint8_t *ciphertext, size_t len,
                       const uint8_t key[16], uint8_t *plaintext) {
    if (len <= 16) return -1;  // 至少需要IV

    SM4_KEY sm4_key;
    uint8_t ctr[16];
    size_t blocks = (len - 16 + 15) / 16;
    
    // 获取IV
    memcpy(ctr, ciphertext, 16);

    // 设置密钥
    sm4_set_encrypt_key(&sm4_key, key);

    // CTR模式解密（与加密相同）
    for (size_t i = 0; i < blocks; i++) {
        uint8_t keystream[16];
        size_t block_len = (i == blocks - 1 && (len - 16) % 16) ? (len - 16) % 16 : 16;

        // 生成密钥流
        sm4_encrypt(&sm4_key, ctr, keystream);

        // 更新计数器
        for (int j = 15; j >= 0; j--) {
            if (++ctr[j]) break;
        }

        // 异或运算
        for (size_t j = 0; j < block_len; j++) {
            plaintext[i * 16 + j] = ciphertext[16 + i * 16 + j] ^ keystream[j];
        }
    }

    return 0;
}

// 生成密码哈希（使用SM3+盐值）
int ecn_generate_password_hash(const char *password, uint8_t salt[16],
                             uint8_t hash[32]) {
    // 生成随机盐值
    if (rand_bytes(salt, 16) != 1) {
        return -1;
    }

    // 组合密码和盐值
    size_t password_len = strlen(password);
    uint8_t *combined = malloc(password_len + 16);
    if (!combined) {
        return -1;
    }

    memcpy(combined, password, password_len);
    memcpy(combined + password_len, salt, 16);

    // 计算SM3哈希
    int ret = ecn_sm3_hash(combined, password_len + 16, hash);
    
    free(combined);
    return ret;
}

// 验证密码（使用SM3+盐值）
int ecn_verify_password(const char *password, const uint8_t salt[16],
                       const uint8_t stored_hash[32]) {
    uint8_t calculated_hash[32];
    
    // 组合密码和盐值
    size_t password_len = strlen(password);
    uint8_t *combined = malloc(password_len + 16);
    if (!combined) {
        return -1;
    }

    memcpy(combined, password, password_len);
    memcpy(combined + password_len, salt, 16);

    // 计算SM3哈希
    int ret = ecn_sm3_hash(combined, password_len + 16, calculated_hash);
    free(combined);
    
    if (ret != 0) {
        return -1;
    }

    // 比较哈希值
    return memcmp(calculated_hash, stored_hash, 32) == 0 ? 0 : -1;
}

// 混合加密：使用SM2加密SM4密钥，使用SM4加密数据
int ecn_hybrid_encrypt(const uint8_t *data, size_t data_len,
                      const uint8_t sm2_public_key[65],
                      uint8_t **encrypted, size_t *encrypted_len) {
    uint8_t sm4_key[16];
    uint8_t *encrypted_key = NULL;
    size_t encrypted_key_len;
    int ret = -1;

    // 生成SM4密钥
    if (ecn_sm4_generate_key(sm4_key) != 0) {
        goto cleanup;
    }

    // 分配空间用于加密的SM4密钥
    encrypted_key = malloc(256); // SM2加密输出的最大长度
    if (!encrypted_key) {
        goto cleanup;
    }

    // 使用SM2加密SM4密钥
    if (ecn_sm2_encrypt(sm4_key, 16, sm2_public_key, encrypted_key, &encrypted_key_len) != 0) {
        goto cleanup;
    }

    // 分配空间用于最终的密文（加密的密钥长度 + 4字节长度 + SM4密文）
    *encrypted_len = encrypted_key_len + 4 + data_len + 16; // +16 for SM4 IV
    *encrypted = malloc(*encrypted_len);
    if (!*encrypted) {
        goto cleanup;
    }

    // 写入加密的密钥长度
    (*encrypted)[0] = (encrypted_key_len >> 24) & 0xFF;
    (*encrypted)[1] = (encrypted_key_len >> 16) & 0xFF;
    (*encrypted)[2] = (encrypted_key_len >> 8) & 0xFF;
    (*encrypted)[3] = encrypted_key_len & 0xFF;

    // 写入加密的密钥
    memcpy(*encrypted + 4, encrypted_key, encrypted_key_len);

    // 使用SM4加密数据
    if (ecn_sm4_encrypt_ctr(data, data_len, sm4_key, *encrypted + 4 + encrypted_key_len) != 0) {
        free(*encrypted);
        goto cleanup;
    }

    ret = 0;

cleanup:
    free(encrypted_key);
    return ret;
}

// 混合解密：使用SM2解密SM4密钥，使用SM4解密数据
int ecn_hybrid_decrypt(const uint8_t *encrypted, size_t encrypted_len,
                      const uint8_t sm2_private_key[32],
                      uint8_t **decrypted, size_t *decrypted_len) {
    uint8_t sm4_key[16];
    size_t encrypted_key_len;
    int ret = -1;

    // 读取加密的密钥长度
    if (encrypted_len < 4) {
        return -1;
    }
    encrypted_key_len = (encrypted[0] << 24) | (encrypted[1] << 16) |
                       (encrypted[2] << 8) | encrypted[3];

    // 检查长度合法性
    if (encrypted_len < 4 + encrypted_key_len + 16) { // +16 for SM4 IV
        return -1;
    }

    // 使用SM2解密SM4密钥
    size_t key_len = 16;
    if (ecn_sm2_decrypt(encrypted + 4, encrypted_key_len,
                       sm2_private_key, sm4_key, &key_len) != 0 || key_len != 16) {
        return -1;
    }

    // 分配空间用于解密后的数据
    *decrypted_len = encrypted_len - 4 - encrypted_key_len - 16; // -16 for SM4 IV
    *decrypted = malloc(*decrypted_len);
    if (!*decrypted) {
        return -1;
    }

    // 使用SM4解密数据
    if (ecn_sm4_decrypt_ctr(encrypted + 4 + encrypted_key_len,
                           encrypted_len - 4 - encrypted_key_len,
                           sm4_key, *decrypted) != 0) {
        free(*decrypted);
        return -1;
    }

    return 0;
}