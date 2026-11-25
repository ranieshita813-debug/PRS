#pragma once
#include "PRS/Core/Math/Vector.h"

namespace PRS
{
// Column-major 4x4 matrix: m[column * 4 + row], so it maps directly onto a
// float[16] that can be uploaded to a shader without transposing.
struct Mat4
{
    f32 m[16] = {1.0f, 0.0f, 0.0f, 0.0f,
                 0.0f, 1.0f, 0.0f, 0.0f,
                 0.0f, 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f};

    constexpr Mat4() = default;

    static constexpr Mat4 Identity() { return Mat4{}; }

    static Mat4 Translation(const Vec3& t)
    {
        Mat4 r;
        r.At(0, 3) = t.x;
        r.At(1, 3) = t.y;
        r.At(2, 3) = t.z;
        return r;
    }

    static Mat4 Scale(const Vec3& s)
    {
        Mat4 r;
        r.At(0, 0) = s.x;
        r.At(1, 1) = s.y;
        r.At(2, 2) = s.z;
        return r;
    }

    static Mat4 FromColumns(const Vec3& c0, const Vec3& c1, const Vec3& c2, const Vec3& c3)
    {
        Mat4 r;
        r.At(0, 0) = c0.x; r.At(1, 0) = c0.y; r.At(2, 0) = c0.z; r.At(3, 0) = 0.0f;
        r.At(0, 1) = c1.x; r.At(1, 1) = c1.y; r.At(2, 1) = c1.z; r.At(3, 1) = 0.0f;
        r.At(0, 2) = c2.x; r.At(1, 2) = c2.y; r.At(2, 2) = c2.z; r.At(3, 2) = 0.0f;
        r.At(0, 3) = c3.x; r.At(1, 3) = c3.y; r.At(2, 3) = c3.z; r.At(3, 3) = 1.0f;
        return r;
    }

    static Mat4 Rotation(f32 angleRadians, const Vec3& axis)
    {
        const Vec3 a = Normalize(axis);
        const f32 c = std::cos(angleRadians);
        const f32 s = std::sin(angleRadians);
        const f32 t = 1.0f - c;

        Mat4 r;
        r.At(0, 0) = c + a.x * a.x * t;
        r.At(0, 1) = a.x * a.y * t - a.z * s;
        r.At(0, 2) = a.x * a.z * t + a.y * s;
        r.At(1, 0) = a.y * a.x * t + a.z * s;
        r.At(1, 1) = c + a.y * a.y * t;
        r.At(1, 2) = a.y * a.z * t - a.x * s;
        r.At(2, 0) = a.z * a.x * t - a.y * s;
        r.At(2, 1) = a.z * a.y * t + a.x * s;
        r.At(2, 2) = c + a.z * a.z * t;
        return r;
    }

    // Vulkan/D3D convention: z maps to [0, 1]. Right-handed view space.
    static Mat4 Perspective(f32 fovYRadians, f32 aspect, f32 nearZ, f32 farZ)
    {
        Mat4 r;
        for (f32& v : r.m) v = 0.0f;
        const f32 f = 1.0f / std::tan(fovYRadians * 0.5f);
        r.At(0, 0) = f / (aspect > Epsilon ? aspect : 1.0f);
        r.At(1, 1) = f;
        r.At(2, 2) = farZ / (nearZ - farZ);
        r.At(2, 3) = (nearZ * farZ) / (nearZ - farZ);
        r.At(3, 2) = -1.0f;
        return r;
    }

    // Vulkan/D3D convention: z maps to [0, 1].
    static Mat4 Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ)
    {
        Mat4 r;
        r.At(0, 0) = 2.0f / (right - left);
        r.At(1, 1) = 2.0f / (top - bottom);
        r.At(2, 2) = 1.0f / (nearZ - farZ);
        r.At(0, 3) = -(right + left) / (right - left);
        r.At(1, 3) = -(top + bottom) / (top - bottom);
        r.At(2, 3) = nearZ / (nearZ - farZ);
        return r;
    }

    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
    {
        const Vec3 forward = Normalize(target - eye);
        Vec3 right = Cross(forward, up);
        if (LengthSquared(right) < Epsilon * Epsilon) right = Cross(forward, AxisY());
        right = Normalize(right);
        const Vec3 trueUp = Cross(right, forward);

        // Rows of the view matrix are the basis vectors (column-major storage).
        Mat4 r;
        r.At(0, 0) = right.x;    r.At(0, 1) = right.y;    r.At(0, 2) = right.z;
        r.At(1, 0) = trueUp.x;   r.At(1, 1) = trueUp.y;   r.At(1, 2) = trueUp.z;
        r.At(2, 0) = -forward.x; r.At(2, 1) = -forward.y; r.At(2, 2) = -forward.z;
        r.At(0, 3) = -Dot(right, eye);
        r.At(1, 3) = -Dot(trueUp, eye);
        r.At(2, 3) = Dot(forward, eye);
        return r;
    }

    f32& At(int row, int column) { return m[column * 4 + row]; }
    f32 At(int row, int column) const { return m[column * 4 + row]; }

    Vec3 Column(int column) const
    {
        return {m[column * 4 + 0], m[column * 4 + 1], m[column * 4 + 2]};
    }

    Mat4 operator*(const Mat4& o) const
    {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
        {
            for (int row = 0; row < 4; ++row)
            {
                f32 sum = 0.0f;
                for (int k = 0; k < 4; ++k) sum += At(row, k) * o.At(k, c);
                r.At(row, c) = sum;
            }
        }
        return r;
    }

    Vec4 operator*(const Vec4& v) const
    {
        return {At(0, 0) * v.x + At(0, 1) * v.y + At(0, 2) * v.z + At(0, 3) * v.w,
                At(1, 0) * v.x + At(1, 1) * v.y + At(1, 2) * v.z + At(1, 3) * v.w,
                At(2, 0) * v.x + At(2, 1) * v.y + At(2, 2) * v.z + At(2, 3) * v.w,
                At(3, 0) * v.x + At(3, 1) * v.y + At(3, 2) * v.z + At(3, 3) * v.w};
    }

    Vec3 TransformPoint(const Vec3& p) const
    {
        const Vec4 r = (*this) * Vec4(p, 1.0f);
        return {r.x, r.y, r.z};
    }

    Vec3 TransformDirection(const Vec3& d) const
    {
        const Vec4 r = (*this) * Vec4(d, 0.0f);
        return {r.x, r.y, r.z};
    }

    Vec3 TranslationPart() const { return {m[12], m[13], m[14]}; }

    Mat4 Transposed() const
    {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row) r.At(row, c) = At(c, row);
        return r;
    }

    // General inverse. Returns identity for a singular matrix instead of NaNs.
    Mat4 Inverted() const
    {
        const f32* a = m;
        f32 inv[16];

        inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] +
                 a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
        inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] -
                 a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
        inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] +
                 a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
        inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] -
                  a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
        inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] -
                 a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
        inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] +
                 a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
        inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] -
                 a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
        inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] +
                  a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
        inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] +
                 a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
        inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] -
                 a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
        inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] +
                  a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
        inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] -
                  a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
        inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] -
                 a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
        inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] +
                 a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
        inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] -
                  a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
        inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] +
                  a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];

        const f32 det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
        if (Abs(det) < 1.0e-12f) return Mat4::Identity();

        const f32 invDet = 1.0f / det;
        Mat4 r;
        for (int i = 0; i < 16; ++i) r.m[i] = inv[i] * invDet;
        return r;
    }
};
} // namespace PRS
