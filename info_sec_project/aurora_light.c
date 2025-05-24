#include "aurora_light.h"
#include <string.h>

// --- Utility macros for ARX operations ---
static inline uint64_t ROTL64(uint64_t x, unsigned r) {
    return (x << r) | (x >> (64 - r));
}
static inline uint64_t ROTR64(uint64_t x, unsigned r) {
    return (x >> r) | (x << (64 - r));
}

// --- ARX-only permutation on all 4 lanes ---
// rounds between AURORA_MIN_ROUNDS and AURORA_MAX_ROUNDS
static void aurora_permute(uint64_t S[AURORA_LANES], unsigned rounds) {
    const uint64_t C = 0x9e3779b97f4a7c15ULL;  // golden ratio constant
    for (unsigned r = 0; r < rounds; r++) {
        for (int i = 0; i < AURORA_LANES; i++) {
            uint64_t x = S[i];
            x = x + ROTL64(x, 13);
            x ^= ROTR64(x, 7);
            x = x + C;
            S[i] = x;
        }
    }
}

// --- Duplex-absorb of up to 3 lanes (rate) ---
// XOR data into lanes 0..2 then permute
static void aurora_absorb(uint64_t S[AURORA_LANES],
                          const uint8_t *in, size_t len,
                          unsigned rounds) {
    size_t off = 0;
    while (len >= AURORA_RATE_BYTES) {
        // full 192-bit block
        for (int i = 0; i < 3; i++) {
            uint64_t v = 0;
            memcpy(&v, in + off + 8*i, 8);
            S[i] ^= v;
        }
        aurora_permute(S, rounds);
        off += AURORA_RATE_BYTES;
        len -= AURORA_RATE_BYTES;
    }
    // partial block + padding
    if (len > 0) {
        uint8_t buf[AURORA_RATE_BYTES] = {0};
        memcpy(buf, in + off, len);
        buf[len] = 0x80;
        for (int i = 0; i < 3; i++) {
            uint64_t v = 0;
            memcpy(&v, buf + 8*i, 8);
            S[i] ^= v;
        }
        aurora_permute(S, rounds);
    }
}

// --- Duplex-squeeze of up to 3 lanes (rate) ---
// write lanes 0..2 to out, then permute
static void aurora_squeeze(uint64_t S[AURORA_LANES],
                           uint8_t *out, size_t len,
                           unsigned rounds) {
    size_t off = 0;
    while (len > 0) {
        size_t chunk = (len > AURORA_RATE_BYTES ? AURORA_RATE_BYTES : len);
        for (int i = 0; i < 3; i++) {
            memcpy(out + off + 8*i, &S[i], 8);
        }
        off += chunk;
        len -= chunk;
        aurora_permute(S, rounds);
    }
}

// --- Public API ---

void aurora_init(aurora_ctx_t *ctx,
                 const uint8_t key[16],
                 const uint8_t nonce[16]) {
    // Zero state
    memset(ctx->S, 0, sizeof(ctx->S));
    // Absorb key || nonce (total 32 bytes) in one duplex call
    uint8_t kn[32];
    memcpy(kn, key, 16);
    memcpy(kn+16, nonce, 16);
    aurora_absorb(ctx->S, kn, 32, AURORA_MIN_ROUNDS/2);
}

void aurora_encrypt(aurora_ctx_t *ctx,
                    const uint8_t *ad, size_t ad_len,
                    const uint8_t *pt, size_t pt_len,
                    uint8_t *ct,
                    uint8_t tag[AURORA_TAG_BYTES]) {
    // 1) Absorb associated data
    if (ad && ad_len)
        aurora_absorb(ctx->S, ad, ad_len, AURORA_MIN_ROUNDS);

    // 2) Encrypt: duplex-squeeze gives keystream XOR plaintext
    size_t off = 0;
    while (off < pt_len) {
        unsigned rounds = AURORA_MIN_ROUNDS +
            (unsigned)((pt_len - off)/64);
        if (rounds > AURORA_MAX_ROUNDS) rounds = AURORA_MAX_ROUNDS;
        uint8_t buf[AURORA_RATE_BYTES];
        aurora_squeeze(ctx->S, buf, AURORA_RATE_BYTES, rounds);
        size_t chunk = (pt_len - off < AURORA_RATE_BYTES
                        ? pt_len - off
                        : AURORA_RATE_BYTES);
        for (size_t i = 0; i < chunk; i++)
            ct[off+i] = pt[off+i] ^ buf[i];
        off += chunk;
    }
    // 3) Finalize tag: squeeze one lane (lane 3) only
    aurora_permute(ctx->S, AURORA_MAX_ROUNDS);
    memcpy(tag, &ctx->S[3], AURORA_TAG_BYTES);
}

bool aurora_decrypt(aurora_ctx_t *ctx,
                    const uint8_t *ad, size_t ad_len,
                    const uint8_t *ct, size_t ct_len,
                    const uint8_t tag[AURORA_TAG_BYTES],
                    uint8_t *pt) {
    // 1) Absorb AD
    if (ad && ad_len)
        aurora_absorb(ctx->S, ad, ad_len, AURORA_MIN_ROUNDS);
    // 2) Decrypt: regenerate keystream
    size_t off = 0;
    while (off < ct_len) {
        unsigned rounds = AURORA_MIN_ROUNDS +
            (unsigned)((ct_len - off)/64);
        if (rounds > AURORA_MAX_ROUNDS) rounds = AURORA_MAX_ROUNDS;
        uint8_t buf[AURORA_RATE_BYTES];
        aurora_squeeze(ctx->S, buf, AURORA_RATE_BYTES, rounds);
        size_t chunk = (ct_len - off < AURORA_RATE_BYTES
                        ? ct_len - off
                        : AURORA_RATE_BYTES);
        for (size_t i = 0; i < chunk; i++)
            pt[off+i] = ct[off+i] ^ buf[i];
        off += chunk;
    }
    // 3) Tag check
    aurora_permute(ctx->S, AURORA_MAX_ROUNDS);
    uint8_t expect[AURORA_TAG_BYTES];
    memcpy(expect, &ctx->S[3], AURORA_TAG_BYTES);
    return (memcmp(expect, tag, AURORA_TAG_BYTES) == 0);
}
