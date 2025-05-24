#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define STATE_SIZE 176 // 88 bits X + 88 bits Y
#define RATE 16 // Rate in bytes
#define TAG_SIZE 8 // 8 bytes tag
#define ROUNDS 512

// S-Box (simple example, replace with Quark-specific if available)
static const uint8_t SBOX[16] = {
    0xC, 0x5, 0x6, 0xB,
    0x9, 0x0, 0xA, 0xD,
    0x3, 0xE, 0xF, 0x8,
    0x4, 0x7, 0x1, 0x2
};

// S-box substitution for a nibble array
void sbox_substitution(uint8_t *state, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        state[i] = (SBOX[state[i] >> 4] << 4) | SBOX[state[i] & 0x0F];
    }
}

// Simple permutation layer (pLayer)
void pLayer(uint8_t *state) {
    uint8_t tmp[STATE_SIZE/8] = {0};
    for (int i = 0; i < STATE_SIZE; ++i) {
        int pos = (i * 13) % STATE_SIZE; // example permutation
        int src_byte = i / 8, src_bit = i % 8;
        int dst_byte = pos / 8, dst_bit = pos % 8;
        tmp[dst_byte] |= ((state[src_byte] >> src_bit) & 1) << dst_bit;
    }
    memcpy(state, tmp, STATE_SIZE/8);
}

// Sponge absorb phase
void absorb(uint8_t *state, const uint8_t *input, size_t length) {
    for (size_t i = 0; i < length; i++) {
        state[i] ^= input[i];
        sbox_substitution(state, RATE);
        pLayer(state);
    }
}

// Sponge squeeze phase
void squeeze(uint8_t *state, uint8_t *output, size_t length) {
    for (size_t i = 0; i < length; i++) {
        output[i] = state[i];
        sbox_substitution(state, RATE);
        pLayer(state);
    }
}

// AEAD Encryption + Tag Generation
void quark_aead_encrypt(
    const uint8_t *key, const uint8_t *nonce,
    const uint8_t *plaintext, size_t pt_len,
    uint8_t *ciphertext, uint8_t *tag
) {
    uint8_t state[STATE_SIZE/8] = {0};

    // Absorb key and nonce
    memcpy(state, key, RATE/2);
    memcpy(state + RATE/2, nonce, RATE/2);
    sbox_substitution(state, RATE);
    pLayer(state);

    // Absorb plaintext and encrypt
    for (size_t i = 0; i < pt_len; i++) {
        state[i % RATE] ^= plaintext[i];
        ciphertext[i] = state[i % RATE];
        if ((i % RATE) == (RATE - 1)) {
            sbox_substitution(state, RATE);
            pLayer(state);
        }
    }

    // Padding if needed
    if (pt_len % RATE != 0) {
        sbox_substitution(state, RATE);
        pLayer(state);
    }

    // Squeeze tag
    squeeze(state, tag, TAG_SIZE);
}

void quark_encrypt(const uint8_t *input, size_t len, uint8_t *output, uint8_t *tag, const uint8_t *key, const uint8_t *nonce) {
    uint8_t state[STATE_SIZE / 8] = {0};

    // Absorb key and nonce
    memcpy(state, key, RATE / 2);
    memcpy(state + RATE / 2, nonce, RATE / 2);
    sbox_substitution(state, RATE);
    pLayer(state);

    // Absorb plaintext and encrypt
    for (size_t i = 0; i < len; i++) {
        state[i % RATE] ^= input[i];
        output[i] = state[i % RATE];
        if ((i % RATE) == (RATE - 1)) {
            sbox_substitution(state, RATE);
            pLayer(state);
        }
    }

    // Padding if needed
    if (len % RATE != 0) {
        sbox_substitution(state, RATE);
        pLayer(state);
    }

    // Squeeze tag
    squeeze(state, tag, TAG_SIZE);
}


// Utility to print hex
// void print_hex(const char *label, const uint8_t *data, size_t len) {
//     printf("%s: ", label);
//     for (size_t i = 0; i < len; i++) printf("%02X", data[i]);
//     printf("\n");
// }

#ifdef TEST_QUARK_MAIN
int main() {
    uint8_t key[RATE/2] = {0x00};
    uint8_t nonce[RATE/2] = {0x00};

    // Example key/nonce
    for (int i = 0; i < RATE/2; i++) {
        key[i] = i;
        nonce[i] = i + 0x10;
    }

    const char *msg = "Hello, world!";
    size_t msg_len = strlen(msg);

    uint8_t ciphertext[64] = {0};
    uint8_t tag[TAG_SIZE] = {0};

    clock_t start = clock();

    quark_aead_encrypt(key, nonce, (const uint8_t*)msg, msg_len, ciphertext, tag);

    clock_t end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    print_hex("Ciphertext", ciphertext, msg_len);
    print_hex("Tag", tag, TAG_SIZE);

    // Metrics
    printf("Execution Time: %.6f seconds\n", elapsed_time);
    printf("Throughput: %.2f bytes/sec\n", msg_len / elapsed_time);
    printf("Static RAM Usage: %lu bytes\n", sizeof(key) + sizeof(nonce) + sizeof(ciphertext) + sizeof(tag));

    return 0;
}
#endif