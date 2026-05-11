/*
 * Lightweight crypto primitives for STM32 bare-metal targets.
 * No malloc, no OS, no external dependency.
 *
 * Footprint: SHA-256 ~2 KB, HMAC ~500 B, XOR cipher ~100 B.
 */

#ifndef CRYPTO_LIGHT_H
#define CRYPTO_LIGHT_H

#include <stdint.h>
#include <string.h>

/* SHA-256 */

typedef struct {
    uint32_t state[8];
    uint32_t count[2];
    uint8_t buffer[64];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t hash[32]);
void sha256_hash(const uint8_t *data, size_t len, uint8_t hash[32]);

/* HMAC-SHA256 */

void hmac_sha256(const uint8_t *key, size_t key_len,
                 const uint8_t *data, size_t data_len,
                 uint8_t hmac[32]);

/* XOR stream cipher (key reuse — for obfuscation, not confidentiality) */

void xor_cipher_encrypt(uint8_t *data, size_t data_len,
                       const uint8_t *key, size_t key_len);

void xor_cipher_decrypt(uint8_t *data, size_t data_len,
                       const uint8_t *key, size_t key_len);

/* Base64 */

size_t base64_encode(const uint8_t *src, size_t src_len, char *dst);
size_t base64_decode(const char *src, uint8_t *dst, size_t dst_len);

/* PRNG seeded from ADC noise */

void crypto_random_init(uint16_t adc_seed);
uint32_t crypto_random_get(void);
void crypto_random_bytes(uint8_t *buffer, size_t len);

#endif
