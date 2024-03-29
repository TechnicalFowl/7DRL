#include "scalar_math.h"

#include <cmath>

namespace scalar
{

    float wrapAngleDeg(float angle) {
        float a = (float)fmod(angle, 360);
        return a < 0 ? a + 360 : a;
    }

    float wrapAngleRad(float angle) {
        float a = (float)fmod(angle, 2 * PI);
        return a < 0 ? a + 2 * PIf : a;
    }

    float getDegreeDiff(float a, float b) {
        float a0 = wrapAngleDeg(a);
        float b0 = wrapAngleDeg(b);
        float d = abs(b0 - a0);
        return d > 180 ? 360 - d : d;
    }

    float getRadianDiff(float a, float b) {
        float a0 = wrapAngleRad(a);
        float b0 = wrapAngleRad(b);
        float d = abs(b0 - a0);
        return d > PIf ? 2 * PIf - d : d;
    }

    u32 flipEndian(u32 v)
    {
        v = (v << 16) | (v >> 16);
        v = ((v & 0xFF00FF00) >> 8) | ((v & 0x00FF00FF) << 8);
        return v;
    }

    u64 flipEndian(u64 v)
    {
        v = (v << 32) | (v >> 32);
        v = ((v & 0xFFFF0000FFFF0000) >> 16) | ((v & 0x0000FFFF0000FFFF) << 16);
        v = ((v & 0xFF00FF00FF00FF00) >> 8) | ((v & 0x00FF00FF00FF00FF) << 8);
        return v;
    }

    u32 convertToGrayscale(u32 color, float mag)
    {
        u32 r = (color >> 16) & 0xff;
        u32 g = (color >> 8) & 0xff;
        u32 b = color & 0xff;

        float gray = (0.2126f * (r / 255.0f) + 0.7152f * (g / 255.0f) + 0.0722f * (b / 255.0f)) * mag;
        u32 grayColor = scalar::floori(gray * 255.0f);
        return (color & 0xFF000000) | (grayColor << 16) | (grayColor << 8) | grayColor;
    }
}