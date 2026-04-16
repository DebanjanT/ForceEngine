#pragma once

#include "Force/Core/Types.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Force
{
    // Type aliases for GLM types
    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;
    
    using IVec2 = glm::ivec2;
    using IVec3 = glm::ivec3;
    using IVec4 = glm::ivec4;
    
    using UVec2 = glm::uvec2;
    using UVec3 = glm::uvec3;
    using UVec4 = glm::uvec4;
    
    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;
    
    using Quat = glm::quat;
    
    // Constants
    constexpr f32 PI = 3.14159265358979323846f;
    constexpr f32 TWO_PI = PI * 2.0f;
    constexpr f32 HALF_PI = PI / 2.0f;
    constexpr f32 DEG_TO_RAD = PI / 180.0f;
    constexpr f32 RAD_TO_DEG = 180.0f / PI;
    constexpr f32 EPSILON = 1e-6f;
    
    // Math utilities
    inline f32 Radians(f32 degrees) { return glm::radians(degrees); }
    inline f32 Degrees(f32 radians) { return glm::degrees(radians); }
    
    inline f32 Lerp(f32 a, f32 b, f32 t) { return glm::mix(a, b, t); }
    inline Vec3 Lerp(const Vec3& a, const Vec3& b, f32 t) { return glm::mix(a, b, t); }
    inline Quat Slerp(const Quat& a, const Quat& b, f32 t) { return glm::slerp(a, b, t); }
    
    inline f32 Clamp(f32 value, f32 min, f32 max) { return glm::clamp(value, min, max); }
    inline Vec3 Clamp(const Vec3& value, const Vec3& min, const Vec3& max) { return glm::clamp(value, min, max); }
    
    inline f32 Abs(f32 value) { return glm::abs(value); }
    inline f32 Sign(f32 value) { return glm::sign(value); }
    inline f32 Floor(f32 value) { return glm::floor(value); }
    inline f32 Ceil(f32 value) { return glm::ceil(value); }
    inline f32 Round(f32 value) { return glm::round(value); }
    
    inline f32 Min(f32 a, f32 b) { return glm::min(a, b); }
    inline f32 Max(f32 a, f32 b) { return glm::max(a, b); }
    
    inline f32 Sqrt(f32 value) { return glm::sqrt(value); }
    inline f32 Pow(f32 base, f32 exp) { return glm::pow(base, exp); }
    
    inline f32 Sin(f32 angle) { return glm::sin(angle); }
    inline f32 Cos(f32 angle) { return glm::cos(angle); }
    inline f32 Tan(f32 angle) { return glm::tan(angle); }
    inline f32 Asin(f32 value) { return glm::asin(value); }
    inline f32 Acos(f32 value) { return glm::acos(value); }
    inline f32 Atan2(f32 y, f32 x) { return glm::atan(y, x); }
    
    // Vector operations
    inline f32 Dot(const Vec3& a, const Vec3& b) { return glm::dot(a, b); }
    inline Vec3 Cross(const Vec3& a, const Vec3& b) { return glm::cross(a, b); }
    inline f32 Length(const Vec3& v) { return glm::length(v); }
    inline f32 LengthSquared(const Vec3& v) { return glm::dot(v, v); }
    inline Vec3 Normalize(const Vec3& v) { return glm::normalize(v); }
    inline Vec3 Reflect(const Vec3& v, const Vec3& n) { return glm::reflect(v, n); }
    
    // Matrix operations
    inline Mat4 Translate(const Mat4& m, const Vec3& v) { return glm::translate(m, v); }
    inline Mat4 Rotate(const Mat4& m, f32 angle, const Vec3& axis) { return glm::rotate(m, angle, axis); }
    inline Mat4 Scale(const Mat4& m, const Vec3& v) { return glm::scale(m, v); }
    
    inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) { return glm::lookAt(eye, center, up); }
    inline Mat4 Perspective(f32 fov, f32 aspect, f32 near, f32 far) { return glm::perspective(fov, aspect, near, far); }
    inline Mat4 Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) { return glm::ortho(left, right, bottom, top, near, far); }
    
    inline Mat4 Inverse(const Mat4& m) { return glm::inverse(m); }
    inline Mat4 Transpose(const Mat4& m) { return glm::transpose(m); }
    
    // Quaternion operations
    inline Quat QuatFromAxisAngle(const Vec3& axis, f32 angle) { return glm::angleAxis(angle, axis); }
    inline Quat QuatFromEuler(const Vec3& euler) { return Quat(euler); }
    inline Vec3 QuatToEuler(const Quat& q) { return glm::eulerAngles(q); }
    inline Mat4 QuatToMat4(const Quat& q) { return glm::toMat4(q); }
    
    // AABB (Axis-Aligned Bounding Box)
    struct AABB
    {
        Vec3 Min = Vec3(0.0f);
        Vec3 Max = Vec3(0.0f);
        
        AABB() = default;
        AABB(const Vec3& min, const Vec3& max) : Min(min), Max(max) {}
        
        Vec3 GetCenter() const { return (Min + Max) * 0.5f; }
        Vec3 GetExtents() const { return (Max - Min) * 0.5f; }
        Vec3 GetSize() const { return Max - Min; }
        
        bool Contains(const Vec3& point) const
        {
            return point.x >= Min.x && point.x <= Max.x &&
                   point.y >= Min.y && point.y <= Max.y &&
                   point.z >= Min.z && point.z <= Max.z;
        }
        
        bool Intersects(const AABB& other) const
        {
            return Min.x <= other.Max.x && Max.x >= other.Min.x &&
                   Min.y <= other.Max.y && Max.y >= other.Min.y &&
                   Min.z <= other.Max.z && Max.z >= other.Min.z;
        }
        
        void Expand(const Vec3& point)
        {
            Min = glm::min(Min, point);
            Max = glm::max(Max, point);
        }
        
        void Expand(const AABB& other)
        {
            Min = glm::min(Min, other.Min);
            Max = glm::max(Max, other.Max);
        }
    };
    
    // Ray
    struct Ray
    {
        Vec3 Origin = Vec3(0.0f);
        Vec3 Direction = Vec3(0.0f, 0.0f, -1.0f);
        
        Ray() = default;
        Ray(const Vec3& origin, const Vec3& direction) : Origin(origin), Direction(Normalize(direction)) {}
        
        Vec3 GetPoint(f32 t) const { return Origin + Direction * t; }
    };
    
    // Frustum planes for culling
    struct Frustum
    {
        Vec4 Planes[6]; // Left, Right, Bottom, Top, Near, Far
        
        void ExtractFromMatrix(const Mat4& viewProj);
        bool ContainsPoint(const Vec3& point) const;
        bool ContainsAABB(const AABB& aabb) const;
    };
}
