#ifndef ECN_CRYPTO_H
#define ECN_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

// SM2密钥对生成
int ecn_sm2_generate_keypair(uint8_t public_key[65], uint8_t private_key[32]);

// SM2加密
int ecn_sm2_encrypt(const uint8_t *plaintext, size_t len,
                   const uint8_t public_key[65], uint8_t *ciphertext,
                   size_t *ciphertext_len);

// SM2解密
int ecn_sm2_decrypt(const uint8_t *ciphertext, size_t len,
                   const uint8_t private_key[32], uint8_t *plaintext,
                   size_t *plaintext_len);

// SM3哈希计算
int ecn_sm3_hash(const uint8_t *data, size_t len, uint8_t hash[32]);

// SM4密钥生成
int ecn_sm4_generate_key(uint8_t key[16]);

// SM4-CTR加密
int ecn_sm4_encrypt_ctr(const uint8_t *plaintext, size_t len,
                       const uint8_t key[16], uint8_t *ciphertext);

// SM4-CTR解密
int ecn_sm4_decrypt_ctr(const uint8_t *ciphertext, size_t len,
                       const uint8_t key[16], uint8_t *plaintext);

// 生成密码哈希（使用SM3+盐值）
int ecn_generate_password_hash(const char *password, uint8_t salt[16],
                             uint8_t hash[32]);

// 验证密码（使用SM3+盐值）
int ecn_verify_password(const char *password, const uint8_t salt[16],
                       const uint8_t stored_hash[32]);

// 混合加密：使用SM2加密SM4密钥，使用SM4加密数据
int ecn_hybrid_encrypt(const uint8_t *data, size_t data_len,
                      const uint8_t sm2_public_key[65],
                      uint8_t **encrypted, size_t *encrypted_len);

// 混合解密：使用SM2解密SM4密钥，使用SM4解密数据
int ecn_hybrid_decrypt(const uint8_t *encrypted, size_t encrypted_len,
                      const uint8_t sm2_private_key[32],
                      uint8_t **decrypted, size_t *decrypted_len);

// 生成随机字节
int ecn_generate_random(uint8_t *buffer, size_t len);

#endif // ECN_CRYPTO_H 