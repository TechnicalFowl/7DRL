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

Ray2::Ray2(float a, float b, float c)
{
    if (a == 0 && b == 0) debug_assert(c == 0);
    if (b == 0)
        origin = vec2f(0, c / a);
    else
        origin = vec2f(0, c / b);
    direction = vec2f(-b, a);
}

float Ray2::distanceTo(const vec2f& p) const {
    vec2f v = p - origin;
    vec2f proj = direction.project(v);
    return (float)(v - proj).length();
}

vec2f Ray2::nearestPoint(const vec2f& p) const {
    return direction.project(p - origin) + origin;
}

vec3f Ray2::getGeneralForm() const
{
    // ax + by = c
    // Using the negative normal vector gives us a general form which is
    // consistent with the general form taken by our ctor.
    vec2f n = -direction.orthogonal();
    float c = n.dot(origin);
    return vec3f(n.x, n.y, c);
}

float Ray::distanceTo(const vec3f& p) const {
    vec3f v = p - origin;
    vec3f proj = direction.project(v);
    return (float)(v - proj).length();
}

vec3f Ray::nearestPoint(const vec3f& p) const {
    return direction.project(p - origin) + origin;
}

rect2i toInt(const rect2f& r)
{
    return rect2i(vec2i(scalar::floori(r.lower.x), scalar::floori(r.lower.y)),
        vec2i(scalar::floori(r.upper.x), scalar::floori(r.upper.y)));
}

Quaternion::Quaternion() : x(0), y(0), z(0), w(1) {
}

Quaternion::Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {
    normalize();
}

Quaternion::Quaternion(const vec3f& axis, float angle) {
    set(axis, angle);
}

Quaternion::Quaternion(const vec3f& u0, const vec3f& v0) {
    vec3f v = v0; v.normalize();
    vec3f u = u0; u.normalize();
    vec3f t = u.cross(v);
    if (t.zero()) {
        if (v != u) {
            vec3f inter = parallel(v, vec3f(1, 0, 0)) ? vec3f(0, 1, 0) : vec3f(1, 0, 0);
            Quaternion q0(inter, u);
            Quaternion q1(v, inter);
            *this = q0 * q1;
        }
        else {
            x = y = z = 0;
            w = 1;
        }
    }
    else {
        set(t, (float)-::acos(v.dot(u)));
    }
}

Quaternion::Quaternion(float roll, float pitch, float yaw) {
    float sy = (float)sin(yaw / 2);
    float cy = (float)cos(yaw / 2);
    float sp = (float)sin(pitch / 2);
    float cp = (float)cos(pitch / 2);
    float sr = (float)sin(roll / 2);
    float cr = (float)cos(roll / 2);

    x = sr * cp * cy - cr * sp * sy;
    y = cr * sp * cy + sr * cp * sy;
    z = cr * cp * sy - sr * sp * cy;
    w = cr * cp * cy + sr * sp * sy;
    normalize();
}

Quaternion& Quaternion::operator*=(const Quaternion& q) {
    float x0 = w * q.x + x * q.w + y * q.z - z * q.y;
    float y0 = w * q.y + y * q.w + z * q.x - x * q.z;
    float z0 = w * q.z + z * q.w + x * q.y - y * q.x;
    float w0 = w * q.w - x * q.x - y * q.y - z * q.z;
    x = x0; y = y0; z = z0; w = w0;
    normalize();
    return *this;
}

float Quaternion::length() const {
    return (float)sqrt(x * x + y * y + z * z + w * w);
}

float Quaternion::lengthSquared() const {
    return x * x + y * y + z * z + w * w;
}

void Quaternion::normalize() {
    float lens = lengthSquared();
    if (lens < 1e-6f) {
        x = 0;
        y = 0;
        z = 0;
        w = 0;
    }
    else {
        float len = (float)scalar::fastInverseSqrt(lens);
        x *= len;
        y *= len;
        z *= len;
        w *= len;
    }
}

void Quaternion::set(const vec3f& axis, float angle) {
    float halfAngle = angle / 2;
    float q = (float)(sin(halfAngle) / axis.length());
    x = axis.x * q;
    y = axis.y * q;
    z = axis.z * q;
    w = (float)cos(halfAngle);
    normalize();
}

Quaternion Quaternion::conjugate() const {
    return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::inverse() const {
    float len = (float)scalar::fastInverseSqrt(lengthSquared());
    return Quaternion(-x * len, -y * len, -z * len, w * len);
}

vec3f Quaternion::rotate(float x0, float y0, float z0) const {
    float px = w * x0 + y * z0 - z * y0;
    float py = w * y0 + z * x0 - x * z0;
    float pz = w * z0 + x * y0 - y * x0;
    float pw = -x * x0 - y * y0 - z * z0;
    return vec3f(
        pw * -x + px * w - py * z + pz * y,
        pw * -y + py * w - pz * x + px * z,
        pw * -z + pz * w - px * y + py * x
    );
}

vec3f Quaternion::to_axis_angles()
{
    double sinr_cosp = 2 * (w * x + y * z);
    double cosr_cosp = 1 - 2 * (x * x + y * y);
    double roll = atan2(sinr_cosp, cosr_cosp);

    // pitch (y-axis rotation)
    double sinp = sqrt(1 + 2 * (w * y - x * z));
    double cosp = sqrt(1 - 2 * (w * y - x * z));
    double pitch = 2 * atan2(sinp, cosp) - scalar::PI / 2;

    // yaw (z-axis rotation)
    double siny_cosp = 2 * (w * z + x * y);
    double cosy_cosp = 1 - 2 * (y * y + z * z);
    double yaw = atan2(siny_cosp, cosy_cosp);

    return vec3f((float)roll, (float)pitch, (float)yaw);
}

vec3f Quaternion::get_rotation_axis() {
    float q = (float)sqrt(1 - w * w);
    return vec3f(x / q, y / q, z / q);
}

float Quaternion::get_rotation_angle() {
    return 2 * (float)acos(w);
}

Quaternion slerp(Quaternion& q0, Quaternion& q1, float t) {
    // @Testing: Quaternion slerp
    // Untested...
    // probably wrong
    Quaternion qi = q0.inverse();
    Quaternion qr = q1 * qi;
    if (qr.w < 0) {
        qr = qr.conjugate();
    }
    vec3f vr(qr.x, qr.y, qr.z);
    float tr = 2 * (float)atan(vr.length() / qr.w);
    vec3f nr = vr; nr.normalize();
    float a = tr * t;
    nr *= (float)sin(a / 2);
    return Quaternion(nr.x, nr.y, nr.z, (float)cos(a / 2)) * q0;
}
