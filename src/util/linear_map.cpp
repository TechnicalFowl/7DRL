#include "linear_map.h"

namespace hash {

    u32 fnv1a(const u8* data, size_t len)
    {
        // FNV-1a with a pcg twist, the loop part is from FNV-1a and the XOR-RR
        // output function from pcg is then used to mix the result.
        //
        // The final result is or'd with 0x80000000 to ensure the hash is
        // non-zero. The high bit is set rather than the low bit as the hash is
        // taken modulo the table_size so setting the low bit to 1 would
        // increase collisions, but the high bit is almost always discarded.
        u64 h = 14695981039346656037U;
        for (u64 i = 0; i < len; i++) {
            h = (h ^ data[i]) * 1099511628211U;
        }
        u32 xorshifted = (u32)(((h >> 18u) ^ h) >> 27u);
        int rot = (int)(h >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31)) | 0x80000000;
    }
}
