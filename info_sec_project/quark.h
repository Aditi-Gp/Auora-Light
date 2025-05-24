#ifndef QUARK_H
#define QUARK_H

#include <stddef.h>
#include <stdint.h>

// 88 bits X + 88 bits Y
#define RATE 16           // 16 bytes (128 bits) rate
   // 8 bytes tag
#define ROUNDS 512        // Placeholder (unused currently)
#ifndef STATE_SIZE
#define STATE_SIZE 176
#endif

#ifndef TAG_SIZE
#define TAG_SIZE 8
#endif

void quark_encrypt(const uint8_t *input, size_t len, uint8_t *output, uint8_t *tag, const uint8_t *key, const uint8_t *nonce);

void quark_aead_encrypt(
    const uint8_t *key,
    const uint8_t *nonce,
    const uint8_t *plaintext,
    size_t pt_len,
    uint8_t *ciphertext,
    uint8_t *tag
);

// void print_hex(const char *label, const uint8_t *data, size_t len);

#endif // QUARK_H
