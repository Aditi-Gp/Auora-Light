#ifndef AURORA_LIGHT_H
#define AURORA_LIGHT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// State is 256 bits = 4 × 64-bit lanes
#define AURORA_LANES      4
#define AURORA_WORD_BITS  64
#define AURORA_TAG_BYTES  (AURORA_WORD_BITS/8)   // 8 bytes tag
#define AURORA_RATE_BITS  192                    // 3 lanes × 64 bits
#define AURORA_RATE_BYTES (AURORA_RATE_BITS/8)
#define AURORA_CAP_BYTES  (AURORA_WORD_BITS/8)

// Security parameters
#define AURORA_MIN_ROUNDS 6
#define AURORA_MAX_ROUNDS 12

// Context holds 256-bit state
typedef struct {
    uint64_t S[AURORA_LANES];
} aurora_ctx_t;

// Initialize with 128-bit key and 128-bit nonce
void aurora_init(aurora_ctx_t *ctx,
                 const uint8_t key[16],
                 const uint8_t nonce[16]);

// AEAD encrypt: plaintext → ciphertext + tag
// ad/ad_len may be NULL/0 if no associated data
void aurora_encrypt(aurora_ctx_t *ctx,
                    const uint8_t *ad, size_t ad_len,
                    const uint8_t *pt, size_t pt_len,
                    uint8_t *ct,
                    uint8_t tag[AURORA_TAG_BYTES]);

// AEAD decrypt with tag verification; returns true if tag OK
bool aurora_decrypt(aurora_ctx_t *ctx,
                    const uint8_t *ad, size_t ad_len,
                    const uint8_t *ct, size_t ct_len,
                    const uint8_t tag[AURORA_TAG_BYTES],
                    uint8_t *pt);

#endif // AURORA_LIGHT_H
