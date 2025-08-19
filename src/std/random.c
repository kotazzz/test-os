#include "random.h"

// Initialize XORShift with seed
void xorshift_seed(xorshift_state_t* rng, uint64_t seed) {
    // Ensure seed is not zero (XORShift requires non-zero state)
    rng->state = seed ? seed : 1;
}

// Generate next random 64-bit number using XORShift64*
uint64_t xorshift_next(xorshift_state_t* rng) {
    rng->state ^= rng->state >> 12;
    rng->state ^= rng->state << 25;
    rng->state ^= rng->state >> 27;
    return rng->state * 0x2545F4914F6CDD1DULL;
}

// Generate random number in range [0, max)
uint64_t xorshift_range(xorshift_state_t* rng, uint64_t max) {
    if (max == 0) return 0;
    return xorshift_next(rng) % max;
}

// Generate random 32-bit number
uint32_t xorshift_next32(xorshift_state_t* rng) {
    return (uint32_t)xorshift_next(rng);
}

// Generate random number in range [min, max]
uint64_t xorshift_between(xorshift_state_t* rng, uint64_t min, uint64_t max) {
    if (min >= max) return min;
    return min + xorshift_range(rng, max - min + 1);
}