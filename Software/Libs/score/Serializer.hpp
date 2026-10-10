/**
 * @file  Serializer.hpp
 * @brief POD framing for flash files: {magic, size, FNV-1a checksum} + raw bytes.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace score::ser {

struct Header {
    uint32_t magic;
    uint32_t size;       ///< payload bytes
    uint32_t checksum;   ///< FNV-1a 32 over payload
};
static_assert(sizeof(Header) == 12, "Header must stay 12 bytes");

constexpr uint32_t kMagicSession  = 0x31535348u;  // 'HSS1'
constexpr uint32_t kMagicIndex    = 0x32584948u;  // 'HIX2'
constexpr uint32_t kMagicWorkouts = 0x314B5748u;  // 'HWK1'
constexpr uint32_t kMagicSettings = 0x31545348u;  // 'HST1'

/// FNV-1a 32-bit.
uint32_t checksum(const void* data, size_t len);
/// Bytes written (sizeof(Header) + size), 0 if cap too small.
size_t encode(uint32_t magic, const void* obj, size_t size, void* out, size_t cap);
/// False on any length/magic/size/checksum mismatch; obj untouched then.
bool decode(uint32_t magic, const void* in, size_t len, void* obj, size_t size);

} // namespace score::ser
