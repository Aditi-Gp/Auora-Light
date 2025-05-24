#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BLOCK_SIZE 16
#define RATE 16
#define CAPACITY 16
#define KEY_SIZE 16
#define NONCE_SIZE 16
#define TAG_SIZE 16
#define STATE_SIZE (RATE + CAPACITY)

// PHOTON-256 S-box (4-bit)
static const uint8_t SBOX[16] = {
    0xc, 0x5, 0x6, 0xb, 0x9, 0x0, 0xa, 0xd,
    0x3, 0xe, 0xf, 0x8, 0x4, 0x7, 0x1, 0x2
};

// State structure
typedef struct {
    uint8_t state[STATE_SIZE];
    uint8_t buffer[RATE];
    size_t buf_len;
} photon_beetle_ctx;

void sbox_substitution(uint8_t *state) {
    for (int i = 0; i < STATE_SIZE; i++) {
        state[i] = (SBOX[state[i] >> 4] << 4) | SBOX[state[i] & 0x0f];
    }
}

void pLayer(uint8_t *state) {
    // PHOTON's bit permutation layer
    uint8_t temp[STATE_SIZE];
    for (int i = 0; i < STATE_SIZE; i++) {
        temp[i] = 0;
        for (int j = 0; j < 8; j++) {
            int pos = (i * 8 + j) % (STATE_SIZE * 8);
            temp[pos / 8] |= ((state[i] >> j) & 1) << (pos % 8);
        }
    }
    memcpy(state, temp, STATE_SIZE);
}

void permute(uint8_t *state, int rounds) {
    for (int r = 0; r < rounds; r++) {
        sbox_substitution(state);
        pLayer(state);
    }
}

void absorb(photon_beetle_ctx *ctx, const uint8_t *data, size_t len, uint8_t domain) {
    size_t blocks = len / RATE;
    for (size_t i = 0; i < blocks; i++) {
        for (int j = 0; j < RATE; j++)
            ctx->state[j] ^= data[i * RATE + j];
        permute(ctx->state, 12);
    }
    
    // Process final block
    size_t remaining = len % RATE;
    if (remaining > 0 || len == 0) {
        for (size_t j = 0; j < remaining; j++)
            ctx->state[j] ^= data[blocks * RATE + j];
        ctx->state[remaining] ^= 0x01; // Padding
        permute(ctx->state, 12);
    }
}

void encrypt(photon_beetle_ctx *ctx, const uint8_t *pt, uint8_t *ct, size_t pt_len) {
    size_t blocks = pt_len / RATE;
    for (size_t i = 0; i < blocks; i++) {
        permute(ctx->state, 6);
        for (int j = 0; j < RATE; j++) {
            ct[i * RATE + j] = ctx->state[j] ^ pt[i * RATE + j];
            ctx->state[j] = ct[i * RATE + j];
        }
    }
    
    // Process final block
    size_t remaining = pt_len % RATE;
    if (remaining > 0) {
        permute(ctx->state, 6);
        for (size_t j = 0; j < remaining; j++) {
            ct[blocks * RATE + j] = ctx->state[j] ^ pt[blocks * RATE + j];
            ctx->state[j] = ct[blocks * RATE + j];
        }
        ctx->state[remaining] ^= 0x01; // Padding
    }
}

void generate_tag(photon_beetle_ctx *ctx, uint8_t *tag) {
    permute(ctx->state, 12);
    memcpy(tag, ctx->state, TAG_SIZE);
}

#ifdef TEST_PHOTON_BEELTE_MAIN
int main() {
    clock_t start = clock();
    
    // Initialize context
    photon_beetle_ctx ctx;
    uint8_t key[KEY_SIZE] = {0};
    uint8_t nonce[NONCE_SIZE] = {0};
    uint8_t ad[] = {0}; // Empty associated data
    uint8_t pt[] = "Hello, world!";
    uint8_t ct[sizeof(pt)] = {0};
    uint8_t tag[TAG_SIZE] = {0};
    
    // Initialize state
    memset(&ctx, 0, sizeof(ctx));
    memcpy(ctx.state, nonce, NONCE_SIZE);
    memcpy(ctx.state + NONCE_SIZE, key, KEY_SIZE);
    permute(ctx.state, 12);
    
    // Process associated data
    absorb(&ctx, ad, sizeof(ad), 0x02);
    
    // Encrypt plaintext
    encrypt(&ctx, pt, ct, sizeof(pt)-1);
    
    // Generate tag
    generate_tag(&ctx, tag);
    
    // Calculate metrics
    clock_t end = clock();
    double latency = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    double throughput = (sizeof(pt)-1) / (latency / 1000.0);
    
    // Output results
    printf("Ciphertext: ");
    for (size_t i = 0; i < sizeof(pt)-1; i++)
        printf("%02x", ct[i]);
    
    printf("\nTag: ");
    for (int i = 0; i < TAG_SIZE; i++)
        printf("%02x", tag[i]);
    
    printf("\n\nIoT Metrics:");
    printf("\n- Latency: %.2f ms", latency);
    printf("\n- Throughput: %.2f bytes/sec", throughput);
    printf("\n- RAM Usage: %zu bytes (static allocations)",
           sizeof(ctx) + sizeof(key) + sizeof(nonce) + sizeof(ct) + sizeof(tag));
    
    return 0;
}
#endif