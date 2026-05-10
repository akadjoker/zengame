#pragma once

#include "pch.hpp"

// ============================================================
// math.hpp - Math types: Vec2, Matrix2D
// Angle convention: degrees in the public API, radians internally
// ============================================================

constexpr float M_PI_F     = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = M_PI_F / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / M_PI_F;

// ----------------------------------------------------------
// Free utilities
// ----------------------------------------------------------

int   sign(float value);
float clamp(float value, float min_val, float max_val);
float lerp(float a, float b, float t);
float normalize_angle(float angle);
float clamp_angle(float angle, float min_val, float max_val);
double get_distx(double angle_deg, double dist);
double get_disty(double angle_deg, double dist);

// ============================================================
// Vec2
// ============================================================

struct Vec2
{
    float x;
    float y;

    Vec2()                 : x(0.0f), y(0.0f) {}
    Vec2(float xy)         : x(xy),   y(xy)   {}
    Vec2(float x, float y) : x(x),    y(y)    {}

    void set(float x, float y);
    void set(const Vec2& other);

    Vec2& add(const Vec2& other);
    Vec2& subtract(const Vec2& other);
    Vec2& multiply(const Vec2& other);
    Vec2& divide(const Vec2& other);

    Vec2& add(float value);
    Vec2& subtract(float value);
    Vec2& multiply(float value);
    Vec2& divide(float value);

    friend Vec2 operator+(Vec2 left, const Vec2& right);
    friend Vec2 operator-(Vec2 left, const Vec2& right);
    friend Vec2 operator*(Vec2 left, const Vec2& right);
    friend Vec2 operator/(Vec2 left, const Vec2& right);
    friend Vec2 operator+(Vec2 left, float v);
    friend Vec2 operator-(Vec2 left, float v);
    friend Vec2 operator*(Vec2 left, float v);
    friend Vec2 operator/(Vec2 left, float v);
    friend Vec2 operator*(float v, Vec2 left);
    friend Vec2 operator-(Vec2 left);

    Vec2& operator+=(const Vec2& other);
    Vec2& operator-=(const Vec2& other);
    Vec2& operator*=(const Vec2& other);
    Vec2& operator/=(const Vec2& other);
    Vec2& operator+=(float v);
    Vec2& operator-=(float v);
    Vec2& operator*=(float v);
    Vec2& operator/=(float v);

    bool operator==(const Vec2& other) const;
    bool operator!=(const Vec2& other) const;
    bool operator< (const Vec2& other) const;
    bool operator<=(const Vec2& other) const;
    bool operator> (const Vec2& other) const;
    bool operator>=(const Vec2& other) const;

    float magnitude()              const;
    float distance(const Vec2& b)  const;
    float dot(const Vec2& b)       const;
    Vec2  normalised()             const;
    Vec2  perp()                   const;
    Vec2  normal()                 const;
    Vec2  rotate(float angle_rad)  const;

    static Vec2 RotatePoint(float X, float Y,
                             float PivotX, float PivotY,
                             float Angle);
    static Vec2 NormalizeAngle(float angle);
    static Vec2 Normalize(const Vec2& v);
};

// ============================================================
// Matrix2D - 2D affine transform
//
//  | a  c  tx |
//  | b  d  ty |
//  | 0  0   1 |
//
//  out.x = a*P.x + c*P.y + tx
//  out.y = b*P.x + d*P.y + ty
// ============================================================

struct Matrix2D
{
    float a, b, c, d;
    float tx, ty;

    Matrix2D();
    void Identity();
    void Set(float a, float b, float c, float d, float tx, float ty);
    void Concat(const Matrix2D& m);

    Vec2 TransformCoords(float x, float y)  const;
    Vec2 TransformCoords(const Vec2& point) const;

    void Rotate(float angle_rad);
    void Scale(float x, float y);
    void Translate(float x, float y);
    void Skew(float skewX, float skewY);

    Matrix2D Invert() const;

    static Matrix2D GetTransformation(float x, float y,
                                      float angle_deg,
                                      const Vec2& pivot,
                                      const Vec2& scale);
};

Matrix2D Matrix2DMult(const Matrix2D& m1, const Matrix2D& m2);

Matrix2D GetRelativeTransformation(float x,       float y,
                                   float scale_x, float scale_y,
                                   float skew_x,  float skew_y,
                                   float pivot_x, float pivot_y,
                                   float angle_deg);
