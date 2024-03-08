#include "vector_math.h"

#include <cmath>

vec2f createDirectionDeg(float a)
{
    float x = cosf(a * scalar::DEG_2_RADf);
    float y = sinf(a * scalar::DEG_2_RADf);
    return vec2f(x, y);
}

vec2f createDirectionRad(float a)
{
    float x = cosf(a);
    float y = sinf(a);
    return vec2f(x, y);
}
