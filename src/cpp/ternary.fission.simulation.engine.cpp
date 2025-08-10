/*
 * File: src/cpp/ternary.fission.simulation.engine.cpp
 * Author: bthlops (David StJ)
 * Date: January 31, 2025
 * Title: Ternary Fission Simulation Engine with HTTP API Integration
 * Purpose: High-performance nuclear ternary fission simulation with HTTP API and JSON serialization
 * Reason: Implements the core physics engine with daemon mode and REST API support
 *
 * Change Log:
 * - 2025-07-29: Initial implementation with complete physics simulation
 * - 2025-07-29: Fixed const-correctness issue with mutex in printSystemStatus()
 * - 2025-07-29: Aligned with actual data structures from physics.constants.definitions.h
 * - 2025-07-29: Added all interface methods expected by main application
 * - 2025-07-30: Fixed atomic<double> issue with proper mutex protection
 *               Complete, working implementation with thread-safe operations
 * - 2025-07-30: bthlops - Fixed infinite recursion bug in dissipateEnergyField
 * - 2025-01-31: Integrated HTTP API support with JSON serialization
 *               Added thread-safe interfaces for HTTP server integration
 *               Added JSON response formatting for all physics data structures
 *               Added energy field management API endpoints
 *               Added simulation control API endpoints
 *               Maintained all existing CLI functionality and performance
 *
 * Carry-over Context:
 * - Engine provides complete HTTP API interface for daemon mode operations
 * - JSON serialization enables REST API responses for physics calculations
 * - Thread-safe interfaces support concurrent HTTP requests
 * - All existing CLI functionality remains unchanged for backward compatibility
 * - Energy field management supports HTTP CRUD operations
 * - Next: Integration with distributed daemon architecture for multi-node physics
 */

#include "ternary.fission.simulation.engine.h"
#include "physics.utilities.h"
#include "config.ternary.fission.server.h"

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
#include <json/json.h>

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
 * Create an energy field with specified energy
 * We allocate computational resources to represent energy
 */
EnergyField TernaryFissionSimulationEngine::createEnergyField(double energy_mev) {
    return ::TernaryFission::createEnergyField(energy_mev);
}

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
      shutdown_requested(false),
      continuous_mode_active(false),
      target_events_per_second(10.0),
      total_computation_time_seconds(0.0),
      api_request_counter_(0),
      json_serialization_enabled_(true) {

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

    std::cout << "Ternary Fission Simulation Engine initialized with HTTP API support" << std::endl;
    std::cout << "Default parent nucleus: U-" << static_cast<int>(default_mass) << std::endl;
    std::cout << "Default excitation energy: " << default_energy << " MeV" << std::endl;
    std::cout << "Worker threads: " << threads << std::endl;
    std::cout << "JSON serialization: " << (json_serialization_enabled_ ? "enabled" : "disabled") << std::endl;
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
    auto start_time = std::chrono::high_resolution_clock::now();

    // Generate the fission event
    TernaryFissionEvent event = generateFissionEvent(parent_mass, excitation_energy);

    // Process the event (create energy fields, apply conservation laws)
    processFissionEvent(event);

    // Update statistics
    total_events_simulated.fetch_add(1, std::memory_order_relaxed);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    {
        std::lock_guard<std::mutex> lock(computation_time_mutex);
        total_computation_time_seconds += duration.count() / 1e6;
    }

    return event;
}

/*
 * Simulate a single ternary fission event with default parameters
 */
TernaryFissionEvent TernaryFissionSimulationEngine::simulateTernaryFissionEvent() {
    return simulateTernaryFissionEvent(default_parent_mass, default_excitation_energy);
}

/*
 * HTTP API: Simulate ternary fission event with JSON response
 * We provide JSON-formatted response for HTTP API calls
 */
Json::Value TernaryFissionSimulationEngine::simulateTernaryFissionEventAPI(const Json::Value& request) {
    std::lock_guard<std::mutex> lock(api_mutex_);
    api_request_counter_++;

    // Parse request parameters
    double parent_mass = request.get("parent_mass", default_parent_mass).asDouble();
    double excitation_energy = request.get("excitation_energy", default_excitation_energy).asDouble();
    int num_events = request.get("num_events", 1).asInt();

    // Validate parameters
    if (parent_mass <= 0 || parent_mass > 300) {
        Json::Value error;
        error["error"] = "Invalid parent_mass: must be between 0 and 300 AMU";
        error["status"] = "error";
        return error;
    }

    if (excitation_energy < 0 || excitation_energy > 100) {
        Json::Value error;
        error["error"] = "Invalid excitation_energy: must be between 0 and 100 MeV";
        error["status"] = "error";
        return error;
    }

    if (num_events <= 0 || num_events > 10000) {
        Json::Value error;
        error["error"] = "Invalid num_events: must be between 1 and 10000";
        error["status"] = "error";
        return error;
    }

    // Generate events
    Json::Value response;
    Json::Value events_array(Json::arrayValue);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_events; i++) {
        try {
            TernaryFissionEvent event = simulateTernaryFissionEvent(parent_mass, excitation_energy);
            events_array.append(serializeFissionEventToJSON(event));
        } catch (const std::exception& e) {
            Json::Value error;
            error["error"] = "Event simulation failed: " + std::string(e.what());
            error["event_index"] = static_cast<Json::Int64>(i);
            events_array.append(error);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // Build response
    response["status"] = "success";
    response["num_events"] = static_cast<Json::Int64>(num_events);
    response["events"] = events_array;
    response["computation_time_microseconds"] = static_cast<Json::Int64>(duration.count());
    response["request_id"] = static_cast<Json::Int64>(api_request_counter_);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream timestamp_ss;
    timestamp_ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    response["timestamp"] = timestamp_ss.str();

    return response;
}

/*
 * HTTP API: Get system status with JSON response
 * We provide comprehensive system status for monitoring
 */
Json::Value TernaryFissionSimulationEngine::getSystemStatusAPI() const {
    std::lock_guard<std::mutex> lock(state_mutex);

    Json::Value status;
    status["simulation_running"] = simulation_state.simulation_running;
    status["continuous_mode_active"] = continuous_mode_active.load();
    status["total_events_simulated"] = static_cast<Json::UInt64>(total_events_simulated.load());
    status["total_energy_fields_created"] = static_cast<Json::UInt64>(total_energy_fields_created.load());

    {
        std::lock_guard<std::mutex> time_lock(computation_time_mutex);
        status["total_computation_time_seconds"] = total_computation_time_seconds;
    }

    status["worker_threads"] = static_cast<Json::Int64>(num_worker_threads);
    status["active_energy_fields"] = static_cast<Json::Int64>(
        simulation_state.active_energy_fields.size());
    status["energy_conservation_enabled"] = simulation_state.energy_conservation_enabled;
    status["momentum_conservation_enabled"] = simulation_state.momentum_conservation_enabled;
    status["target_events_per_second"] = target_events_per_second.load();

    // Calculate performance metrics
    uint64_t total_events = total_events_simulated.load();
    double total_time = 0.0;
    {
        std::lock_guard<std::mutex> time_lock(computation_time_mutex);
        total_time = total_computation_time_seconds;
    }

    if (total_time > 0 && total_events > 0) {
        status["average_events_per_second"] = total_events / total_time;
        status["average_microseconds_per_event"] = (total_time * 1e6) / total_events;
    } else {
        status["average_events_per_second"] = 0.0;
        status["average_microseconds_per_event"] = 0.0;
    }

    status["api_requests_processed"] = static_cast<Json::UInt64>(api_request_counter_);
    status["json_serialization_enabled"] = json_serialization_enabled_;

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream timestamp_ss;
    timestamp_ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    status["timestamp"] = timestamp_ss.str();

    return status;
}

/*
 * HTTP API: Get energy fields list with JSON response
 * We provide all active energy fields for monitoring
 */
Json::Value TernaryFissionSimulationEngine::getEnergyFieldsAPI() const {
    std::lock_guard<std::mutex> lock(state_mutex);

    Json::Value response;
    Json::Value fields_array(Json::arrayValue);

    for (const auto& field : simulation_state.active_energy_fields) {
        fields_array.append(serializeEnergyFieldToJSON(field));
    }

    response["energy_fields"] = fields_array;
    response["total_fields"] = static_cast<Json::Int64>(
        simulation_state.active_energy_fields.size());
    response["status"] = "success";

    return response;
}

/*
 * HTTP API: Start continuous simulation
 * We provide HTTP control for continuous simulation mode
 */
Json::Value TernaryFissionSimulationEngine::startContinuousSimulationAPI(const Json::Value& request) {
    double events_per_sec = request.get("events_per_second", 10.0).asDouble();

    if (events_per_sec <= 0 || events_per_sec > 10000) {
        Json::Value error;
        error["error"] = "Invalid events_per_second: must be between 0 and 10000";
        error["status"] = "error";
        return error;
    }

    startContinuousSimulation(events_per_sec);

    Json::Value response;
    response["status"] = "success";
    response["message"] = "Continuous simulation started";
    response["events_per_second"] = events_per_sec;
    response["simulation_running"] = true;

    return response;
}

/*
 * HTTP API: Stop continuous simulation
 * We provide HTTP control to stop continuous simulation
 */
Json::Value TernaryFissionSimulationEngine::stopContinuousSimulationAPI() {
    stopContinuousSimulation();

    Json::Value response;
    response["status"] = "success";
    response["message"] = "Continuous simulation stopped";
    response["simulation_running"] = false;

    return response;
}

/*
 * HTTP API: Create energy field
 * We provide HTTP endpoint to create energy fields
 */
Json::Value TernaryFissionSimulationEngine::createEnergyFieldAPI(const Json::Value& request) {
    double energy_mev = request.get("energy_mev", 100.0).asDouble();
    std::string field_type = request.get("field_type", "electromagnetic").asString();

    if (energy_mev <= 0 || energy_mev > 10000) {
        Json::Value error;
        error["error"] = "Invalid energy_mev: must be between 0 and 10000";
        error["status"] = "error";
        return error;
    }

    try {
        EnergyField field = createEnergyField(energy_mev);
        std::lock_guard<std::mutex> lock(state_mutex);
        simulation_state.active_energy_fields.push_back(field);
        total_energy_fields_created.fetch_add(1, std::memory_order_relaxed);

        Json::Value response;
        response["status"] = "success";
        response["message"] = "Energy field created successfully";
        response["energy_field"] = serializeEnergyFieldToJSON(field);

        return response;

    } catch (const std::exception& e) {
        Json::Value error;
        error["error"] = "Failed to create energy field: " + std::string(e.what());
        error["status"] = "error";
        return error;
    }
}

/*
 * Dissipate energy from an existing field
 * We apply physics utility dissipation for the specified rounds
 */
void TernaryFissionSimulationEngine::dissipateEnergyField(EnergyField& field, int rounds) {
    for (int i = 0; i < rounds; ++i) {
        ::TernaryFission::dissipateEnergyField(field);
        if (field.energy_mev <= 0.0) {
            break;
        }
    }
}

/*
 * Serialize fission event to JSON format
 * We convert physics data structures to JSON for HTTP API
 */
Json::Value TernaryFissionSimulationEngine::serializeFissionEventToJSON(const TernaryFissionEvent& event) const {
    Json::Value json_event;

    // Event metadata
    auto time_since_epoch = event.timestamp.time_since_epoch();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    json_event["timestamp_ms"] = static_cast<Json::Int64>(timestamp_ms);

    // Fragment data
    Json::Value heavy_fragment;
    heavy_fragment["mass"] = event.heavy_fragment.mass;
    heavy_fragment["atomic_number"] = static_cast<Json::Int64>(
        event.heavy_fragment.atomic_number);
    heavy_fragment["mass_number"] = static_cast<Json::Int64>(
        event.heavy_fragment.mass_number);
    heavy_fragment["kinetic_energy"] = event.heavy_fragment.kinetic_energy;
    heavy_fragment["binding_energy"] = event.heavy_fragment.binding_energy;
    heavy_fragment["excitation_energy"] = event.heavy_fragment.excitation_energy;
    heavy_fragment["half_life"] = event.heavy_fragment.half_life;
    Json::Value heavy_momentum;
    heavy_momentum["x"] = event.heavy_fragment.momentum.x;
    heavy_momentum["y"] = event.heavy_fragment.momentum.y;
    heavy_momentum["z"] = event.heavy_fragment.momentum.z;
    heavy_fragment["momentum"] = heavy_momentum;
    Json::Value heavy_position;
    heavy_position["x"] = event.heavy_fragment.position.x;
    heavy_position["y"] = event.heavy_fragment.position.y;
    heavy_position["z"] = event.heavy_fragment.position.z;
    heavy_fragment["position"] = heavy_position;
    json_event["heavy_fragment"] = heavy_fragment;

    Json::Value light_fragment;
    light_fragment["mass"] = event.light_fragment.mass;
    light_fragment["atomic_number"] = static_cast<Json::Int64>(
        event.light_fragment.atomic_number);
    light_fragment["mass_number"] = static_cast<Json::Int64>(
        event.light_fragment.mass_number);
    light_fragment["kinetic_energy"] = event.light_fragment.kinetic_energy;
    light_fragment["binding_energy"] = event.light_fragment.binding_energy;
    light_fragment["excitation_energy"] = event.light_fragment.excitation_energy;
    light_fragment["half_life"] = event.light_fragment.half_life;
    Json::Value light_momentum;
    light_momentum["x"] = event.light_fragment.momentum.x;
    light_momentum["y"] = event.light_fragment.momentum.y;
    light_momentum["z"] = event.light_fragment.momentum.z;
    light_fragment["momentum"] = light_momentum;
    Json::Value light_position;
    light_position["x"] = event.light_fragment.position.x;
    light_position["y"] = event.light_fragment.position.y;
    light_position["z"] = event.light_fragment.position.z;
    light_fragment["position"] = light_position;
    json_event["light_fragment"] = light_fragment;

    Json::Value alpha_particle;
    alpha_particle["mass"] = event.alpha_particle.mass;
    alpha_particle["atomic_number"] = static_cast<Json::Int64>(
        event.alpha_particle.atomic_number);
    alpha_particle["mass_number"] = static_cast<Json::Int64>(
        event.alpha_particle.mass_number);
    alpha_particle["kinetic_energy"] = event.alpha_particle.kinetic_energy;
    alpha_particle["binding_energy"] = event.alpha_particle.binding_energy;
    alpha_particle["excitation_energy"] = event.alpha_particle.excitation_energy;
    alpha_particle["half_life"] = event.alpha_particle.half_life;
    Json::Value alpha_momentum;
    alpha_momentum["x"] = event.alpha_particle.momentum.x;
    alpha_momentum["y"] = event.alpha_particle.momentum.y;
    alpha_momentum["z"] = event.alpha_particle.momentum.z;
    alpha_particle["momentum"] = alpha_momentum;
    Json::Value alpha_position;
    alpha_position["x"] = event.alpha_particle.position.x;
    alpha_position["y"] = event.alpha_particle.position.y;
    alpha_position["z"] = event.alpha_particle.position.z;
    alpha_particle["position"] = alpha_position;
    json_event["alpha_particle"] = alpha_particle;

    // Energy data
    json_event["total_kinetic_energy"] = event.total_kinetic_energy;
    json_event["q_value"] = event.q_value;
    json_event["binding_energy_released"] = event.binding_energy_released;

    // Conservation law verification
    json_event["energy_conserved"] = event.energy_conserved;
    json_event["momentum_conserved"] = event.momentum_conserved;
    json_event["energy_conservation_error"] = event.energy_conservation_error;
    json_event["momentum_conservation_error"] = event.momentum_conservation_error;

    // Energy field reference
    if (event.energy_field_id != 0) {
        json_event["energy_field_id"] = static_cast<Json::UInt64>(event.energy_field_id);
    }

    return json_event;
}

/*
 * Serialize energy field to JSON format
 * We convert energy field data to JSON for HTTP API
 */
Json::Value TernaryFissionSimulationEngine::serializeEnergyFieldToJSON(const EnergyField& field) const {
    Json::Value json_field;

    json_field["field_id"] = static_cast<Json::UInt64>(field.field_id);
    json_field["energy_mev"] = field.energy_mev;
    json_field["memory_bytes"] = static_cast<Json::UInt64>(field.memory_bytes);
    json_field["cpu_cycles"] = static_cast<Json::UInt64>(field.cpu_cycles);
    json_field["entropy_factor"] = field.entropy_factor;
    json_field["creation_time_ms"] = static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            field.creation_time.time_since_epoch()
        ).count()
    );

    // Field properties
    Json::Value properties;
    properties["dissipation_rate"] = field.dissipation_rate;
    properties["stability_factor"] = field.stability_factor;
    properties["interaction_strength"] = field.interaction_strength;
    json_field["properties"] = properties;

    // Memory mapping info
    if (field.memory_ptr && field.memory_bytes > 0) {
        json_field["memory_address"] = static_cast<Json::UInt64>(
            reinterpret_cast<uintptr_t>(field.memory_ptr));
    }

    return json_field;
}

// Existing methods continue unchanged from original implementation...

/*
 * Start continuous simulation mode
 * We run simulation at target rate in background thread
 */
void TernaryFissionSimulationEngine::startContinuousSimulation(double events_per_second) {
    if (continuous_mode_active.load()) {
        std::cout << "Continuous simulation already running" << std::endl;
        return;
    }

    target_events_per_second.store(events_per_second);
    continuous_mode_active.store(true);

    {
        std::lock_guard<std::mutex> lock(state_mutex);
        simulation_state.simulation_running = true;
    }

    continuous_thread = std::thread(&TernaryFissionSimulationEngine::continuousGeneratorFunction, this);

    std::cout << "Continuous simulation started at " << events_per_second << " events/sec" << std::endl;
}

/*
 * Stop continuous simulation mode
 * We gracefully stop the background simulation
 */
void TernaryFissionSimulationEngine::stopContinuousSimulation() {
    if (!continuous_mode_active.load()) {
        return;
    }

    continuous_mode_active.store(false);

    {
        std::lock_guard<std::mutex> lock(state_mutex);
        simulation_state.simulation_running = false;
    }

    if (continuous_thread.joinable()) {
        continuous_thread.join();
    }

    std::cout << "Continuous simulation stopped" << std::endl;
}

/*
 * Get total events simulated
 */
uint64_t TernaryFissionSimulationEngine::getTotalEventsSimulated() const {
    return total_events_simulated.load(std::memory_order_relaxed);
}

/*
 * Get total energy fields created
 */
uint64_t TernaryFissionSimulationEngine::getTotalEnergyFieldsCreated() const {
    return total_energy_fields_created.load(std::memory_order_relaxed);
}

/*
 * Get total computation time
 */
double TernaryFissionSimulationEngine::getTotalComputationTimeSeconds() const {
    std::lock_guard<std::mutex> lock(computation_time_mutex);
    return total_computation_time_seconds;
}

/*
 * Print system status
 */
void TernaryFissionSimulationEngine::printSystemStatus() const {
    std::lock_guard<std::mutex> lock(state_mutex);

    std::cout << "\n=== System Status ===" << std::endl;
    std::cout << "Simulation Running: " << (simulation_state.simulation_running ? "Yes" : "No") << std::endl;
    std::cout << "Continuous Mode: " << (continuous_mode_active.load() ? "Active" : "Inactive") << std::endl;
    std::cout << "Active Energy Fields: " << simulation_state.active_energy_fields.size() << std::endl;
    std::cout << "Energy Conservation: " << (simulation_state.energy_conservation_enabled ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Momentum Conservation: " << (simulation_state.momentum_conservation_enabled ? "Enabled" : "Disabled") << std::endl;

    if (continuous_mode_active.load()) {
        std::cout << "Target Rate: " << target_events_per_second.load() << " events/sec" << std::endl;
    }
}

/*
 * Shutdown the simulation engine
 * We ensure all threads are properly terminated
 */
void TernaryFissionSimulationEngine::shutdown() {
    std::cout << "Shutting down simulation engine..." << std::endl;

    // Stop any running simulation
    stopContinuousSimulation();

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

    static std::atomic<std::uint64_t> event_id_counter{1};
    event.event_id = event_id_counter.fetch_add(1, std::memory_order_relaxed);
    event.energy_field_id = generateFieldId();

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

    // Estimate atomic numbers (proportional to mass)
    double z_ratio = static_cast<double>(parent_atomic_number - 2) / remaining_mass;
    event.light_fragment.atomic_number = static_cast<int>(event.light_fragment.mass * z_ratio);
    event.heavy_fragment.atomic_number = parent_atomic_number - 2 - event.light_fragment.atomic_number;

    // Mass numbers (approximately equal to mass)
    event.light_fragment.mass_number = static_cast<int>(event.light_fragment.mass + 0.5);
    event.heavy_fragment.mass_number = static_cast<int>(event.heavy_fragment.mass + 0.5);

    // Calculate Q-value (simplified)
    event.q_value = excitation_energy + (parent_mass - event.heavy_fragment.mass -
                    event.light_fragment.mass - event.alpha_particle.mass) * 931.5;  // MeV

    // Distribute kinetic energy among fragments
    double total_ke = event.q_value;
    if (total_ke > 0) {
        // Energy distribution based on momentum conservation
        double alpha_ke_fraction = 0.1;  // Alpha gets ~10% of kinetic energy
        double light_ke_fraction = 0.4;  // Light fragment gets ~40%
        double heavy_ke_fraction = 0.5;  // Heavy fragment gets ~50%

        event.alpha_particle.kinetic_energy = total_ke * alpha_ke_fraction;
        event.light_fragment.kinetic_energy = total_ke * light_ke_fraction;
        event.heavy_fragment.kinetic_energy = total_ke * heavy_ke_fraction;
    }

    event.total_kinetic_energy = event.alpha_particle.kinetic_energy +
                                event.light_fragment.kinetic_energy +
                                event.heavy_fragment.kinetic_energy;
    event.binding_energy_released = event.q_value - event.total_kinetic_energy;

    // Generate random momentum directions (conservation will be applied)
    generateRandomMomentum(event.alpha_particle);
    generateRandomMomentum(event.light_fragment);
    generateRandomMomentum(event.heavy_fragment);

    // Apply conservation laws
    applyConservationLaws(event);

    // Calculate conservation errors
    event.energy_conservation_error = std::abs(event.q_value - event.total_kinetic_energy);
    event.energy_conserved = event.energy_conservation_error < 1e-3;

    double total_px = event.heavy_fragment.momentum.x + event.light_fragment.momentum.x +
                      event.alpha_particle.momentum.x;
    double total_py = event.heavy_fragment.momentum.y + event.light_fragment.momentum.y +
                      event.alpha_particle.momentum.y;
    double total_pz = event.heavy_fragment.momentum.z + event.light_fragment.momentum.z +
                      event.alpha_particle.momentum.z;
    event.momentum_conservation_error =
        std::sqrt(total_px * total_px + total_py * total_py + total_pz * total_pz);
    event.momentum_conserved = event.momentum_conservation_error < 1e-6;

    return event;
}

/*
 * Process a fission event (private method)
 * We handle event processing and energy field creation
 */
void TernaryFissionSimulationEngine::processFissionEvent(const TernaryFissionEvent& event) {
    // Create energy field based on event
    try {
        EnergyField energy_field = createEnergyField(event.total_kinetic_energy);
        energy_field.field_id = event.energy_field_id;

        {
            std::lock_guard<std::mutex> lock(state_mutex);
            simulation_state.active_energy_fields.push_back(energy_field);
            simulation_state.fission_events.push_back(event);
        }

        total_energy_fields_created.fetch_add(1, std::memory_order_relaxed);

        // Log event if requested
        logFissionEvent(event);

    } catch (const std::exception& e) {
        std::cerr << "Error processing fission event: " << e.what() << std::endl;
    }
}

/*
 * Worker thread function (private method)
 * We process events from the queue in parallel
 */
void TernaryFissionSimulationEngine::workerThreadFunction(int thread_id) {
    (void)thread_id;

    while (!shutdown_requested.load()) {
        std::unique_lock<std::mutex> lock(queue_mutex);

        queue_cv.wait(lock, [this] {
            return !event_queue.empty() || shutdown_requested.load();
        });

        if (shutdown_requested.load()) {
            break;
        }

        if (event_queue.empty()) {
            continue;
        }

        TernaryFissionEvent event = event_queue.front();
        event_queue.pop();
        lock.unlock();

        // Process the event
        processFissionEvent(event);
    }
}

/*
 * Continuous generator function (private method)
 * We generate events at the target rate
 */
void TernaryFissionSimulationEngine::continuousGeneratorFunction() {
    auto last_event_time = std::chrono::high_resolution_clock::now();
    double target_rate = target_events_per_second.load();
    auto target_interval = std::chrono::microseconds(static_cast<long>(1e6 / target_rate));

    while (continuous_mode_active.load()) {
        auto now = std::chrono::high_resolution_clock::now();

        if (now - last_event_time >= target_interval) {
            // Generate and queue event
            TernaryFissionEvent event = generateFissionEvent(default_parent_mass, default_excitation_energy);

            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                event_queue.push(event);
            }
            queue_cv.notify_one();

            last_event_time = now;

            // Update rate if changed
            double new_rate = target_events_per_second.load();
            if (new_rate != target_rate) {
                target_rate = new_rate;
                target_interval = std::chrono::microseconds(static_cast<long>(1e6 / target_rate));
            }
        }

        // Small sleep to prevent busy waiting
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
        // Apply dissipation using physics utilities
        dissipateEnergyField(*it, 1);

        // Remove fields with very low energy
        if (it->energy_mev < 0.001) {
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
    (void)event;

    // Event logging implementation would write to log files
    // For now, we just track in memory
}

} // namespace TernaryFission
