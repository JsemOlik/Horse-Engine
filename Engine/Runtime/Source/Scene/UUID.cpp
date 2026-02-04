#include "HorseEngine/Scene/UUID.h"
#include <iomanip>
#include <random>
#include <sstream>


namespace Horse {

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<u64> s_UniformDistribution;

UUID::UUID() : m_UUID(s_UniformDistribution(s_Engine)) {}

UUID::UUID(u64 uuid) : m_UUID(uuid) {}

std::string UUID::ToString() const {
  std::stringstream ss;
  ss << std::hex << std::setw(16) << std::setfill('0') << m_UUID;
  return ss.str();
}

UUID UUID::FromString(const std::string &str) {
  std::stringstream ss(str);
  u64 uuid;
  ss >> std::hex >> uuid;
  return UUID(uuid);
}

} // namespace Horse
