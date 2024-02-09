#include "random.h"

#include <chrono>

#define PCG_DEFAULT_MULTIPLIER_64  6364136223846793005ULL
#define PCG_DEFAULT_INCREMENT_64   1442695040888963407ULL

pcg32::pcg32() : pcg32(std::chrono::steady_clock::now().time_since_epoch().count()) {}

pcg32::pcg32(uint64_t s)
{
    setSeed(s);
}

void pcg32::setSeed(u64 s)
{
    seed = s;
    increment = PCG_DEFAULT_INCREMENT_64;
    state = seed + increment;
    state = state * PCG_DEFAULT_MULTIPLIER_64 + increment;
}

u32 pcg32::next()
{
    const uint64_t oldstate = state;
    // LCG state update
    state = oldstate * PCG_DEFAULT_MULTIPLIER_64 + increment;
    // Output function (XSH-RR)
    const u32 xorshifted = (u32)(((oldstate >> 18U) ^ oldstate) >> 27U);
    const u32 rot = oldstate >> 59U;
    return (xorshifted >> rot) | (xorshifted << ((~(rot)+1) & 31));
}

u32 pcg32::nextUInt(u32 min, u32 max)
{
    debug_assert(min <= max);
    u32 range = max - min;
    if (range == 0) return min;
    u32 threshold = ((~range) + 1) % range;
    // Here we find the largest interval that is an integer multiple
    // of our required range (to ensure all values are uniformly
    // likely. The worse case is when the required range is just
    // over half the maximum range in which case the generated number
    // has a 50% change of falling within the threshold. Farther from
    // this mid point the change rises towards 100%.
    while (true)
    {
        u32 rand = next();
        if (rand >= threshold)
        {
            return (rand % range) + min;
        }
    }
    // Note: This is dead code, the above always returns eventually.
    return 0;
}

int pcg32::nextInt(int min, int max)
{
    debug_assert(min <= max);
    u32 range = (u32)(s64(max) - min);
    if (range == 0) return min;
    u32 threshold = ((~range) + 1) % range;
    // Here we find the largest interval that is an integer multiple
    // of our required range (to ensure all values are uniformly
    // likely. The worse case is when the required range is just
    // over half the maximum range in which case the generated number
    // has a 50% change of falling within the threshold. Farther from
    // this mid point the change rises towards 100%.
    while (true)
    {
        u32 rand = next();
        if (rand >= threshold)
        {
            return (rand % range) + min;
        }
    }
    // Note: This is dead code, the above always returns eventually.
    return 0;
}

u64 pcg32::nextLong(u64 min, u64 max)
{
    u64 range = max - min;
    if (range > 0xFFFFFFFFU)
    {
        u64 half_range = range / 2;
        return nextLong(min, min + half_range) + nextLong(min + half_range + 1, max);
    }
    return nextInt(0, (u32)range) + min;
}

double pcg32::nextDouble(double min, double max)
{
    // This factor is range / 2^32, since our generator produces values in the range
    // [0, 2^32-1] this will map these values onto the range of [0, range). There are
    // no significant precision issues as the mantissa of the double is 52-bits wide.
    // However, not all possible doubles in the range are able to be returned, only
    // up to 2^32 possibilities are possible (for the [0, 1) case this means that
    // only 1 in a million possible double values are used...).
    const double range = max - min;
    const double factor = range / (((double)UINT64_MAX) + 1.0);
    return nextLong() * factor + min;
}