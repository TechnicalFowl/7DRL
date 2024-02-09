#pragma once

#ifdef OS_WINDOWS
#   include <intrin.h>
#endif

template <typename T> struct float_type { using type = T; };
template <> struct float_type<s8> { using type = float; };
template <> struct float_type<s16> { using type = float; };
template <> struct float_type<s32> { using type = float; };
template <> struct float_type<s64> { using type = double; };

namespace scalar
{
    constexpr double EULER = 2.718281828459045;
    constexpr double PI = 3.141592653589793;
    constexpr float PIf = 3.141592653589793f;
    constexpr double DEG_2_RAD = 0.017453292519943295;
    constexpr float DEG_2_RADf = 0.017453292519943295f;
    constexpr double RAD_2_DEG = 57.29577951308232;
    constexpr double SQRT_2 = 1.414213562373095145;
    constexpr double PHI = 1.6180339887498948482; // golden ratio

    template<typename T, typename Y>
    T min(T a, Y b) { return a < b ? a : b; }

    template<typename T, typename Y>
    T max(T a, Y b) { return a > b ? a : b; }

    template<typename T>
    T clamp(T val, T min, T max) {
        if (val < min) return min;
        if (val > max) return max;
        return val;
    }

    template <typename T>
    T mod(T val, T mod)
    {
        if (val < 0)    return (T)(val + ceil(-val / (double)mod) * mod);
        if (val >= mod)  return (T)(val - floor(val / mod) * mod);
        return val;
    }

    template<typename T>
    float_type<T>::type lerp(T a, T b, float pct)
    {
        return (float_type<T>::type)((1 - pct) * a + pct * b);
    }

    template<typename T>
    int constexpr floori(T f)
    {
        s32 i = (s32)f;
        return f < i ? i - 1 : i;
    }

    template<typename T>
    int constexpr ceili(T f)
    {
        s32 i = (s32)f;
        return f < i ? i : i + 1;
    }

    inline u32 floatToIntBits(float f)
    {
        return *((u32*)&f);
    }

    inline float intBitsToFloat(u32 bits)
    {
        return *((float*)&bits);
    }

    inline u64 doubleToLongBits(double f)
    {
        return *((u64*)&f);
    }

    inline double longBitsToDouble(u64 bits)
    {
        return *((double*)&bits);
    }

    inline float fast_sqrt(float val)
    {
        float x = float(val * 0.5);
        u32 i = floatToIntBits(val);
        i = 0x5f3759df - (i >> 1);
        float y = intBitsToFloat(i);
        // More iterations raises quality
        y = float(y * (1.5f - (x * y * y)));
        y = float(y * (1.5f - (x * y * y)));
        return val * y;
    }

    inline double fastInverseSqrt(double a)
    {
        const double halfA = 0.5 * a;
        a = longBitsToDouble(0x5FE6EB50C7B537AAl - (doubleToLongBits(a) >> 1));
        return a * (1.5 - halfA * a * a);
    }

    // Fast square root approximation, valid to approximately 2 decimal places for small values.
    inline double fastSqrt(double a)
    {
        return a * fastInverseSqrt(a);
    }

    // Rounds up to the next power of 2. If the number is already a power of 2, it is returned unchanged.
    // Valid for up to 0x80000000, higher values return INT32_MAX. Zero returns 1.
    inline u32 roundUpPow2(u32 a) {
        if (a == 0) return 1;
        a--;
        a |= a >> 1;
        a |= a >> 2;
        a |= a >> 4;
        a |= a >> 8;
        a |= a >> 16;
        if (a != 0xFFFFFFFF) a++;
        return a;
    }
    inline u64 roundUpPow2(u64 a) {
        if (a == 0) return 1;
        a--;
        a |= a >> 1;
        a |= a >> 2;
        a |= a >> 4;
        a |= a >> 8;
        a |= a >> 16;
        a |= a >> 32;
        if (a != 0xFFFFFFFFFFFFFFFF) a++;
        return a;
    }

    inline int bsf(u64 val)
    {
        unsigned long result = 0;
#ifdef OS_WINDOWS
        if (!_BitScanForward64(&result, val))
        {
            return -1;
        }
#else
        result = __builtin_ffsll(val) - 1;
#endif
        return (int)result;
    }

    inline int bsr(u64 val)
    {
        unsigned long result = 0;
#ifdef OS_WINDOWS
        if (!_BitScanReverse64(&result, val))
        {
            return -1;
        }
#else
        if (val == 0) return -1;
        result = __builtin_clzll(val);
#endif
        return (int)result;
    }

    float wrapAngleDeg(float angle);
    float wrapAngleRad(float angle);

    float getDegreeDiff(float a, float b);
    float getRadianDiff(float a, float b);

    u32 flipEndian(u32 val);
    u64 flipEndian(u64 val);

} // namespace scalar