#pragma once

/**
 * RandomMapper.h — Seeded bijective permutation utilities.
 *
 * Given a seed and a domain size N, produces a pseudo-random
 * permutation of [0, N) that is:
 *   - Deterministic (same seed → same mapping)
 *   - Bijective (every input maps to a unique output)
 *   - Efficient (O(1) per lookup via format-preserving encryption)
 *
 * Used by all hook modules to decide "block X renders as block Y",
 * "item slot A shows item B", etc.
 */

#include <cstdint>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

namespace personalized {

class RandomMapper {
public:
    /**
     * Construct a mapper for domain [0, size) with the given seed.
     * Uses a Feistel-network format-preserving encryption to create
     * a bijective map without materializing the full permutation table.
     */
    RandomMapper(uint64_t seed, size_t size);

    /// Map index i ∈ [0, size) to its permuted position
    size_t map(size_t i) const;

    /// Inverse: find the index that maps to j
    size_t unmap(size_t j) const;

    /// Get the domain size
    size_t size() const { return m_size; }

    /**
     * Convenience: build a full shuffled vector using Fisher-Yates.
     * Use this for small domains (e.g. creative inventory ~900 items)
     * where O(N) materialization is fine and you need random access.
     */
    static std::vector<size_t> fisherYatesShuffle(uint64_t seed, size_t n, int passes = 1);

    /**
     * Intensity-filtered partial scramble:
     * Only remap a fraction `intensity` of elements; the rest stay fixed.
     * Returns a vector v where v[i] = i for unswapped items.
     */
    static std::vector<size_t> partialScramble(
        uint64_t seed,
        size_t n,
        double intensity,
        int passes = 1
    );

private:
    size_t   m_size;
    uint64_t m_seed;

    // Feistel round keys (4 rounds)
    static constexpr int kRounds = 4;
    uint64_t m_keys[kRounds];

    /// Single Feistel round function: hash right half with round key
    uint64_t feistelF(uint64_t right, uint64_t roundKey) const;

    /// Apply kRounds of Feistel network to produce a permuted index
    size_t feistelEncrypt(size_t index) const;

    /// Reverse the Feistel network (same structure, keys in reverse)
    size_t feistelDecrypt(size_t index) const;
};

} // namespace personalized
