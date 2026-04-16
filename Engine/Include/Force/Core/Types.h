#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <functional>

namespace Force
{
    // Fixed-width integer types
    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;
    
    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    
    // Floating point types
    using f32 = float;
    using f64 = double;
    
    // Size types
    using usize = std::size_t;
    using isize = std::ptrdiff_t;
    
    // Byte type
    using byte = std::byte;
    
    // UUID type (128-bit)
    struct UUID
    {
        u64 High = 0;
        u64 Low = 0;
        
        bool operator==(const UUID& other) const
        {
            return High == other.High && Low == other.Low;
        }
        
        bool operator!=(const UUID& other) const
        {
            return !(*this == other);
        }
        
        bool IsValid() const { return High != 0 || Low != 0; }
        
        static UUID Generate();
        static UUID FromString(std::string_view str);
        std::string ToString() const;
    };
    
    // Handle type for resources
    template<typename T>
    struct Handle
    {
        u32 Index = UINT32_MAX;
        u32 Generation = 0;
        
        bool IsValid() const { return Index != UINT32_MAX; }
        
        bool operator==(const Handle& other) const
        {
            return Index == other.Index && Generation == other.Generation;
        }
        
        bool operator!=(const Handle& other) const
        {
            return !(*this == other);
        }
    };
}

// Hash specialization for UUID
template<>
struct std::hash<Force::UUID>
{
    std::size_t operator()(const Force::UUID& uuid) const noexcept
    {
        return std::hash<Force::u64>{}(uuid.High) ^ (std::hash<Force::u64>{}(uuid.Low) << 1);
    }
};
