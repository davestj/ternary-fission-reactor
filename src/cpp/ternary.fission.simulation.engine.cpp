/*
 * File: src/cpp/ternary.fission.simulation.engine.cpp
 * Author: bthlops (David StJ)
 * Date: July 29, 2025
 * Title: Ternary Fission Simulation Engine - Core Implementation
 * Purpose: High-performance nuclear ternary fission simulation with energy field mapping
 * Reason: Implements the core physics engine for ternary fission event simulation
 *
 * Change Log:
 * - 2025-07-29: Initial implementation with complete physics simulation
 * - 2025-07-29: Fixed const-correctness issue with mutex in printSystemStatus()
 * - 2025-07-29: Aligned with actual data structures from physics.constants.definitions.h
 * - 2025-07-29: Added all interface methods expected by main application
 * - 2025-07-30: Fixed atomic<double> issue with proper mutex protection
 *               Complete, working implementation with thread-safe operations
 * - 2025-07-30: bthlops - Fixed infinite recursion bug in dissipateEnergyField by explicitly calling the utility function (::TernaryFission::dissipateEnergyField).
 *               Added inline comments to clarify proper namespace usage and prevent similar issues.
 *
 * Leave-off Context:
 * - Engine fully implements all methods expected by main application
 * - Thread-safe computation time tracking with mutex
 * - All compilation issues resolved
 * - Ready for production deployment
 * - Next: Optimize continuous simulation timing accuracy
 */

#include "ternary.fission.simulation.engine.h"
#include "physics.utilities.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <cstring>
#include <memory>

// OpenSSL headers for encryption
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

// System headers for resource monitoring
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

namespace TernaryFission {

/*
 * Default constructor implementation
 * We use standard U-235 fission parameters
 */
TernaryFissionSimulationEngine::TernaryFissionSimulationEngine()
    : TernaryFissionSimulationEngine(235.0, 6.5, std::thread::hardware_concurrency()) {
}

/*
 * Parameterized constructor implementation
 * We initialize all components of the simulation engine
 */
TernaryFissionSimulationEngine::TernaryFissionSimulationEngine(double default_mass,
                                                               double default_energy,
                                                               int threads)
    : default_parent_mass(default_mass),
      default_excitation_energy(default_energy),
      num_worker_threads(threads),
      total_events_simulated(0),
      total_energy_fields_created(0),
      total_computation_time_seconds(0.0),
      shutdown_requested(false),
      continuous_mode_active(false),
      target_events_per_second(10.0) {

    // Initialize simulation state
    simulation_state.simulation_running = false;
    simulation_state.energy_conservation_enabled = true;
    simulation_state.momentum_conservation_enabled = true;

    // Start worker threads for event processing
    for (int i = 0; i < num_worker_threads; ++i) {
        worker_threads.emplace_back(&TernaryFissionSimulationEngine::workerThreadFunction, this, i);
    }

    // Initialize physics utilities
    initializePhysicsUtilities();

    std::cout << "Ternary Fission Simulation Engine initialized" << std::endl;
    std::cout << "Default parent nucleus: U-" << static_cast<int>(default_mass) << std::endl;
    std::cout << "Default excitation energy: " << default_energy << " MeV" << std::endl;
    std::cout << "Worker threads: " << threads << std::endl;
}

/*
 * Destructor implementation
 * We ensure proper cleanup of all resources
 */
TernaryFissionSimulationEngine::~TernaryFissionSimulationEngine() {
    shutdown();
}

/*
 * Simulate a single ternary fission event
 * This is the main interface method for the application
 */
TernaryFissionEvent TernaryFissionSimulationEngine::simulateTernaryFissionEvent(double parent_mass,
                                                                               double excitation_energy) {
    // Generate the event
    TernaryFissionEvent event = generateFissionEvent(parent_mass, excitation_energy);

    // Update statistics
    total_events_simulated.fetch_add(1);

    // Add to simulation state
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        simulation_state.fission_events.push_back(event);
        simulation_state.total_energy_simulated += event.total_kinetic_energy;
        simulation_state.total_fission_events++;
    }

    return event;
}

/*
 * Create an energy field with specified energy
 * We allocate computational resources to represent the field
 */
EnergyField TernaryFissionSimulationEngine::createEnergyField(double energy_mev) {
    EnergyField field;

    // Allocate resources for the field
    allocateEnergyField(field, energy_mev);

    // Track the creation
    total_energy_fields_created.fetch_add(1);

    // Add to active fields
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        simulation_state.active_energy_fields.push_back(field);

        // Update peak memory usage
        size_t current_memory = 0;
        for (const auto& f : simulation_state.active_energy_fields) {
            current_memory += f.memory_allocated;
        }
        if (current_memory > simulation_state.peak_memory_usage) {
            simulation_state.peak_memory_usage = current_memory;
        }
    }

    return field;
}

/*
 * Dissipate energy from a field
 * We apply encryption-based dissipation using the physics utility.
 * We must call the utility function in the TernaryFission namespace to avoid recursion.
 */
void TernaryFissionSimulationEngine::dissipateEnergyField(EnergyField& field, int rounds) {
    // We explicitly call the namespace-qualified utility to prevent infinite recursion.
    ::TernaryFission::dissipateEnergyField(field, rounds);

    // Update the field in our active list if it exists.
    std::lock_guard<std::mutex> lock(state_mutex);
    for (auto& active_field : simulation_state.active_energy_fields) {
        if (active_field.memory_allocated == field.memory_allocated &&
            active_field.initial_energy_level == field.initial_energy_level) {
            active_field = field;
            break;
        }
    }
}

/*
 * Start continuous simulation at specified rate
 * We spawn a generator thread for continuous event creation
 */
void TernaryFissionSimulationEngine::startContinuousSimulation(double events_per_second) {
    if (continuous_mode_active.load()) {
        std::cout << "Continuous simulation already running" << std::endl;
        return;
    }

    target_events_per_second.store(events_per_second);
    continuous_mode_active.store(true);
    simulation_state.simulation_running = true;

    // Start the continuous generator thread
    continuous_generator_thread = std::thread(&TernaryFissionSimulationEngine::continuousGeneratorFunction, this);

    std::cout << "Started continuous simulation at " << events_per_second << " events/second" << std::endl;
}

/*
 * Stop continuous simulation
 * We halt event generation and wait for completion
 */
void TernaryFissionSimulationEngine::stopSimulation() {
    if (!continuous_mode_active.load()) {
        return;
    }

    continuous_mode_active.store(false);
    simulation_state.simulation_running = false;

    // Wait for generator thread to finish
    if (continuous_generator_thread.joinable()) {
        continuous_generator_thread.join();
    }

    // Wait for queue to empty
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        while (!event_queue.empty()) {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            lock.lock();
        }
    }

    std::cout << "Stopped continuous simulation" << std::endl;
}

/*
 * Check if simulation is running
 * We provide status for the main application
 */
bool TernaryFissionSimulationEngine::isSimulationRunning() const {
    return continuous_mode_active.load() || simulation_state.simulation_running;
}

/*
 * Get current performance metrics
 * We calculate and return current system metrics
 */
PerformanceMetrics TernaryFissionSimulationEngine::getCurrentMetrics() const {
    PerformanceMetrics metrics = getCurrentPerformanceMetrics();

    // Fill in simulation-specific metrics
    metrics.total_energy_fields_active = simulation_state.active_energy_fields.size();
    metrics.events_per_second = continuous_mode_active.load() ? target_events_per_second.load() : 0.0;

    if (total_events_simulated.load() > 0) {
        std::lock_guard<std::mutex> lock(computation_time_mutex);
        metrics.average_event_processing_time_ms =
            (total_computation_time_seconds / total_events_simulated.load()) * 1000.0;
    }

    return metrics;
}

/*
 * Set number of worker threads
 * We allow runtime modification (requires restart)
 */
void TernaryFissionSimulationEngine::setNumThreads(int threads) {
    if (threads != num_worker_threads) {
        std::cout << "Warning: Changing thread count requires engine restart" << std::endl;
        num_worker_threads = threads;
    }
}

/*
 * Run simulation for specified duration (legacy interface)
 * We maintain compatibility with original implementation
 */
void TernaryFissionSimulationEngine::runSimulation(double duration_seconds, double events_per_second) {
    startContinuousSimulation(events_per_second);

    auto start_time = std::chrono::steady_clock::now();
    auto last_status_time = start_time;

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        double elapsed_seconds = std::chrono::duration<double>(current_time - start_time).count();

        if (elapsed_seconds >= duration_seconds) {
            break;
        }

        // Update energy fields
        updateEnergyFields();

        // Print status every 5 seconds
        double time_since_last_status = std::chrono::duration<double>(current_time - last_status_time).count();
        if (time_since_last_status >= 5.0) {
            printSystemStatus();
            last_status_time = current_time;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    stopSimulation();

    std::cout << "\nSimulation completed!" << std::endl;
    printSystemStatus();
}

/*
 * Shutdown the simulation engine
 * We ensure all threads are properly terminated
 */
void TernaryFissionSimulationEngine::shutdown() {
    std::cout << "Shutting down simulation engine..." << std::endl;

    // Stop any running simulation
    stopSimulation();

    // Signal shutdown to worker threads
    shutdown_requested.store(true);
    queue_cv.notify_all();

    // Wait for all worker threads
    for (auto& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Clear remaining data
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        simulation_state.active_energy_fields.clear();
        simulation_state.fission_events.clear();
    }

    // Cleanup physics utilities
    cleanupPhysicsUtilities();

    std::cout << "Simulation engine shutdown complete" << std::endl;
}

/*
 * Generate a ternary fission event (private method)
 * We create realistic fission events with proper physics
 */
TernaryFissionEvent TernaryFissionSimulationEngine::generateFissionEvent(double parent_mass,
                                                                        double excitation_energy) {
    TernaryFissionEvent event;
    event.timestamp = std::chrono::high_resolution_clock::now();

    // Parent nucleus properties (U-235 defaults)
    const int parent_atomic_number = 92;
    const int parent_mass_number = static_cast<int>(parent_mass);

    // Total mass available for fragments
    double total_fragment_mass = parent_mass;

    // Generate fragment masses using empirical distributions
    double mass_ratio = normalRandom(1.4, 0.15);

    // Alpha particle
    event.alpha_particle.mass = ALPHA_PARTICLE_MASS;
    event.alpha_particle.atomic_number = 2;
    event.alpha_particle.mass_number = 4;
    event.alpha_particle.half_life = 1e100;  // Stable

    // Split remaining mass
    double remaining_mass = total_fragment_mass - ALPHA_PARTICLE_MASS;
    event.light_fragment.mass = remaining_mass / (1 + mass_ratio);
    event.heavy_fragment.mass = remaining_mass - event.light_fragment.mass;

    // Estimate atomic numbers
    double z_ratio = static_cast<double>(parent_atomic_number) / parent_mass_number;
    event.light_fragment.atomic_number = static_cast<int>(event.light_fragment.mass * z_ratio);
    event.heavy_fragment.atomic_number = parent_atomic_number - event.light_fragment.atomic_number - 2;

    event.light_fragment.mass_number = static_cast<int>(event.light_fragment.mass);
    event.heavy_fragment.mass_number = static_cast<int>(event.heavy_fragment.mass);

    // Set half-lives (simplified)
    event.light_fragment.half_life = 100.0;
    event.heavy_fragment.half_life = 1000.0;

    // Calculate Q-value
    event.q_value = TERNARY_Q_VALUE + excitation_energy;
    event.total_kinetic_energy = event.q_value;

    // Energy distribution
    double alpha_energy_fraction = 0.05;
    double light_heavy_ratio = event.heavy_fragment.mass / event.light_fragment.mass;

    event.alpha_particle.kinetic_energy = event.total_kinetic_energy * alpha_energy_fraction;
    double remaining_energy = event.total_kinetic_energy * (1 - alpha_energy_fraction);

    event.light_fragment.kinetic_energy = remaining_energy * light_heavy_ratio / (1 + light_heavy_ratio);
    event.heavy_fragment.kinetic_energy = remaining_energy - event.light_fragment.kinetic_energy;

    // Generate momenta with conservation
    double alpha_theta = acos(1 - 2 * uniformRandom());
    double alpha_phi = 2 * M_PI * uniformRandom();

    double alpha_momentum = sqrt(2 * event.alpha_particle.mass * ATOMIC_MASS_UNIT *
                                event.alpha_particle.kinetic_energy * MEV_TO_JOULES);

    event.alpha_particle.momentum_x = alpha_momentum * sin(alpha_theta) * cos(alpha_phi) / MEV_TO_JOULES;
    event.alpha_particle.momentum_y = alpha_momentum * sin(alpha_theta) * sin(alpha_phi) / MEV_TO_JOULES;
    event.alpha_particle.momentum_z = alpha_momentum * cos(alpha_theta) / MEV_TO_JOULES;

    double light_theta = acos(1 - 2 * uniformRandom());
    double light_phi = 2 * M_PI * uniformRandom();

    double light_momentum = sqrt(2 * event.light_fragment.mass * ATOMIC_MASS_UNIT *
                                event.light_fragment.kinetic_energy * MEV_TO_JOULES);

    event.light_fragment.momentum_x = light_momentum * sin(light_theta) * cos(light_phi) / MEV_TO_JOULES;
    event.light_fragment.momentum_y = light_momentum * sin(light_theta) * sin(light_phi) / MEV_TO_JOULES;
    event.light_fragment.momentum_z = light_momentum * cos(light_theta) / MEV_TO_JOULES;

    // Heavy fragment momentum from conservation
    event.heavy_fragment.momentum_x = -(event.alpha_particle.momentum_x + event.light_fragment.momentum_x);
    event.heavy_fragment.momentum_y = -(event.alpha_particle.momentum_y + event.light_fragment.momentum_y);
    event.heavy_fragment.momentum_z = -(event.alpha_particle.momentum_z + event.light_fragment.momentum_z);

    // Verify conservation laws
    event.momentum_conserved = verifyMomentumConservation(event);
    event.energy_conserved = verifyEnergyConservation(event);
    event.mass_number_conserved = (event.light_fragment.mass_number +
                                  event.heavy_fragment.mass_number +
                                  event.alpha_particle.mass_number) == parent_mass_number;
    event.charge_conserved = (event.light_fragment.atomic_number +
                             event.heavy_fragment.atomic_number +
                             event.alpha_particle.atomic_number) == parent_atomic_number;

    return event;
}

/*
 * Process a fission event (private method)
 * We handle event processing and energy field creation
 */
void TernaryFissionSimulationEngine::processFissionEvent(const TernaryFissionEvent& event) {
    // Create energy fields for each fragment
    createEnergyField(event.light_fragment.kinetic_energy);
    createEnergyField(event.heavy_fragment.kinetic_energy);
    createEnergyField(event.alpha_particle.kinetic_energy);

    // Log the event
    logFissionEvent(event);
}

/*
 * Worker thread function (private method)
 * We process events from the queue
 */
void TernaryFissionSimulationEngine::workerThreadFunction(int thread_id) {
    std::cout << "Worker thread " << thread_id << " started" << std::endl;

    while (!shutdown_requested.load()) {
        std::unique_lock<std::mutex> lock(queue_mutex);

        queue_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
            return !event_queue.empty() || shutdown_requested.load();
        });

        if (shutdown_requested.load()) {
            break;
        }

        if (!event_queue.empty()) {
            TernaryFissionEvent event = event_queue.front();
            event_queue.pop();
            lock.unlock();

            auto start_time = std::chrono::high_resolution_clock::now();
            processFissionEvent(event);
            auto end_time = std::chrono::high_resolution_clock::now();

            double processing_time = std::chrono::duration<double>(end_time - start_time).count();

            // Thread-safe update of computation time
            {
                std::lock_guard<std::mutex> lock(computation_time_mutex);
                total_computation_time_seconds += processing_time;
            }
        }
    }

    std::cout << "Worker thread " << thread_id << " shutting down" << std::endl;
}

/*
 * Continuous generator function (private method)
 * We generate events at the target rate
 */
void TernaryFissionSimulationEngine::continuousGeneratorFunction() {
    auto last_event_time = std::chrono::steady_clock::now();
    double event_interval = 1.0 / target_events_per_second.load();

    while (continuous_mode_active.load()) {
        auto current_time = std::chrono::steady_clock::now();
        double time_since_last = std::chrono::duration<double>(current_time - last_event_time).count();

        if (time_since_last >= event_interval) {
            // Generate event with default parameters
            TernaryFissionEvent event = generateFissionEvent(default_parent_mass, default_excitation_energy);

            // Add to queue
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                event_queue.push(event);
            }
            queue_cv.notify_one();

            total_events_simulated.fetch_add(1);
            last_event_time = current_time;

            // Update event interval if rate changed
            event_interval = 1.0 / target_events_per_second.load();
        }

        // Short sleep to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

/*
 * Update energy fields (private method)
 * We apply dissipation to active fields
 */
void TernaryFissionSimulationEngine::updateEnergyFields() {
    std::lock_guard<std::mutex> lock(state_mutex);

    auto it = simulation_state.active_energy_fields.begin();
    while (it != simulation_state.active_energy_fields.end()) {
        // Apply one round of dissipation using the physics utility
        ::TernaryFission::dissipateEnergyField(*it, 1);

        // Remove if energy is too low
        if (it->current_energy_level < it->initial_energy_level * 0.01) {
            it = simulation_state.active_energy_fields.erase(it);
        } else {
            ++it;
        }
    }
}

/*
 * Log fission event (private method)
 * We record events for analysis
 */
void TernaryFissionSimulationEngine::logFissionEvent(const TernaryFissionEvent& event) {
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);

    // Use the utility function for logging
    ::TernaryFission::logFissionEvent(event, "fission_events.log");
}

/*
 * Print system status (public method)
 * We display current simulation metrics
 */
void TernaryFissionSimulationEngine::printSystemStatus() const {
    std::lock_guard<std::mutex> lock(state_mutex);

    std::cout << "\n=== Ternary Fission Simulation Status ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);

    // Basic statistics
    std::cout << "Total events simulated: " << total_events_simulated.load() << std::endl;
    std::cout << "Total energy fields created: " << total_energy_fields_created.load() << std::endl;
    std::cout << "Active energy fields: " << simulation_state.active_energy_fields.size() << std::endl;
    std::cout << "Total energy simulated: " << simulation_state.total_energy_simulated << " MeV" << std::endl;

    // Memory usage
    size_t current_memory = 0;
    uint64_t total_cpu_cycles = 0;
    for (const auto& field : simulation_state.active_energy_fields) {
        current_memory += field.memory_allocated;
        total_cpu_cycles += field.cpu_cycles_consumed;
    }
    std::cout << "Current memory allocated: " << current_memory / (1024.0 * 1024.0) << " MB" << std::endl;
    std::cout << "Peak memory usage: " << simulation_state.peak_memory_usage / (1024.0 * 1024.0) << " MB" << std::endl;
    std::cout << "Total CPU cycles: " << total_cpu_cycles / 1e9 << " billion" << std::endl;

    // Performance metrics
    if (total_events_simulated.load() > 0) {
        double avg_time;
        {
            std::lock_guard<std::mutex> lock(computation_time_mutex);
            avg_time = (total_computation_time_seconds / total_events_simulated.load()) * 1000;
        }
        std::cout << "Average computation time per event: " << avg_time << " ms" << std::endl;
    }

    if (continuous_mode_active.load()) {
        std::cout << "Continuous mode: ACTIVE (" << target_events_per_second.load() << " events/sec)" << std::endl;
    } else {
        std::cout << "Continuous mode: INACTIVE" << std::endl;
    }

    // System resource usage
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    std::cout << "Process CPU time: " << usage.ru_utime.tv_sec + usage.ru_stime.tv_sec << " seconds" << std::endl;
    std::cout << "Process memory (RSS): " << usage.ru_maxrss / 1024.0 << " MB" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

/*
 * Get statistics as JSON (public method)
 * We provide machine-readable output
 */
std::string TernaryFissionSimulationEngine::getStatisticsJSON() const {
    std::lock_guard<std::mutex> lock(state_mutex);

    std::stringstream json;
    json << "{";
    json << "\"total_events\": " << total_events_simulated.load() << ",";
    json << "\"active_fields\": " << simulation_state.active_energy_fields.size() << ",";
    json << "\"total_fields_created\": " << total_energy_fields_created.load() << ",";
    json << "\"total_energy_mev\": " << simulation_state.total_energy_simulated << ",";
    json << "\"peak_memory_mb\": " << simulation_state.peak_memory_usage / (1024.0 * 1024.0) << ",";

    size_t current_memory = 0;
    uint64_t total_cpu_cycles = 0;
    for (const auto& field : simulation_state.active_energy_fields) {
        current_memory += field.memory_allocated;
        total_cpu_cycles += field.cpu_cycles_consumed;
    }

    json << "\"current_memory_mb\": " << current_memory / (1024.0 * 1024.0) << ",";
    json << "\"cpu_cycles_billions\": " << total_cpu_cycles / 1e9 << ",";
    json << "\"continuous_mode\": " << (continuous_mode_active.load() ? "true" : "false") << ",";
    json << "\"events_per_second\": " << (continuous_mode_active.load() ? target_events_per_second.load() : 0.0);
    json << "}";

    return json.str();
}

} // namespace TernaryFission
