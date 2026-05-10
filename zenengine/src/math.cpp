
#include "pch.hpp"
#include "math.hpp"

// ============================================================
// math.cpp - Implementation
// ============================================================

// ----------------------------------------------------------
// Free utilities
// ----------------------------------------------------------

int sign(float value)
{
    return value < 0 ? -1 : (value > 0 ? 1 : 0);
}

float clamp(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float normalize_angle(float angle)
{
    while (angle >= 360.0f) angle -= 360.0f;
    while (angle <    0.0f) angle += 360.0f;
    return angle;
}

float clamp_angle(float angle, float min_val, float max_val)
{
    angle   = normalize_angle(angle);
    min_val = normalize_angle(min_val);
    max_val = normalize_angle(max_val);

    if (angle   > 180.0f) angle   -= 360.0f;
    if (min_val > 180.0f) min_val -= 360.0f;
    if (max_val > 180.0f) max_val -= 360.0f;

    return clamp(angle, min_val, max_val);
}

double get_distx(double angle_deg, double dist)
{
    return std::cos(angle_deg * static_cast<double>(DEG_TO_RAD)) * dist;
}

double get_disty(double angle_deg, double dist)
{
    return -std::sin(angle_deg * static_cast<double>(DEG_TO_RAD)) * dist;
}

// ----------------------------------------------------------
// Vec2 - setters
// ----------------------------------------------------------

void Vec2::set(float px, float py) { x = px; y = py; }
void Vec2::set(const Vec2& other)  { x = other.x; y = other.y; }

// ----------------------------------------------------------
// Vec2 - mutating arithmetic
// ----------------------------------------------------------

Vec2& Vec2::add(const Vec2& o)      { x += o.x; y += o.y; return *this; }
Vec2& Vec2::subtract(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
Vec2& Vec2::multiply(const Vec2& o) { x *= o.x; y *= o.y; return *this; }
Vec2& Vec2::divide(const Vec2& o)   { x /= o.x; y /= o.y; return *this; }

Vec2& Vec2::add(float v)      { x += v; y += v; return *this; }
Vec2& Vec2::subtract(float v) { x -= v; y -= v; return *this; }
Vec2& Vec2::multiply(float v) { x *= v; y *= v; return *this; }
Vec2& Vec2::divide(float v)   { x /= v; y /= v; return *this; }

// ----------------------------------------------------------
// Vec2 - free operators
// ----------------------------------------------------------

Vec2 operator+(Vec2 l, const Vec2& r) { return l.add(r); }
Vec2 operator-(Vec2 l, const Vec2& r) { return l.subtract(r); }
Vec2 operator*(Vec2 l, const Vec2& r) { return l.multiply(r); }
Vec2 operator/(Vec2 l, const Vec2& r) { return l.divide(r); }

Vec2 operator+(Vec2 l, float v) { return Vec2(l.x + v, l.y + v); }
Vec2 operator-(Vec2 l, float v) { return Vec2(l.x - v, l.y - v); }
Vec2 operator*(Vec2 l, float v) { return Vec2(l.x * v, l.y * v); }
Vec2 operator/(Vec2 l, float v) { return Vec2(l.x / v, l.y / v); }
Vec2 operator*(float v, Vec2 l) { return Vec2(l.x * v, l.y * v); }
Vec2 operator-(Vec2 l)          { return Vec2(-l.x, -l.y); }

Vec2& Vec2::operator+=(const Vec2& o) { return add(o); }
Vec2& Vec2::operator-=(const Vec2& o) { return subtract(o); }
Vec2& Vec2::operator*=(const Vec2& o) { return multiply(o); }
Vec2& Vec2::operator/=(const Vec2& o) { return divide(o); }
Vec2& Vec2::operator+=(float v)       { return add(v); }
Vec2& Vec2::operator-=(float v)       { return subtract(v); }
Vec2& Vec2::operator*=(float v)       { return multiply(v); }
Vec2& Vec2::operator/=(float v)       { return divide(v); }

bool Vec2::operator==(const Vec2& o) const { return x == o.x && y == o.y; }
bool Vec2::operator!=(const Vec2& o) const { return !(*this == o); }
bool Vec2::operator< (const Vec2& o) const { return x <  o.x && y <  o.y; }
bool Vec2::operator<=(const Vec2& o) const { return x <= o.x && y <= o.y; }
bool Vec2::operator> (const Vec2& o) const { return x >  o.x && y >  o.y; }
bool Vec2::operator>=(const Vec2& o) const { return x >= o.x && y >= o.y; }

// ----------------------------------------------------------
// Vec2 - geometry
// ----------------------------------------------------------

float Vec2::magnitude() const
{
    return std::sqrt(x * x + y * y);
}

float Vec2::distance(const Vec2& b) const
{
    float dx = x - b.x;
    float dy = y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

float Vec2::dot(const Vec2& b) const
{
    return x * b.x + y * b.y;
}

Vec2 Vec2::normalised() const
{
    float len = magnitude();
    return Vec2(x / len, y / len);
}

Vec2 Vec2::perp() const
{
    return Vec2(-y, x);
}

Vec2 Vec2::normal() const
{
    return Vec2(y, -x).normalised();
}

Vec2 Vec2::rotate(float angle_rad) const
{
    float s = std::sin(angle_rad);
    float c = std::cos(angle_rad);
    return Vec2(x * c - y * s, x * s + y * c);
}

Vec2 Vec2::RotatePoint(float X, float Y, float PivotX, float PivotY, float Angle)
{
    // Fast polynomial sin/cos approximation (original algorithm preserved)
    float s = 0.0f;
    float c = 0.0f;
    float r = Angle * -0.017453293f;

    while (r < -3.14159265f) r += 6.28318531f;
    while (r >  3.14159265f) r -= 6.28318531f;

    if (r < 0)
    {
        s = 1.27323954f * r + 0.405284735f * r * r;
        s = (s < 0) ? 0.225f * (s * -s - s) + s : 0.225f * (s * s - s) + s;
    }
    else
    {
        s = 1.27323954f * r - 0.405284735f * r * r;
        s = (s < 0) ? 0.225f * (s * -s - s) + s : 0.225f * (s * s - s) + s;
    }

    r += 1.57079632f;
    if (r > 3.14159265f) r -= 6.28318531f;

    if (r < 0)
    {
        c = 1.27323954f * r + 0.405284735f * r * r;
        c = (c < 0) ? 0.225f * (c * -c - c) + c : 0.225f * (c * c - c) + c;
    }
    else
    {
        c = 1.27323954f * r - 0.405284735f * r * r;
        c = (c < 0) ? 0.225f * (c * -c - c) + c : 0.225f * (c * c - c) + c;
    }

    float dx = X - PivotX;
    float dy = PivotY - Y;

    return Vec2(PivotX + c * dx - s * dy,
                PivotY - s * dx - c * dy);
}

Vec2 Vec2::Normalize(const Vec2 &v)
{
    float len = v.magnitude();
    return Vec2(v.x / len, v.y / len);
}

// ----------------------------------------------------------
// Matrix2D
// ----------------------------------------------------------

Matrix2D::Matrix2D()
{
    Identity();
}

void Matrix2D::Identity()
{
    a  = 1.0f; b  = 0.0f;
    c  = 0.0f; d  = 1.0f;
    tx = 0.0f; ty = 0.0f;
}

void Matrix2D::Set(float pa, float pb, float pc, float pd, float ptx, float pty)
{
    a = pa; b = pb; c = pc; d = pd; tx = ptx; ty = pty;
}

void Matrix2D::Concat(const Matrix2D& m)
{
    float a1  = a  * m.a + b  * m.c;
    float b1  = a  * m.b + b  * m.d;
    float c1  = c  * m.a + d  * m.c;
    float d1  = c  * m.b + d  * m.d;
    float tx1 = tx * m.a + ty * m.c + m.tx;
    float ty1 = tx * m.b + ty * m.d + m.ty;

    a = a1; b = b1; c = c1; d = d1; tx = tx1; ty = ty1;
}

Vec2 Matrix2D::TransformCoords(float px, float py) const
{
    return Vec2(a * px + c * py + tx,
                b * px + d * py + ty);
}

Vec2 Matrix2D::TransformCoords(const Vec2& point) const
{
    return TransformCoords(point.x, point.y);
}

void Matrix2D::Rotate(float angle_rad)
{
    float ac = std::cos(angle_rad);
    float as = std::sin(angle_rad);

    float a1  = a  * ac - b  * as;
    float b1  = a  * as + b  * ac;
    float c1  = c  * ac - d  * as;
    float d1  = c  * as + d  * ac;
    float tx1 = tx * ac - ty * as;
    float ty1 = tx * as + ty * ac;

    a = a1; b = b1; c = c1; d = d1; tx = tx1; ty = ty1;
}

void Matrix2D::Scale(float sx, float sy)
{
    a *= sx;  b *= sy;
    c *= sx;  d *= sy;
    tx *= sx; ty *= sy;
}

void Matrix2D::Translate(float x, float y)
{
    tx += x;
    ty += y;
}

void Matrix2D::Skew(float skewX, float skewY)
{
    float sinX = std::sin(skewX);
    float cosX = std::cos(skewX);
    float sinY = std::sin(skewY);
    float cosY = std::cos(skewY);

    Set(a  * cosY - b  * sinX,
        a  * sinY + b  * cosX,
        c  * cosY - d  * sinX,
        c  * sinY + d  * cosX,
        tx * cosY - ty * sinX,
        tx * sinY + ty * cosX);
}

Matrix2D Matrix2D::Invert() const
{
    float det = a * d - b * c;

    if (std::fabs(det) < 1e-6f)
    {
        // Degenerate matrix (zero scale) - return identity
        return Matrix2D();
    }

    float inv_det = 1.0f / det;

    Matrix2D inv;
    inv.a  =  d * inv_det;
    inv.b  = -b * inv_det;
    inv.c  = -c * inv_det;
    inv.d  =  a * inv_det;
    inv.tx = (c * ty - d * tx) * inv_det;
    inv.ty = (b * tx - a * ty) * inv_det;

    return inv;
}

Matrix2D Matrix2D::GetTransformation(float x, float y,
                                     float angle_deg,
                                     const Vec2& pivot,
                                     const Vec2& scale)
{
    Matrix2D mat;

    if (angle_deg == 0.0f)
    {
        mat.Set(scale.x, 0.0f, 0.0f, scale.y,
                x - pivot.x * scale.x,
                y - pivot.y * scale.y);
    }
    else
    {
        float rad  = angle_deg * DEG_TO_RAD;
        float acos = std::cos(rad);
        float asin = std::sin(rad);

        float ma = scale.x *  acos;
        float mb = scale.x *  asin;
        float mc = scale.y * -asin;
        float md = scale.y *  acos;

        mat.Set(ma, mb, mc, md,
                x - pivot.x * ma - pivot.y * mc,
                y - pivot.x * mb - pivot.y * md);
    }

    return mat;
}

// ----------------------------------------------------------
// Free matrix functions
// ----------------------------------------------------------

Matrix2D Matrix2DMult(const Matrix2D& m1, const Matrix2D& m2)
{
    Matrix2D r;
    r.a  = m1.a  * m2.a + m1.b  * m2.c;
    r.b  = m1.a  * m2.b + m1.b  * m2.d;
    r.c  = m1.c  * m2.a + m1.d  * m2.c;
    r.d  = m1.c  * m2.b + m1.d  * m2.d;
    r.tx = m1.tx * m2.a + m1.ty * m2.c + m2.tx;
    r.ty = m1.tx * m2.b + m1.ty * m2.d + m2.ty;
    return r;
}

Matrix2D GetRelativeTransformation(float x,       float y,
                                   float scale_x, float scale_y,
                                   float skew_x,  float skew_y,
                                   float pivot_x, float pivot_y,
                                   float angle_deg)
{
    Matrix2D mat;

    if (skew_x == 0.0f && skew_y == 0.0f)
    {
        return Matrix2D::GetTransformation(x, y, angle_deg,
                                           Vec2(pivot_x, pivot_y),
                                           Vec2(scale_x, scale_y));
    }

    mat.Identity();
    mat.Scale(scale_x, scale_y);
    mat.Skew(skew_x, skew_y);
    mat.Rotate(angle_deg * DEG_TO_RAD);
    mat.Translate(x, y);

    if (pivot_x != 0.0f || pivot_y != 0.0f)
    {
        mat.tx = x - mat.a * pivot_x - mat.c * pivot_y;
        mat.ty = y - mat.b * pivot_x - mat.d * pivot_y;
    }

    return mat;
}
