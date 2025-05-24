#ifndef PHOTON_BEETLE_H
#define PHOTON_BEETLE_H

#include <stdint.h>
#include <stddef.h>

// Constants
#define BLOCK_SIZE  16
#define RATE        16
#define CAPACITY    16
#define KEY_SIZE    16
#define NONCE_SIZE  16
#ifndef STATE_SIZE
#define STATE_SIZE (RATE + CAPACITY)
#endif

#ifndef TAG_SIZE
#define TAG_SIZE 16
#endif


// Context structure for Photon-Beetle
typedef struct {
    uint8_t state[STATE_SIZE];
    uint8_t buffer[RATE];
    size_t buf_len;
} photon_beetle_ctx;

// Function declarations

// Applies S-box substitution to the state
extern void sbox_substitution(uint8_t *state);

// Applies the permutation layer to the state
extern void pLayer(uint8_t *state);

// Applies rounds of permutation
extern void permute(uint8_t *state, int rounds);

// Absorbs associated data or plaintext into the state
void absorb(photon_beetle_ctx *ctx, const uint8_t *data, size_t len, uint8_t domain);

// Encrypts plaintext into ciphertext and updates the state
void encrypt(photon_beetle_ctx *ctx, const uint8_t *pt, uint8_t *ct, size_t pt_len);

// Generates authentication tag from the state
void generate_tag(photon_beetle_ctx *ctx, uint8_t *tag);

// extern void print_hex(const char *label, const uint8_t *data, size_t len);
#endif // PHOTON_BEETLE_H
