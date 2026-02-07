#pragma once

#include "HorseEngine/Core.h"
#include <string>

namespace Horse {

namespace Hash {

// Stable FNV-1a 64-bit hash
inline u64 HashString(const std::string &str) {
  u64 hash = 0xcbf29ce484222325; // FNV offset basis
  for (char c : str) {
    hash ^= static_cast<u64>(c);
    hash *= 0x100000001b3; // FNV prime
  }
  return hash;
}

} // namespace Hash

} // namespace Horse
