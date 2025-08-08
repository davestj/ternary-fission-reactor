#include "config.ternary.fission.server.h"

namespace TernaryFission {

Configuration::Configuration() : physics_config_() {}

const EnergyFieldConfig& Configuration::getPhysicsConfig() const {
    return physics_config_;
}

} // namespace TernaryFission
