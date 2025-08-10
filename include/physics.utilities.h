/*
 * File: include/physics.utilities.h
 * Author: bthlops (David StJ)
 * Date: July 30, 2025
 * Title: Physics Utilities Header - Function and Class Declarations - FIXED
 * Purpose: Declares utility functions and classes for physics calculations and energy field management
 * Reason: Provides the interface for physics utility functions used throughout the simulation
 *
 * Change Log:
 * - 2025-07-29: Initial creation with complete function declarations
 *               Includes all physics calculation utilities
 *               Energy field management functions
 *               Statistical analysis structures
 * - 2025-07-30: FIXED missing standard library includes causing size_t compilation errors
 *               Added complete C++ standard library headers for GCC 12.2/13.3 compatibility
 *               Ensures proper Ubuntu 24.04 and Debian 12 compatibility
 *
 * Leave-off Context:
 * - Header provides complete interface for physics utilities
 * - All functions properly declared with documentation
 * - Ready for integration with simulation engine
 * - Fixed compilation issues by adding proper standard library includes
 * - Next: Consider templatizing some functions for different precision types
 */

#ifndef TERNARY_FISSION_PHYSICS_UTILITIES_H
#define TERNARY_FISSION_PHYSICS_UTILITIES_H

// We include all necessary C++ standard library headers for proper compilation
#include <cstddef>      // For size_t, ptrdiff_t
#include <cstdint>      // For uint64_t, int64_t, etc.
#include <cmath>        // For mathematical functions
#include <vector>       // For std::vector
#include <string>       // For std::string
#include <chrono>       // For time measurements
#include <random>       // For random number generation
#include <memory>       // For smart pointers
#include <atomic>       // For atomic operations
#include <mutex>        // For thread synchronization

#include "physics.constants.definitions.h"

namespace TernaryFission {

// Forward declaration of memory pool class
class EnergyFieldMemoryPool;

/*
 * Watt spectrum function for fission neutron energy distribution
 * We model the energy spectrum of prompt fission neutrons
 *
 * @param x: Random variable [0,1] for sampling
 * @param a: Watt spectrum parameter (typically ~0.988 for U-235)
 * @param b: Watt spectrum parameter (typically ~2.249 for U-235)
 * @return: Energy value following Watt distribution
 */
double wattSpectrum(double x, double a, double b);

/*
 * Maxwell-Boltzmann distribution for thermal velocities
 * We calculate the probability distribution of particle velocities at temperature T
 *
 * @param v: Velocity in m/s
 * @param m: Mass of particle in kg
 * @param T: Temperature in Kelvin
 * @return: Probability density at velocity v
 */
double maxwellBoltzmann(double v, double m, double T);

/*
 * Calculate relativistic kinetic energy
 * We account for relativistic effects when particles approach speed of light
 *
 * @param mass: Rest mass in kg
 * @param velocity: Velocity in m/s
 * @return: Kinetic energy in Joules (convert to MeV as needed)
 */
double relativisticKineticEnergy(double mass, double velocity);

/*
 * Generate a unique field ID
 * We create unique identifiers for energy field tracking
 *
 * @return: 64-bit unique identifier
 */
std::uint64_t generateFieldId();

/*
 * Calculate entropy from system state
 * We implement Boltzmann entropy S = k * ln(W) based on microstates
 *
 * @param memory_bytes: Amount of memory allocated
 * @param cpu_cycles: Number of CPU cycles consumed
 * @return: Normalized entropy value [0,1]
 */
double calculateEntropy(std::size_t memory_bytes, std::uint64_t cpu_cycles);

/*
 * Verify conservation laws for a fission event
 * We check energy, momentum, mass, and charge conservation
 *
 * @param event: The fission event to verify
 * @param energy_tolerance: Allowed deviation in energy conservation
 * @param momentum_tolerance: Allowed deviation in momentum conservation
 * @return: true if all conservation laws are satisfied within tolerance
 */
bool verifyConservationLaws(const TernaryFissionEvent& event, double energy_tolerance, double momentum_tolerance);

/*
 * Allocate memory and CPU cycles to represent energy field
 * We map energy to computational resources (memory and CPU)
 *
 * @param field: Energy field structure to initialize
 * @param energy_mev: Energy level in MeV to allocate
 */
void allocateEnergyField(EnergyField& field, double energy_mev);

/*
 * Dissipate energy through encryption operations
 * We model energy loss through computational work
 *
 * @param field: Energy field to dissipate
 * @param rounds: Number of encryption rounds to perform
 */
void dissipateEnergyField(EnergyField& field, int rounds);

/*
 * Generate random momentum for a fission fragment
 * We create realistic momentum vectors for physics simulation
 *
 * @param fragment: Fragment to assign momentum
 */
void generateRandomMomentum(FissionFragment& fragment);

/*
 * Apply conservation laws to a fission event
 * We adjust fragment properties to ensure conservation
 *
 * @param event: Ternary fission event to normalize
 */
void applyConservationLaws(TernaryFissionEvent& event);

/*
 * Calculate field interference between two energy fields
 * We model quantum interference effects between fields
 *
 * @param field1: First energy field
 * @param field2: Second energy field
 * @return: Interference factor [-1, 1], where 1 is constructive, -1 is destructive
 */
double calculateFieldInterference(const EnergyField& field1, const EnergyField& field2);

/*
 * Merge two energy fields that have collided
 * We conserve energy and momentum in the merger
 *
 * @param field1: Target field (will contain merged result)
 * @param field2: Source field (will be absorbed)
 */
void mergeEnergyFields(EnergyField& field1, const EnergyField& field2);

/*
 * Log fission event to file for analysis
 * We record all event parameters for post-processing
 *
 * @param event: Fission event to log
 * @param filename: Output file name (default: "fission_events.log")
 */
void logFissionEvent(const TernaryFissionEvent& event, const std::string& filename = "fission_events.log");

/*
 * Structure to hold statistical analysis results
 * We track averages and standard deviations of key parameters
 */
struct FissionStatistics {
    double average_q_value;
    double std_dev_q_value;
    double average_fragment1_mass;
    double std_dev_fragment1_mass;
    double average_fragment2_mass;
    double std_dev_fragment2_mass;
    double average_fragment3_mass;
    double std_dev_fragment3_mass;
    double average_fragment1_energy;
    double average_fragment2_energy;
    double average_fragment3_energy;
    std::size_t total_events;
};

/*
 * Calculate average properties from multiple events
 * We compute statistical measures for analysis
 *
 * @param events: Vector of fission events to analyze
 * @return: Statistical summary of the events
 */
FissionStatistics calculateStatistics(const std::vector<TernaryFissionEvent>& events);

/*
 * Optimized energy field allocation using memory pool
 * We use pre-allocated memory blocks for better performance
 *
 * @param field: Energy field structure to initialize
 * @param energy_mev: Energy level in MeV to allocate
 */
void allocateEnergyFieldOptimized(EnergyField& field, double energy_mev);

/*
 * Deallocate energy field using memory pool
 * We return memory to the pool for reuse
 *
 * @param field: Energy field to deallocate
 */
void deallocateEnergyFieldOptimized(EnergyField& field);

/*
 * Performance monitoring structure
 * We track system performance metrics
 */
struct PerformanceMetrics {
    double events_per_second;
    double average_event_processing_time_ms;
    double memory_usage_mb;
    double cpu_utilization_percent;
    double cpu_time_seconds;
    std::uint64_t total_energy_fields_active;
    std::uint64_t total_memory_pool_allocated;
    std::uint64_t page_faults;
    std::uint64_t context_switches;
    std::chrono::steady_clock::time_point measurement_time;
};

/*
 * Additional constants not in physics.constants.definitions.h
 * We define these here for the utilities to use
 */
constexpr double INTERACTION_RANGE = 1.0e-15;      // Nuclear interaction range in meters
constexpr double ENERGY_THRESHOLD = 0.01;          // Minimum energy threshold in MeV
constexpr double AMU_TO_KG = ATOMIC_MASS_UNIT;     // Alias for consistency
constexpr double AMU_TO_MEV = 931.494;             // Conversion factor

/*
 * Energy field configuration parameters
 * We allow runtime configuration of field behavior
 */
struct EnergyFieldConfig {
    double memory_per_mev = ENERGY_TO_MEMORY_SCALE;
    double cpu_cycles_per_mev = ENERGY_TO_CPU_CYCLES;
    double decay_constant_base = ENTROPY_DECAY_CONSTANT;
    double dissipation_rate_default = 0.01;
    double interaction_range = INTERACTION_RANGE;
    double energy_threshold = ENERGY_THRESHOLD;
    bool use_memory_pool = true;
    std::size_t memory_pool_block_size = 1024 * 1024;  // 1MB default
    std::size_t memory_pool_max_blocks = 1000;
};

/*
 * Global configuration instance
 * We provide a global config that can be modified at runtime
 */
extern EnergyFieldConfig g_energy_field_config;

/*
 * Initialize the physics utilities subsystem
 * We set up memory pools and other resources
 *
 * @param config: Configuration parameters (optional)
 */
void initializePhysicsUtilities(const EnergyFieldConfig* config = nullptr);

/*
 * Cleanup the physics utilities subsystem
 * We release all allocated resources
 */
void cleanupPhysicsUtilities();

/*
 * Get current performance metrics
 * We provide real-time performance monitoring
 *
 * @return: Current performance metrics snapshot
 */
PerformanceMetrics getCurrentPerformanceMetrics();

/*
 * Utility functions for debugging and visualization
 */

/*
 * Convert energy field to JSON representation
 * We provide serialization for analysis and visualization
 *
 * @param field: Energy field to convert
 * @return: JSON string representation
 */
std::string energyFieldToJSON(const EnergyField& field);

/*
 * Convert fission event to JSON representation
 * We provide serialization for logging and analysis
 *
 * @param event: Fission event to convert
 * @return: JSON string representation
 */
std::string fissionEventToJSON(const TernaryFissionEvent& event);

/*
 * Validate energy field state
 * We check for consistency and corruption
 *
 * @param field: Energy field to validate
 * @return: true if field state is valid
 */
bool validateEnergyField(const EnergyField& field);

/*
 * Calculate total system energy
 * We sum all energy in active fields
 *
 * @param fields: Vector of energy field pointers
 * @return: Total energy in MeV
 */
double calculateTotalSystemEnergy(const std::vector<EnergyField*>& fields);

/*
 * Physics calculation helpers
 */

/*
 * Convert between energy units
 * We provide convenient unit conversions
 */
inline double mevToJoules(double mev) { return mev * MEV_TO_JOULES; }
inline double joulesToMev(double joules) { return joules / MEV_TO_JOULES; }
inline double amuToKg(double amu) { return amu * AMU_TO_KG; }
inline double kgToAmu(double kg) { return kg / AMU_TO_KG; }

/*
 * Calculate de Broglie wavelength
 * We compute matter wave wavelength for quantum effects
 *
 * @param momentum: Momentum in kg*m/s
 * @return: Wavelength in meters
 */
inline double deBroglieWavelength(double momentum) {
    return PLANCK_CONSTANT / momentum;
}

/*
 * Calculate Compton wavelength
 * We compute the quantum scale for a particle
 *
 * @param mass: Mass in kg
 * @return: Compton wavelength in meters
 */
inline double comptonWavelength(double mass) {
    return PLANCK_CONSTANT / (mass * SPEED_OF_LIGHT);
}

/*
 * Thread-safe random number generation
 * We provide utilities for parallel simulations
 */

/*
 * Get thread-local random generator
 * We ensure each thread has its own RNG
 *
 * @return: Reference to thread-local RNG
 */
std::mt19937_64& getThreadLocalRNG();

/*
 * Generate uniform random double
 * We provide convenient random number generation
 *
 * @param min: Minimum value (inclusive)
 * @param max: Maximum value (exclusive)
 * @return: Random double in range [min, max)
 */
double uniformRandom(double min = 0.0, double max = 1.0);

/*
 * Generate normal (Gaussian) random double
 * We provide normally distributed random numbers
 *
 * @param mean: Mean of the distribution
 * @param stddev: Standard deviation
 * @return: Random double following N(mean, stddev^2)
 */
double normalRandom(double mean = 0.0, double stddev = 1.0);

/*
 * Generate Poisson random integer
 * We model discrete random events
 *
 * @param lambda: Rate parameter
 * @return: Random integer following Poisson(lambda)
 */
int poissonRandom(double lambda);

} // namespace TernaryFission

#endif // TERNARY_FISSION_PHYSICS_UTILITIES_H