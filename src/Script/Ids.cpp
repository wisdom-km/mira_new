// Ids: Implementation for the DirectorDesk Script module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Script/Ids.h"

#include <iomanip>
#include <random>
#include <sstream>

namespace DirectorDesk::Script {
namespace {

std::string RandomSuffix() {
    static thread_local std::mt19937 engine{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 15);
    std::ostringstream out;
    for (int i = 0; i < 8; ++i) {
        out << std::hex << dist(engine);
    }
    return out.str();
}

} // namespace

bool IsValidId(const std::string& id) {
    if (id.empty() || id.size() > 64) {
        return false;
    }
    const unsigned char first = static_cast<unsigned char>(id[0]);
    if (!((first >= 'a' && first <= 'z') || (first >= '0' && first <= '9'))) {
        return false;
    }
    for (std::size_t i = 1; i < id.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(id[i]);
        if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-')) {
            return false;
        }
    }
    return true;
}

std::string GenerateSceneId() {
    return "scene-" + RandomSuffix();
}

std::string GenerateShotId() {
    return "shot-" + RandomSuffix();
}

} // namespace DirectorDesk::Script
