/*
 * File: include/ternary.fission.simulation.engine.h
 * Author: bthlops (David StJ)
 * Date: July 30, 2025
 * Title: Ternary Fission Simulation Engine Header - Core Class Declaration
 * Purpose: Declares the main simulation engine class for ternary fission events
 * Reason: Provides the interface for the core physics simulation engine
 *
 * Change Log:
 * - 2025-07-30: Initial creation with complete engine class declaration
 *               Added all public/private methods and member variables
 *               Fixed missing standard library includes for GCC 12.2/13.3 compatibility
 *               Ensures proper Ubuntu 24.04 and Debian 12 compatibility
 *
 * Leave-off Context:
 * - Header provides complete interface for simulation engine
 * - All methods properly declared with documentation
 * - Ready for integration with main application
 * - Fixed compilation issues by adding proper standard library includes
 * - Next: GPU acceleration integration and machine learning enhancement
 */

#ifndef TERNARY_FISSION_SIMULATION_ENGINE_H
#define TERNARY_FISSION_SIMULATION_ENGINE_H

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
#include <thread>       // For threading
#include <condition_variable> // For thread coordination
#include <queue>        // For event queue

#include "physics.constants.definitions.h"
#include "physics.utilities.h"

namespace TernaryFission {

/**
 * We implement the main ternary fission simulation engine
 * This class manages the complete physics simulation system
 */
class TernaryFissionSimulationEngine {
public:
    // We provide public access to default parameters for external configuration
    double default_parent_mass;
    double default_excitation_energy;

    /**
     * Default constructor using U-235 parameters
     * We initialize with standard uranium-235 fission parameters
     */
    TernaryFissionSimulationEngine();

    /**
     * Parameterized constructor for custom simulation parameters
     * We allow configuration of parent nucleus and threading
     *
     * @param default_mass: Parent nucleus mass in AMU
     * @param default_energy: Excitation energy in MeV
     * @param threads: Number of worker threads
     */
    TernaryFissionSimulationEngine(double default_mass, double default_energy, int threads);

    /**
     * Destructor ensures proper cleanup
     * We guarantee all resources are released
     */
    ~TernaryFissionSimulationEngine();

    /**
     * Simulate a single ternary fission event
     * We generate realistic physics events with conservation laws
     *
     * @param parent_mass: Parent nucleus mass in AMU
     * @param excitation_energy: Nuclear excitation energy in MeV
     * @return: Complete ternary fission event structure
     */
    TernaryFissionEvent simulateTernaryFissionEvent(double parent_mass, double excitation_energy);

    /**
     * Create an energy field with specified energy
     * We allocate computational resources to represent energy
     *
     * @param energy_mev: Energy level in MeV
     * @return: Energy field structure
     */
    EnergyField createEnergyField(double energy_mev);

    /**
     * Dissipate energy from an existing field
     * We apply encryption-based energy dissipation
     *
     * @param field: Energy field to dissipate
     * @param rounds: Number of encryption rounds to apply
     */
    void dissipateEnergyField(EnergyField& field, int rounds);

    /**
     * Start continuous simulation at specified rate
     * We spawn background threads for continuous event generation
     *
     * @param events_per_second: Target simulation rate
     */
    void startContinuousSimulation(double events_per_second);

    /**
     * Stop continuous simulation
     * We halt background generation and wait for completion
     */
    void stopSimulation();

    /**
     * Check if simulation is currently running
     * We provide status information for external monitoring
     *
     * @return: true if simulation is active
     */
    bool isSimulationRunning() const;

    /**
     * Get current performance metrics
     * We provide real-time system performance data
     *
     * @return: Current performance metrics
     */
    PerformanceMetrics getCurrentMetrics() const;

    /**
     * Set number of worker threads
     * We allow runtime thread count modification
     *
     * @param threads: New thread count (requires restart)
     */
    void setNumThreads(int threads);

    /**
     * Run simulation for specified duration (legacy interface)
     * We maintain compatibility with original implementation
     *
     * @param duration_seconds: How long to run simulation
     * @param events_per_second: Target event rate
     */
    void runSimulation(double duration_seconds, double events_per_second);

    /**
     * Shutdown the simulation engine
     * We ensure all threads are properly terminated
     */
    void shutdown();

    /**
     * Print current system status to console
     * We display comprehensive simulation metrics
     */
    void printSystemStatus() const;

    /**
     * Get statistics as JSON string
     * We provide machine-readable performance data
     *
     * @return: JSON formatted statistics
     */
    std::string getStatisticsJSON() const;

private:
    // We define private member variables for internal state
    int num_worker_threads;
    std::vector<std::thread> worker_threads;
    std::thread continuous_generator_thread;

    // We use atomic counters for thread-safe statistics
    std::atomic<std::uint64_t> total_events_simulated;
    std::atomic<std::uint64_t> total_energy_fields_created;
    std::atomic<bool> shutdown_requested;
    std::atomic<bool> continuous_mode_active;
    std::atomic<double> target_events_per_second;
    double total_computation_time_seconds;

    // We provide thread-safe computation time tracking
    mutable std::mutex computation_time_mutex;

    // We maintain thread-safe simulation state
    mutable std::mutex state_mutex;
    SimulationState simulation_state;

    // We implement thread-safe event queue processing
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::queue<TernaryFissionEvent> event_queue;

    /**
     * Generate a ternary fission event (private method)
     * We create realistic fission events with proper physics
     *
     * @param parent_mass: Parent nucleus mass
     * @param excitation_energy: Nuclear excitation energy
     * @return: Generated fission event
     */
    TernaryFissionEvent generateFissionEvent(double parent_mass, double excitation_energy);

    /**
     * Process a fission event (private method)
     * We handle event processing and energy field creation
     *
     * @param event: Fission event to process
     */
    void processFissionEvent(const TernaryFissionEvent& event);

    /**
     * Worker thread function (private method)
     * We process events from the queue in parallel
     *
     * @param thread_id: Unique thread identifier
     */
    void workerThreadFunction(int thread_id);

    /**
     * Continuous generator function (private method)
     * We generate events at the target rate
     */
    void continuousGeneratorFunction();

    /**
     * Update energy fields (private method)
     * We apply dissipation to active fields
     */
    void updateEnergyFields();

    /**
     * Log fission event (private method)
     * We record events for analysis
     *
     * @param event: Event to log
     */
    void logFissionEvent(const TernaryFissionEvent& event);
};

} // namespace TernaryFission

#endif // TERNARY_FISSION_SIMULATION_ENGINE_H