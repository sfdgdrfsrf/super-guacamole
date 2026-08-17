/**
 * RandomMapper.cpp — Feistel-network FPE and Fisher-Yates implementations.
 */

#include "RandomMapper.h"
#include "Utils/Logger.h"
#include <cassert>

namespace personalized {

// ─────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────
RandomMapper::RandomMapper(uint64_t seed, size_t size)
    : m_size(size), m_seed(seed)
{
    // Derive independent round keys from the master seed
    std::mt19937_64 rng(seed);
    for (int i = 0; i < kRounds; ++i) {
        m_keys[i] = rng();
    }
    PZ_LOG_TRACE("RandomMapper created: size={}, seed=0x{:016X}", size, seed);
}

// ─────────────────────────────────────────────
//  Feistel round function (FNV-1a based)
// ─────────────────────────────────────────────
uint64_t RandomMapper::feistelF(uint64_t right, uint64_t roundKey) const {
    uint64_t h = 0xcbf29ce484222325ULL;
    // Hash right half
    for (int i = 0; i < 8; ++i) {
        h ^= (right >> (i * 8)) & 0xFF;
        h *= 0x100000001b3ULL;
    }
    // XOR in round key
    h ^= roundKey;
    h *= 0x100000001b3ULL;
    // Avalanche
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    return h;
}

// ─────────────────────────────────────────────
//  Feistel encrypt (forward permutation)
// ─────────────────────────────────────────────
size_t RandomMapper::feistelEncrypt(size_t index) const {
    if (m_size <= 1) return index;

    // Split into two halves based on bit width of domain
    int bits = 0;
    {
        size_t s = m_size - 1;
        while (s > 0) { s >>= 1; ++bits; }
    }

    int leftBits  = bits / 2;
    int rightBits = bits - leftBits;
    uint64_t leftMask  = (1ULL << leftBits) - 1;
    uint64_t rightMask = (1ULL << rightBits) - 1;

    uint64_t left  = (index >> rightBits) & leftMask;
    uint64_t right = index & rightMask;

    // 4-round Feistel
    for (int r = 0; r < kRounds; ++r) {
        uint64_t newLeft = right;
        uint64_t fOut = feistelF(right, m_keys[r]);
        // Keep fOut within left domain
        right = (left ^ (fOut & leftMask)) & rightMask;
        left = newLeft;
    }

    size_t result = static_cast<size_t>((left << rightBits) | right);

    // Reduce into domain [0, m_size) via cycle walking
    while (result >= m_size) {
        result = feistelEncrypt(result);  // recursive walk
    }

    return result;
}

// ─────────────────────────────────────────────
//  Feistel decrypt (inverse permutation)
// ─────────────────────────────────────────────
size_t RandomMapper::feistelDecrypt(size_t index) const {
    if (m_size <= 1) return index;

    int bits = 0;
    {
        size_t s = m_size - 1;
        while (s > 0) { s >>= 1; ++bits; }
    }

    int leftBits  = bits / 2;
    int rightBits = bits - leftBits;
    uint64_t leftMask  = (1ULL << leftBits) - 1;
    uint64_t rightMask = (1ULL << rightBits) - 1;

    uint64_t left  = (index >> rightBits) & leftMask;
    uint64_t right = index & rightMask;

    // Reverse rounds
    for (int r = kRounds - 1; r >= 0; --r) {
        uint64_t newRight = left;
        uint64_t fOut = feistelF(left, m_keys[r]);
        left = (right ^ (fOut & leftMask)) & leftMask;
        right = newRight;
    }

    size_t result = static_cast<size_t>((left << rightBits) | right);
    while (result >= m_size) {
        result = feistelDecrypt(result);
    }

    return result;
}

// ─────────────────────────────────────────────
//  Public map / unmap
// ─────────────────────────────────────────────
size_t RandomMapper::map(size_t i) const {
    if (i >= m_size) {
        PZ_LOG_ERROR("RandomMapper::map index {} out of range [0, {})", i, m_size);
        return i;
    }
    return feistelEncrypt(i);
}

size_t RandomMapper::unmap(size_t j) const {
    if (j >= m_size) {
        PZ_LOG_ERROR("RandomMapper::unmap index {} out of range [0, {})", j, m_size);
        return j;
    }
    return feistelDecrypt(j);
}

// ─────────────────────────────────────────────
//  Fisher-Yates shuffle (materialized)
// ─────────────────────────────────────────────
std::vector<size_t> RandomMapper::fisherYatesShuffle(uint64_t seed, size_t n, int passes) {
    PZ_LOG_DEBUG("Fisher-Yates shuffle: n={}, passes={}, seed=0x{:016X}", n, passes, seed);

    std::vector<size_t> perm(n);
    for (size_t i = 0; i < n; ++i) perm[i] = i;

    std::mt19937_64 rng(seed);

    for (int p = 0; p < passes; ++p) {
        for (size_t i = n; i > 1; --i) {
            std::uniform_int_distribution<size_t> dist(0, i - 1);
            size_t j = dist(rng);
            std::swap(perm[i - 1], perm[j]);
        }
    }

    PZ_LOG_TRACE("Shuffle complete: perm[0..4] = [{}, {}, {}, {}, {}]",
                 (n > 0 ? perm[0] : 0), (n > 1 ? perm[1] : 0),
                 (n > 2 ? perm[2] : 0), (n > 3 ? perm[3] : 0),
                 (n > 4 ? perm[4] : 0));

    return perm;
}

// ─────────────────────────────────────────────
//  Partial scramble (intensity filter)
// ─────────────────────────────────────────────
std::vector<size_t> RandomMapper::partialScramble(
    uint64_t seed,
    size_t n,
    double intensity,
    int passes
) {
    PZ_LOG_DEBUG("Partial scramble: n={}, intensity={:.2f}, passes={}", n, intensity, passes);

    if (intensity <= 0.0) {
        // Identity
        std::vector<size_t> perm(n);
        for (size_t i = 0; i < n; ++i) perm[i] = i;
        return perm;
    }

    if (intensity >= 1.0) {
        return fisherYatesShuffle(seed, n, passes);
    }

    // Decide which indices participate in the shuffle
    std::mt19937_64 rng(seed);
    std::vector<size_t> perm(n);
    for (size_t i = 0; i < n; ++i) perm[i] = i;

    // Collect swap candidates
    std::vector<size_t> candidates;
    candidates.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < intensity) {
            candidates.push_back(i);
        }
    }

    PZ_LOG_DEBUG("Selected {}/{} indices for scrambling ({:.1f}%)",
                 candidates.size(), n, 100.0 * candidates.size() / n);

    // Shuffle just the candidate values
    for (int p = 0; p < passes; ++p) {
        for (size_t k = candidates.size(); k > 1; --k) {
            std::uniform_int_distribution<size_t> dist(0, k - 1);
            size_t j = dist(rng);
            std::swap(perm[candidates[k - 1]], perm[candidates[j]]);
        }
    }

    return perm;
}

} // namespace personalized
