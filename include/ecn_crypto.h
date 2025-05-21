#ifndef ECN_CRYPTO_H
#define ECN_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

// SM3 哈希函数接口
int ecn_sm3_hash(const uint8_t *data, size_t len, uint8_t hash[32]);

// SM4 加密接口
int ecn_sm4_encrypt_ctr(const uint8_t *plaintext, size_t len, 
                       const uint8_t key[16], uint8_t *ciphertext);

// SM4 解密接口
int ecn_sm4_decrypt_ctr(const uint8_t *ciphertext, size_t len,
                       const uint8_t key[16], uint8_t *plaintext);

// SM2 密钥生成
int ecn_sm2_generate_keypair(uint8_t public_key[65], uint8_t private_key[32]);

// SM2 加密
int ecn_sm2_encrypt(const uint8_t *plaintext, size_t len,
                   const uint8_t public_key[65], uint8_t *ciphertext);

// SM2 解密
int ecn_sm2_decrypt(const uint8_t *ciphertext, size_t len,
                   const uint8_t private_key[32], uint8_t *plaintext);

#endif // ECN_CRYPTO_H 