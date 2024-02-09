#pragma once

struct pcg32
{
    u64 state;
    u64 increment;
    u64 seed;

    pcg32();
    pcg32(u64 s);

    void setSeed(u64 s);

    u32 next();


    u32 operator()() { return next(); }

    bool nextBool()
    {
        return (next() & 0x1000) == 0;
    }

    u8 nextByte() { return nextByte(0, 0xFFu); }
    u8 nextByte(u8 min, u8 max)
    {
        return (u8)nextInt(min, max);
    }

    u16 nextShort() { return nextShort(0, 0xFFFFu); }
    u16 nextShort(u16 min, u16 max)
    {
        return (u16)nextInt(min, max);
    }

    u32 nextUInt()
    {
        return next();
    }

    u32 nextUInt(u32 min, u32 max);

    int nextInt()
    {
        return (int)next();
    }

    int nextInt(int min, int max);

    u64 nextLong()
    {
        return ((u64)next()) << 32 | next();
    }

    u64 nextLong(u64 min, u64 max);

    float nextFloat() { return nextFloat(0.0f, 1.0f); }
    float nextFloat(float min, float max)
    {
        return (float)nextDouble(min, max);
    }

    double nextDouble() { return nextDouble(0.0, 1.0); }
    double nextDouble(double min, double max);
};
