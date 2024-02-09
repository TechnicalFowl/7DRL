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

struct Ray2 {
    vec2f origin;
    vec2f direction;

    Ray2() : origin(0, 0), direction(1, 0) {}
    Ray2(vec2f o, vec2f d) : origin(o), direction(d) {}

    // General form ax + by = c
    Ray2(float a, float b, float c);

    vec2f operator()(float f) const { return origin + f * direction; }

    float distanceTo(const vec2f& p) const;
    vec2f nearestPoint(const vec2f& p) const;

    vec3f getGeneralForm() const;

    static Ray2 fromPoints(const vec2f& a, const vec2f& b) { return Ray2(a, b - a); }
};

struct RayHit2
{
    float distance;
    vec2f normal;
    vec2f point;
    RayHit2() : distance(1e30f), normal(0, 0), point(0, 0) {}
    RayHit2(float d, vec2f n, vec2f p) : distance(d), normal(n), point(p) {}
    bool operator<(const RayHit2& b) const { return distance < b.distance; }
};

struct Ray {
    vec3f origin;
    vec3f direction;

    Ray() : origin(0, 0, 0), direction(1, 0, 0) {}
    Ray(vec3f o, vec3f d) : origin(o), direction(d) {}

    vec3f operator()(float f) const { return origin + f * direction; }

    float distanceTo(const vec3f& p) const;
    vec3f nearestPoint(const vec3f& p) const;

    static Ray fromPoints(const vec3f& a, const vec3f& b) { return Ray(a, b - a); }
};

struct RayHit
{
    float distance;
    vec3f normal;
    vec3f point;

    RayHit() : distance(1e30f), normal(0, 0, 0), point(0, 0, 0) {}
    RayHit(float d, vec3f n, vec3f p) : distance(d), normal(n), point(p) {}

    bool operator<(const RayHit& b) const { return distance < b.distance; }

    bool isHit() const { return distance < 1e99; }
};

template<typename T>
struct rect2 {
    vec2<T> lower;
    vec2<T> upper;

    rect2() : lower(0, 0), upper(1, 1) {}
    rect2(const vec2<T>& l, const vec2<T>& u) {
        lower = min(l, u);
        upper = max(l, u);
    }

    friend bool operator==(const rect2& a, const rect2& b) {
        return a.lower == b.lower && a.upper == b.upper;
    }

    friend rect2<T> operator+(rect2<T> a, const vec2f& o)
    {
        a.lower += o;
        a.upper += o;
        return a;
    }
    rect2<T>& operator+=(const vec2f& o)
    {
        lower += o;
        upper += o;
        return *this;
    }
    friend rect2<T> operator-(rect2<T> a, const vec2f& o)
    {
        a.lower -= o;
        a.upper -= o;
        return a;
    }
    rect2<T>& operator-=(const vec2f& o)
    {
        lower -= o;
        upper -= o;
        return *this;
    }

    bool hasArea() const {
        return lower != upper;
    }

    bool contains(const vec2<T>& v) const {
        return (v.x >= lower.x && v.x <= upper.x)
            && (v.y >= lower.y && v.y <= upper.y);
    }

    vec2<T> getCenter() const {
        T cx = (upper.x - lower.x) / 2 + lower.x;
        T cy = (upper.y - lower.y) / 2 + lower.y;
        return vec2<T>(cx, cy);
    }

    vec2<T> getSize() const {
        return upper - lower;
    }

    T width() const { return upper.x - lower.x; }
    T height() const { return upper.y - lower.y; }

    void contract(T x, T y) {
        if (x < 0) {
            lower.x -= x;
        }
        else {
            upper.x -= x;
        }
        if (y < 0) {
            lower.y -= y;
        }
        else {
            upper.y -= y;
        }
    }
    void expand(T x, T y) {
        if (x < 0) {
            lower.x += x;
        }
        else {
            upper.x += x;
        }
        if (y < 0) {
            lower.y += y;
        }
        else {
            upper.y += y;
        }
    }
    void inflate(T x, T y) {
        lower.x -= x;
        upper.x += x;
        lower.y -= y;
        upper.y += y;
    }

    rect2<T> intersect(const rect2<T>& other) {
        vec2<T> lower0 = max(lower, other.lower);
        vec2<T> upper0 = min(upper, other.upper);
        return rect2<T>(lower0, upper0);
    }
    rect2<T> surround(const rect2<T>& other) {
        vec2<T> lower0 = min(lower, other.lower);
        vec2<T> upper0 = max(upper, other.upper);
        return rect2<T>(lower0, upper0);
    }
    bool intersects(const rect2<T>& other) const {
        return lower.x < other.upper.x && upper.x > other.lower.x
            && lower.y < other.upper.y && upper.y > other.lower.y;
    }
    bool contains(const rect2<T>& other) const {
        return lower.x <= other.lower.x && upper.x >= other.upper.x
            && lower.y <= other.lower.y && upper.y >= other.upper.y;
    }

    struct rect_iterator {
        rect2<T> r;
        vec2<T> index;

        rect_iterator(const rect2<T>& r) : r(r), index(r.lower) {}
        rect_iterator(const rect2<T>& r, vec2<T> s) : r(r), index(s) {}

        friend bool operator==(const rect_iterator& a, const rect_iterator& b) {
            return a.index == b.index && a.r == b.r;
        }
        friend bool operator!=(const rect_iterator& a, const rect_iterator& b) {
            return !(a == b);
        }

        vec2<T> operator*() {
            return index;
        }
        vec2<T> operator->() {
            return index;
        }

        rect_iterator& operator++() {
            advance();
            return *this;
        }

        rect_iterator& operator--() {
            if (index.y < r.lower.y) return *this;
            index.x--;
            if (index.x < r.lower.x) {
                index.x = r.upper.x;
                index.y--;
            }
            return *this;
        }

        void advance() {
            if (index.y > r.upper.y) return;
            index.x++;
            if (index.x > r.upper.x) {
                index.x = r.lower.x;
                index.y++;
            }
        }
        friend bool operator<(const rect_iterator& a, const rect_iterator& b) {
            return a.index.y < b.index.y || (a.index.y == b.index.y && a.index.x < b.index.x);
        }
        friend bool operator<=(const rect_iterator& a, const rect_iterator& b) {
            return a.index.y < b.index.y || (a.index.y == b.index.y && a.index.x <= b.index.x);
        }
        friend bool operator>(const rect_iterator& a, const rect_iterator& b) {
            return a.index.y > b.index.y || (a.index.y == b.index.y && a.index.x > b.index.x);
        }
        friend bool operator>=(const rect_iterator& a, const rect_iterator& b) {
            return a.index.y > b.index.y || (a.index.y == b.index.y && a.index.x >= b.index.x);
        }
    };

    rect_iterator begin() const {
        return rect_iterator(*this);
    }
    rect_iterator cbegin() const {
        return rect_iterator(*this);
    }
    rect_iterator end() const {
        return rect_iterator(*this, vec2<T>(lower.x, upper.y + 1));
    }
    rect_iterator cend() const {
        return rect_iterator(*this, vec2<T>(lower.x, upper.y + 1));
    }
};

typedef rect2<float> rect2f;
typedef rect2<s32> rect2i;

rect2i toInt(const rect2f& r);

template<typename T>
struct rect3 {
    vec3<T> lower;
    vec3<T> upper;

    rect3() : lower(0, 0, 0), upper(1, 1, 1) {}
    rect3(const vec3<T>& l, const vec3<T>& u) {
        lower = min(l, u);
        upper = max(l, u);
    }

    friend bool operator==(const rect3& a, const rect3& b) {
        return a.lower == b.lower && a.upper == b.upper;
    }

    friend rect3<T> operator+(rect3<T> a, const vec3f& o)
    {
        a.lower += o;
        a.upper += o;
        return a;
    }
    rect3<T>& operator+=(const vec3f& o)
    {
        lower += o;
        upper += o;
        return *this;
    }
    friend rect3<T> operator-(rect3<T> a, const vec3f& o)
    {
        a.lower -= o;
        a.upper -= o;
        return a;
    }
    rect3<T>& operator-=(const vec3f& o)
    {
        lower -= o;
        upper -= o;
        return *this;
    }

    bool hasArea() const {
        return lower != upper;
    }

    bool contains(const vec3<T>& v) const {
        return (v.x >= lower.x && v.x <= upper.x)
            && (v.y >= lower.y && v.y <= upper.y)
            && (v.z >= lower.z && v.z <= upper.z);
    }

    vec3<T> getCenter() const {
        T cx = (upper.x - lower.x) / 2 + lower.x;
        T cy = (upper.y - lower.y) / 2 + lower.y;
        T cz = (upper.z - lower.z) / 2 + lower.z;
        return vec3<T>(cx, cy, cz);
    }

    vec3<T> getSize() const {
        return upper - lower;
    }

    void contract(T x, T y, T z) {
        if (x < 0) {
            lower.x -= x;
        }
        else {
            upper.x -= x;
        }
        if (y < 0) {
            lower.y -= y;
        }
        else {
            upper.y -= y;
        }
        if (z < 0) {
            lower.z -= z;
        }
        else {
            upper.z -= z;
        }
    }
    void expand(T x, T y, T z) {
        if (x < 0) {
            lower.x += x;
        }
        else {
            upper.x += x;
        }
        if (y < 0) {
            lower.y += y;
        }
        else {
            upper.y += y;
        }
        if (z < 0) {
            lower.z += z;
        }
        else {
            upper.z += z;
        }
    }
    void inflate(T x, T y, T z) {
        lower.x -= x;
        upper.x += x;
        lower.y -= y;
        upper.y += y;
        lower.z -= z;
        upper.z += z;
    }

    rect3<T> intersect(const rect3<T>& other) {
        vec3<T> lower0 = max(lower, other.lower);
        vec3<T> upper0 = min(upper, other.upper);
        return rect3<T>(lower0, upper0);
    }
    rect3<T> surround(const rect3<T>& other) {
        vec3<T> lower0 = min(lower, other.lower);
        vec3<T> upper0 = max(upper, other.upper);
        return rect3<T>(lower0, upper0);
    }
    bool intersects(const rect3<T>& other) {
        return lower.x < other.upper.x && upper.x > other.lower.x
            && lower.y < other.upper.y && upper.y > other.lower.y
            && lower.z < other.upper.z && upper.z > other.lower.z;
    }
    bool contains(const rect3<T>& other) {
        return lower.x <= other.lower.x && upper.x >= other.upper.x
            && lower.y <= other.lower.y && upper.y >= other.upper.y
            && lower.z <= other.lower.z && upper.z >= other.upper.z;
    }
};

typedef rect3<s32> rect3i;
typedef rect3<float> rect3f;

struct Quaternion {
    float x, y, z, w;

    Quaternion();
    Quaternion(float x, float y, float z, float w);
    Quaternion(const vec3f& axis, float angle);
    Quaternion(const vec3f& u, const vec3f& v);
    Quaternion(float roll, float pitch, float yaw);

    Quaternion(const Quaternion& o) = default;
    Quaternion(Quaternion&& o) noexcept = default;
    Quaternion& operator=(const Quaternion& o) = default;
    Quaternion& operator=(Quaternion&& o) noexcept = default;

    friend Quaternion operator-(const Quaternion& q) {
        return q.conjugate();
    }

    Quaternion& operator*=(const Quaternion& q);

    friend Quaternion operator*(const Quaternion& q0, const Quaternion& q1) {
        float x0 = q0.w * q1.x + q0.x * q1.w + q0.y * q1.z - q0.z * q1.y;
        float y0 = q0.w * q1.y + q0.y * q1.w + q0.z * q1.x - q0.x * q1.z;
        float z0 = q0.w * q1.z + q0.z * q1.w + q0.x * q1.y - q0.y * q1.x;
        float w0 = q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z * q1.z;
        return Quaternion(x0, y0, z0, w0);
    }

    friend vec3f operator*(const Quaternion& q, const vec3f& v) {
        return q.rotate(v.x, v.y, v.z);
    }

    friend vec4f operator*(const Quaternion& q, const vec4f& v) {
        float px = q.w * v.x + q.y * v.z - q.z * v.y;
        float py = q.w * v.y + q.z * v.x - q.x * v.z;
        float pz = q.w * v.z + q.x * v.y - q.y * v.x;
        float pw = -q.x * v.x - q.y * v.y - q.z * v.z;
        return vec4f(
            pw * -q.x + px * q.w - py * q.z + pz * q.y,
            pw * -q.y + py * q.w - pz * q.x + px * q.z,
            pw * -q.z + pz * q.w - px * q.y + py * q.x,
            1
        );
    }

    float length() const;
    float lengthSquared() const;
    void normalize();

    void set(const vec3f& axis, float angle);

    Quaternion conjugate() const;
    Quaternion inverse() const;

    vec3f rotate(float x, float y, float z) const;

    // TODO: rename to `to_euler_angles`
    vec3f to_axis_angles();

    vec3f get_rotation_axis();
    float get_rotation_angle();
};

Quaternion slerp(Quaternion& q0, Quaternion& q1, float t);

template <typename T>
struct mat2
{
    T m[4];

    mat2()
    {
        m[0] = 1; m[1] = 0;
        m[2] = 0; m[3] = 1;
    }
    mat2(const mat2<T>& o)
    {
        memcpy(m, o.m, 4 * sizeof(T));
    }

    T& operator()(s32 x, s32 y)
    {
        return m[x + y * 2];
    }
    T operator()(s32 x, s32 y) const
    {
        return m[x + y * 2];
    }

    friend bool operator==(const mat2<T>& a, const mat2<T>& b)
    {
        for (int i = 0; i < 4; i++)
            if (::abs(a.m[i] - b.m[i]) > 1e-6)
                return false;
        return true;
    }

    friend vec2<T> operator*(const mat2<T>& m, const vec2<T>& v)
    {
        return vec2<T>(v.x * m.m[0] + v.y * m.m[1]
            , v.x * m.m[2] + v.y * m.m[3]);
    }
    friend mat2<T> operator*(const mat2<T>& a, const mat2<T>& b)
    {
        mat2<T> r;
        r.m[0] = a.m[0] * b.m[0] + a.m[1] * b.m[2];
        r.m[1] = a.m[0] * b.m[1] + a.m[1] * b.m[3];
        r.m[2] = a.m[2] * b.m[0] + a.m[3] * b.m[2];
        r.m[3] = a.m[2] * b.m[1] + a.m[3] * b.m[3];
        return r;
    }

    vec2<T> diagonal() const
    {
        return vec2<T>(m[0], m[3]);
    }

    T det() const
    {
        return m[0] * m[3] - m[1] * m[2];
    }
};

typedef mat2<float> mat2f;
typedef mat2<double> mat2d;

template <typename T>
mat2<T> createRotationMatrix(T angle) {
    mat2<T> m;
    m(0, 0) = (T)cos(angle);
    m(1, 0) = (T)-sin(angle);
    m(0, 1) = (T)sin(angle);
    m(1, 1) = (T)cos(angle);
    return m;
}

template <typename T>
struct mat3 {
    // row-major
    T m[9];

    mat3() {
        m[0] = 1; m[1] = 0; m[2] = 0;
        m[3] = 0; m[4] = 1; m[5] = 0;
        m[6] = 0; m[7] = 0; m[8] = 1;
    }
    mat3(const mat3<T>& o) {
        memcpy(m, o.m, 9 * sizeof(T));
    }

    T& operator()(s32 x, s32 y) {
        return m[x + y * 3];
    }
    T operator()(s32 x, s32 y) const {
        return m[x + y * 3];
    }

    mat3<T>& operator*=(const mat3<T> b) {
        const mat3<T>& a = *this;
        float cpy[9]{ 0 };
        cpy[0] = a(0, 0) * b(0, 0) + a(0, 1) * b(1, 0) + a(0, 2) * b(2, 0);
        cpy[3] = a(0, 0) * b(0, 1) + a(0, 1) * b(1, 1) + a(0, 2) * b(2, 1);
        cpy[6] = a(0, 0) * b(0, 2) + a(0, 1) * b(1, 2) + a(0, 2) * b(2, 2);
        cpy[1] = a(1, 0) * b(0, 0) + a(1, 1) * b(1, 0) + a(1, 2) * b(2, 0);
        cpy[4] = a(1, 0) * b(0, 1) + a(1, 1) * b(1, 1) + a(1, 2) * b(2, 1);
        cpy[7] = a(1, 0) * b(0, 2) + a(1, 1) * b(1, 2) + a(1, 2) * b(2, 2);
        cpy[2] = a(2, 0) * b(0, 0) + a(2, 1) * b(1, 0) + a(2, 2) * b(2, 0);
        cpy[5] = a(2, 0) * b(0, 1) + a(2, 1) * b(1, 1) + a(2, 2) * b(2, 1);
        cpy[8] = a(2, 0) * b(0, 2) + a(2, 1) * b(1, 2) + a(2, 2) * b(2, 2);
        memcpy(m, cpy, 9 * sizeof(T));
        return *this;
    }
    friend mat3<T> operator*(mat3<T> a, const mat3<T>& b) {
        a *= b;
        return a;
    }

    mat3<T>& operator*=(T val) {
        for (u32 i = 0; i < 9; i++) {
            m[i] *= val;
        }
        return *this;
    }
    friend mat3<T> operator*(mat3<T> a, T b) {
        a *= b;
        return a;
    }

    void transpose() {
        std::swap(m[3], m[1]);
        std::swap(m[6], m[2]);
        std::swap(m[7], m[5]);
    }

    T det() const {
        T d = m[0] * (m[4] * m[8] - m[5] * m[7]);
        d -= m[1] * (m[3] * m[8] - m[5] * m[6]);
        d += m[2] * (m[3] * m[7] - m[4] * m[6]);
        return d;
    }
};

typedef mat3<float> mat3f;
typedef mat3<double> mat3d;

template<typename T>
mat3<T> fromQuaternion(const Quaternion& q) {
    mat3<T> result;
    float f0 = q.x;
    float f1 = q.y;
    float f2 = q.z;
    float f3 = q.w;
    float f4 = 2 * f0 * f0;
    float f5 = 2 * f1 * f1;
    float f6 = 2 * f2 * f2;
    result(0, 0) = 1.0f - f5 - f6;
    result(1, 1) = 1.0f - f6 - f4;
    result(2, 2) = 1.0f - f4 - f5;
    result(1, 0) = 2 * (f0 * f1 + f2 * f3);
    result(0, 1) = 2 * (f0 * f1 - f2 * f3);
    result(2, 0) = 2 * (f2 * f0 - f1 * f3);
    result(0, 2) = 2 * (f2 * f0 + f1 * f3);
    result(2, 1) = 2 * (f2 * f1 + f0 * f3);
    result(1, 2) = 2 * (f2 * f1 - f0 * f3);
    return result;
}

template <typename T>
mat3<T> createScaleMatrix(float x, float y, float z) {
    mat3<T> result;
    result(0, 0) = x;
    result(1, 1) = y;
    result(2, 2) = z;
    return result;
}

template <typename T>
struct mat4 {
    // row-major
    // 0  1  2  3
    // 4  5  6  7
    // 8  9  10 11
    // 12 13 14 15
    T m[16];

    mat4() {
        memset(m, 0, 16 * sizeof(T));
        m[0] = T(1);
        m[5] = T(1);
        m[10] = T(1);
        m[15] = T(1);
    }
    mat4(T t) {
        memset(m, 0, 16 * sizeof(T));
        m[0] = t;
        m[5] = t;
        m[10] = t;
        m[15] = t;
    }
    mat4(T v[16]) {
        memcpy(m, v, 16 * sizeof(T));
    }
    mat4(const mat3<T>& o) {
        memcpy(m, o.m, 16 * sizeof(T));
    }

    T& operator()(s32 x, s32 y) {
        return m[x + y * 4];
    }
    T operator()(s32 x, s32 y) const {
        return m[x + y * 4];
    }

    friend bool operator==(const mat4<T>& a, const mat4<T>& b) {
        for (int i = 0; i < 16; i++) {
            if (a.m[i] != b.m[i]) {
                return false;
            }
        }
        return true;
    }
    friend bool operator!=(const mat4<T>& a, const mat4<T>& b) {
        return !(a == b);
    }

    mat4<T>& operator*=(const mat4<T> b) {
        const mat4<T>& a = *this;
        T cpy[16]{ 0 };
        cpy[0] = a(0, 0) * b(0, 0) + a(0, 1) * b(1, 0) + a(0, 2) * b(2, 0) + a(0, 3) * b(3, 0);
        cpy[4] = a(0, 0) * b(0, 1) + a(0, 1) * b(1, 1) + a(0, 2) * b(2, 1) + a(0, 3) * b(3, 1);
        cpy[8] = a(0, 0) * b(0, 2) + a(0, 1) * b(1, 2) + a(0, 2) * b(2, 2) + a(0, 3) * b(3, 2);
        cpy[12] = a(0, 0) * b(0, 3) + a(0, 1) * b(1, 3) + a(0, 2) * b(2, 3) + a(0, 3) * b(3, 3);
        cpy[1] = a(1, 0) * b(0, 0) + a(1, 1) * b(1, 0) + a(1, 2) * b(2, 0) + a(1, 3) * b(3, 0);
        cpy[5] = a(1, 0) * b(0, 1) + a(1, 1) * b(1, 1) + a(1, 2) * b(2, 1) + a(1, 3) * b(3, 1);
        cpy[9] = a(1, 0) * b(0, 2) + a(1, 1) * b(1, 2) + a(1, 2) * b(2, 2) + a(1, 3) * b(3, 2);
        cpy[13] = a(1, 0) * b(0, 3) + a(1, 1) * b(1, 3) + a(1, 2) * b(2, 3) + a(1, 3) * b(3, 3);
        cpy[2] = a(2, 0) * b(0, 0) + a(2, 1) * b(1, 0) + a(2, 2) * b(2, 0) + a(2, 3) * b(3, 0);
        cpy[6] = a(2, 0) * b(0, 1) + a(2, 1) * b(1, 1) + a(2, 2) * b(2, 1) + a(2, 3) * b(3, 1);
        cpy[10] = a(2, 0) * b(0, 2) + a(2, 1) * b(1, 2) + a(2, 2) * b(2, 2) + a(2, 3) * b(3, 2);
        cpy[14] = a(2, 0) * b(0, 3) + a(2, 1) * b(1, 3) + a(2, 2) * b(2, 3) + a(2, 3) * b(3, 3);
        cpy[3] = a(3, 0) * b(0, 0) + a(3, 1) * b(1, 0) + a(3, 2) * b(2, 0) + a(3, 3) * b(3, 0);
        cpy[7] = a(3, 0) * b(0, 1) + a(3, 1) * b(1, 1) + a(3, 2) * b(2, 1) + a(3, 3) * b(3, 1);
        cpy[11] = a(3, 0) * b(0, 2) + a(3, 1) * b(1, 2) + a(3, 2) * b(2, 2) + a(3, 3) * b(3, 2);
        cpy[15] = a(3, 0) * b(0, 3) + a(3, 1) * b(1, 3) + a(3, 2) * b(2, 3) + a(3, 3) * b(3, 3);
        memcpy(m, cpy, 16 * sizeof(T));
        return *this;
    }
    friend mat4<T> operator*(mat4<T> a, const mat4<T>& b) {
        a *= b;
        return a;
    }

    friend vec4<T> operator*(const vec4<T>& a, const mat4<T>& b)
    {
        T x = a.x * b(0, 0) + a.y * b(1, 0) + a.z * b(2, 0) + a.w * b(3, 0);
        T y = a.x * b(0, 1) + a.y * b(1, 1) + a.z * b(2, 1) + a.w * b(3, 1);
        T z = a.x * b(0, 2) + a.y * b(1, 2) + a.z * b(2, 2) + a.w * b(3, 2);
        T w = a.x * b(0, 3) + a.y * b(1, 3) + a.z * b(2, 3) + a.w * b(3, 3);
        return vec4<T>(x, y, z, w);
    }

    friend vec4<T> operator*(const mat4<T>& b, const vec4<T>& a)
    {
        T x = a.x * b(0, 0) + a.y * b(0, 1) + a.z * b(0, 2) + a.w * b(0, 3);
        T y = a.x * b(1, 0) + a.y * b(1, 1) + a.z * b(1, 2) + a.w * b(1, 3);
        T z = a.x * b(2, 0) + a.y * b(2, 1) + a.z * b(2, 2) + a.w * b(2, 3);
        T w = a.x * b(3, 0) + a.y * b(3, 1) + a.z * b(3, 3) + a.w * b(3, 3);
        return vec4<T>(x, y, z, w);
    }

    mat4<T>& operator*=(T val) {
        for (u32 i = 0; i < 16; i++) {
            m[i] *= val;
        }
        return *this;
    }
    friend mat4<T> operator*(mat4<T> a, T b) {
        a *= b;
        return a;
    }

    bool checkClose(const mat4<T>& b, T eps) {
        for (int i = 0; i < 16; i++) {
            if (abs(m[i] - b.m[i]) > eps) {
                return false;
            }
        }
        return true;
    }

    mat4<T> transpose() const {
        mat4<T> res(*this);
        res.transposeInPlace();
        return res;
    }

    void transposeInPlace() {
        std::swap(m[4], m[1]);
        std::swap(m[8], m[2]);
        std::swap(m[12], m[3]);

        std::swap(m[9], m[6]);
        std::swap(m[13], m[7]);

        std::swap(m[14], m[11]);
    }

    void translate(vec3f offset) {
        mat4<T>& a = *this;
        a(0, 3) += offset.x;
        a(1, 3) += offset.y;
        a(2, 3) += offset.z;
    }
};

typedef mat4<float> mat4f;
typedef mat4<double> mat4d;

template <typename T>
mat4<T> lookAt(vec3<T> eye, vec3<T> center, vec3<T> up) {
    vec3<T> f = normalize(eye - center);
    vec3<T> s = normalize(up.cross(f));
    vec3<T> u = normalize(f.cross(s));

    mat4<T> result;
    result(0, 0) = s.x;
    result(0, 1) = s.y;
    result(0, 2) = s.z;
    result(1, 0) = u.x;
    result(1, 1) = u.y;
    result(1, 2) = u.z;
    result(2, 0) = f.x;
    result(2, 1) = f.y;
    result(2, 2) = f.z;
    result(0, 3) = -s.dot(eye);
    result(1, 3) = -u.dot(eye);
    result(2, 3) = -f.dot(eye);
    result(3, 3) = 1;
    return result;
}

template <typename T>
mat4<T> perspective(T fov, T aspect, T near, T far) {
    T f = (T)(tan(fov / 2));
    mat4<T> result;
    result(0, 0) = 1 / (aspect * f);
    result(1, 1) = 1 / f;
    result(2, 2) = -(far) / (far - near);
    result(3, 3) = 0;
    result(3, 2) = -1;
    result(2, 3) = -1 * near * far / (far - near);
    return result;
}

template <typename T>
mat4<T> orthographic(T width, T height, T near, T far) {
    mat4<T> result;
    result(0, 0) = 2 / width;
    result(1, 1) = 2 / height;
    float f = far - near;
    result(2, 2) = -2 / f;
    result(3, 3) = 1;
    result(0, 3) = -1;
    result(1, 3) = -1;
    result(2, 3) = -(far + near) / f;
    return result;
}

template <typename T>
mat4<T> ortho(T left, T right, T bottom, T top) {
    mat4<T> result;
    result(0, 0) = static_cast<T>(2) / (right - left);
    result(1, 1) = static_cast<T>(2) / (top - bottom);
    result(2, 2) = -static_cast<T>(1);
    result(0, 3) = -(right + left) / (right - left);
    result(1, 3) = -(top + bottom) / (top - bottom);
    return result;
}

template <typename T>
mat4<T> createScaleMatrix4(T x, T y, T z) {
    mat4<T> result;
    result(0, 0) = x;
    result(1, 1) = y;
    result(2, 2) = z;
    result(3, 3) = 1;
    return result;
}

template <typename T>
mat4<T> createTranslationMatrix4(T x, T y, T z) {
    mat4<T> result;
    result(0, 0) = 1;
    result(1, 1) = 1;
    result(2, 2) = 1;
    result(3, 3) = 1;
    result(0, 3) = x;
    result(1, 3) = y;
    result(2, 3) = z;
    return result;
}
