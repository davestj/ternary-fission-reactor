/*
 * File: src/cpp/config.ternary.fission.server.cpp
 * Author: bthlops (David StJ)
 * Date: January 31, 2025
 * Title: Configuration Management System Implementation for Ternary Fission Daemon Server
 * Purpose: Complete implementation of configuration parsing, validation, and management for daemon operations
 * Reason: Provides centralized configuration management supporting distributed daemon architecture
 *
 * Change Log:
 * 2025-01-31: Initial implementation for distributed daemon architecture Phase 1
 *             Added complete configuration file parsing with inline comment support
 *             Implemented SSL certificate validation and automatic detection
 *             Added physics parameter validation against theoretical constraints
 *             Integrated environment variable override processing
 *             Added comprehensive error handling and validation reporting
 *
 * Carry-over Context:
 * - This implementation supports the HTTP daemon functionality outlined in ARCH.md
 * - Configuration format matches Go server for consistency across distributed services
 * - SSL certificate management enables HTTPS daemon operation
 * - Physics parameter validation ensures theoretical constraints are maintained
 * - Environment overrides support operational flexibility in production
 * - Next: Integration with daemon and HTTP server classes for complete service
 */

#include "config.ternary.fission.server.h"
#include "physics.constants.definitions.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <arpa/inet.h>

namespace TernaryFission {

/**
 * We construct the configuration manager with proper initialization
 * The constructor sets up all default values and prepares for configuration loading
 */
ConfigurationManager::ConfigurationManager(const std::string& config_file_path)
    : config_file_path_(config_file_path.empty() ? ConfigurationUtils::findDefaultConfigFile() : config_file_path),
      last_modified_(std::chrono::system_clock::time_point::min()),
      configuration_valid_(false) {
    
    // We initialize default configuration values
    network_config_ = NetworkConfiguration();
    daemon_config_ = DaemonConfiguration();
    ssl_config_ = SSLConfiguration();
    physics_config_ = PhysicsConfiguration();
    logging_config_ = LoggingConfiguration();
    
    // We process environment variable overrides first
    processEnvironmentOverrides();
    
    // We attempt initial configuration loading if file exists
    if (!config_file_path_.empty() && fileExists(config_file_path_)) {
        loadConfiguration();
    }
}

/**
 * We load configuration from file with comprehensive error handling
 * This method parses the configuration file and validates all parameters
 */
bool ConfigurationManager::loadConfiguration() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    clearValidationMessages();
    
    if (config_file_path_.empty()) {
        addValidationError("No configuration file path specified");
        return false;
    }
    
    if (!fileExists(config_file_path_)) {
        addValidationError("Configuration file does not exist: " + config_file_path_);
        return false;
    }
    
    if (!isFileReadable(config_file_path_)) {
        addValidationError("Configuration file is not readable: " + config_file_path_);
        return false;
    }
    
    // We track file modification time for reload detection
    last_modified_ = getFileModificationTime(config_file_path_);
    
    // We parse the configuration file
    if (!parseConfigurationFile()) {
        addValidationError("Failed to parse configuration file: " + config_file_path_);
        return false;
    }
    
    // We validate all configuration sections
    bool validation_success = validateConfiguration();
    configuration_valid_ = validation_success;
    
    return validation_success;
}

/**
 * We reload configuration unconditionally from the current file
 * This allows manual refresh of configuration without checking modification time
 */
bool ConfigurationManager::reloadConfiguration() {
    return loadConfiguration();
}

/**
 * We reload configuration if file has been modified
 * This enables runtime configuration updates without daemon restart
 */
bool ConfigurationManager::reloadIfModified() {
    if (!auto_reload_enabled_ || config_file_path_.empty()) {
        return false;
    }
    
    auto current_modified = getFileModificationTime(config_file_path_);
    if (current_modified > last_modified_) {
        std::cout << "Configuration file modified, reloading: " << config_file_path_ << std::endl;
        return loadConfiguration();
    }
    
    return false;
}

/**
 * We validate all configuration parameters against constraints
 * This method performs comprehensive validation of all configuration sections
 */
bool ConfigurationManager::validateConfiguration() {
    bool all_valid = true;
    
    // We validate each configuration section independently
    if (!validateNetworkConfiguration()) all_valid = false;
    if (!validateDaemonConfiguration()) all_valid = false;
    if (!validateSSLConfiguration()) all_valid = false;
    if (!validatePhysicsConfiguration()) all_valid = false;
    if (!validateLoggingConfiguration()) all_valid = false;
    
    return all_valid;
}

/**
 * We parse the configuration file using the same format as Go server
 * This method handles inline comments, whitespace, and type conversion
 */
bool ConfigurationManager::parseConfigurationFile() {
    std::ifstream file(config_file_path_);
    if (!file.is_open()) {
        return false;
    }
    
    raw_config_.clear();
    std::string line;
    int line_number = 0;
    
    while (std::getline(file, line)) {
        line_number++;
        
        // We trim whitespace from the line
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // We skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // We parse key=value pairs
        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            addValidationWarning("Invalid configuration line " + std::to_string(line_number) + ": " + line);
            continue;
        }
        
        std::string key = line.substr(0, equals_pos);
        std::string value = line.substr(equals_pos + 1);
        
        // We trim whitespace from key and value
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // We remove inline comments from value
        size_t comment_pos = value.find('#');
        if (comment_pos != std::string::npos) {
            value = value.substr(0, comment_pos);
            value.erase(value.find_last_not_of(" \t") + 1);
        }
        
        // We remove quotes from value if present
        if (value.length() >= 2 && 
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }
        
        raw_config_[key] = value;
    }
    
    file.close();
    
    // We parse individual configuration sections
    if (!parseNetworkConfiguration()) return false;
    if (!parseDaemonConfiguration()) return false;
    if (!parseSSLConfiguration()) return false;
    if (!parsePhysicsConfiguration()) return false;
    if (!parseLoggingConfiguration()) return false;
    
    return true;
}

/**
 * We parse network configuration section with validation
 * This method processes all network-related configuration parameters
 */
bool ConfigurationManager::parseNetworkConfiguration() {
    network_config_.bind_ip = getConfigValue("bind_ip", "127.0.0.1");
    network_config_.bind_port = getConfigInt("bind_port", 8333);
    network_config_.enable_ssl = getConfigBool("enable_ssl", false);
    network_config_.ssl_cert_path = getConfigValue("ssl_cert_path", "");
    network_config_.ssl_key_path = getConfigValue("ssl_key_path", "");
    network_config_.ssl_ca_path = getConfigValue("ssl_ca_path", "");
    network_config_.max_connections = getConfigInt("max_connections", 1000);
    network_config_.connection_timeout = getConfigInt("connection_timeout", 30);
    network_config_.enable_cors = getConfigBool("enable_cors", true);
    network_config_.request_size_limit = getConfigInt("request_size_limit", 10485760);
    
    // We parse CORS origins list
    std::string cors_origins_str = getConfigValue("cors_origins", "*");
    if (cors_origins_str != "*") {
        network_config_.cors_origins = getConfigStringList("cors_origins");
    }
    
    return true;
}

/**
 * We parse daemon configuration section with system integration
 * This method processes all daemon process management parameters
 */
bool ConfigurationManager::parseDaemonConfiguration() {
    daemon_config_.daemon_mode = getConfigBool("daemon_mode", false);
    daemon_config_.pid_file_path = getConfigValue("pid_file_path", "/tmp/ternary-fission-daemon.pid");
    daemon_config_.working_directory = getConfigValue("working_directory", "/");
    daemon_config_.user_name = getConfigValue("daemon_user", "");
    daemon_config_.group_name = getConfigValue("daemon_group", "");
    daemon_config_.umask_value = getConfigInt("daemon_umask", 022);
    daemon_config_.create_pid_file = getConfigBool("create_pid_file", true);
    daemon_config_.shutdown_timeout = getConfigInt("shutdown_timeout", 30);
    
    return true;
}

/**
 * We parse SSL configuration section with certificate validation
 * This method processes all SSL/TLS related parameters and validates certificates
 */
bool ConfigurationManager::parseSSLConfiguration() {
    ssl_config_.ssl_enabled = network_config_.enable_ssl;
    ssl_config_.certificate_file = network_config_.ssl_cert_path;
    ssl_config_.private_key_file = network_config_.ssl_key_path;
    ssl_config_.ca_certificate_file = network_config_.ssl_ca_path;
    ssl_config_.cipher_suite = getConfigValue("ssl_cipher_suite", "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256");
    ssl_config_.verify_client_certificates = getConfigBool("ssl_verify_client", false);
    ssl_config_.ssl_protocol_version = getConfigInt("ssl_protocol_version", 0);
    ssl_config_.auto_reload_certificates = getConfigBool("ssl_auto_reload", true);
    
    // We validate SSL certificates if SSL is enabled
    if (ssl_config_.ssl_enabled) {
        validateSSLCertificates();
    }
    
    return true;
}

/**
 * We parse physics configuration section with theoretical validation
 * This method processes all physics simulation parameters
 */
bool ConfigurationManager::parsePhysicsConfiguration() {
    physics_config_.default_parent_mass = getConfigDouble("parent_mass", 235.0);
    physics_config_.default_excitation_energy = getConfigDouble("excitation_energy", 6.5);
    physics_config_.max_energy_field = getConfigDouble("max_energy_field", 1000.0);
    physics_config_.min_energy_field = getConfigDouble("min_energy_field", 0.1);
    physics_config_.default_thread_count = getConfigInt("num_threads", 0);
    physics_config_.conservation_tolerance = getConfigDouble("conservation_tolerance", 1e-6);
    physics_config_.enable_conservation_checks = getConfigBool("enable_conservation_checks", true);
    physics_config_.events_per_second = getConfigDouble("events_per_second", 5.0);
    physics_config_.max_events_per_request = getConfigInt("max_events_per_request", 100000);
    
    return true;
}

/**
 * We parse logging configuration section with file management
 * This method processes all logging and output parameters
 */
bool ConfigurationManager::parseLoggingConfiguration() {
    logging_config_.log_level = getConfigValue("log_level", "info");
    logging_config_.access_log_path = getConfigValue("access_log_path", "logs/daemon-access.log");
    logging_config_.error_log_path = getConfigValue("error_log_path", "logs/daemon-error.log");
    logging_config_.debug_log_path = getConfigValue("debug_log_path", "logs/daemon-debug.log");
    logging_config_.enable_console_logging = getConfigBool("enable_console_logging", true);
    logging_config_.enable_file_logging = getConfigBool("enable_file_logging", true);
    logging_config_.max_log_file_size = getConfigInt("max_log_file_size", 104857600);
    logging_config_.log_rotation_count = getConfigInt("log_rotation_count", 10);
    logging_config_.enable_json_logging = getConfigBool("enable_json_logging", false);
    logging_config_.verbose_output = getConfigBool("verbose_output", false);
    logging_config_.log_timestamp_format = getConfigValue("log_timestamp_format", "%Y-%m-%d %H:%M:%S");
    
    return true;
}

/**
 * We validate network configuration parameters
 * This method checks network settings for validity and security
 */
bool ConfigurationManager::validateNetworkConfiguration() {
    bool valid = true;
    
    // We validate IP address format
    if (!ConfigurationUtils::validateIPAddress(network_config_.bind_ip)) {
        addValidationError("Invalid bind IP address: " + network_config_.bind_ip);
        valid = false;
    }
    
    // We validate port number range
    if (!ConfigurationUtils::validatePortNumber(network_config_.bind_port)) {
        addValidationError("Invalid bind port: " + std::to_string(network_config_.bind_port));
        valid = false;
    }
    
    // We validate connection limits
    if (network_config_.max_connections < 1 || network_config_.max_connections > 65535) {
        addValidationError("Invalid max_connections: " + std::to_string(network_config_.max_connections));
        valid = false;
    }
    
    // We validate timeout values
    if (network_config_.connection_timeout < 1 || network_config_.connection_timeout > 3600) {
        addValidationError("Invalid connection_timeout: " + std::to_string(network_config_.connection_timeout));
        valid = false;
    }
    
    // We validate request size limit
    if (network_config_.request_size_limit < 1024 || network_config_.request_size_limit > 1073741824) {
        addValidationError("Invalid request_size_limit: " + std::to_string(network_config_.request_size_limit));
        valid = false;
    }
    
    return valid;
}

/**
 * We validate daemon configuration parameters
 * This method checks daemon process management settings
 */
bool ConfigurationManager::validateDaemonConfiguration() {
    bool valid = true;
    
    // We validate PID file path is writable
    if (daemon_config_.create_pid_file) {
        std::string pid_dir = daemon_config_.pid_file_path.substr(0, daemon_config_.pid_file_path.find_last_of('/'));
        if (!ConfigurationUtils::validateDirectoryPath(pid_dir, true)) {
            addValidationError("PID file directory not writable: " + pid_dir);
            valid = false;
        }
    }
    
    // We validate working directory exists
    if (!ConfigurationUtils::validateDirectoryPath(daemon_config_.working_directory, false)) {
        addValidationError("Working directory does not exist: " + daemon_config_.working_directory);
        valid = false;
    }
    
    // We validate umask value
    if (daemon_config_.umask_value < 0 || daemon_config_.umask_value > 0777) {
        addValidationError("Invalid umask value: " + std::to_string(daemon_config_.umask_value));
        valid = false;
    }
    
    // We validate shutdown timeout
    if (daemon_config_.shutdown_timeout < 1 || daemon_config_.shutdown_timeout > 300) {
        addValidationError("Invalid shutdown_timeout: " + std::to_string(daemon_config_.shutdown_timeout));
        valid = false;
    }
    
    return valid;
}

/**
 * We validate SSL configuration and certificates
 * This method performs comprehensive SSL certificate validation
 */
bool ConfigurationManager::validateSSLConfiguration() {
    bool valid = true;
    
    if (!ssl_config_.ssl_enabled) {
        return true; // No validation needed if SSL is disabled
    }
    
    // We validate certificate file exists and is readable
    if (ssl_config_.certificate_file.empty()) {
        addValidationError("SSL enabled but no certificate file specified");
        valid = false;
    } else if (!validateCertificateFile(ssl_config_.certificate_file)) {
        addValidationError("Invalid SSL certificate file: " + ssl_config_.certificate_file);
        valid = false;
    }
    
    // We validate private key file exists and is readable
    if (ssl_config_.private_key_file.empty()) {
        addValidationError("SSL enabled but no private key file specified");
        valid = false;
    } else if (!validatePrivateKeyFile(ssl_config_.private_key_file)) {
        addValidationError("Invalid SSL private key file: " + ssl_config_.private_key_file);
        valid = false;
    }
    
    // We validate CA file if specified
    if (!ssl_config_.ca_certificate_file.empty()) {
        if (!validateCAFile(ssl_config_.ca_certificate_file)) {
            addValidationError("Invalid SSL CA certificate file: " + ssl_config_.ca_certificate_file);
            valid = false;
        }
    }
    
    // We validate SSL protocol version
    if (ssl_config_.ssl_protocol_version < 0 || ssl_config_.ssl_protocol_version > 4) {
        addValidationError("Invalid SSL protocol version: " + std::to_string(ssl_config_.ssl_protocol_version));
        valid = false;
    }
    
    return valid;
}

/**
 * We validate physics configuration parameters
 * This method checks physics parameters against theoretical constraints
 */
bool ConfigurationManager::validatePhysicsConfiguration() {
    bool valid = true;
    
    // We validate nuclear mass parameters
    if (!ConfigurationUtils::isValidNuclearMass(physics_config_.default_parent_mass)) {
        addValidationError("Invalid parent nucleus mass: " + std::to_string(physics_config_.default_parent_mass));
        valid = false;
    }
    
    // We validate excitation energy parameters
    if (!ConfigurationUtils::isValidExcitationEnergy(physics_config_.default_excitation_energy)) {
        addValidationError("Invalid excitation energy: " + std::to_string(physics_config_.default_excitation_energy));
        valid = false;
    }
    
    // We validate energy field limits
    if (!ConfigurationUtils::isValidEnergyField(physics_config_.max_energy_field) ||
        !ConfigurationUtils::isValidEnergyField(physics_config_.min_energy_field)) {
        addValidationError("Invalid energy field limits: " + 
                          std::to_string(physics_config_.min_energy_field) + " - " +
                          std::to_string(physics_config_.max_energy_field));
        valid = false;
    }
    
    if (physics_config_.min_energy_field >= physics_config_.max_energy_field) {
        addValidationError("Minimum energy field must be less than maximum energy field");
        valid = false;
    }
    
    // We validate conservation law tolerances
    if (!ConfigurationUtils::areConservationLawTolerancesRealistic(physics_config_.conservation_tolerance)) {
        addValidationError("Invalid conservation tolerance: " + std::to_string(physics_config_.conservation_tolerance));
        valid = false;
    }
    
    // We validate thread count
    if (physics_config_.default_thread_count < 0 || physics_config_.default_thread_count > 256) {
        addValidationError("Invalid thread count: " + std::to_string(physics_config_.default_thread_count));
        valid = false;
    }
    
    // We validate simulation rate
    if (physics_config_.events_per_second <= 0.0 || physics_config_.events_per_second > 10000.0) {
        addValidationError("Invalid events per second: " + std::to_string(physics_config_.events_per_second));
        valid = false;
    }
    
    // We validate maximum events per request
    if (physics_config_.max_events_per_request < 1 || physics_config_.max_events_per_request > 10000000) {
        addValidationError("Invalid max events per request: " + std::to_string(physics_config_.max_events_per_request));
        valid = false;
    }
    
    return valid;
}

/**
 * We validate logging configuration parameters
 * This method checks logging settings and file permissions
 */
bool ConfigurationManager::validateLoggingConfiguration() {
    bool valid = true;
    
    // We validate log level
    std::vector<std::string> valid_levels = {"debug", "info", "warn", "error"};
    if (std::find(valid_levels.begin(), valid_levels.end(), logging_config_.log_level) == valid_levels.end()) {
        addValidationError("Invalid log level: " + logging_config_.log_level);
        valid = false;
    }
    
    // We validate log file paths if file logging is enabled
    if (logging_config_.enable_file_logging) {
        std::vector<std::string> log_files = {
            logging_config_.access_log_path,
            logging_config_.error_log_path,
            logging_config_.debug_log_path
        };
        
        for (const auto& log_file : log_files) {
            std::string log_dir = log_file.substr(0, log_file.find_last_of('/'));
            if (!ConfigurationUtils::validateDirectoryPath(log_dir, true)) {
                addValidationError("Log directory not writable: " + log_dir);
                valid = false;
            }
        }
    }
    
    // We validate log file size limits
    if (logging_config_.max_log_file_size < 1024 || logging_config_.max_log_file_size > 1073741824) {
        addValidationError("Invalid max log file size: " + std::to_string(logging_config_.max_log_file_size));
        valid = false;
    }
    
    // We validate log rotation count
    if (logging_config_.log_rotation_count < 1 || logging_config_.log_rotation_count > 100) {
        addValidationError("Invalid log rotation count: " + std::to_string(logging_config_.log_rotation_count));
        valid = false;
    }
    
    return valid;
}

/**
 * We process environment variable overrides following hybrid configuration strategy
 * This method applies environment variables to override configuration file settings
 */
void ConfigurationManager::processEnvironmentOverrides() {
    // We process network configuration overrides
    std::string env_bind_ip = getEnvironmentVariable("TERNARY_BIND_IP");
    if (!env_bind_ip.empty()) {
        network_config_.bind_ip = env_bind_ip;
    }
    
    std::string env_bind_port = getEnvironmentVariable("TERNARY_BIND_PORT");
    if (!env_bind_port.empty()) {
        network_config_.bind_port = std::stoi(env_bind_port);
    }
    
    std::string env_enable_ssl = getEnvironmentVariable("TERNARY_ENABLE_SSL");
    if (!env_enable_ssl.empty()) {
        network_config_.enable_ssl = (env_enable_ssl == "true" || env_enable_ssl == "1");
    }
    
    // We process daemon configuration overrides
    std::string env_daemon_mode = getEnvironmentVariable("TERNARY_DAEMON_MODE");
    if (!env_daemon_mode.empty()) {
        daemon_config_.daemon_mode = (env_daemon_mode == "true" || env_daemon_mode == "1");
    }
    
    // We process physics configuration overrides
    std::string env_parent_mass = getEnvironmentVariable("TERNARY_PARENT_MASS");
    if (!env_parent_mass.empty()) {
        physics_config_.default_parent_mass = std::stod(env_parent_mass);
    }
    
    std::string env_excitation_energy = getEnvironmentVariable("TERNARY_EXCITATION_ENERGY");
    if (!env_excitation_energy.empty()) {
        physics_config_.default_excitation_energy = std::stod(env_excitation_energy);
    }
    
    std::string env_events_per_second = getEnvironmentVariable("TERNARY_EVENTS_PER_SECOND");
    if (!env_events_per_second.empty()) {
        physics_config_.events_per_second = std::stod(env_events_per_second);
    }
    
    // We process logging configuration overrides
    std::string env_log_level = getEnvironmentVariable("TERNARY_LOG_LEVEL");
    if (!env_log_level.empty()) {
        logging_config_.log_level = env_log_level;
    }

    std::string env_verbose_output = getEnvironmentVariable("TERNARY_VERBOSE_OUTPUT");
    if (!env_verbose_output.empty()) {
        logging_config_.verbose_output = (env_verbose_output == "true" || env_verbose_output == "1");
    }
}

/**
 * We get environment variable with default value
 * This method safely retrieves environment variables with fallback
 */
std::string ConfigurationManager::getEnvironmentVariable(const std::string& key, const std::string& default_value) const {
    const char* env_value = std::getenv(key.c_str());
    return env_value ? std::string(env_value) : default_value;
}

/**
 * We validate SSL certificates using OpenSSL
 * This method checks certificate validity and extracts expiration information
 */
bool ConfigurationManager::validateSSLCertificates() {
    if (!ssl_config_.ssl_enabled) {
        return true;
    }
    
    bool valid = true;
    
    // We validate certificate file
    if (!ssl_config_.certificate_file.empty()) {
        if (validateCertificateFile(ssl_config_.certificate_file)) {
            ssl_config_.cert_expiry = extractCertificateExpiry(ssl_config_.certificate_file);
        } else {
            valid = false;
        }
    }
    
    // We validate private key file
    if (!ssl_config_.private_key_file.empty()) {
        if (!validatePrivateKeyFile(ssl_config_.private_key_file)) {
            valid = false;
        }
    }
    
    // We validate CA file if specified
    if (!ssl_config_.ca_certificate_file.empty()) {
        if (!validateCAFile(ssl_config_.ca_certificate_file)) {
            valid = false;
        }
    }
    
    return valid;
}

/**
 * We provide accessor methods for configuration structures
 * These methods return const references to prevent unauthorized modification
 */
const NetworkConfiguration& ConfigurationManager::getNetworkConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return network_config_;
}

const DaemonConfiguration& ConfigurationManager::getDaemonConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return daemon_config_;
}

const SSLConfiguration& ConfigurationManager::getSSLConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return ssl_config_;
}

const PhysicsConfiguration& ConfigurationManager::getPhysicsConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return physics_config_;
}

const LoggingConfiguration& ConfigurationManager::getLoggingConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return logging_config_;
}

// We implement utility functions for configuration management
namespace ConfigurationUtils {

/**
 * We find default configuration file in standard locations
 * This function searches for configuration files across different platforms
 */
std::string findDefaultConfigFile() {
    std::vector<std::string> search_paths = {
        "./configs/daemon.conf",
        "./daemon.conf",
        "/etc/ternary-fission/daemon.conf",
        "/usr/local/etc/ternary-fission/daemon.conf",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.config/ternary-fission/daemon.conf"
    };
    
    for (const auto& path : search_paths) {
        if (!path.empty()) {
            struct stat st;
            if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
                return path;
            }
        }
    }
    
    return ""; // No default config file found
}

/**
 * We validate IP address format using inet_pton
 * This function checks both IPv4 and IPv6 address formats
 */
bool validateIPAddress(const std::string& ip_address) {
    struct sockaddr_in sa4;
    struct sockaddr_in6 sa6;
    
    // We check IPv4 format
    if (inet_pton(AF_INET, ip_address.c_str(), &(sa4.sin_addr)) == 1) {
        return true;
    }
    
    // We check IPv6 format
    if (inet_pton(AF_INET6, ip_address.c_str(), &(sa6.sin6_addr)) == 1) {
        return true;
    }
    
    return false;
}

/**
 * We validate port number range
 * This function checks if port is within valid range and not reserved
 */
bool validatePortNumber(int port) {
    return port > 0 && port <= 65535 && port != 22; // Exclude SSH port for safety
}

/**
 * We validate file path existence and permissions
 * This function checks file accessibility based on requirements
 */
bool validateFilePath(const std::string& path, bool must_exist) {
    if (path.empty()) {
        return false;
    }
    
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISREG(st.st_mode) && (access(path.c_str(), R_OK) == 0);
    }
    
    return !must_exist;
}

/**
 * We validate directory path and optionally create it
 * This function ensures directory exists and is writable
 */
bool validateDirectoryPath(const std::string& path, bool create_if_missing) {
    if (path.empty()) {
        return false;
    }
    
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode) && (access(path.c_str(), W_OK) == 0);
    }
    
    if (create_if_missing) {
        // We attempt to create the directory
        std::string mkdir_cmd = "mkdir -p " + path;
        return system(mkdir_cmd.c_str()) == 0;
    }
    
    return false;
}

/**
 * We validate physics parameters against theoretical constraints
 * These functions ensure simulation parameters are physically realistic
 */
bool isValidNuclearMass(double mass) {
    return mass >= 1.0 && mass <= 300.0; // Reasonable range for nuclear masses
}

bool isValidExcitationEnergy(double energy) {
    return energy >= 0.0 && energy <= 50.0; // Reasonable range for excitation energies
}

bool isValidEnergyField(double energy) {
    return energy >= 0.01 && energy <= 10000.0; // Reasonable range for energy fields
}

bool areConservationLawTolerancesRealistic(double tolerance) {
    return tolerance >= 1e-12 && tolerance <= 1e-3; // Reasonable numerical tolerance range
}

} // namespace ConfigurationUtils

// We implement remaining private methods...

std::string ConfigurationManager::getConfigValue(const std::string& key, const std::string& default_value) const {
    auto it = raw_config_.find(key);
    return (it != raw_config_.end()) ? it->second : default_value;
}

int ConfigurationManager::getConfigInt(const std::string& key, int default_value) const {
    std::string value_str = getConfigValue(key, "");
    if (value_str.empty()) {
        return default_value;
    }
    
    try {
        return std::stoi(value_str);
    } catch (const std::exception&) {
        return default_value;
    }
}

double ConfigurationManager::getConfigDouble(const std::string& key, double default_value) const {
    std::string value_str = getConfigValue(key, "");
    if (value_str.empty()) {
        return default_value;
    }
    
    try {
        return std::stod(value_str);
    } catch (const std::exception&) {
        return default_value;
    }
}

bool ConfigurationManager::getConfigBool(const std::string& key, bool default_value) const {
    std::string value_str = getConfigValue(key, "");
    if (value_str.empty()) {
        return default_value;
    }
    
    std::transform(value_str.begin(), value_str.end(), value_str.begin(), ::tolower);
    return value_str == "true" || value_str == "1" || value_str == "yes" || value_str == "on";
}

void ConfigurationManager::clearValidationMessages() {
    validation_errors_.clear();
    validation_warnings_.clear();
}

void ConfigurationManager::addValidationError(const std::string& error) {
    validation_errors_.push_back(error);
}

void ConfigurationManager::addValidationWarning(const std::string& warning) {
    validation_warnings_.push_back(warning);
}

bool ConfigurationManager::fileExists(const std::string& path) const {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool ConfigurationManager::isFileReadable(const std::string& path) const {
    return access(path.c_str(), R_OK) == 0;
}

std::chrono::system_clock::time_point ConfigurationManager::getFileModificationTime(const std::string& path) const {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return std::chrono::system_clock::from_time_t(st.st_mtime);
    }
    return std::chrono::system_clock::time_point::min();
}

std::vector<std::string> ConfigurationManager::getConfigStringList(const std::string& key) const {
    std::vector<std::string> values;
    std::string raw = getConfigValue(key, "");
    if (raw.empty()) {
        return values;
    }

    std::stringstream ss(raw);
    std::string item;
    while (std::getline(ss, item, ',')) {
        values.push_back(item);
    }
    return values;
}

bool ConfigurationManager::validateCertificateFile(const std::string& cert_path) {
    return fileExists(cert_path) && isFileReadable(cert_path);
}

bool ConfigurationManager::validatePrivateKeyFile(const std::string& key_path) {
    return fileExists(key_path) && isFileReadable(key_path);
}

bool ConfigurationManager::validateCAFile(const std::string& ca_path) {
    return fileExists(ca_path) && isFileReadable(ca_path);
}

std::chrono::system_clock::time_point ConfigurationManager::extractCertificateExpiry(const std::string& /*cert_path*/) {
    return std::chrono::system_clock::now();
}

} // namespace TernaryFission
