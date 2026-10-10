#include "score/Serializer.hpp"

#include <cstring>

namespace score::ser {

uint32_t checksum(const void* data, size_t len)
{
    const auto* p = static_cast<const uint8_t*>(data);
    uint32_t h = 0x811C9DC5u;
    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 0x01000193u;
    }
    return h;
}

size_t encode(uint32_t magic, const void* obj, size_t size, void* out, size_t cap)
{
    if (cap < sizeof(Header) + size) return 0;
    const Header h{ magic, static_cast<uint32_t>(size), checksum(obj, size) };
    auto* o = static_cast<uint8_t*>(out);
    std::memcpy(o, &h, sizeof h);
    std::memcpy(o + sizeof h, obj, size);
    return sizeof h + size;
}

bool decode(uint32_t magic, const void* in, size_t len, void* obj, size_t size)
{
    if (len != sizeof(Header) + size) return false;
    const auto* p = static_cast<const uint8_t*>(in);
    Header h;
    std::memcpy(&h, p, sizeof h);
    if (h.magic != magic || h.size != size) return false;
    if (h.checksum != checksum(p + sizeof h, size)) return false;
    std::memcpy(obj, p + sizeof h, size);
    return true;
}

} // namespace score::ser
