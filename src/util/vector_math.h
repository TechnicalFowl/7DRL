#pragma once

#include <cmath>
#include <cstddef>
#include <cstdio>

#include "scalar_math.h"

template <typename T>
struct vec2
{
    T x;
    T y;

    vec2() : x(0), y(0) {}
    vec2(T x, T y) : x(x), y(y) {}

    vec2(const vec2&) = default;
    vec2(vec2&&) noexcept = default;
    vec2& operator=(const vec2&) = default;
    vec2& operator=(vec2&&) noexcept = default;

    template <typename U>
    vec2<U> cast() const { return vec2<U>(static_cast<U>(::round(x)), static_cast<U>(::round(y))); }

    bool operator==(const vec2<T>& o) const { return equals(o); }
    bool operator!=(const vec2<T>& o) const { return !equals(o); }

    bool equals(const vec2<T>& o, T tolerance = T(1e-6)) const
    {
        return ::abs(x - o.x) <= tolerance && ::abs(y - o.y) <= tolerance;
    }

    T operator[](s32 i) const
    {
        debug_assert(i <= 1 && i >= 0);
        const T* v = &x;
        return v[i];
    }

    T& operator[](s32 i)
    {
        switch (i) {
        case 0: return x;
        case 1: return y;
        }
        debug_assert(false);
        return x;
    }

    vec2<T> operator-() const { return vec2<T>(-x, -y); }

    friend vec2<T> operator+(const vec2<T>& a, vec2<T> b)
    {
        b += a;
        return b;
    }
    vec2<T>& add(T x0, T y0)
    {
        x += x0;
        y += y0;
        return *this;
    }
    vec2<T>& operator+=(const vec2<T>& b)
    {
        add(b.x, b.y);
        return *this;
    }

    friend vec2<T> operator-(vec2<T> a, const vec2<T>& b)
    {
        a -= b;
        return a;
    }
    vec2<T>& sub(T x0, T y0)
    {
        x -= x0;
        y -= y0;
        return *this;
    }
    vec2<T>& operator-=(const vec2<T>& b)
    {
        sub(b.x, b.y);
        return *this;
    }

    friend vec2<T> operator*(const vec2<T> a, T b)
    {
        return vec2<T>(a.x * b, a.y * b);
    }
    friend vec2<T> operator*(T b, const vec2<T> a)
    {
        return vec2<T>(a.x * b, a.y * b);
    }
    friend vec2<T> operator*(vec2<T> a, const vec2<T>& b)
    {
        a.x *= b.x;
        a.y *= b.y;
        return a;
    }
    vec2<T>& mul(T x0, T y0)
    {
        x *= x0;
        y *= y0;
        return *this;
    }
    vec2<T>& operator*=(T b)
    {
        x *= b;
        y *= b;
        return *this;
    }

    friend vec2<T> operator/(const vec2<T> a, T b)
    {
        return vec2<T>(a.x / b, a.y / b);
    }
    friend vec2<T> operator/(vec2<T> a, const vec2<T>& b)
    {
        a.x /= b.x;
        a.y /= b.y;
        return a;
    }
    vec2<T>& div(T x0, T y0)
    {
        x /= x0;
        y /= y0;
        return *this;
    }
    vec2<T>& operator/=(T b)
    {
        x /= b;
        y /= b;
        return *this;
    }

    bool zero() const
    {
        return ::abs(x) < 1e-6 && ::abs(y) < 1e-6;
    }

    // Returns the length of this vector.
    float_type<T>::type length() const
    {
        return static_cast<float_type<T>::type>(sqrt(x * x + y * y));
    }

    // Returns the squared length of this vector.
    T lengthSquared() const
    {
        return x * x + y * y;
    }

    // Normalizes this vector.
    vec2<T>& normalize()
    {
        if (x == 0 && y == 0) return *this;
        const double len = length();
        x = (T)(x / len);
        y = (T)(y / len);
        return *this;
    }

    // Normalizes this vector.
    vec2<T>& normalize(double new_length)
    {
        if (x == 0 && y == 0) return *this;
        const double len = length() / new_length;
        x = (T)(x / len);
        y = (T)(y / len);
        return *this;
    }

    // Replaces this vector with its absolute value.
    vec2<T>& abs()
    {
        if (x < T(0)) x = -x;
        if (y < T(0)) y = -y;
        return *this;
    }

    // Returns the dot product of this vector and the given vector.
    // When both vectors are normalized, the dot product is the cosine of the angle between them.
    T dot(const vec2<T>& b) const
    {
        return x * b.x + y * b.y;
    }

    // Returns true if this vector is orthogonal to the given vector.
    bool isOrthogonal(const vec2<T>& b) const
    {
        return dot(b) < 1e-6;
    }

    // Returns an orthogonal vector to this vector. The result is not normalized.
    // The result is not unique; any vector orthogonal to this vector is a valid result.
    vec2<T> orthogonal() const
    {
        return vec2<T>(-y, x);
    }

    // Projects this vector onto the given vector.
    vec2<T> project(const vec2<T> v) const
    {
        const T len_squared = lengthSquared();
        if (len_squared < 1e-6) {
            fprintf(stderr, "Cannot project onto zero-length vector.\n");
            return vec2<T>();
        }
        const T a = dot(v) / len_squared;
        return a * (*this);
    }
};

typedef vec2<float> vec2f;
typedef vec2<double> vec2d;
typedef vec2<s32> vec2i;
typedef vec2<s16> vec2s;

// Static asserts to make sure the structs are packed.
static_assert(offsetof(vec2f, x) == 0);
static_assert(offsetof(vec2f, y) == sizeof(float));
static_assert(offsetof(vec2d, x) == 0);
static_assert(offsetof(vec2d, y) == sizeof(double));
static_assert(offsetof(vec2i, x) == 0);
static_assert(offsetof(vec2i, y) == sizeof(s32));
static_assert(offsetof(vec2s, x) == 0);
static_assert(offsetof(vec2s, y) == sizeof(s16));

inline vec2f operator+(const vec2f& a, const vec2i& b) { return vec2f(a.x + b.x, a.y + b.y); }
inline vec2f operator+(const vec2i& a, const vec2f& b) { return vec2f(a.x + b.x, a.y + b.y); }

// Returns the normalized form of a vector.
template <typename T>
vec2<T> normalize(const vec2<T>& a)
{
    return a / a.length();
}

// Returns the dot product of two vectors.
template <typename T>
T dot(const vec2<T>& a, const vec2<T>& b)
{
    return a.dot(b);
}

// Returns the absolute value of a vector.
template <typename T>
vec2<T> abs(const vec2<T>& a)
{
    return vec2<T>(abs(a.x), abs(a.y));
}

// Returns an orthogonal vector to the given vector. The result is not normalized.
// The result is not unique; any vector orthogonal to this vector is a valid result.
template <typename T>
vec2<T> orthogonal(const vec2<T>& a)
{
    return a.orthogonal();
}

// Returns the piecewise minimum of two vectors.
template <typename T>
vec2<T> min(const vec2<T>& a, const vec2<T>& b) {
    return vec2<T>((T)fmin(a.x, b.x), (T)fmin(a.y, b.y));
}

// Returns the piecewise maximum of two vectors.
template <typename T>
vec2<T> max(const vec2<T>& a, const vec2<T>& b) {
    return vec2<T>((T)fmax(a.x, b.x), (T)fmax(a.y, b.y));
}

// Returns the distance between two points.
template <typename T>
float_type<T>::type distance(const vec2<T>& a, const vec2<T>& b) {
    return (a - b).length();
}

// Returns the angle between two vectors in radians.
template <typename T>
double angle(const vec2<T>& a, const vec2<T>& b) {
    double a_len = a.lengthSquared();
    if (a_len == 0) return 0;
    double b_len = b.lengthSquared();
    if (b_len == 0) return 0;
    return acos(a.dot(b) / sqrt(a_len * b_len));
}

// Creates a vector from an angle in degrees. 0 degrees is along the positive X axis.
// The vector is normalized. The angle is counter-clockwise.
vec2f createDirectionDeg(float a);

// Creates a vector from an angle in radians. 0 radians is along the positive X axis.
// The vector is normalized. The angle is counter-clockwise.
vec2f createDirectionRad(float a);


template <typename T>
struct vec3 {
    T x;
    T y;
    T z;

    vec3() : x(0), y(0), z(0) {}
    vec3(T x, T y, T z) : x(x), y(y), z(z) {}

    vec3(const vec2<T> o) : x(o.x), y(o.y), z(0) {}
    vec3(const vec2<T> o, T z) : x(o.x), y(o.y), z(z) {}

    vec3(const vec3&) = default;
    vec3(vec3&&) noexcept = default;
    vec3& operator=(const vec3&) = default;
    vec3& operator=(vec3&&) noexcept = default;

    template <typename U>
    vec3<U> cast() const { return vec3<U>(static_cast<U>(::round(x)), static_cast<U>(::round(y)), static_cast<U>(::round(z))); }

    bool operator==(const vec3<T>& o) const
    {
        return equals(o);
    }
    bool operator!=(const vec3<T>& o) const
    {
        return !equals(o);
    }

    bool equals(const vec3<T>& o, T tolerance = T(1e-6)) const
    {
        return ::abs(x - o.x) <= tolerance
            && ::abs(y - o.y) <= tolerance
            && ::abs(z - o.z) <= tolerance;
    }

    T operator[](s32 i) const
    {
        debug_assert(i <= 2 && i >= 0);
        const T* v = &x;
        return v[i];
    }

    T& operator[](s32 i)
    {
        switch (i) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        }
        debug_assert(false);
        return x;
    }

    vec3<T> operator-() const
    {
        return vec3<T>(-x, -y, -z);
    }

    friend vec3<T> operator+(const vec3<T>& a, vec3<T> b)
    {
        b.x += a.x;
        b.y += a.y;
        b.z += a.z;
        return b;
    }
    vec3<T>& add(T x0, T y0, T z0)
    {
        x += x0;
        y += y0;
        z += z0;
        return *this;
    }
    vec3<T>& operator+=(const vec3<T>& b)
    {
        x += b.x;
        y += b.y;
        z += b.z;
        return *this;
    }

    friend vec3<T> operator-(vec3<T> a, const vec3<T>& b)
    {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        return a;
    }
    vec3<T>& sub(T x0, T y0, T z0)
    {
        x -= x0;
        y -= y0;
        z -= z0;
        return *this;
    }
    vec3<T>& operator-=(const vec3<T>& b)
    {
        x -= b.x;
        y -= b.y;
        z -= b.z;
        return *this;
    }

    friend vec3<T> operator*(const vec3<T> a, T b)
    {
        return vec3<T>(a.x * b, a.y * b, a.z * b);
    }
    friend vec3<T> operator*(T b, const vec3<T> a)
    {
        return vec3<T>(a.x * b, a.y * b, a.z * b);
    }
    friend vec3<T> operator*(vec3<T> a, const vec3<T>& b)
    {
        a.x *= b.x;
        a.y *= b.y;
        a.z *= b.z;
        return a;
    }
    vec3<T>& operator*=(T b)
    {
        x *= b;
        y *= b;
        z *= b;
        return *this;
    }
    vec3<T>& operator*=(const vec3<T>& b)
    {
        x *= b.x;
        y *= b.y;
        z *= b.z;
        return *this;
    }
    vec3<T>& mul(T x0, T y0, T z0)
    {
        x *= x0;
        y *= y0;
        z *= z0;
        return *this;
    }

    friend vec3<T> operator/(const vec3<T> a, T b)
    {
        return vec3<T>(a.x / b, a.y / b, a.z / b);
    }
    friend vec3<T> operator/(vec3<T> a, const vec3<T>& b)
    {
        a.x /= b.x;
        a.y /= b.y;
        a.z /= b.z;
        return a;
    }
    vec3<T>& operator/=(T b)
    {
        x /= b;
        y /= b;
        z /= b;
        return *this;
    }
    vec3<T>& div(T x0, T y0, T z0)
    {
        x /= x0;
        y /= y0;
        z /= z0;
        return *this;
    }

    bool zero() const
    {
        return ::abs(x) < 1e-6 && ::abs(y) < 1e-6 && ::abs(z) < 1e-6;
    }

    // Returns the length of the vector.
    float_type<T>::type length() const
    {
        return static_cast<float_type<T>::type>(sqrt(x * x + y * y + z * z));
    }

    // Returns the squared length of the vector.
    T lengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    // Normalizes the vector.
    vec3<T>& normalize()
    {
        if (x == 0 && y == 0 && z == 0) return *this;
        const double len = length();
        x = (T)(x / len);
        y = (T)(y / len);
        z = (T)(z / len);
        return *this;
    }

    // Normalizes the vector to the given length.
    vec3<T>& normalize(double new_length)
    {
        if (x == 0 && y == 0 && z == 0) return *this;
        const double len = length() / new_length;
        x = (T)(x / len);
        y = (T)(y / len);
        z = (T)(z / len);
        return *this;
    }

    // Replaces the vector with its absolute value.
    vec3<T>& abs()
    {
        if (x < T(0)) x = -x;
        if (y < T(0)) y = -y;
        if (z < T(0)) z = -z;
        return *this;
    }

    // Returns the dot product of this vector and the given vector.
    T dot(const vec3<T>& b) const
    {
        return x * b.x + y * b.y + z * b.z;
    }

    bool isOrthogonal(const vec3<T>& b) const {
        return dot(b) < 1e-6;
    }

    vec3<T> project(const vec3<T> v) const {
        const T len_squared = lengthSquared();
        if (len_squared < 1e-6) {
            fprintf(stderr, "Cannot project onto zero-length vector.");
            return vec3<T>();
        }
        const T a = dot(v) / len_squared;
        return a * (*this);
    }

    vec3<T> cross(const vec3<T>& o) const {
        return vec3<T>(
            y * o.z - o.y * z,
            z * o.x - o.z * x,
            x * o.y - o.x * y
        );
    }
};

typedef vec3<float> vec3f;
typedef vec3<double> vec3d;
typedef vec3<int> vec3i;

// Validate that the structs are packed.
static_assert(offsetof(vec3f, x) == 0, "vec3f.x is not at offset 0");
static_assert(offsetof(vec3f, y) == sizeof(float), "vec3f.y is not at offset 4");
static_assert(offsetof(vec3f, z) == sizeof(float) * 2, "vec3f.z is not at offset 8");
static_assert(offsetof(vec3d, x) == 0, "vec3d.x is not at offset 0");
static_assert(offsetof(vec3d, y) == sizeof(double), "vec3d.y is not at offset 8");
static_assert(offsetof(vec3d, z) == sizeof(double) * 2, "vec3d.z is not at offset 16");
static_assert(offsetof(vec3i, x) == 0, "vec3i.x is not at offset 0");
static_assert(offsetof(vec3i, y) == sizeof(int), "vec3i.y is not at offset 4");
static_assert(offsetof(vec3i, z) == sizeof(int) * 2, "vec3i.z is not at offset 8");

// Returns true if the two vectors are parallel.
template <typename T>
bool parallel(const vec3<T>& u, const vec3<T>& v)
{
    return u.cross(v).zero();
}

// Returns the given vector rotated by the given angle around the given axis.
template <typename T>
vec3<T> rotate(const vec3<T>& v, const vec3<T>& axis, float angle)
{
    // via Rodrigues' rotation formula
    return v * (T)cos(angle) + axis.cross(v) * (T)sin(angle) + axis * axis.dot(v) * T(1 - cos(angle));
}

// Returns the reflection of the given vector around the given normal.
template <typename T>
vec3<T> reflect(const vec3<T>& v, const vec3<T>& n)
{
    return v - 2 * v.dot(n) * n;
}

// Returns the refraction of the given vector across a boundary with the given normal and ratio of
// indices of refraction.
template <typename T>
vec3<T> refract(const vec3<T>& v, const vec3<T>& n, T etai_over_etat)
{
    T N_dot_I = dot(n, v);
    T k = T(1) - etai_over_etat * etai_over_etat * (T(1) - N_dot_I * N_dot_I);
    if (k < T(0)) return vec3<T>(0, 0, 0);
    return etai_over_etat * v - (etai_over_etat * N_dot_I + (T)sqrt(k)) * n;
}

// Returns the piecewise minimum of the given vectors.
template <typename T>
vec3<T> min(const vec3<T>& a, const vec3<T>& b)
{
    return vec3<T>((T)fmin(a.x, b.x), (T)fmin(a.y, b.y), (T)fmin(a.z, b.z));
}

// Returns the piecewise maximum of the given vectors.
template <typename T>
vec3<T> max(const vec3<T>& a, const vec3<T>& b)
{
    return vec3<T>((T)fmax(a.x, b.x), (T)fmax(a.y, b.y), (T)fmax(a.z, b.z));
}

// Returns the distance between the given vectors.
template <typename T>
float_type<T>::type distance(const vec3<T>& a, const vec3<T>& b)
{
    return (a - b).length();
}

// Returns the absolute value of the given vector.
template <typename T>
vec3<T> abs(const vec3<T>& v)
{
    return vec3<T>(::abs(v.x), ::abs(v.y), ::abs(v.z));
}

// Returns the dot product of two vectors.
template <typename T>
T dot(const vec3<T>& a, const vec3<T>& b)
{
    return a.dot(b);
}

// Returns the normalized version of the given vector.
template <typename T>
vec3<T> normalize(const vec3<T>& a)
{
    double len = a.length();
    return len < 1e-12 ? vec3<T>() : vec3<T>(T(a.x / len), T(a.y / len), T(a.z / len));
}

template <typename T>
vec3<T> cross(const vec3<T>& a, const vec3<T>& b)
{
    return a.cross(b);
}

// Returns the angle between the given vectors.
template <typename T>
double angle(const vec3<T>& a, const vec3<T>& b)
{
    double a_len = a.lengthSquared();
    if (a_len == 0) return 0;
    double b_len = b.lengthSquared();
    if (b_len == 0) return 0;
    return acos(a.dot(b) / sqrt(a_len * b_len));
}

// Returns the triple product of the given vectors.
// A x (B x C)
// = B * (A . C) - C * (A . B)
template <typename T>
vec3<T> tripleProduct(const vec3<T>& a, const vec3<T>& b, const vec3<T>& c)
{
    // A x (B x C)
    return b * (a.dot(c)) - c * (a.dot(b));
}

template <typename T>
struct vec4 {
    T x;
    T y;
    T z;
    T w;

    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    vec4(const vec2<T> o) : x(o.x), y(o.y), z(0), w(0) {}
    vec4(const vec2<T> o, T z, T w) : x(o.x), y(o.y), z(z), w(w) {}

    explicit vec4(const vec3<T> o) : x(o.x), y(o.y), z(o.z), w(0) {}
    vec4(const vec3<T> o, T w) : x(o.x), y(o.y), z(o.z), w(w) {}

    vec4(const vec4&) = default;
    vec4(vec4&&) noexcept = default;
    vec4& operator=(const vec4&) = default;
    vec4& operator=(vec4&&) noexcept = default;

    template <typename U>
    vec4<U> cast() const
    {
        return vec4<U>(
            static_cast<U>(::round(x))
            , static_cast<U>(::round(y))
            , static_cast<U>(::round(z))
            , static_cast<U>(::round(w)));
    }

    bool operator==(const vec4<T>& o) const
    {
        return equals(o);
    }
    bool operator!=(const vec4<T>& o) const
    {
        return !equals(o);
    }

    bool equals(const vec4<T>& o, T tolerance = T(1e-6)) const
    {
        return ::abs(x - o.x) <= tolerance
            && ::abs(y - o.y) <= tolerance
            && ::abs(z - o.z) <= tolerance
            && ::abs(w - o.w) <= tolerance;
    }

    T operator[](s32 i) const
    {
        debug_assert(i <= 3 && i >= 0);
        const T* v = &x;
        return v[i];
    }

    T& operator[](s32 i)
    {
        switch (i) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        case 3: return w;
        }
        debug_assert(false);
        return x;
    }

    vec4<T> operator-() const
    {
        return vec4<T>(-x, -y, -z, -w);
    }

    friend vec4<T> operator+(const vec4<T>& a, vec4<T> b)
    {
        b.x += a.x;
        b.y += a.y;
        b.z += a.z;
        b.w += a.w;
        return b;
    }
    vec4<T>& operator+=(const vec4<T>& b)
    {
        x += b.x;
        y += b.y;
        z += b.z;
        w += b.w;
        return *this;
    }

    friend vec4<T> operator-(vec4<T> a, const vec4<T>& b)
    {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        a.w -= b.w;
        return a;
    }
    vec4<T>& operator-=(const vec4<T>& b)
    {
        x -= b.x;
        y -= b.y;
        z -= b.z;
        w -= b.w;
        return *this;
    }

    friend vec4<T> operator*(const vec4<T> a, T b)
    {
        return vec4<T>(a.x * b, a.y * b, a.z * b, a.w * b);
    }
    friend vec4<T> operator*(T b, const vec4<T> a)
    {
        return vec4<T>(a.x * b, a.y * b, a.z * b, a.w * b);
    }
    friend vec4<T> operator*(vec4<T> a, const vec4<T>& b)
    {
        a.x *= b.x;
        a.y *= b.y;
        a.z *= b.z;
        a.w *= b.w;
        return a;
    }
    vec4<T>& operator*=(T b)
    {
        x *= b;
        y *= b;
        z *= b;
        w *= b;
        return *this;
    }
    vec4<T>& mul(T x0, T y0, T z0, T w0)
    {
        x *= x0;
        y *= y0;
        z *= z0;
        w *= w0;
        return *this;
    }

    friend vec4<T> operator/(const vec4<T> a, T b)
    {
        return vec4<T>(a.x / b, a.y / b, a.z / b, a.w / b);
    }
    friend vec4<T> operator/(vec4<T> a, const vec4<T>& b)
    {
        a.x /= b.x;
        a.y /= b.y;
        a.z /= b.z;
        a.w /= b.w;
        return a;
    }
    vec4<T>& operator/=(T b)
    {
        x /= b;
        y /= b;
        z /= b;
        w /= b;
        return *this;
    }
    vec4<T>& div(T x0, T y0, T z0, T w0)
    {
        x /= x0;
        y /= y0;
        z /= z0;
        w /= w0;
        return *this;
    }

    bool zero() const
    {
        return ::abs(x) < 1e-6 && ::abs(y) < 1e-6 && ::abs(z) < 1e-6 && ::abs(w) < 1e-6;
    }

    // Returns the length of this vector.
    float_type<T>::type length() const
    {
        return static_cast<float_type<T>::type>(sqrt(x * x + y * y + z * z + w * w));
    }

    // Returns the squared length of this vector.
    T lengthSquared() const
    {
        return x * x + y * y + z * z + w * w;
    }

    // Normalizes this vector.
    vec4<T>& normalize()
    {
        if (x == 0 && y == 0 && z == 0 && w == 0) return *this;
        const double len = length();
        x = (T)(x / len);
        y = (T)(y / len);
        z = (T)(z / len);
        w = (T)(w / len);
        return *this;
    }

    // Replaces this vector with its absolute value.
    vec4<T>& abs()
    {
        if (x < T(0)) x = -x;
        if (y < T(0)) y = -y;
        if (z < T(0)) z = -z;
        if (w < T(0)) w = -w;
        return *this;
    }

    // Returns the dot product of this vector with the given vector.
    T dot(const vec4<T>& b) const
    {
        return x * b.x + y * b.y + z * b.z + w * b.w;
    }

    // Returns true if this vector is orthogonal to the given vector.
    bool isOrthogonal(const vec4<T>& b) const
    {
        return dot(b) < 1e-6;
    }

    // Returns the projection of this vector onto the given vector.
    vec4<T> project(const vec4<T> v) const
    {
        const T len_squared = lengthSquared();
        if (len_squared < 1e-6) {
            fprintf(stderr, "Cannot project onto zero-length vector.");
            return vec4<T>();
        }
        const T a = dot(v) / len_squared;
        return a * (*this);
    }

    vec3<T> asVec3() const
    {
        return vec3<T>(x, y, z);
    }

};

typedef vec4<float> vec4f;
typedef vec4<double> vec4d;
typedef vec4<int> vec4i;

// Returns the piecewise minimum of two vectors.
template <typename T>
vec4<T> min(const vec4<T>& a, const vec4<T>& b) {
    return vec4<T>((T)fmin(a.x, b.x), (T)fmin(a.y, b.y), (T)fmin(a.z, b.z), (T)fmin(a.w, b.w));
}

// Returns the piecewise maximum of two vectors.
template <typename T>
vec4<T> max(const vec4<T>& a, const vec4<T>& b) {
    return vec4<T>((T)fmax(a.x, b.x), (T)fmax(a.y, b.y), (T)fmax(a.z, b.z), (T)fmax(a.w, b.w));
}

// Returns the distance between two vectors.
template <typename T>
float_type<T>::type distance(const vec4<T>& a, const vec4<T>& b)
{
    return (a - b).length();
}

// Returns the absolute value of the given vector.
template <typename T>
vec4<T> abs(const vec4<T>& v)
{
    return vec4<T>(::abs(v.x), ::abs(v.y), ::abs(v.z), ::abs(v.w));
}

// Returns the normalized version of the given vector.
template <typename T>
vec4<T> normalize(const vec4<T>& a)
{
    double len = a.length();
    return len < 1e-12 ? vec4<T>() : vec4<T>(T(a.x / len), T(a.y / len), T(a.z / len), T(a.w / len));
}

// Returns the dot product of two vectors.
template <typename T>
T dot(const vec4<T>& a, const vec4<T>& b)
{
    return a.dot(b);
}

template <typename T>
double angle(const vec4<T>& a, const vec4<T>& b)
{
    double a_len = a.lengthSquared();
    if (a_len == 0) return 0;
    double b_len = b.lengthSquared();
    if (b_len == 0) return 0;
    return acos(a.dot(b) / sqrt(a_len * b_len));
}

namespace hash {

    u32 fnv1a(const u8* data, size_t len);

    template <typename T> struct hash;

    template <>
    struct hash<vec2i>
    {
        u32 operator()(vec2i key) const noexcept
        {
            return fnv1a((u8*)&key.x, sizeof(key));
        }
    };
    template <>
    struct hash<vec3i>
    {
        u32 operator()(vec3i key) const noexcept
        {
            return fnv1a((u8*)&key.x, sizeof(key));
        }
    };
    template <>
    struct hash<vec4i>
    {
        u32 operator()(vec4i key) const noexcept
        {
            return fnv1a((u8*)&key.x, sizeof(key));
        }
    };
}
