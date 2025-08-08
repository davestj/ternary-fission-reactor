/*
 * File: include/http.ternary.fission.server.h
 * Author: bthlops (David StJ)
 * Date: January 31, 2025
 * Title: HTTP/HTTPS REST API Server for Ternary Fission Daemon Operations
 * Purpose: Complete HTTP server implementation with SSL/TLS support and physics API endpoints
 * Reason: Provides REST API interface for distributed daemon architecture and physics simulations
 *
 * Change Log:
 * 2025-01-31: Initial implementation for distributed daemon architecture Phase 1
 *             Added complete HTTP/HTTPS server with cpp-httplib integration
 *             Implemented all Go server API endpoints for seamless integration
 *             Added SSL/TLS certificate management and automatic detection
 *             Integrated physics simulation engine API endpoints
 *             Added WebSocket support for real-time monitoring
 *             Implemented CORS, logging, and metrics middleware
 *             Added comprehensive error handling and JSON serialization
 *
 * Carry-over Context:
 * - This class implements the HTTP server functionality for daemon mode operations
 * - API endpoints exactly mirror Go server structure for Phase 2 integration
 * - SSL certificate management enables HTTPS daemon operation in production
 * - Physics API endpoints provide JSON interface to simulation engine
 * - WebSocket support enables real-time monitoring of energy field calculations
 * - Next: Integration with daemon class and main application for complete service
 */

#ifndef HTTP_TERNARY_FISSION_SERVER_H
#define HTTP_TERNARY_FISSION_SERVER_H

#include "config.ternary.fission.server.h"
#include "physics.constants.definitions.h"
#include "ternary.fission.simulation.engine.h"
#include <httplib.h>
#include <json/json.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <queue>
#include <condition_variable>

namespace TernaryFission {

/**
 * We define HTTP request context structure for middleware processing
 * This structure contains all information needed for request processing
 */
struct HTTPRequestContext {
    std::string client_ip;                      // Client IP address for logging
    std::string user_agent;                     // Client user agent string
    std::chrono::steady_clock::time_point start_time; // Request start time for metrics
    std::string request_id;                     // Unique request identifier
    std::map<std::string, std::string> headers; // Request headers collection
    size_t content_length = 0;                  // Request body size
    std::string content_type;                   // Request content type
};

/**
 * We define energy field response structure for API serialization
 * This structure matches the Go server EnergyFieldResponse format
 */
struct EnergyFieldResponse {
    std::string field_id;                       // Unique field identifier
    double energy_level_mev = 0.0;             // Current energy level in MeV
    double stability_factor = 0.0;             // Field stability coefficient
    double dissipation_rate = 0.0;             // Energy dissipation rate
    double base_three_mev_per_sec = 0.0;       // Base-3 energy generation rate
    double entropy_factor = 0.0;               // Entropy calculation factor
    bool active = false;                       // Field active state
    double total_energy_mev = 0.0;             // Cumulative energy processed
    std::chrono::system_clock::time_point created_at; // Field creation timestamp
    std::chrono::system_clock::time_point last_updated; // Last update timestamp
    std::string status = "inactive";           // Field operational status
    
    // We provide JSON serialization method
    Json::Value toJson() const;
    
    // We provide deserialization from JSON
    bool fromJson(const Json::Value& json);
};

/**
 * We define system status response structure for monitoring
 * This structure provides comprehensive system health information
 */
struct SystemStatusResponse {
    int64_t uptime_seconds = 0;                 // System uptime in seconds
    uint64_t total_fission_events = 0;          // Total fission events processed
    double total_energy_simulated_mev = 0.0;    // Total energy simulated in MeV
    int active_energy_fields = 0;              // Number of active energy fields
    uint64_t peak_memory_usage_bytes = 0;       // Peak memory usage in bytes
    double average_calc_time_microseconds = 0.0; // Average calculation time
    uint64_t total_calculations = 0;            // Total calculations performed
    bool simulation_running = false;            // Simulation engine status
    double cpu_usage_percent = 0.0;            // Current CPU usage percentage
    double memory_usage_percent = 0.0;         // Current memory usage percentage
    
    // We provide JSON serialization method
    Json::Value toJson() const;
};

/**
 * We define WebSocket connection management structure
 * This structure handles real-time monitoring connections
 */
struct WebSocketConnection {
    std::string connection_id;                  // Unique connection identifier
    std::chrono::system_clock::time_point connected_at; // Connection timestamp
    std::string client_ip;                      // Client IP address
    bool active = true;                         // Connection active status
    std::queue<std::string> message_queue;      // Outbound message queue
    std::mutex queue_mutex;                     // Message queue synchronization
};

/**
 * We define HTTP metrics collection structure
 * This structure tracks server performance and usage statistics
 */
struct HTTPServerMetrics {
    std::atomic<uint64_t> total_requests{0};    // Total requests processed
    std::atomic<uint64_t> successful_requests{0}; // Successful requests count  
    std::atomic<uint64_t> error_requests{0};    // Error requests count
    std::atomic<double> average_response_time{0.0}; // Average response time
    std::atomic<uint64_t> active_connections{0}; // Current active connections
    std::atomic<uint64_t> websocket_connections{0}; // Active WebSocket connections
    std::map<std::string, uint64_t> endpoint_counters; // Per-endpoint request counts
    std::mutex metrics_mutex;                   // Metrics synchronization
    
    // We provide methods for metrics updates
    void incrementRequests();
    void incrementSuccessful();
    void incrementErrors();
    void updateResponseTime(double time_ms);
    void incrementConnections();
    void decrementConnections();
};

/**
 * We define the main HTTP server class for ternary fission daemon operations
 * This class provides complete REST API functionality with physics integration
 */
class HTTPTernaryFissionServer {
private:
    std::unique_ptr<ConfigurationManager> config_manager_;     // Configuration management
    std::unique_ptr<httplib::Server> http_server_;            // HTTP server instance
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    std::unique_ptr<httplib::SSLServer> https_server_;        // HTTPS server instance
#endif
    std::shared_ptr<TernaryFissionSimulationEngine> simulation_engine_; // Physics engine
    std::mutex simulation_mutex_;                // Simulation state synchronization
    
    // We maintain server state and configuration
    std::string bind_ip_;                       // Network binding IP address
    int bind_port_;                            // Network binding port
    bool ssl_enabled_;                         // SSL/TLS enablement flag
    bool server_running_;                      // Server operational status
    std::chrono::system_clock::time_point start_time_; // Server start timestamp
    
    // We manage energy fields and monitoring
    std::map<std::string, std::unique_ptr<EnergyFieldResponse>> energy_fields_;
    mutable std::mutex fields_mutex_;                   // Energy fields synchronization
    std::atomic<int64_t> field_id_counter_;     // Field ID generation counter
    
    // We handle WebSocket connections
    std::map<std::string, std::unique_ptr<WebSocketConnection>> websocket_connections_;
    std::mutex websocket_mutex_;                // WebSocket synchronization
    std::thread websocket_broadcast_thread_;    // WebSocket broadcast worker
    std::atomic<bool> websocket_broadcasting_;  // WebSocket broadcast control
    
    // We collect performance metrics
    std::unique_ptr<HTTPServerMetrics> metrics_; // Server performance metrics
    std::thread metrics_collection_thread_;     // Metrics collection worker
    std::atomic<bool> metrics_collecting_;      // Metrics collection control
    
    // We provide middleware implementations
    void setupMiddleware();                     // Configure all middleware
    void loggingMiddleware(const httplib::Request& req, httplib::Response& res); // Request logging
    void corsMiddleware(const httplib::Request& req, httplib::Response& res);    // CORS handling
    void metricsMiddleware(const httplib::Request& req, httplib::Response& res); // Metrics collection
    void authenticationMiddleware(const httplib::Request& req, httplib::Response& res); // Auth validation
    
    // We implement API endpoint handlers
    void setupAPIEndpoints();                   // Configure all API endpoints
    void handleHealthCheck(const httplib::Request& req, httplib::Response& res); // Health endpoint
    void handleSystemStatus(const httplib::Request& req, httplib::Response& res); // Status endpoint
    void handleEnergyFieldsList(const httplib::Request& req, httplib::Response& res); // Fields list
    void handleEnergyFieldCreate(const httplib::Request& req, httplib::Response& res); // Create field
    void handleEnergyFieldGet(const httplib::Request& req, httplib::Response& res); // Get field
    void handleEnergyFieldUpdate(const httplib::Request& req, httplib::Response& res); // Update field
    void handleEnergyFieldDelete(const httplib::Request& req, httplib::Response& res); // Delete field
    void handleSimulationStart(const httplib::Request& req, httplib::Response& res); // Start simulation
    void handleSimulationStop(const httplib::Request& req, httplib::Response& res); // Stop simulation
    void handleSimulationReset(const httplib::Request& req, httplib::Response& res); // Reset simulation
    void handleFissionCalculation(const httplib::Request& req, httplib::Response& res); // Fission calc
    void handleConservationLaws(const httplib::Request& req, httplib::Response& res); // Conservation check
    void handleEnergyGeneration(const httplib::Request& req, httplib::Response& res); // Energy generation
    void handleFieldStatistics(const httplib::Request& req, httplib::Response& res); // Field statistics
    
    // We handle WebSocket connections and broadcasting
    void setupWebSocketEndpoints();             // Configure WebSocket endpoints
    void handleWebSocketConnection(const httplib::Request& req, httplib::Response& res); // WebSocket upgrade
    void broadcastWebSocketUpdates();           // WebSocket broadcast worker
    void sendWebSocketMessage(const std::string& connection_id, const std::string& message); // Send message
    void cleanupWebSocketConnections();         // Connection cleanup
    
    // We provide utility methods for response handling
    void sendJSONResponse(httplib::Response& res, int status_code, const Json::Value& json); // JSON response
    void sendErrorResponse(httplib::Response& res, int status_code, const std::string& message); // Error response
    bool parseJSONRequest(const httplib::Request& req, Json::Value& json); // JSON parsing
    std::string generateFieldID();              // Generate unique field ID
    std::string generateConnectionID();         // Generate unique connection ID
    
    // We implement SSL/TLS certificate management  
    bool loadSSLCertificates();                // Load SSL certificates
    bool validateSSLCertificate(const std::string& cert_path); // Validate certificate
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    void setupSSLServer();                     // Configure HTTPS server
#endif
    
    // We collect and manage server metrics
    void collectMetrics();                     // Metrics collection worker
    SystemStatusResponse generateSystemStatus(); // Generate status response
    void updateFieldStatistics();             // Update field statistics
    
    // We provide physics engine integration
    bool initializePhysicsEngine();            // Initialize simulation engine
    void shutdownPhysicsEngine();             // Shutdown simulation engine
    Json::Value processPhysicsRequest(const Json::Value& request); // Process physics request
    
public:
    /**
     * We construct the HTTP server with configuration
     * The constructor initializes all server components and prepares for operation
     */
    explicit HTTPTernaryFissionServer(std::unique_ptr<ConfigurationManager> config_manager);
    
    /**
     * We destruct the HTTP server with proper cleanup
     * The destructor ensures all resources are properly released
     */
    virtual ~HTTPTernaryFissionServer();
    
    /**
     * We initialize the HTTP server with configuration validation
     * This method prepares all server components for operation
     */
    bool initialize();
    
    /**
     * We start the HTTP server and begin accepting connections
     * This method starts the server in either HTTP or HTTPS mode based on configuration
     */
    bool start();
    
    /**
     * We stop the HTTP server and gracefully shutdown all connections
     * This method ensures proper cleanup of all server resources
     */
    void stop();
    
    /**
     * We check if the server is currently running
     * This method returns the current operational status
     */
    bool isRunning() const;
    
    /**
     * We get the server's current binding address
     * This method returns the IP:port combination the server is bound to
     */
    std::string getBindAddress() const;
    
    /**
     * We set the physics simulation engine for API integration
     * This method configures the physics engine for API endpoint processing
     */
    void setSimulationEngine(std::shared_ptr<TernaryFissionSimulationEngine> engine);
    
    /**
     * We get current server performance metrics
     * This method returns complete server statistics and performance data
     */
    HTTPServerMetrics getMetrics() const;
    
    /**
     * We get current system status information
     * This method returns comprehensive system health and operational data
     */
    SystemStatusResponse getSystemStatus() const;
    
    /**
     * We reload server configuration without restart
     * This method updates server configuration while maintaining active connections
     */
    bool reloadConfiguration();
    
    /**
     * We validate server configuration parameters
     * This method checks all configuration values for validity and consistency
     */
    bool validateConfiguration() const;

    /**
     * We add an energy field to internal storage
     * This method is primarily for testing and initialization
     */
    void addEnergyField(const EnergyFieldResponse& field);

    /**
     * We compute statistics about all energy fields
     * This method calculates aggregate metrics for monitoring
     */
    Json::Value computeFieldStatistics() const;

    /**
     * We get list of active energy fields
     * This method returns all currently active energy field configurations
     */
    std::vector<EnergyFieldResponse> getActiveEnergyFields() const;
    
    /**
     * We get count of active WebSocket connections
     * This method returns the number of currently active monitoring connections
     */
    size_t getActiveWebSocketConnections() const;
};

} // namespace TernaryFission

#endif // HTTP_TERNARY_FISSION_SERVER_H
