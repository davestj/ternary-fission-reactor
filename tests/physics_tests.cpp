#include <cassert>
#include <cmath>
#include <iostream>

#include "physics.constants.definitions.h"

int main() {
    using namespace TernaryFission;

    // Q-value should be positive for realistic mass split
    double q = CALCULATE_Q_VALUE(240.0, 100.0, 132.0, 4.0);
    assert(q > 0.0);

    // Momentum from kinetic energy should be non-negative
    double p = ENERGY_TO_MOMENTUM(5.0, 1.0);
    assert(p >= 0.0);

    // Energy dissipation macro should reduce energy
    double initial = 100.0;
    double dissipated = CALCULATE_ENERGY_DISSIPATION(initial, 1);
    assert(dissipated < initial);

    std::cout << "Physics constant tests passed" << std::endl;
    return 0;
}
