/*
 * File: include/daemon.ternary.fission.server.h
 * Author: bthlops (David StJ)
 * Date: January 31, 2025
 * Title: Daemon Process Management System for Ternary Fission Server Operations
 * Purpose: Complete Unix daemon implementation with systemd integration and process lifecycle management
 * Reason: Provides production-ready daemon functionality for distributed physics simulation architecture
 *
 * Change Log:
 * 2025-01-31: Initial implementation for distributed daemon architecture Phase 1
 *             Added complete Unix daemon process management with double-forking
 *             Implemented PID file management and signal handling for graceful shutdown
 *             Added systemd integration support with proper service lifecycle
 *             Integrated log file management with rotation and monitoring
 *             Added platform-specific process management for macOS, Ubuntu, and Debian
 *             Implemented file descriptor cleanup and security hardening
 *
 * Carry-over Context:
 * - This class implements Unix daemon conventions for background process operation
 * - PID file management enables systemd integration and process monitoring
 * - Signal handling provides graceful shutdown with configurable timeout
 * - Log management supports rotation and multiple output destinations
 * - Security features include user/group switching and umask configuration
 * - Platform compatibility supports macOS, Ubuntu 24, and Debian 12
 * - Next: Integration with HTTP server and configuration classes for complete service
 */

#ifndef DAEMON_TERNARY_FISSION_SERVER_H
#define DAEMON_TERNARY_FISSION_SERVER_H

#include "config.ternary.fission.server.h"
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <vector>
#include <map>
#include <csignal>
#include <sys/types.h>

namespace TernaryFission {

/**
 * We define daemon status enumeration for lifecycle tracking
 * This enumeration tracks daemon operational state through all phases
 */
enum class DaemonStatus {
    STOPPED,            // Daemon is not running
    STARTING,           // Daemon initialization in progress
    RUNNING,            // Daemon operating normally
    STOPPING,           // Graceful shutdown in progress
    ERROR,              // Daemon encountered fatal error
    RESTARTING          // Daemon restart in progress
};

/**
 * We define signal handler information structure
 * This structure tracks registered signal handlers for proper cleanup
 */
struct SignalHandlerInfo {
    int signal_number;                          // Signal number (SIGTERM, SIGINT, etc.)
    std::function<void(int)> handler_function;  // Signal handler callback function
    struct sigaction original_action;           // Original signal action for restoration
    bool handler_installed = false;             // Handler installation status
};

/**
 * We define daemon statistics structure for monitoring
 * This structure provides comprehensive daemon health and performance data
 */
struct DaemonStatistics {
    std::chrono::system_clock::time_point start_time;    // Daemon start timestamp
    std::atomic<uint64_t> total_requests{0};             // Total requests processed
    std::atomic<uint64_t> successful_operations{0};      // Successful operations count
    std::atomic<uint64_t> error_count{0};                // Error occurrence count
    std::atomic<uint64_t> signal_count{0};               // Signals received count
    std::atomic<double> cpu_usage_percent{0.0};          // Current CPU usage
    std::atomic<uint64_t> memory_usage_bytes{0};         // Current memory usage
    std::atomic<uint64_t> file_descriptors_open{0};      // Open file descriptors
    std::map<int, uint64_t> signal_statistics;           // Per-signal occurrence counts
    std::mutex statistics_mutex;                         // Statistics synchronization
    
    // We provide methods for statistics updates
    void incrementRequests();
    void incrementSuccessful();
    void incrementErrors();
    void incrementSignals(int signal_num);
    void updateResourceUsage();
};

/**
 * We define process information structure for system integration
 * This structure contains essential process identification and control data
 */
struct ProcessInfo {
    pid_t daemon_pid = -1;                      // Main daemon process ID
    pid_t parent_pid = -1;                      // Parent process ID before daemonization
    pid_t session_id = -1;                      // Session ID after setsid()
    pid_t process_group_id = -1;                // Process group ID
    std::string pid_file_path;                  // PID file absolute path
    std::string working_directory;              // Daemon working directory
    uid_t daemon_uid = -1;                      // Daemon user ID
    gid_t daemon_gid = -1;                      // Daemon group ID
    mode_t file_creation_mask = 022;            // File creation umask
    bool pid_file_created = false;              // PID file creation status
};

/**
 * We define the main daemon management class for ternary fission server operations
 * This class provides complete Unix daemon functionality with systemd integration
 */
class DaemonTernaryFissionServer {
private:
    std::unique_ptr<ConfigurationManager> config_manager_;     // Configuration management
    std::shared_ptr<DaemonStatistics> statistics_;            // Daemon performance statistics
    std::unique_ptr<ProcessInfo> process_info_;               // Process identification info
    
    // We maintain daemon operational state
    std::atomic<DaemonStatus> daemon_status_;   // Current daemon operational status
    std::atomic<bool> shutdown_requested_;      // Graceful shutdown request flag
    std::atomic<bool> restart_requested_;       // Daemon restart request flag
    std::atomic<bool> debug_mode_;              // Verbose diagnostics flag
    std::chrono::system_clock::time_point start_time_; // Daemon startup timestamp
    
    // We manage signal handling system
    std::map<int, std::unique_ptr<SignalHandlerInfo>> signal_handlers_; // Registered signal handlers
    std::mutex signal_mutex_;                   // Signal handler synchronization
    static DaemonTernaryFissionServer* instance_; // Static instance for signal handlers
    
    // We handle log file management
    std::string access_log_path_;               // Access log file path
    std::string error_log_path_;                // Error log file path
    std::string debug_log_path_;                // Debug log file path
    std::atomic<bool> log_rotation_enabled_;    // Log rotation enablement flag
    std::thread log_rotation_thread_;           // Log rotation worker thread
    std::atomic<bool> log_rotation_active_;     // Log rotation thread control
    
    // We manage resource monitoring
    std::thread resource_monitor_thread_;       // Resource monitoring worker
    std::atomic<bool> resource_monitoring_;     // Resource monitoring control
    std::chrono::seconds monitoring_interval_; // Resource monitoring frequency
    
    // We implement daemon process management methods
    bool performDoubleFork();                   // Execute Unix double-fork process
    bool createSession();                       // Create new session with setsid()
    bool changeWorkingDirectory();              // Change to daemon working directory
    bool switchUserAndGroup();                  // Switch to configured user/group
    bool setFileCreationMask();                 // Set daemon file creation umask
    bool redirectStandardStreams();             // Redirect stdin/stdout/stderr
    bool closeInheritedFileDescriptors();       // Close inherited file descriptors
    
    // We implement PID file management methods
    bool createPIDFile();                       // Create and lock PID file
    bool removePIDFile();                       // Remove PID file on shutdown
    bool validatePIDFile();                     // Validate existing PID file
    pid_t readPIDFromFile() const;              // Read PID from existing file
    bool lockPIDFile(int fd);                   // Lock PID file exclusively
    
    // We implement signal handling methods
    void installSignalHandlers();               // Install all required signal handlers
    void removeSignalHandlers();                // Remove installed signal handlers
    static void signalHandlerWrapper(int sig); // Static signal handler wrapper
    void handleTerminationSignal(int sig);     // Handle SIGTERM/SIGINT gracefully
    void handleReloadSignal(int sig);           // Handle SIGHUP configuration reload
    void handleInfoSignal(int sig);             // Handle SIGUSR1/SIGUSR2 status info
    
    // We implement log management methods
    bool initializeLogFiles();                  // Initialize all log file streams
    void rotateLogFiles();                      // Rotate log files based on size/age
    void cleanupOldLogFiles();                  // Remove old rotated log files
    void logRotationWorker();                   // Log rotation background worker
    bool createLogDirectory(const std::string& log_path); // Create log directory structure
    
    // We implement resource monitoring methods
    void resourceMonitorWorker();               // Resource monitoring background worker
    void collectSystemMetrics();                // Collect CPU/memory/FD statistics
    uint64_t getCurrentMemoryUsage();          // Get current process memory usage
    double getCurrentCPUUsage();               // Get current process CPU usage
    uint64_t getOpenFileDescriptorCount();     // Count open file descriptors
    
    // We implement status reporting methods
    void updateDaemonStatus(DaemonStatus new_status); // Update daemon status atomically
    std::string getStatusString() const;        // Get human-readable status string
    static std::string statusToString(DaemonStatus status); // Convert status enum to string
    void logStatusChange(DaemonStatus old_status, DaemonStatus new_status); // Log status changes
    
    // We implement configuration validation methods
    bool validateDaemonConfiguration();        // Validate daemon-specific configuration
    bool checkRequiredPermissions();           // Check filesystem and system permissions
    bool validateLogPaths();                   // Validate log file paths and permissions
    
public:
    /**
     * We construct the daemon manager with configuration
     * The constructor initializes all daemon components and prepares for operation
     */
    explicit DaemonTernaryFissionServer(std::unique_ptr<ConfigurationManager> config_manager);
    
    /**
     * We destruct the daemon manager with proper cleanup
     * The destructor ensures all resources are properly released
     */
    virtual ~DaemonTernaryFissionServer();
    
    /**
     * We initialize the daemon with configuration validation
     * This method prepares all daemon components for background operation
     */
    bool initialize();
    
    /**
     * We start the daemon process with full daemonization
     * This method performs complete Unix daemon initialization
     */
    bool startDaemon();
    
    /**
     * We stop the daemon process with graceful shutdown
     * This method ensures proper cleanup of all daemon resources
     */
    void stopDaemon();
    
    /**
     * We restart the daemon process with configuration reload
     * This method performs graceful restart without service interruption
     */
    bool restartDaemon();
    
    /**
     * We check if the daemon is currently running
     * This method returns the current operational status
     */
    bool isRunning() const;
    
    /**
     * We get the current daemon status
     * This method returns detailed operational status information
     */
    DaemonStatus getStatus() const;
    
    /**
     * We get the daemon process ID
     * This method returns the current daemon PID or -1 if not running
     */
    pid_t getDaemonPID() const;
    
    /**
     * We register custom signal handler for daemon control
     * This method allows registration of application-specific signal handlers
     */
    bool registerSignalHandler(int signal_num, std::function<void(int)> handler);
    
    /**
     * We unregister custom signal handler
     * This method removes previously registered signal handlers
     */
    bool unregisterSignalHandler(int signal_num);
    
    /**
     * We reload daemon configuration without restart
     * This method updates daemon configuration while maintaining operation
     */
    bool reloadConfiguration();
    
    /**
     * We get daemon performance statistics
     * This method returns comprehensive daemon health and performance data
     */
    std::shared_ptr<DaemonStatistics> getStatistics() const;
    
    /**
     * We get daemon process information
     * This method returns complete process identification and control data
     */
    ProcessInfo getProcessInfo() const;
    
    /**
     * We check if another daemon instance is already running
     * This method prevents multiple daemon instances from running simultaneously
     */
    bool isAnotherInstanceRunning() const;
    
    /**
     * We send signal to running daemon instance
     * This method allows external control of running daemon processes
     */
    bool sendSignalToDaemon(int signal_num) const;
    
    /**
     * We wait for daemon shutdown with timeout
     * This method blocks until daemon shutdown completes or timeout expires
     */
    bool waitForShutdown(std::chrono::seconds timeout = std::chrono::seconds(30)) const;
    
    /**
     * We validate daemon configuration parameters
     * This method checks all daemon-specific configuration for validity
     */
    bool validateConfiguration() const;
    
    /**
     * We get daemon uptime duration
     * This method returns how long the daemon has been running
     */
    std::chrono::seconds getUptime() const;
    
    /**
     * We enable or disable debug mode for daemon operation
     * This method controls verbose logging and diagnostic output
     */
    void setDebugMode(bool enable_debug);
    
    /**
     * We get daemon working directory path
     * This method returns the current daemon working directory
     */
    std::string getWorkingDirectory() const;
    
    /**
     * We get daemon log file paths
     * This method returns all configured log file paths
     */
    std::vector<std::string> getLogFilePaths() const;
    
    /**
     * We force log rotation for all log files
     * This method immediately rotates all daemon log files
     */
    void forceLogRotation();
    
    /**
     * We get daemon resource usage summary
     * This method returns current CPU, memory, and file descriptor usage
     */
    std::map<std::string, double> getResourceUsage() const;
};

} // namespace TernaryFission

#endif // DAEMON_TERNARY_FISSION_SERVER_H
