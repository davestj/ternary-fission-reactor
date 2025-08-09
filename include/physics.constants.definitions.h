/*
 * File: include/physics.constants.definitions.h
 * Author: bthlops (David StJ)
 * Date: July 30, 2025
 * Title: Ternary Fission Physics Constants and Definitions - FIXED
 * Purpose: Core physics constants, structures, and definitions for ternary fission energy emulation
 * Reason: Provides foundational physics parameters and data structures for simulation engine
 *
 * Change Log:
 * 2025-07-29: Initial creation with fundamental physics constants and ternary fission parameters
 * 2025-07-29: Added energy field structures and entropy simulation definitions
 * 2025-07-29: Implemented memory-CPU cycle mapping for energy representation
 * 2025-07-30: FIXED missing standard library includes causing size_t compilation errors
 *             Added complete C++ standard library headers for GCC 12.2/13.3 compatibility
 *             Ensures proper Ubuntu 24.04 and Debian 12 compatibility
 * 2025-07-31: Updated EnergyField defaults and cleaned legacy references
 *
 * Carry-over Context:
 * - We use these constants throughout the C++ simulation engine
 * - Energy fields are represented as memory allocation and CPU cycle consumption
 * - Ternary fission modeling includes three-fragment decay with momentum conservation
 * - Encryption rounds simulate energy dissipation over time
 * - Fixed compilation issues by adding proper standard library includes
 */

#ifndef PHYSICS_CONSTANTS_DEFINITIONS_H
#define PHYSICS_CONSTANTS_DEFINITIONS_H

// We include all necessary C++ standard library headers for proper compilation
#include <cstddef>      // For size_t, ptrdiff_t
#include <cstdint>      // For uint64_t, int64_t, etc.
#include <cmath>        // For mathematical functions
#include <vector>       // For std::vector
#include <memory>       // For smart pointers
#include <chrono>       // For time measurements
#include <random>       // For random number generation
#include <atomic>       // For atomic operations
#include <mutex>        // For thread synchronization

namespace TernaryFission {

    // =============================================================================
    // FUNDAMENTAL PHYSICS CONSTANTS
    // =============================================================================

    // Speed of light in vacuum (m/s)
    constexpr double SPEED_OF_LIGHT = 299792458.0;

    // Planck's constant (J⋅s)
    constexpr double PLANCK_CONSTANT = 6.62607015e-34;

    // Reduced Planck constant ℏ = h/(2π)
    constexpr double HBAR = PLANCK_CONSTANT / (2.0 * M_PI);

    // Boltzmann constant (J/K)
    constexpr double BOLTZMANN_CONSTANT = 1.380649e-23;

    // Avogadro's number (mol⁻¹)
    constexpr double AVOGADRO_NUMBER = 6.02214076e23;

    // Atomic mass unit (kg)
    constexpr double ATOMIC_MASS_UNIT = 1.66053906660e-27;

    // Electron rest mass (kg)
    constexpr double ELECTRON_MASS = 9.1093837015e-31;

    // Proton rest mass (kg)
    constexpr double PROTON_MASS = 1.67262192369e-27;

    // Neutron rest mass (kg)
    constexpr double NEUTRON_MASS = 1.67492749804e-27;

    // =============================================================================
    // TERNARY FISSION SPECIFIC CONSTANTS
    // =============================================================================

    // Typical ternary fission Q-value (MeV) - Energy released
    constexpr double TERNARY_Q_VALUE = 200.0;

    // Alpha particle binding energy (MeV)
    constexpr double ALPHA_BINDING_ENERGY = 28.3;

    // Typical fission fragment masses (u - atomic mass units)
    constexpr double LIGHT_FRAGMENT_MASS = 95.0;
    constexpr double HEAVY_FRAGMENT_MASS = 140.0;
    constexpr double ALPHA_PARTICLE_MASS = 4.002603;

    // Energy conversion factor (MeV to Joules)
    constexpr double MEV_TO_JOULES = 1.602176634e-13;

    // =============================================================================
    // ENERGY FIELD EMULATION PARAMETERS
    // =============================================================================

    // Scaling factors for energy representation
    constexpr double ENERGY_TO_MEMORY_SCALE = 1.0e6;      // 1 MeV = 1MB memory allocation
    constexpr double ENERGY_TO_CPU_CYCLES = 1.0e9;        // 1 MeV = 1B CPU cycles
    constexpr double ENTROPY_DECAY_CONSTANT = 0.693147;   // ln(2) for half-life calculations

    // Encryption-based energy dissipation parameters
    constexpr int MAX_ENCRYPTION_ROUNDS = 256;
    constexpr double DISSIPATION_PER_ROUND = 0.01;        // 1% energy loss per encryption round

    // =============================================================================
    // DATA STRUCTURES FOR TERNARY FISSION MODELING
    // =============================================================================

    /**
     * Simple 3D vector structure for momentum and position
     */
    struct Vector3 {
        double x, y, z;

        Vector3() : x(0.0), y(0.0), z(0.0) {}
    };

    /**
     * We define the fission fragment structure to represent decay products
     * Each fragment carries mass, energy, momentum, position and decay properties
     */
    struct FissionFragment {
        double mass;              // Fragment mass in atomic mass units
        int atomic_number;        // Number of protons
        int mass_number;          // Number of nucleons
        double kinetic_energy;    // Kinetic energy in MeV
        double binding_energy;    // Binding energy in MeV
        double excitation_energy; // Excitation energy in MeV
        Vector3 momentum;         // Momentum vector (MeV/c)
        Vector3 position;         // Position vector (fm or arbitrary units)
        double half_life;         // Decay half-life in seconds

        FissionFragment()
            : mass(0.0), atomic_number(0), mass_number(0),
              kinetic_energy(0.0), binding_energy(0.0), excitation_energy(0.0),
              momentum(), position(), half_life(0.0) {}
    };

    /**
     * We represent the complete ternary fission event
     * This structure contains all three fragments and conservation laws
     */
    struct TernaryFissionEvent {
        std::uint64_t event_id;            // Unique event identifier
        std::uint64_t energy_field_id;     // Associated energy field identifier

        FissionFragment light_fragment;    // Lighter fission fragment
        FissionFragment heavy_fragment;    // Heavier fission fragment
        FissionFragment alpha_particle;    // Third particle (usually alpha)

        double total_kinetic_energy;       // Total KE released (MeV)
        double q_value;                    // Q-value of reaction (MeV)
        double binding_energy_released;    // Binding energy released (MeV)

        // Conservation verification
        bool momentum_conserved;
        bool energy_conserved;
        double energy_conservation_error;    // Energy conservation deviation (MeV)
        double momentum_conservation_error;  // Momentum conservation deviation (MeV/c)
        bool mass_number_conserved;
        bool charge_conserved;

        // Simulation timestamp
        std::chrono::high_resolution_clock::time_point timestamp;

        TernaryFissionEvent() : event_id(0), energy_field_id(0),
                               total_kinetic_energy(0.0), q_value(0.0),
                               binding_energy_released(0.0),
                               momentum_conserved(false), energy_conserved(false),
                               energy_conservation_error(0.0),
                               momentum_conservation_error(0.0),
                               mass_number_conserved(false),
                               charge_conserved(false) {}
    };

    /**
     * We model the energy field as computational resource consumption
     * Memory allocation and CPU cycles represent energy field intensity
     */
    struct EnergyField {
        std::uint64_t field_id;                                            // Unique field identifier
        double energy_mev;                                                 // Energy level in MeV
        std::size_t memory_bytes;                                         // Bytes allocated to represent energy
        std::uint64_t cpu_cycles;                                         // CPU cycles used for calculations
        double entropy_factor;                                            // Thermodynamic entropy component
        double dissipation_rate;                                          // Energy dissipation rate
        double stability_factor;                                          // Field stability coefficient
        double interaction_strength;                                      // Interaction strength coefficient
        std::chrono::high_resolution_clock::time_point creation_time;     // Creation timestamp
        void* memory_ptr;                                                 // Pointer to allocated memory

        EnergyField() : field_id(0), energy_mev(0.0),
                       memory_bytes(0), cpu_cycles(0),
                       entropy_factor(1.0), dissipation_rate(0.0),
                       stability_factor(1.0), interaction_strength(0.0),
                       creation_time(std::chrono::high_resolution_clock::now()),
                       memory_ptr(nullptr) {}
    };

    /**
     * We define the simulation state to track system-wide parameters
     * This allows us to monitor and control the entire emulation process
     */
    struct SimulationState {
        std::vector<TernaryFissionEvent> fission_events;
        std::vector<EnergyField> active_energy_fields;

        // System-wide metrics
        double total_energy_simulated;     // Total energy processed (MeV)
        std::uint64_t total_fission_events;     // Number of events processed
        std::size_t peak_memory_usage;          // Maximum memory allocated

        // Random number generation for physics simulation
        std::mt19937_64 random_generator;
        std::uniform_real_distribution<double> uniform_dist;
        std::normal_distribution<double> gaussian_dist;

        // System state flags
        bool simulation_running;
        bool energy_conservation_enabled;
        bool momentum_conservation_enabled;

        SimulationState() : total_energy_simulated(0.0), total_fission_events(0),
                           peak_memory_usage(0), uniform_dist(0.0, 1.0),
                           gaussian_dist(0.0, 1.0), simulation_running(false),
                           energy_conservation_enabled(true),
                           momentum_conservation_enabled(true) {
            // We seed the random generator with current time
            auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            random_generator.seed(static_cast<std::mt19937_64::result_type>(seed));
        }
    };

    // =============================================================================
    // PHYSICS CALCULATION MACROS
    // =============================================================================

    /**
     * We calculate the Q-value for ternary fission using mass-energy equivalence
     * Q = (M_parent - M_fragment1 - M_fragment2 - M_fragment3) * c²
     */
    #define CALCULATE_Q_VALUE(parent_mass, frag1_mass, frag2_mass, frag3_mass) \
        ((parent_mass) - (frag1_mass) - (frag2_mass) - (frag3_mass)) * SPEED_OF_LIGHT * SPEED_OF_LIGHT

    /**
     * We convert kinetic energy to momentum using relativistic formula
     * p = √(E² - (mc²)²) / c for relativistic particles
     */
    #define ENERGY_TO_MOMENTUM(kinetic_energy, rest_mass) \
        (sqrt(pow((kinetic_energy) + (rest_mass) * SPEED_OF_LIGHT * SPEED_OF_LIGHT, 2) - \
              pow((rest_mass) * SPEED_OF_LIGHT * SPEED_OF_LIGHT, 2)) / SPEED_OF_LIGHT)

    /**
     * We calculate energy dissipation based on encryption rounds
     * Energy loss follows exponential decay: E(t) = E₀ * e^(-λt)
     */
    #define CALCULATE_ENERGY_DISSIPATION(initial_energy, rounds) \
        ((initial_energy) * exp(-DISSIPATION_PER_ROUND * (rounds)))

    // =============================================================================
    // UTILITY FUNCTIONS DECLARATIONS
    // =============================================================================

    // We declare utility functions for physics calculations
    double calculateTernaryFissionQ(double parent_mass, const FissionFragment& frag1,
                                   const FissionFragment& frag2, const FissionFragment& frag3);

    bool verifyMomentumConservation(const TernaryFissionEvent& event, double tolerance = 1e-6);
    bool verifyEnergyConservation(const TernaryFissionEvent& event, double tolerance = 1e-3);

    void allocateEnergyField(EnergyField& field, double energy_mev);
    void dissipateEnergyField(EnergyField& field, int encryption_rounds);

    // We provide random number generation for physics simulation
    double generateGaussianRandom(SimulationState& state, double mean, double sigma);
    double generateUniformRandom(SimulationState& state, double min_val, double max_val);

} // namespace TernaryFission

#endif // PHYSICS_CONSTANTS_DEFINITIONS_H
