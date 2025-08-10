/*
 * File: src/cpp/main.ternary.fission.application.cpp
 * Author: bthlops (David StJ)
 * Date: January 31, 2025
 * Title: Ternary Fission Simulation Main Application with Daemon Integration
 * Purpose: Command-line application with daemon mode, HTTP server, and
 * configuration management Reason: Provides complete operational entry point
 * for distributed daemon architecture
 *
 * Change Log:
 * - 2025-07-30: Initial implementation with CLI interface and simulation
 * control
 * - 2025-07-30: Added full system and event reporting output
 * - 2025-07-30: Integrated with TernaryFissionSimulationEngine interface for
 * all core functions
 * - 2025-07-30: Added JSON statistics dump for machine-readable output
 * - 2025-07-30: Added error handling and command validation
 * - 2025-01-31: Integrated daemon management and HTTP server functionality
 *               Added configuration file support with daemon.config parsing
 *               Added --daemon, --config, --bind-ip, --bind-port command line
 * options Integrated ConfigurationManager, DaemonTernaryFissionServer,
 * HTTPTernaryFissionServer Added graceful shutdown coordination between daemon
 * and HTTP server Preserved all existing CLI functionality for backward
 * compatibility
 * - 2025-08-10: Stubbed implementation to restore build after source truncation
 *               Provides minimal entry point and placeholder helpers
 * - 2025-08-11: Restored full CLI, daemon, and HTTP server integration
 */

#include "config.ternary.fission.server.h"
#include "daemon.ternary.fission.server.h"
#include "http.ternary.fission.server.h"
#include "ternary.fission.simulation.engine.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <json/json.h>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace TernaryFission;

// Forward declarations for helper functions
void printBanner();
void printHelp();
void printEngineSummary(const TernaryFissionSimulationEngine &engine);
void printEvent(const TernaryFissionEvent &event);
void dumpStatisticsJSON(const TernaryFissionSimulationEngine &engine,
                        const std::string &filename);
void printProgressBar(double progress, size_t width = 60);
void cliREPL(TernaryFissionSimulationEngine &engine);
void printDaemonHelp();
void runDaemonMode(const std::string &config_file, const std::string &bind_ip,
                   int bind_port);
void runHTTPServerMode(const std::string &config_file,
                       const std::string &bind_ip, int bind_port);
bool createDefaultConfigFile(const std::string &config_path);

/**
 * Application entry point with full CLI support.
 * We parse command line options, load configuration, and execute requested
 * mode.
 */
int main(int argc, char *argv[]) {
  printBanner();

  // Simulation parameters with defaults
  int events = 10;
  int threads = 0;
  bool continuous = false;
  int duration = 10;
  double rate = 0.0; // Events per second override
  bool rate_specified = false;
  bool repl = false;
  std::string json_file;
  std::string logdir = "./logs";
  std::string config_path;
  bool daemon_mode = false;
  std::string bind_ip;
  int bind_port = 0;
  bool parent_specified = false;
  bool excitation_specified = false;
  double parent_mass = 235.0;
  double excitation_energy = 6.5;

  enum OptionCodes {
    OPT_CONFIG = 1000,
    OPT_DAEMON,
    OPT_BIND_IP,
    OPT_BIND_PORT
  };

  static struct option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"events", required_argument, nullptr, 'n'},
      {"parent", required_argument, nullptr, 'p'},
      {"excitation", required_argument, nullptr, 'e'},
      {"threads", required_argument, nullptr, 't'},
      {"continuous", no_argument, nullptr, 'c'},
      {"duration", required_argument, nullptr, 'd'},
      {"rate", required_argument, nullptr, 'r'},
      {"json", optional_argument, nullptr, 'j'},
      {"repl", no_argument, nullptr, 'x'},
      {"logdir", required_argument, nullptr, 'l'},
      {"config", required_argument, nullptr, OPT_CONFIG},
      {"daemon", no_argument, nullptr, OPT_DAEMON},
      {"bind-ip", required_argument, nullptr, OPT_BIND_IP},
      {"bind-port", required_argument, nullptr, OPT_BIND_PORT},
      {0, 0, 0, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "hn:p:e:t:cd:r:j::xl:", long_options,
                            nullptr)) != -1) {
    switch (opt) {
    case 'h':
      printHelp();
      return 0;
    case 'n':
      events = std::stoi(optarg);
      break;
    case 'p':
      parent_mass = std::stod(optarg);
      parent_specified = true;
      break;
    case 'e':
      excitation_energy = std::stod(optarg);
      excitation_specified = true;
      break;
    case 't':
      threads = std::stoi(optarg);
      break;
    case 'c':
      continuous = true;
      break;
    case 'd':
      duration = std::stoi(optarg);
      break;
    case 'r':
      rate = std::stod(optarg);
      rate_specified = true;
      break;
    case 'j':
      if (optarg) {
        json_file = optarg;
      } else {
        json_file = "simulation_stats.json";
      }
      break;
    case 'x':
      repl = true;
      break;
    case 'l':
      logdir = optarg;
      break;
    case OPT_CONFIG:
      config_path = optarg;
      break;
    case OPT_DAEMON:
      daemon_mode = true;
      break;
    case OPT_BIND_IP:
      bind_ip = optarg;
      break;
    case OPT_BIND_PORT:
      bind_port = std::stoi(optarg);
      break;
    default:
      printHelp();
      return 1;
    }
  }

  // Apply environment overrides for configuration manager
  if (parent_specified) {
    ::setenv("TERNARY_PARENT_MASS", std::to_string(parent_mass).c_str(), 1);
  }
  if (excitation_specified) {
    ::setenv("TERNARY_EXCITATION_ENERGY",
             std::to_string(excitation_energy).c_str(), 1);
  }
  if (rate_specified) {
    ::setenv("TERNARY_EVENTS_PER_SECOND", std::to_string(rate).c_str(), 1);
  }
  if (!bind_ip.empty()) {
    ::setenv("TERNARY_BIND_IP", bind_ip.c_str(), 1);
  }
  if (bind_port > 0) {
    ::setenv("TERNARY_BIND_PORT", std::to_string(bind_port).c_str(), 1);
  }
  if (daemon_mode) {
    ::setenv("TERNARY_DAEMON_MODE", "1", 1);
  }

  if (daemon_mode) {
    runDaemonMode(config_path, bind_ip, bind_port);
    return 0;
  }

  if (!bind_ip.empty() || bind_port > 0) {
    runHTTPServerMode(config_path, bind_ip, bind_port);
    return 0;
  }

  // We load configuration to obtain physics defaults
  auto config_manager = std::make_unique<ConfigurationManager>(config_path);
  const auto &physics_cfg = config_manager->getPhysicsConfig();
  if (!parent_specified) {
    parent_mass = physics_cfg.default_parent_mass;
  }
  if (!excitation_specified) {
    excitation_energy = physics_cfg.default_excitation_energy;
  }
  if (!rate_specified) {
    rate = physics_cfg.events_per_second;
  }

  TernaryFissionSimulationEngine engine(parent_mass, excitation_energy,
                                        threads);

  if (repl) {
    cliREPL(engine);
    return 0;
  }

  if (continuous) {
    int total_events = static_cast<int>(duration * rate);
    for (int i = 0; i < total_events; ++i) {
      auto ev = engine.simulateTernaryFissionEvent();
      printEvent(ev);
      printProgressBar(static_cast<double>(i + 1) / total_events);
      if (rate > 0) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<int>(1000.0 / rate)));
      }
    }
  } else {
    for (int i = 0; i < events; ++i) {
      auto ev = engine.simulateTernaryFissionEvent();
      printEvent(ev);
      printProgressBar(static_cast<double>(i + 1) / events);
    }
  }

  std::cout << std::endl;
  printEngineSummary(engine);

  if (!json_file.empty()) {
    dumpStatisticsJSON(engine, json_file);
  }

  (void)logdir; // Placeholder until logging integration
  return 0;
}

// === Helper implementations ===

void printBanner() {
  std::cout << "Ternary Fission Simulation Application" << std::endl;
}

void printHelp() {
  std::cout
      << "Usage: ternary-fission [options]\n"
      << "\nOptions:\n"
      << "  -h, --help               Show this help message\n"
      << "  -n, --events <N>         Number of events to simulate\n"
      << "  -p, --parent <mass>      Parent nucleus mass in AMU\n"
      << "  -e, --excitation <MeV>   Excitation energy in MeV\n"
      << "  -t, --threads <N>        Worker thread count (0=auto)\n"
      << "  -c, --continuous         Continuous mode\n"
      << "  -d, --duration <sec>     Duration for continuous mode\n"
      << "  -r, --rate <N>           Events per second in continuous mode\n"
      << "  -j, --json [file]        Write statistics JSON (default: "
         "simulation_stats.json)\n"
      << "  -x, --repl               Interactive REPL mode\n"
      << "  -l, --logdir <path>      Log directory\n"
      << "      --config <file>      Configuration file path\n"
      << "      --daemon             Run in daemon mode\n"
      << "      --bind-ip <ip>       Override bind IP address\n"
      << "      --bind-port <port>   Override bind port\n";
}

void printEngineSummary(const TernaryFissionSimulationEngine &engine) {
  std::cout << "Simulation Summary:" << std::endl;
  std::cout << "  Total events: " << engine.getTotalEventsSimulated()
            << std::endl;
  std::cout << "  Energy fields: " << engine.getTotalEnergyFieldsCreated()
            << std::endl;
  std::cout << "  Computation time (s): "
            << engine.getTotalComputationTimeSeconds() << std::endl;
}

void printEvent(const TernaryFissionEvent &event) {
  std::cout << "Event " << event.event_id
            << ": TKE=" << event.total_kinetic_energy
            << " MeV, Q=" << event.q_value << " MeV" << std::endl;
}

void dumpStatisticsJSON(const TernaryFissionSimulationEngine &engine,
                        const std::string &filename) {
  Json::Value root;
  root["total_events"] =
      static_cast<Json::UInt64>(engine.getTotalEventsSimulated());
  root["energy_fields"] =
      static_cast<Json::UInt64>(engine.getTotalEnergyFieldsCreated());
  root["computation_time_sec"] = engine.getTotalComputationTimeSeconds();

  std::ofstream out(filename);
  if (!out.is_open()) {
    std::cerr << "Error: Unable to open JSON output file: " << filename
              << std::endl;
    return;
  }
  out << root.toStyledString();
  std::cout << "Statistics written to " << filename << std::endl;
}

void printProgressBar(double progress, size_t width) {
  size_t pos = static_cast<size_t>(progress * width);
  std::cout << '\r' << '[';
  for (size_t i = 0; i < width; ++i) {
    if (i < pos)
      std::cout << '=';
    else if (i == pos)
      std::cout << '>';
    else
      std::cout << ' ';
  }
  std::cout << "] " << static_cast<int>(progress * 100.0) << "%";
  std::cout.flush();
}

void cliREPL(TernaryFissionSimulationEngine &engine) {
  std::cout << "Interactive mode. Type 'help' for commands." << std::endl;
  std::string line;
  while (true) {
    std::cout << "> ";
    if (!std::getline(std::cin, line)) {
      break;
    }
    if (line == "quit" || line == "exit") {
      break;
    } else if (line == "help") {
      std::cout << "Commands: event, summary, quit" << std::endl;
    } else if (line == "event") {
      auto ev = engine.simulateTernaryFissionEvent();
      printEvent(ev);
    } else if (line == "summary") {
      printEngineSummary(engine);
    } else if (!line.empty()) {
      std::cout << "Unknown command: " << line << std::endl;
    }
  }
}

void printDaemonHelp() {
  std::cout << "Daemon Mode Options:\n"
            << "  --daemon             Run application as background daemon\n"
            << "  --config <file>      Daemon configuration file\n"
            << "  --bind-ip <ip>       Override bind IP\n"
            << "  --bind-port <port>   Override bind port\n";
}

void runDaemonMode(const std::string &config_file, const std::string &bind_ip,
                   int bind_port) {
  std::cout << "Starting daemon mode..." << std::endl;
  auto config_manager = std::make_unique<ConfigurationManager>(config_file);
  DaemonTernaryFissionServer daemon(std::move(config_manager));
  if (!daemon.initialize()) {
    std::cerr << "Failed to initialize daemon" << std::endl;
    return;
  }
  if (!daemon.startDaemon()) {
    std::cerr << "Failed to start daemon" << std::endl;
    return;
  }
  daemon.waitForShutdown(std::chrono::seconds::max());
}

void runHTTPServerMode(const std::string &config_file,
                       const std::string &bind_ip, int bind_port) {
  std::cout << "Starting HTTP server mode..." << std::endl;
  auto config_manager = std::make_unique<ConfigurationManager>(config_file);
  HTTPTernaryFissionServer server(std::move(config_manager));
  if (!server.initialize()) {
    std::cerr << "Failed to initialize HTTP server" << std::endl;
    return;
  }
  server.start();
}

bool createDefaultConfigFile(const std::string &config_path) {
  std::ofstream out(config_path);
  if (!out.is_open()) {
    return false;
  }
  out << "# Default daemon configuration\n"
      << "bind_ip=127.0.0.1\n"
      << "bind_port=8333\n";
  return true;
}
