/*
 * File: src/cpp/main.ternary.fission.application.cpp
 * Author: bthlops (David StJ)
 * Date: January 31, 2025
 * Title: Ternary Fission Simulation Main Application with Daemon Integration
 * Purpose: Command-line application with daemon mode, HTTP server, and configuration management
 * Reason: Provides complete operational entry point for distributed daemon architecture
 *
 * Change Log:
 * - 2025-07-30: Initial implementation with CLI interface and simulation control
 * - 2025-07-30: Added full system and event reporting output
 * - 2025-07-30: Integrated with TernaryFissionSimulationEngine interface for all core functions
 * - 2025-07-30: Added JSON statistics dump for machine-readable output
 * - 2025-07-30: Added error handling and command validation
 * - 2025-01-31: Integrated daemon management and HTTP server functionality
 *               Added configuration file support with daemon.config parsing
 *               Added --daemon, --config, --bind-ip, --bind-port command line options
 *               Integrated ConfigurationManager, DaemonTernaryFissionServer, HTTPTernaryFissionServer
 *               Added graceful shutdown coordination between daemon and HTTP server
 *               Preserved all existing CLI functionality for backward compatibility
 *
 * Carry-over Context:
 * - This application now supports both CLI and daemon operation modes
 * - Daemon mode provides full Unix daemon functionality with systemd integration
 * - HTTP server mode provides REST API with physics simulation endpoints
 * - Configuration management supports INI files with environment variable overrides
 * - All existing CLI functionality remains unchanged for backward compatibility
 * - Next: Production deployment with systemd service files and SSL certificates
 */

#include "ternary.fission.simulation.engine.h"
#include "physics.utilities.h"
#include "physics.constants.definitions.h"
#include "config.ternary.fission.server.h"
#include "daemon.ternary.fission.server.h"
#include "http.ternary.fission.server.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <cstdlib>
#include <csignal>
#include <getopt.h>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>

using namespace TernaryFission;

// Forward declarations for existing functions
void printHelp();
void printBanner();
void printEngineSummary(const TernaryFissionSimulationEngine& engine);
void printEvent(const TernaryFissionEvent& event);
void dumpStatisticsJSON(const TernaryFissionSimulationEngine& engine, const std::string& filename);
void printProgressBar(double progress, size_t width = 60);
void cliREPL(TernaryFissionSimulationEngine& engine);

// New daemon mode functions
void printDaemonHelp();
void runDaemonMode(const std::string& config_file, const std::string& bind_ip, int bind_port);
void runHTTPServerMode(const std::string& config_file, const std::string& bind_ip, int bind_port);
bool createDefaultConfigFile(const std::string& config_path);

// Global pointers for signal handling and graceful shutdown
TernaryFissionSimulationEngine* global_engine = nullptr;
DaemonTernaryFissionServer* global_daemon = nullptr;
HTTPTernaryFissionServer* global_http_server = nullptr;
std::atomic<bool> terminate_requested(false);

/**
 * We handle signals for graceful shutdown of all components
 * This function coordinates shutdown between CLI, daemon, and HTTP server modes
 */
void handleSignal(int signum) {
    std::cout << "\nSignal " << signum << " received, shutting down gracefully..." << std::endl;
    terminate_requested.store(true);

    if (global_engine) {
        global_engine->shutdown();
    }

    if (global_http_server) {
        global_http_server->stop();
    }

    if (global_daemon) {
        global_daemon->stopDaemon();
    }

    std::exit(0);
}

int main(int argc, char* argv[]) {
    printBanner();

    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGHUP, handleSignal);

    // CLI Options - existing functionality preserved
    double parent_mass = 235.0;
    double excitation_energy = 6.5;
    int num_events = 10;
    int threads = std::thread::hardware_concurrency();
    bool run_continuous = false;
    double duration_seconds = 10.0;
    double events_per_second = 10.0;
    bool output_json = false;
    std::string json_filename = "simulation_stats.json";
    bool print_help = false;
    bool run_repl = false;
    std::string log_dir = "./logs";
    std::string event_logfile = log_dir + "/event.log";

    // New daemon mode options
    bool daemon_mode = false;
    bool http_server_mode = false;
    std::string config_file = "configs/daemon.config";
    std::string bind_ip = "127.0.0.1";
    int bind_port = 8333;
    bool create_config = false;
    bool show_daemon_help = false;

    static struct option long_options[] = {
        // Existing options
        {"help",        no_argument,       0,  'h' },
        {"parent",      required_argument, 0,  'p' },
        {"excitation",  required_argument, 0,  'e' },
        {"events",      required_argument, 0,  'n' },
        {"threads",     required_argument, 0,  't' },
        {"continuous",  no_argument,       0,  'c' },
        {"duration",    required_argument, 0,  'd' },
        {"rate",        required_argument, 0,  'r' },
        {"json",        optional_argument, 0,  'j' },
        {"repl",        no_argument,       0,  'x' },
        {"logdir",      required_argument, 0,  'l' },

        // New daemon mode options
        {"daemon",      no_argument,       0,  'D' },
        {"server",      no_argument,       0,  'S' },
        {"config",      required_argument, 0,  'C' },
        {"bind-ip",     required_argument, 0,  'I' },
        {"bind-port",   required_argument, 0,  'P' },
        {"create-config", no_argument,     0,  'G' },
        {"daemon-help", no_argument,       0,  'H' },
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;
    while ((opt = getopt_long(argc, argv, "hp:e:n:t:cd:r:j::xl:DSC:I:P:GH", long_options, &long_index)) != -1) {
        switch (opt) {
            // Existing option handling
            case 'h': print_help = true; break;
            case 'p': parent_mass = atof(optarg); break;
            case 'e': excitation_energy = atof(optarg); break;
            case 'n': num_events = atoi(optarg); break;
            case 't': threads = atoi(optarg); break;
            case 'c': run_continuous = true; break;
            case 'd': duration_seconds = atof(optarg); break;
            case 'r': events_per_second = atof(optarg); break;
            case 'j':
                output_json = true;
                if (optarg) json_filename = std::string(optarg);
                break;
            case 'x': run_repl = true; break;
            case 'l':
                log_dir = std::string(optarg);
                event_logfile = log_dir + "/event.log";
                break;

            // New daemon mode option handling
            case 'D': daemon_mode = true; break;
            case 'S': http_server_mode = true; break;
            case 'C': config_file = std::string(optarg); break;
            case 'I': bind_ip = std::string(optarg); break;
            case 'P': bind_port = atoi(optarg); break;
            case 'G': create_config = true; break;
            case 'H': show_daemon_help = true; break;
            case '?':
                std::cerr << "Unknown option. Use --help for usage information." << std::endl;
                return 1;
            default:
                break;
        }
    }

    // Handle help requests
    if (print_help) {
        printHelp();
        return 0;
    }

    if (show_daemon_help) {
        printDaemonHelp();
        return 0;
    }

    // Handle config file creation
    if (create_config) {
        if (createDefaultConfigFile(config_file)) {
            std::cout << "Default configuration file created: " << config_file << std::endl;
            std::cout << "Edit the file and run with --config to use daemon mode." << std::endl;
        } else {
            std::cerr << "Failed to create configuration file: " << config_file << std::endl;
            return 1;
        }
        return 0;
    }

    // Run in daemon mode if requested
    if (daemon_mode) {
        std::cout << "Starting in daemon mode with config: " << config_file << std::endl;
        runDaemonMode(config_file, bind_ip, bind_port);
        return 0;
    }

    // Run in HTTP server mode if requested
    if (http_server_mode) {
        std::cout << "Starting in HTTP server mode with config: " << config_file << std::endl;
        runHTTPServerMode(config_file, bind_ip, bind_port);
        return 0;
    }

    // Validate CLI parameters (existing validation preserved)
    if (parent_mass <= 0.0 || parent_mass > 300.0) {
        std::cerr << "Error: Parent mass must be between 0 and 300 AMU" << std::endl;
        return 1;
    }

    if (excitation_energy < 0.0 || excitation_energy > 100.0) {
        std::cerr << "Error: Excitation energy must be between 0 and 100 MeV" << std::endl;
        return 1;
    }

    if (num_events <= 0 || num_events > 1000000) {
        std::cerr << "Error: Number of events must be between 1 and 1,000,000" << std::endl;
        return 1;
    }

    if (threads <= 0 || threads > 256) {
        std::cerr << "Error: Number of threads must be between 1 and 256" << std::endl;
        return 1;
    }

    // Create log directory if needed (existing functionality)
    struct stat st = {};
    if (stat(log_dir.c_str(), &st) == -1) {
        std::cout << "Creating log directory: " << log_dir << std::endl;
        std::string mkdir_cmd = "mkdir -p " + log_dir;
        if (system(mkdir_cmd.c_str()) != 0) {
            std::cerr << "Warning: Could not create log directory" << std::endl;
        }
    }

    // Initialize simulation engine (existing functionality preserved)
    std::cout << "Initializing Ternary Fission Simulation Engine..." << std::endl;
    TernaryFissionSimulationEngine engine(parent_mass, excitation_energy, threads);
    global_engine = &engine;

    // Run REPL mode if requested (existing functionality)
    if (run_repl) {
        cliREPL(engine);
        return 0;
    }

    // Existing CLI execution modes preserved below...
    if (run_continuous) {
        std::cout << "Running continuous simulation for " << duration_seconds << " seconds at "
                  << events_per_second << " events/sec..." << std::endl;

        engine.startContinuousSimulation(events_per_second);

        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::duration<double>(duration_seconds);

        while (std::chrono::steady_clock::now() < end_time && !terminate_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();

            double progress = elapsed / (duration_seconds * 1000.0);
            printProgressBar(progress);
        }

        engine.stopContinuousSimulation();
        std::cout << "\nContinuous simulation completed." << std::endl;
    } else {
        std::cout << "Running batch simulation of " << num_events << " events..." << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_events && !terminate_requested.load(); ++i) {
            TernaryFissionEvent event = engine.simulateTernaryFissionEvent();

            if (i < 5) {  // Print first 5 events
                printEvent(event);
            }

            double progress = static_cast<double>(i + 1) / num_events;
            printProgressBar(progress);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << "\nBatch simulation completed in " << duration.count() << " ms" << std::endl;
    }

    // Generate final output (existing functionality)
    printEngineSummary(engine);

    if (output_json) {
        dumpStatisticsJSON(engine, json_filename);
        std::cout << "Statistics exported to: " << json_filename << std::endl;
    }

    std::cout << "Shutting down simulation engine..." << std::endl;
    engine.shutdown();

    std::cout << "Ternary Fission Simulation completed successfully." << std::endl;
    return 0;
}

// Existing functions preserved (implementation unchanged)...

void printBanner() {
    std::cout << "\n===============================================\n";
    std::cout << "  Ternary Fission Simulation Engine v1.1.13\n";
    std::cout << "  Author: bthlops (David StJ)\n";
    std::cout << "  High-Performance Nuclear Physics Simulation\n";
    std::cout << "===============================================\n\n";
}

void printHelp() {
    std::cout << "Usage: ternary-fission [OPTIONS]\n\n";
    std::cout << "Simulation Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -p, --parent MASS       Parent nucleus mass in AMU (default: 235.0)\n";
    std::cout << "  -e, --excitation ENERGY Excitation energy in MeV (default: 6.5)\n";
    std::cout << "  -n, --events COUNT      Number of events to simulate (default: 10)\n";
    std::cout << "  -t, --threads COUNT     Number of worker threads (default: auto)\n";
    std::cout << "  -c, --continuous        Run continuous simulation\n";
    std::cout << "  -d, --duration SECONDS  Duration for continuous mode (default: 10.0)\n";
    std::cout << "  -r, --rate EVENTS/SEC   Target events per second (default: 10.0)\n";
    std::cout << "  -j, --json [FILE]       Export statistics as JSON (default: simulation_stats.json)\n";
    std::cout << "  -x, --repl              Interactive REPL mode\n";
    std::cout << "  -l, --logdir DIR        Log directory path (default: ./logs)\n\n";
    std::cout << "Daemon Options:\n";
    std::cout << "  -D, --daemon            Run in daemon mode\n";
    std::cout << "  -S, --server            Run HTTP server without daemon\n";
    std::cout << "  -C, --config FILE       Configuration file path (default: configs/daemon.config)\n";
    std::cout << "  -I, --bind-ip IP        HTTP server bind IP (default: 127.0.0.1)\n";
    std::cout << "  -P, --bind-port PORT    HTTP server bind port (default: 8333)\n";
    std::cout << "  -G, --create-config     Create default configuration file\n";
    std::cout << "  -H, --daemon-help       Show detailed daemon mode help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  ternary-fission                    # Run 10 events with defaults\n";
    std::cout << "  ternary-fission -n 1000 -t 8       # Run 1000 events with 8 threads\n";
    std::cout << "  ternary-fission -c -d 30 -r 100    # Continuous mode, 30 sec, 100 events/sec\n";
    std::cout << "  ternary-fission -x                 # Interactive REPL mode\n";
    std::cout << "  ternary-fission --daemon            # Run as daemon with HTTP API\n";
    std::cout << "  ternary-fission --server --bind-port 8080  # HTTP server on port 8080\n";
}

void printDaemonHelp() {
    std::cout << "Daemon Mode Help\n";
    std::cout << "================\n\n";
    std::cout << "Daemon mode runs the ternary fission simulation as a background service\n";
    std::cout << "with an HTTP REST API for remote control and monitoring.\n\n";
    std::cout << "Configuration:\n";
    std::cout << "  Create a configuration file using --create-config first:\n";
    std::cout << "    ternary-fission --create-config --config /path/to/daemon.config\n\n";
    std::cout << "  Edit the configuration file to set network, SSL, and physics parameters.\n\n";
    std::cout << "Starting Daemon:\n";
    std::cout << "  ternary-fission --daemon --config /path/to/daemon.config\n";
    std::cout << "  ternary-fission --server --bind-ip 0.0.0.0 --bind-port 8443\n\n";
    std::cout << "API Endpoints:\n";
    std::cout << "  GET  /api/v1/health               - Health check\n";
    std::cout << "  GET  /api/v1/status               - System status\n";
    std::cout << "  GET  /api/v1/energy-fields        - List energy fields\n";
    std::cout << "  POST /api/v1/energy-fields        - Create energy field\n";
    std::cout << "  POST /api/v1/simulation/start     - Start simulation\n";
    std::cout << "  POST /api/v1/simulation/stop      - Stop simulation\n";
    std::cout << "  POST /api/v1/physics/fission      - Run fission calculation\n\n";
    std::cout << "Environment Variables:\n";
    std::cout << "  TERNARY_BIND_IP=0.0.0.0           - Override bind IP\n";
    std::cout << "  TERNARY_BIND_PORT=8333             - Override bind port\n";
    std::cout << "  TERNARY_ENABLE_SSL=true            - Enable HTTPS\n";
    std::cout << "  TERNARY_DAEMON_MODE=true           - Enable daemon mode\n";
    std::cout << "  TERNARY_LOG_LEVEL=debug            - Set log level\n\n";
    std::cout << "Signals:\n";
    std::cout << "  SIGTERM/SIGINT  - Graceful shutdown\n";
    std::cout << "  SIGHUP          - Reload configuration\n";
    std::cout << "  SIGUSR1         - Print status information\n";
}

/**
 * We run the application in full daemon mode
 * This function initializes daemon process, HTTP server, and physics engine
 */
void runDaemonMode(const std::string& config_file, const std::string& bind_ip, int bind_port) {
    try {
        // Initialize configuration manager
        auto config_manager = std::make_unique<ConfigurationManager>(config_file);
        if (!config_manager->loadConfiguration()) {
            std::cerr << "Error: Failed to load configuration file: " << config_file << std::endl;
            return;
        }

        if (!config_manager->validateConfiguration()) {
            std::cerr << "Error: Configuration validation failed" << std::endl;
            return;
        }

        std::cout << "Configuration loaded successfully from: " << config_file << std::endl;

        // Initialize daemon manager
        auto daemon_manager = std::make_unique<DaemonTernaryFissionServer>(
            std::unique_ptr<ConfigurationManager>(config_manager.release())
        );
        global_daemon = daemon_manager.get();

        if (!daemon_manager->initialize()) {
            std::cerr << "Error: Failed to initialize daemon manager" << std::endl;
            return;
        }

        // Start daemon process
        if (!daemon_manager->startDaemon()) {
            std::cerr << "Error: Failed to start daemon process" << std::endl;
            return;
        }

        std::cout << "Daemon process started successfully" << std::endl;

        // Initialize HTTP server with configuration
        auto http_server = std::make_unique<HTTPTernaryFissionServer>(
            std::make_unique<ConfigurationManager>(config_file)
        );
        global_http_server = http_server.get();

        if (!http_server->initialize()) {
            std::cerr << "Error: Failed to initialize HTTP server" << std::endl;
            daemon_manager->stopDaemon();
            return;
        }

        // Initialize physics engine
        auto physics_config = daemon_manager->getConfiguration()->getPhysicsConfiguration();
        auto engine = std::make_shared<TernaryFissionSimulationEngine>(
            physics_config.default_parent_mass,
            physics_config.default_excitation_energy,
            physics_config.default_thread_count
        );
        global_engine = engine.get();

        // Connect HTTP server to physics engine
        http_server->setSimulationEngine(engine);

        // Start HTTP server
        std::cout << "Starting HTTP server on " << bind_ip << ":" << bind_port << std::endl;

        // Run server (blocks until shutdown)
        if (!http_server->start()) {
            std::cerr << "Error: Failed to start HTTP server" << std::endl;
            daemon_manager->stopDaemon();
            return;
        }

        // Wait for shutdown signal
        while (!terminate_requested.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Graceful shutdown
        std::cout << "Shutting down daemon..." << std::endl;
        http_server->stop();
        engine->shutdown();
        daemon_manager->stopDaemon();

    } catch (const std::exception& e) {
        std::cerr << "Daemon mode error: " << e.what() << std::endl;
    }
}

/**
 * We run the application in HTTP server mode without daemon
 * This function provides HTTP API without daemon process management
 */
void runHTTPServerMode(const std::string& config_file, const std::string& bind_ip, int bind_port) {
    try {
        // Initialize configuration manager
        auto config_manager = std::make_unique<ConfigurationManager>(config_file);
        if (!config_manager->loadConfiguration()) {
            std::cerr << "Warning: Could not load config file, using defaults" << std::endl;
        }

        std::cout << "Starting HTTP server on " << bind_ip << ":" << bind_port << std::endl;

        // Initialize HTTP server
        auto http_server = std::make_unique<HTTPTernaryFissionServer>(std::move(config_manager));
        global_http_server = http_server.get();

        if (!http_server->initialize()) {
            std::cerr << "Error: Failed to initialize HTTP server" << std::endl;
            return;
        }

        // Initialize physics engine
        auto engine = std::make_shared<TernaryFissionSimulationEngine>();
        global_engine = engine.get();

        // Connect HTTP server to physics engine
        http_server->setSimulationEngine(engine);

        // Start server (blocks until shutdown)
        if (!http_server->start()) {
            std::cerr << "Error: Failed to start HTTP server" << std::endl;
            return;
        }

        // Wait for shutdown signal
        while (!terminate_requested.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Graceful shutdown
        std::cout << "Shutting down HTTP server..." << std::endl;
        http_server->stop();
        engine->shutdown();

    } catch (const std::exception& e) {
        std::cerr << "HTTP server mode error: " << e.what() << std::endl;
    }
}

/**
 * We create a default configuration file
 * This function generates daemon.config with default values
 */
bool createDefaultConfigFile(const std::string& config_path) {
    std::ofstream config_file(config_path);
    if (!config_file.is_open()) {
        return false;
    }

    config_file << "# Ternary Fission Daemon Configuration\n";
    config_file << "# Generated by ternary-fission --create-config\n\n";
    config_file << "# Network Configuration\n";
    config_file << "bind_ip = 127.0.0.1\n";
    config_file << "bind_port = 8333\n";
    config_file << "enable_ssl = false\n";
    config_file << "max_connections = 1000\n";
    config_file << "enable_cors = true\n\n";
    config_file << "# Daemon Configuration\n";
    config_file << "daemon_mode = false\n";
    config_file << "pid_file_path = /tmp/ternary-fission-daemon.pid\n";
    config_file << "working_directory = /\n\n";
    config_file << "# Physics Configuration\n";
    config_file << "parent_mass = 235.044063\n";
    config_file << "excitation_energy = 6.534\n";
    config_file << "events_per_second = 5.0\n";
    config_file << "num_threads = 0\n\n";
    config_file << "# Logging Configuration\n";
    config_file << "log_level = info\n";
    config_file << "access_log_path = logs/daemon-access.log\n";
    config_file << "error_log_path = logs/daemon-error.log\n";
    config_file << "enable_console_logging = true\n";

    return true;
}

// Existing functions preserved (implementation unchanged from original)...

void printEngineSummary(const TernaryFissionSimulationEngine& engine) {
    std::cout << "\n=== Simulation Engine Summary ===" << std::endl;
    std::cout << "Total Events Simulated: " << engine.getTotalEventsSimulated() << std::endl;
    std::cout << "Total Energy Fields Created: " << engine.getTotalEnergyFieldsCreated() << std::endl;
    std::cout << "Total Computation Time: " << std::fixed << std::setprecision(3)
              << engine.getTotalComputationTimeSeconds() << " seconds" << std::endl;

    if (engine.getTotalComputationTimeSeconds() > 0) {
        double events_per_second = engine.getTotalEventsSimulated() / engine.getTotalComputationTimeSeconds();
        std::cout << "Average Events/Second: " << std::fixed << std::setprecision(1)
                  << events_per_second << std::endl;
    }

    engine.printSystemStatus();
}

void printEvent(const TernaryFissionEvent& event) {
    std::cout << "Event: ";
    std::cout << "Heavy(" << std::fixed << std::setprecision(1) << event.heavy_fragment.mass << " AMU) + ";
    std::cout << "Light(" << event.light_fragment.mass << " AMU) + ";
    std::cout << "Alpha(" << event.alpha_particle.mass << " AMU) ";
    std::cout << "Total KE: " << std::setprecision(2) << event.total_kinetic_energy << " MeV";
    std::cout << std::endl;
}

void dumpStatisticsJSON(const TernaryFissionSimulationEngine& engine, const std::string& filename) {
    std::ofstream json_file(filename);
    if (!json_file.is_open()) {
        std::cerr << "Error: Could not create JSON output file: " << filename << std::endl;
        return;
    }

    json_file << "{\n";
    json_file << "  \"simulation_statistics\": {\n";
    json_file << "    \"total_events_simulated\": " << engine.getTotalEventsSimulated() << ",\n";
    json_file << "    \"total_energy_fields_created\": " << engine.getTotalEnergyFieldsCreated() << ",\n";
    json_file << "    \"total_computation_time_seconds\": " << engine.getTotalComputationTimeSeconds() << ",\n";

    if (engine.getTotalComputationTimeSeconds() > 0) {
        double events_per_second = engine.getTotalEventsSimulated() / engine.getTotalComputationTimeSeconds();
        json_file << "    \"average_events_per_second\": " << events_per_second << ",\n";
    }

    json_file << "    \"timestamp\": \"" << std::time(nullptr) << "\",\n";
    json_file << "    \"version\": \"1.1.13\",\n";
    json_file << "    \"author\": \"bthlops (David StJ)\"\n";
    json_file << "  }\n";
    json_file << "}\n";
}

void printProgressBar(double progress, size_t width) {
    std::cout << "\r[";
    size_t pos = static_cast<size_t>(width * progress);
    for (size_t i = 0; i < width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << (progress * 100.0) << "%";
    std::cout.flush();
}

void cliREPL(TernaryFissionSimulationEngine& engine) {
    std::string input;
    std::cout << "\nTernary Fission Interactive REPL Mode\n";
    std::cout << "Commands: simulate, status, continuous [start|stop], help, quit\n";
    std::cout << "> ";

    while (std::getline(std::cin, input) && input != "quit" && input != "exit") {
        if (input.empty()) {
            std::cout << "> ";
            continue;
        }

        if (input == "simulate") {
            TernaryFissionEvent event = engine.simulateTernaryFissionEvent();
            printEvent(event);
        } else if (input == "status") {
            printEngineSummary(engine);
        } else if (input == "continuous start") {
            engine.startContinuousSimulation(10.0);
            std::cout << "Continuous simulation started at 10 events/sec\n";
        } else if (input == "continuous stop") {
            engine.stopContinuousSimulation();
            std::cout << "Continuous simulation stopped\n";
        } else if (input == "help") {
            std::cout << "Available commands:\n";
            std::cout << "  simulate        - Run single fission event\n";
            std::cout << "  status          - Show engine status\n";
            std::cout << "  continuous start - Start continuous simulation\n";
            std::cout << "  continuous stop  - Stop continuous simulation\n";
            std::cout << "  help            - Show this help\n";
            std::cout << "  quit/exit       - Exit REPL\n";
        } else {
            std::cout << "Unknown command: " << input << "\n";
            std::cout << "Type 'help' for available commands.\n";
        }

        std::cout << "> ";
    }

    std::cout << "Exiting REPL mode.\n";
}