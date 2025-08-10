/*
 * File: include/config.ternary.fission.server.h
 * Author: bthlops (David StJ)
 * Date: January 31, 2025
 * Title: Configuration Management System for Ternary Fission Daemon Server
 * Purpose: Handles configuration file parsing, validation, and runtime management for daemon operations
 * Reason: Provides centralized configuration management supporting HTTP daemon mode and SSL/TLS operations
 *
 * Change Log:
 * 2025-01-31: Initial implementation for distributed daemon architecture Phase 1
 *             Added complete configuration parsing for daemon mode operations
 *             Implemented SSL/TLS certificate management and validation
 *             Added network configuration options for HTTP server binding
 *             Integrated physics parameter validation with theoretical constraints
 *             Added platform-specific path handling for certificate management
 *
 * Carry-over Context:
 * - This class supports the distributed daemon architecture outlined in ARCH.md
 * - Configuration format matches Go server format for consistency across services
 * - SSL certificate paths are validated and certificates are checked for validity
 * - Physics parameters are validated against theoretical constraints
 * - Environment variable overrides follow hybrid configuration strategy
 * - Next: Integration with daemon and HTTP server classes for complete service
 */
#ifndef CONFIG_TERNARY_FISSION_SERVER_H
#define CONFIG_TERNARY_FISSION_SERVER_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <fstream>

namespace TernaryFission {

/**
 * We define configuration categories for organized parameter management
 * Each category groups related configuration parameters for validation and access
 */
enum class ConfigCategory {
    DAEMON_CONFIG,      // Daemon process management settings
    NETWORK_CONFIG,     // HTTP server and network binding settings
    SSL_CONFIG,         // SSL/TLS certificate and encryption settings
    PHYSICS_CONFIG,     // Physics simulation and mathematical parameters
    LOGGING_CONFIG,     // Log file management and output settings
    PERFORMANCE_CONFIG  // Threading and resource allocation settings
};

/**
 * We define network configuration structure for HTTP server operations
 * This structure contains all parameters needed for daemon network operations
 */
struct NetworkConfiguration {
    std::string bind_ip = "127.0.0.1";          // Network interface to bind
    int bind_port = 8333;                       // HTTP/HTTPS port number
    bool enable_ssl = false;                    // SSL/TLS enablement flag
    std::string ssl_cert_path;                  // Path to SSL certificate file
    std::string ssl_key_path;                   // Path to SSL private key file
    std::string ssl_ca_path;                    // Path to SSL CA certificate or chain
    int max_connections = 1000;                 // Maximum concurrent connections
    int connection_timeout = 30;                // Connection timeout in seconds
    bool enable_cors = true;                    // Cross-Origin Resource Sharing
    std::vector<std::string> cors_origins;      // Allowed CORS origins
    int request_size_limit = 10485760;          // Maximum request size (10MB)
    std::string web_root;                       // Filesystem path for static assets

    NetworkConfiguration() {
        cors_origins = {"*"};  // Default to allow all origins
    }
};

/**
 * We define daemon configuration structure for process management
 * This structure controls daemon lifecycle and system integration
 */
struct DaemonConfiguration {
    bool daemon_mode = false;                   // Enable daemon background operation
    std::string pid_file_path = "/tmp/ternary-fission-daemon.pid";
    std::string working_directory = "/";        // Daemon working directory
    std::string user_name;                      // User to run daemon as
    std::string group_name;                     // Group to run daemon as
    int umask_value = 022;                      // File creation mask
    bool create_pid_file = true;                // Create PID file for process tracking
    int shutdown_timeout = 30;                  // Graceful shutdown timeout seconds
    std::vector<std::string> signal_handlers;   // Custom signal handler configuration
};

/**
 * We define SSL configuration structure for certificate management
 * This structure handles all SSL/TLS certificate validation and loading
 */
struct SSLConfiguration {
    bool ssl_enabled = false;                   // Master SSL enable flag
    std::string certificate_file;               // SSL certificate file path
    std::string private_key_file;               // SSL private key file path
    std::string ca_certificate_file;            // CA certificate or chain file
    std::string cipher_suite;                   // SSL cipher suite specification
    bool verify_client_certificates = false;    // Client certificate verification
    int ssl_protocol_version = 0;              // SSL/TLS protocol version (0=auto)
    std::chrono::system_clock::time_point cert_expiry; // Certificate expiry tracking
    bool auto_reload_certificates = true;       // Automatically reload on change
};

/**
 * We define physics configuration structure for simulation parameters
 * This structure validates physics parameters against theoretical constraints
 */
struct PhysicsConfiguration {
    double default_parent_mass = 235.0;         // Default nuclear mass (AMU)
    double default_excitation_energy = 6.5;     // Default excitation energy (MeV)
    double max_energy_field = 1000.0;          // Maximum energy field (MeV)
    double min_energy_field = 0.1;             // Minimum energy field (MeV)
    int default_thread_count = 0;              // Physics calculation threads (0=auto)
    double conservation_tolerance = 1e-6;       // Conservation law tolerance
    bool enable_conservation_checks = true;     // Enable conservation verification
    double events_per_second = 5.0;            // Default simulation rate
    int max_events_per_request = 100000;       // Maximum events in single request
};

/**
 * We define logging configuration structure for output management
 * This structure controls all logging behavior and file management
 */
struct LoggingConfiguration {
    std::string log_level = "info";            // Logging verbosity level
    std::string access_log_path = "logs/daemon-access.log";
    std::string error_log_path = "logs/daemon-error.log";
    std::string debug_log_path = "logs/daemon-debug.log";
    bool enable_console_logging = true;        // Enable console output
    bool enable_file_logging = true;           // Enable file output
    int max_log_file_size = 104857600;         // Maximum log file size (100MB)
    int log_rotation_count = 10;               // Number of rotated log files to keep
    bool enable_json_logging = false;          // Enable structured JSON logging
    bool verbose_output = false;               // Enable verbose logging output
    std::string log_timestamp_format = "%Y-%m-%d %H:%M:%S"; // Log timestamp format
};

/**
 * We define media streaming configuration structure for external audio/video feeds
 * This structure controls streaming tool invocation and media file locations
 */
struct MediaStreamingConfiguration {
    bool media_streaming_enabled = false;      // Enable media streaming subsystem
    std::string media_root;                    // Root directory for media files
    std::string icecast_mount;                 // Target Icecast mount point
};

/**
 * We define the main configuration manager class for centralized parameter management
 * This class handles loading, parsing, validation, and runtime updates of all configuration
 */
class ConfigurationManager {
private:
    std::string config_file_path_;              // Path to configuration file
    std::map<std::string, std::string> raw_config_; // Raw configuration key-value pairs
    std::chrono::system_clock::time_point last_modified_; // File modification time
    mutable std::mutex config_mutex_;           // Thread-safe configuration access
    bool auto_reload_enabled_ = false;          // Automatic configuration reloading
    
    // We store parsed configuration structures
    NetworkConfiguration network_config_;
    DaemonConfiguration daemon_config_;
    SSLConfiguration ssl_config_;
    PhysicsConfiguration physics_config_;
    LoggingConfiguration logging_config_;
    MediaStreamingConfiguration media_streaming_config_;
    
    // We track configuration validation status
    bool configuration_valid_ = false;
    std::vector<std::string> validation_errors_;
    std::vector<std::string> validation_warnings_;

public:
    /**
     * We construct configuration manager with specified config file path
     * The constructor validates the config file exists and is readable
     */
    explicit ConfigurationManager(const std::string& config_file_path = "");
    
    /**
     * We provide destructor for proper cleanup of resources
     */
    ~ConfigurationManager() = default;
    
    // We prevent copying to ensure single configuration instance
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;
    
    /**
     * We load configuration from file with comprehensive error handling
     * This method parses the configuration file and validates all parameters
     */
    bool loadConfiguration();
    
    /**
     * We reload configuration unconditionally from the current file
     * This allows manual refresh of configuration without checking modification time
     */
    bool reloadConfiguration();

    /**
     * We reload configuration if file has been modified
     * This enables runtime configuration updates without daemon restart
     */
    bool reloadIfModified();
    
    /**
     * We validate all configuration parameters against constraints
     * This method checks physics parameters, network settings, and SSL certificates
     */
    bool validateConfiguration();
    
    /**
     * We provide access to configuration structures for different subsystems
     * These methods return const references to prevent unauthorized modification
     */
    const NetworkConfiguration& getNetworkConfig() const;
    const DaemonConfiguration& getDaemonConfig() const;
    const SSLConfiguration& getSSLConfig() const;
    const PhysicsConfiguration& getPhysicsConfig() const;
    const LoggingConfiguration& getLoggingConfig() const;
    const MediaStreamingConfiguration& getMediaStreamingConfig() const;
    
    /**
     * We provide methods to update specific configuration categories
     * These methods validate changes before applying them
     */
    bool updateNetworkConfig(const NetworkConfiguration& new_config);
    bool updatePhysicsConfig(const PhysicsConfiguration& new_config);
    bool updateLoggingConfig(const LoggingConfiguration& new_config);
    
    /**
     * We provide configuration status and diagnostic information
     * These methods help with debugging and monitoring configuration health
     */
    bool isConfigurationValid() const { return configuration_valid_; }
    const std::vector<std::string>& getValidationErrors() const { return validation_errors_; }
    const std::vector<std::string>& getValidationWarnings() const { return validation_warnings_; }
    
    /**
     * We provide methods for environment variable override processing
     * This supports the hybrid configuration strategy outlined in architecture
     */
    void processEnvironmentOverrides();
    std::string getEnvironmentVariable(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * We provide configuration serialization for debugging and API responses
     * These methods generate JSON representations of configuration state
     */
    std::string toJSON() const;
    std::string getConfigurationSummary() const;
    
    /**
     * We provide SSL certificate validation and management
     * These methods check certificate validity and handle automatic reloading
     */
    bool validateSSLCertificates();
    bool areSSLCertificatesValid() const;
    std::chrono::system_clock::time_point getCertificateExpiry() const;
    
    /**
     * We provide configuration file monitoring for automatic reloading
     * This enables runtime configuration updates in production environments
     */
    void enableAutoReload(bool enable = true) { auto_reload_enabled_ = enable; }
    bool isAutoReloadEnabled() const { return auto_reload_enabled_; }

private:
    /**
     * We implement internal methods for configuration processing
     * These methods handle the detailed parsing and validation logic
     */
    bool parseConfigurationFile();
    bool parseNetworkConfiguration();
    bool parseDaemonConfiguration();
    bool parseSSLConfiguration();
    bool parsePhysicsConfiguration();
    bool parseLoggingConfiguration();
    bool parseMediaStreamingConfiguration();
    
    /**
     * We implement configuration value parsing and type conversion
     * These methods handle string-to-type conversion with error checking
     */
    std::string getConfigValue(const std::string& key, const std::string& default_value = "") const;
    int getConfigInt(const std::string& key, int default_value = 0) const;
    double getConfigDouble(const std::string& key, double default_value = 0.0) const;
    bool getConfigBool(const std::string& key, bool default_value = false) const;
    std::vector<std::string> getConfigStringList(const std::string& key) const;
    
    /**
     * We implement validation methods for different configuration categories
     * These methods check parameter ranges and theoretical constraints
     */
    bool validateNetworkConfiguration();
    bool validateDaemonConfiguration();
    bool validateSSLConfiguration();
    bool validatePhysicsConfiguration();
    bool validateLoggingConfiguration();
    bool validateMediaStreamingConfiguration();
    
    /**
     * We implement file system utility methods for configuration management
     * These methods handle file existence, permissions, and modification tracking
     */
    bool fileExists(const std::string& path) const;
    bool isFileReadable(const std::string& path) const;
    std::chrono::system_clock::time_point getFileModificationTime(const std::string& path) const;
    bool createDirectoryIfNeeded(const std::string& path) const;
    
    /**
     * We implement SSL certificate utility methods
     * These methods validate certificate files and extract expiration information
     */
    bool validateCertificateFile(const std::string& cert_path);
    bool validatePrivateKeyFile(const std::string& key_path);
    bool validateCAFile(const std::string& ca_path);
    std::chrono::system_clock::time_point extractCertificateExpiry(const std::string& cert_path);
    
    /**
     * We implement physics parameter validation against theoretical constraints
     * These methods ensure simulation parameters are within realistic ranges
     */
    bool validatePhysicsParameterRanges();
    bool validateConservationLawSettings();
    bool validateEnergyFieldLimits();
    
    /**
     * We implement error and warning management for configuration diagnostics
     * These methods track validation issues for debugging and monitoring
     */
    void clearValidationMessages();
    void addValidationError(const std::string& error);
    void addValidationWarning(const std::string& warning);
};

/**
 * We provide utility functions for configuration management across the application
 * These functions support common configuration operations and debugging
 */
namespace ConfigurationUtils {
    /**
     * We provide default configuration file path detection
     * This function searches standard locations for configuration files
     */
    std::string findDefaultConfigFile();
    
    /**
     * We provide configuration template generation
     * This function creates example configuration files with documentation
     */
    bool generateConfigurationTemplate(const std::string& output_path);
    
    /**
     * We provide configuration validation utilities
     * These functions validate specific configuration aspects independently
     */
    bool validateIPAddress(const std::string& ip_address);
    bool validatePortNumber(int port);
    bool validateFilePath(const std::string& path, bool must_exist = true);
    bool validateDirectoryPath(const std::string& path, bool create_if_missing = false);
    
    /**
     * We provide SSL certificate utility functions
     * These functions handle certificate operations across different platforms
     */
    bool isCertificateFileValid(const std::string& cert_path);
    bool isPrivateKeyFileValid(const std::string& key_path);
    std::string generateSelfSignedCertificate(const std::string& hostname);
    
    /**
     * We provide physics parameter validation utilities
     * These functions check parameters against theoretical physics constraints
     */
    bool isValidNuclearMass(double mass);
    bool isValidExcitationEnergy(double energy);
    bool isValidEnergyField(double energy);
    bool areConservationLawTolerancesRealistic(double tolerance);
}


} // namespace TernaryFission

#endif // CONFIG_TERNARY_FISSION_SERVER_H
