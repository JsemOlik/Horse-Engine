#pragma once

#include "HorseEngine/Core.h"
#include <functional>
#include <string>

namespace Horse {

// 64-bit UUID for entities and assets
class HORSE_API UUID {
public:
  UUID();
  explicit UUID(u64 uuid);

  operator u64() const { return m_UUID; }

  bool operator==(const UUID &other) const { return m_UUID == other.m_UUID; }
  bool operator!=(const UUID &other) const { return m_UUID != other.m_UUID; }

  std::string ToString() const;
  static UUID FromString(const std::string &str);

private:
  u64 m_UUID;
};

} // namespace Horse

namespace std {
template <> struct hash<Horse::UUID> {
  size_t operator()(const Horse::UUID &uuid) const {
    return hash<u64>()(static_cast<u64>(uuid));
  }
};
} // namespace std
