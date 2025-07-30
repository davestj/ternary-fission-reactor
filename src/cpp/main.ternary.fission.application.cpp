/*
 * File: src/cpp/main.ternary.fission.application.cpp
 * Author: bthlops (David StJ)
 * Date: July 30, 2025
 * Title: Ternary Fission Simulation Main Application
 * Purpose: Command-line application to run, monitor, and control the Ternary Fission Simulation Engine
 * Reason: Provides operational entry point, demo interface, and CLI for benchmarking, simulation runs, and reporting
 *
 * Change Log:
 * - 2025-07-30: Initial implementation with CLI interface and simulation control
 * - 2025-07-30: Added full system and event reporting output
 * - 2025-07-30: Integrated with TernaryFissionSimulationEngine interface for all core functions
 * - 2025-07-30: Added JSON statistics dump for machine-readable output
 * - 2025-07-30: Added error handling and command validation
 * - 2025-07-30: Fixed GCC struct stat initializer warning by using {} value-initialization instead of {0}
 *
 * Leave-off Context:
 * - Full implementation, ready for CLI operation and integration testing
 * - Next: Integrate with Go API (CGO/IPC)
 */

#include "ternary.fission.simulation.engine.h"
#include "physics.utilities.h"
#include "physics.constants.definitions.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdlib>
#include <csignal>
#include <getopt.h>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>

using namespace TernaryFission;

// Forward declarations
void printHelp();
void printBanner();
void printEngineSummary(const TernaryFissionSimulationEngine& engine);
void printEvent(const TernaryFissionEvent& event);
void dumpStatisticsJSON(const TernaryFissionSimulationEngine& engine, const std::string& filename);
void printProgressBar(double progress, size_t width = 60);
void cliREPL(TernaryFissionSimulationEngine& engine);

// Global pointer for signal handling
TernaryFissionSimulationEngine* global_engine = nullptr;
std::atomic<bool> terminate_requested(false);

void handleSignal(int signum) {
    std::cout << "\nSignal " << signum << " received, shutting down gracefully..." << std::endl;
    terminate_requested.store(true);
    if (global_engine) {
        global_engine->shutdown();
    }
    std::exit(0);
}

int main(int argc, char* argv[]) {
    printBanner();

    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    // CLI Options
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

    static struct option long_options[] = {
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
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;
    while ((opt = getopt_long(argc, argv, "hp:e:n:t:cd:r:j::xl:", long_options, &long_index)) != -1) {
        switch (opt) {
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
                if (optarg)
                    json_filename = std::string(optarg);
                break;
            case 'x':
                run_repl = true;
                break;
            case 'l':
                log_dir = std::string(optarg);
                event_logfile = log_dir + "/event.log";
                break;
            default: print_help = true; break;
        }
    }

    if (print_help) {
        printHelp();
        return 0;
    }

    // Ensure log directory exists
    struct stat st = {}; // Fixed for GCC/C++11+
    if (stat(log_dir.c_str(), &st) == -1) {
        mkdir(log_dir.c_str(), 0700);
    }

    // Print simulation parameters
    std::cout << "Simulation Parameters:" << std::endl;
    std::cout << "  Parent mass (AMU):      " << parent_mass << std::endl;
    std::cout << "  Excitation energy (MeV):" << excitation_energy << std::endl;
    std::cout << "  Events:                 " << num_events << std::endl;
    std::cout << "  Threads:                " << threads << std::endl;
    if (run_continuous) {
        std::cout << "  Continuous mode:        ENABLED" << std::endl;
        std::cout << "  Duration (s):           " << duration_seconds << std::endl;
        std::cout << "  Events per second:      " << events_per_second << std::endl;
    } else {
        std::cout << "  Continuous mode:        DISABLED" << std::endl;
    }
    if (output_json) {
        std::cout << "  Output JSON file:       " << json_filename << std::endl;
    }
    if (run_repl) {
        std::cout << "  REPL/Interactive:       ENABLED" << std::endl;
    }
    std::cout << "  Log directory:          " << log_dir << std::endl;
    std::cout << "---------------------------------------------" << std::endl;

    // Instantiate the simulation engine
    TernaryFissionSimulationEngine engine(parent_mass, excitation_energy, threads);
    global_engine = &engine;

    if (run_repl) {
        cliREPL(engine);
        return 0;
    }

    if (run_continuous) {
        // Continuous mode with progress bar
        std::cout << "Running continuous simulation..." << std::endl;
        std::thread progress_thread([&engine, duration_seconds]() {
            double elapsed = 0.0;
            const double tick = 0.2;
            while (!terminate_requested.load() && elapsed < duration_seconds) {
                std::cout << "\r[";
                printProgressBar(elapsed / duration_seconds);
                std::cout << "] " << std::fixed << std::setprecision(1) << elapsed << "s / "
                          << duration_seconds << "s   " << std::flush;
                std::this_thread::sleep_for(std::chrono::duration<double>(tick));
                elapsed += tick;
            }
            std::cout << std::endl;
        });

        engine.runSimulation(duration_seconds, events_per_second);
        progress_thread.join();
        if (output_json) {
            dumpStatisticsJSON(engine, json_filename);
        }
        engine.printSystemStatus();
        return 0;
    }

    // Single-run mode: simulate specified number of events
    std::vector<TernaryFissionEvent> events;
    for (int i = 0; i < num_events && !terminate_requested.load(); ++i) {
        auto event = engine.simulateTernaryFissionEvent(parent_mass, excitation_energy);
        events.push_back(event);
        printEvent(event);
    }
    engine.printSystemStatus();

    if (output_json) {
        dumpStatisticsJSON(engine, json_filename);
    }
    return 0;
}

void printBanner() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   Ternary Fission Reactor Simulation CLI v1.0.0   " << std::endl;
    std::cout << "   (C) 2025 Beyond The Horizon Labs - David StJ    " << std::endl;
    std::cout << "   Unified Stargate Physics Test Platform          " << std::endl;
    std::cout << "==================================================" << std::endl << std::endl;
}

void printHelp() {
    std::cout << "Usage: ternary-fission [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help                Show this help message" << std::endl;
    std::cout << "  -p, --parent <mass>       Set parent nucleus mass (default: 235.0)" << std::endl;
    std::cout << "  -e, --excitation <MeV>    Set excitation energy (default: 6.5)" << std::endl;
    std::cout << "  -n, --events <N>          Number of events to simulate (default: 10)" << std::endl;
    std::cout << "  -t, --threads <N>         Number of worker threads (default: hardware concurrency)" << std::endl;
    std::cout << "  -c, --continuous          Enable continuous simulation mode" << std::endl;
    std::cout << "  -d, --duration <sec>      Duration of continuous run (default: 10)" << std::endl;
    std::cout << "  -r, --rate <N>            Events per second (default: 10)" << std::endl;
    std::cout << "  -j, --json [<file>]       Output statistics in JSON format (default: simulation_stats.json)" << std::endl;
    std::cout << "  -x, --repl                Enable REPL/interactive mode" << std::endl;
    std::cout << "  -l, --logdir <path>       Set log directory (default: ./logs)" << std::endl;
    std::cout << std::endl;
    std::cout << "Example: ./ternary-fission -n 100 -p 238 -e 7.1 -t 8 -j results.json" << std::endl;
    std::cout << std::endl;
}

void printProgressBar(double progress, size_t width) {
    size_t pos = static_cast<size_t>(width * progress);
    for (size_t i = 0; i < width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
}

void printEvent(const TernaryFissionEvent& event) {
    std::cout << "----- Ternary Fission Event -----" << std::endl;
    std::cout << "Q-value:              " << event.q_value << " MeV" << std::endl;
    std::cout << "Total kinetic energy:  " << event.total_kinetic_energy << " MeV" << std::endl;

    std::cout << "Light fragment:        "
              << "Mass=" << event.light_fragment.mass << " AMU, "
              << "Z=" << event.light_fragment.atomic_number << ", "
              << "A=" << event.light_fragment.mass_number << ", "
              << "KE=" << event.light_fragment.kinetic_energy << " MeV" << std::endl;

    std::cout << "Heavy fragment:        "
              << "Mass=" << event.heavy_fragment.mass << " AMU, "
              << "Z=" << event.heavy_fragment.atomic_number << ", "
              << "A=" << event.heavy_fragment.mass_number << ", "
              << "KE=" << event.heavy_fragment.kinetic_energy << " MeV" << std::endl;

    std::cout << "Alpha particle:        "
              << "Mass=" << event.alpha_particle.mass << " AMU, "
              << "KE=" << event.alpha_particle.kinetic_energy << " MeV" << std::endl;

    std::cout << "Conservation:          "
              << "momentum=" << (event.momentum_conserved ? "OK" : "FAIL") << ", "
              << "energy=" << (event.energy_conserved ? "OK" : "FAIL") << ", "
              << "mass=" << (event.mass_number_conserved ? "OK" : "FAIL") << ", "
              << "charge=" << (event.charge_conserved ? "OK" : "FAIL") << std::endl;

    std::cout << "----------------------------------" << std::endl << std::endl;
}

void dumpStatisticsJSON(const TernaryFissionSimulationEngine& engine, const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }
    ofs << engine.getStatisticsJSON() << std::endl;
    ofs.close();
    std::cout << "Statistics written to " << filename << std::endl;
}

// Interactive CLI REPL
void cliREPL(TernaryFissionSimulationEngine& engine) {
    std::cout << "\nEntering Interactive REPL mode (type 'help' for commands, 'exit' to quit)\n";
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd == "exit" || cmd == "quit") break;
        if (cmd == "help") {
            std::cout << "Commands: event <n> | status | stats | json <file> | exit" << std::endl;
            continue;
        }
        if (cmd == "event") {
            int n = 1;
            iss >> n;
            for (int i = 0; i < n; ++i) {
                auto event = engine.simulateTernaryFissionEvent(engine.default_parent_mass, engine.default_excitation_energy);
                printEvent(event);
            }
            continue;
        }
        if (cmd == "status") {
            engine.printSystemStatus();
            continue;
        }
        if (cmd == "stats") {
            std::cout << engine.getStatisticsJSON() << std::endl;
            continue;
        }
        if (cmd == "json") {
            std::string fname;
            iss >> fname;
            if (fname.empty()) fname = "repl_stats.json";
            dumpStatisticsJSON(engine, fname);
            continue;
        }
        std::cout << "Unknown command: " << cmd << std::endl;
    }
}
