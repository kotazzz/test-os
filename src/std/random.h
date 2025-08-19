#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

// XORShift state structure
typedef struct {
    uint64_t state;
} xorshift_state_t;

// Initialize XORShift with seed
void xorshift_seed(xorshift_state_t* rng, uint64_t seed);

// Generate next random 64-bit number
uint64_t xorshift_next(xorshift_state_t* rng);

// Generate random number in range [0, max)
uint64_t xorshift_range(xorshift_state_t* rng, uint64_t max);

// Generate random 32-bit number
uint32_t xorshift_next32(xorshift_state_t* rng);

// Generate random number in range [min, max]
uint64_t xorshift_between(xorshift_state_t* rng, uint64_t min, uint64_t max);

#endif // RANDOM_H