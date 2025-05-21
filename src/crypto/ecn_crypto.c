#include <string.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include "../../include/ecn_crypto.h"

// SM3哈希计算实现
int ecn_sm3_hash(const uint8_t *data, size_t len, uint8_t hash[32]) {
    EVP_MD_CTX *ctx;
    unsigned int hash_len;

    // 创建哈希上下文
    ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return -1;
    }

    // 初始化SM3
    if (!EVP_DigestInit_ex(ctx, EVP_sm3(), NULL)) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    // 更新数据
    if (!EVP_DigestUpdate(ctx, data, len)) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    // 完成哈希计算
    if (!EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    EVP_MD_CTX_free(ctx);
    return 0;
}

// SM4-CTR模式加密实现
int ecn_sm4_encrypt_ctr(const uint8_t *plaintext, size_t len,
                       const uint8_t key[16], uint8_t *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int outlen, tmplen;
    uint8_t iv[16] = {0}; // 初始化向量

    // 生成随机IV
    if (!RAND_bytes(iv, sizeof(iv))) {
        return -1;
    }

    // 创建加密上下文
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }

    // 初始化SM4-CTR
    if (!EVP_EncryptInit_ex(ctx, EVP_sm4_ctr(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // 复制IV到密文开头
    memcpy(ciphertext, iv, 16);

    // 加密数据
    if (!EVP_EncryptUpdate(ctx, ciphertext + 16, &outlen, plaintext, len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // 完成加密
    if (!EVP_EncryptFinal_ex(ctx, ciphertext + 16 + outlen, &tmplen)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

// SM4-CTR模式解密实现
int ecn_sm4_decrypt_ctr(const uint8_t *ciphertext, size_t len,
                       const uint8_t key[16], uint8_t *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int outlen, tmplen;
    const uint8_t *iv = ciphertext; // IV存储在密文开头

    // 检查长度
    if (len <= 16) {
        return -1;
    }

    // 创建解密上下文
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }

    // 初始化SM4-CTR
    if (!EVP_DecryptInit_ex(ctx, EVP_sm4_ctr(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // 解密数据
    if (!EVP_DecryptUpdate(ctx, plaintext, &outlen, ciphertext + 16, len - 16)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // 完成解密
    if (!EVP_DecryptFinal_ex(ctx, plaintext + outlen, &tmplen)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

// SM2密钥对生成实现
int ecn_sm2_generate_keypair(uint8_t public_key[65], uint8_t private_key[32]) {
    EC_KEY *key = NULL;
    const EC_GROUP *group = NULL;
    const EC_POINT *pub_point = NULL;
    const BIGNUM *priv_bn = NULL;
    BN_CTX *ctx = NULL;
    int ret = -1;

    // 创建SM2密钥
    key = EC_KEY_new_by_curve_name(NID_sm2);
    if (!key) {
        fprintf(stderr, "Failed to create EC_KEY\n");
        goto cleanup;
    }

    // 生成密钥对
    if (!EC_KEY_generate_key(key)) {
        fprintf(stderr, "Failed to generate key pair\n");
        goto cleanup;
    }

    // 获取私钥
    priv_bn = EC_KEY_get0_private_key(key);
    if (!priv_bn) {
        fprintf(stderr, "Failed to get private key\n");
        goto cleanup;
    }

    // 获取公钥点
    pub_point = EC_KEY_get0_public_key(key);
    group = EC_KEY_get0_group(key);
    if (!pub_point || !group) {
        fprintf(stderr, "Failed to get public key point\n");
        goto cleanup;
    }

    // 创建BN_CTX
    ctx = BN_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Failed to create BN_CTX\n");
        goto cleanup;
    }

    // 导出私钥
    if (BN_bn2binpad(priv_bn, private_key, 32) != 32) {
        fprintf(stderr, "Failed to export private key\n");
        goto cleanup;
    }

    // 导出公钥（未压缩格式：0x04 || x || y）
    public_key[0] = 0x04;  // 未压缩格式标记
    if (!EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_UNCOMPRESSED,
                           public_key, 65, ctx)) {
        fprintf(stderr, "Failed to export public key\n");
        goto cleanup;
    }

    ret = 0;

cleanup:
    if (ret != 0) {
        ERR_print_errors_fp(stderr);
    }
    EC_KEY_free(key);
    BN_CTX_free(ctx);
    return ret;
}

// SM2加密实现
int ecn_sm2_encrypt(const uint8_t *plaintext, size_t len,
                   const uint8_t public_key[65], uint8_t *ciphertext) {
    EC_KEY *key = NULL;
    const EC_GROUP *group = NULL;
    EC_POINT *pub_point = NULL;
    int ret = -1;

    // 创建SM2密钥
    key = EC_KEY_new_by_curve_name(NID_sm2);
    if (!key) {
        fprintf(stderr, "Failed to create EC_KEY\n");
        goto cleanup;
    }

    // 获取群
    group = EC_KEY_get0_group(key);
    if (!group) {
        fprintf(stderr, "Failed to get group\n");
        goto cleanup;
    }

    // 创建公钥点
    pub_point = EC_POINT_new(group);
    if (!pub_point) {
        fprintf(stderr, "Failed to create point\n");
        goto cleanup;
    }

    // 从字节数组导入公钥点
    if (!EC_POINT_oct2point(group, pub_point, public_key, 65, NULL)) {
        fprintf(stderr, "Failed to import public key\n");
        goto cleanup;
    }

    // 设置公钥
    if (!EC_KEY_set_public_key(key, pub_point)) {
        fprintf(stderr, "Failed to set public key\n");
        goto cleanup;
    }

    // 加密数据
    memcpy(ciphertext, plaintext, len);  // 临时：直接复制数据
    ret = 0;

cleanup:
    if (ret != 0) {
        ERR_print_errors_fp(stderr);
    }
    EC_POINT_free(pub_point);
    EC_KEY_free(key);
    return ret;
}

// SM2解密实现
int ecn_sm2_decrypt(const uint8_t *ciphertext, size_t len,
                   const uint8_t private_key[32], uint8_t *plaintext) {
    EC_KEY *key = NULL;
    BIGNUM *priv_bn = NULL;
    int ret = -1;

    // 创建SM2密钥
    key = EC_KEY_new_by_curve_name(NID_sm2);
    if (!key) {
        fprintf(stderr, "Failed to create EC_KEY\n");
        goto cleanup;
    }

    // 导入私钥
    priv_bn = BN_bin2bn(private_key, 32, NULL);
    if (!priv_bn) {
        fprintf(stderr, "Failed to convert private key\n");
        goto cleanup;
    }

    // 设置私钥
    if (!EC_KEY_set_private_key(key, priv_bn)) {
        fprintf(stderr, "Failed to set private key\n");
        goto cleanup;
    }

    // 解密数据
    memcpy(plaintext, ciphertext, len);  // 临时：直接复制数据
    ret = 0;

cleanup:
    if (ret != 0) {
        ERR_print_errors_fp(stderr);
    }
    BN_free(priv_bn);
    EC_KEY_free(key);
    return ret;
} 