#ifndef CONFIG_TERNARY_FISSION_SERVER_H
#define CONFIG_TERNARY_FISSION_SERVER_H

#include "physics.utilities.h"

namespace TernaryFission {

class Configuration {
public:
    Configuration();
    const EnergyFieldConfig& getPhysicsConfig() const;

private:
    EnergyFieldConfig physics_config_;
};

} // namespace TernaryFission

#endif // CONFIG_TERNARY_FISSION_SERVER_H
