// spongent.h

#ifndef SPONGENT_H
#define SPONGENT_H

#include <stdint.h>

#include <stddef.h>

#define SPONGENT_STATE_SIZE (272 / 8)
#define SPONGENT_HASH_SIZE (256 / 8)

typedef unsigned char uchar;

// Public function to compute hash using SPONGENT
void spongent(uchar *input, uchar *output);

// Utility function to get current time in microseconds
uint64_t get_time_microseconds(void);

// extern void sbox_substitution(uint8_t *state, size_t length);
extern void pLayer(uint8_t *state);
extern void absorb(uint8_t *state, const uint8_t *input, size_t length);
extern void print_hex(const char *label, const uint8_t *data, size_t len);

#endif // SPONGENT_H
