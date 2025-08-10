/*
 * File: src/go/api.ternary.fission.server.go
 * Author: bthlops (David StJ)
 * Date: July 31, 2025
 * Title: Enhanced Ternary Fission Energy Control API Server with Immersive Dashboard - FIXED
 * Purpose: Production-grade Go API server with comprehensive web dashboard for nuclear physics simulation
 * Reason: Provides engaging browser interface for Ternary Fission Energy Emulation System control and monitoring
 *
 * Change Log:
 * 2025-07-31: FIXED dashboard routing and template execution with proper error handling
 *             Created immersive, educational dashboard explaining project purpose and capabilities
 *             Added comprehensive project information, physics background, and usage instructions
 *             Enhanced visual design with modern responsive interface and interactive elements
 *             Fixed config parsing to properly handle inline comments and whitespace
 *             Added real-time energy field visualization and advanced monitoring capabilities
 *
 * Carry-over Context:
 * - We fixed the critical routing bug preventing dashboard access at root URL
 * - We created an immersive, educational interface explaining the Ternary Fission project
 * - We provide complete physics background, usage instructions, and API documentation
 * - We use modern responsive design with proper font sizing and spacing as specified
 * - We maintain production-grade error handling and comprehensive logging throughout
 * - Next: Integration with C++ simulation engine via CGO for real physics calculations
 */

package main

import (
	"bufio"
	"bytes"
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"html/template"
	"io"
	"log"
	"net/http"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/gorilla/mux"
	"github.com/gorilla/websocket"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

var (
	Version   = "dev"
	BuildDate = "unknown"
	GitCommit = "unknown"
)

// =============================================================================
// CONFIGURATION STRUCTURE
// =============================================================================

// We define the configuration structure to hold all settings
type Config struct {
	// API Server settings
	APIPort                  int    `config:"api_port"`
	APIHost                  string `config:"api_host"`
	APITimeout               int    `config:"api_timeout"`
	MaxRequestSize           int64  `config:"max_request_size"`
	MaxConcurrentConnections int    `config:"max_concurrent_connections"`
	ReactorBaseURL           string `config:"reactor_base_url"`
	StatusPollInterval       int    `config:"status_poll_interval"`

	// WebSocket settings
	WebSocketEnabled      bool `config:"websocket_enabled"`
	WebSocketBufferSize   int  `config:"websocket_buffer_size"`
	WebSocketTimeout      int  `config:"websocket_timeout"`
	WebSocketPingInterval int  `config:"websocket_ping_interval"`

	// Physics simulation settings
	ParentMass       float64 `config:"parent_mass"`
	ExcitationEnergy float64 `config:"excitation_energy"`
	EventsPerSecond  float64 `config:"events_per_second"`
	MaxEnergyField   float64 `config:"max_energy_field"`

	// Logging settings
	LogLevel      string `config:"log_level"`
	VerboseOutput bool   `config:"verbose_output"`

	// Feature flags
	PrometheusEnabled   bool `config:"prometheus_enabled"`
	CORSEnabled         bool `config:"cors_enabled"`
	RateLimitingEnabled bool `config:"rate_limiting_enabled"`
}

// We provide default configuration values with port 8238
func defaultConfig() *Config {
	return &Config{
		APIPort:                  8238,
		APIHost:                  "0.0.0.0",
		APITimeout:               30,
		MaxRequestSize:           10485760,
		MaxConcurrentConnections: 1000,
		ReactorBaseURL:           "http://127.0.0.1:8333",
		StatusPollInterval:       15,
		WebSocketEnabled:         true,
		WebSocketBufferSize:      4096,
		WebSocketTimeout:         300,
		WebSocketPingInterval:    30,
		ParentMass:               235.0,
		ExcitationEnergy:         6.5,
		EventsPerSecond:          5.0,
		MaxEnergyField:           1000.0,
		LogLevel:                 "info",
		VerboseOutput:            false,
		PrometheusEnabled:        true,
		CORSEnabled:              true,
		RateLimitingEnabled:      true,
	}
}

// We parse configuration file with proper inline comment handling and whitespace trimming
func parseConfigFile(filename string) (*Config, error) {
	config := defaultConfig()

	file, err := os.Open(filename)
	if err != nil {
		return config, fmt.Errorf("failed to open config file: %v", err)
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	lineNum := 0

	for scanner.Scan() {
		lineNum++
		line := strings.TrimSpace(scanner.Text())

		// We skip empty lines and comments
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}

		// We parse key=value pairs and handle inline comments properly
		parts := strings.SplitN(line, "=", 2)
		if len(parts) != 2 {
			continue
		}

		key := strings.TrimSpace(parts[0])
		value := strings.TrimSpace(parts[1])

		// We remove inline comments (everything after # including whitespace)
		if commentIndex := strings.Index(value, "#"); commentIndex != -1 {
			value = strings.TrimSpace(value[:commentIndex])
		}

		// We remove any surrounding quotes from values
		if len(value) >= 2 && ((value[0] == '"' && value[len(value)-1] == '"') || (value[0] == '\'' && value[len(value)-1] == '\'')) {
			value = value[1 : len(value)-1]
		}

		// We set configuration values based on key
		switch key {
		case "api_port":
			if port, err := strconv.Atoi(value); err == nil {
				config.APIPort = port
			}
		case "api_host":
			config.APIHost = value
		case "reactor_base_url":
			// Base URL for the backing reactor service
			config.ReactorBaseURL = value
		case "status_poll_interval":
			if interval, err := strconv.Atoi(value); err == nil {
				config.StatusPollInterval = interval
			}
		case "events_per_second":
			if eps, err := strconv.ParseFloat(value, 64); err == nil {
				config.EventsPerSecond = eps
			}
		case "parent_mass":
			if mass, err := strconv.ParseFloat(value, 64); err == nil {
				config.ParentMass = mass
			}
		case "excitation_energy":
			if energy, err := strconv.ParseFloat(value, 64); err == nil {
				config.ExcitationEnergy = energy
			}
		case "max_energy_field":
			if max, err := strconv.ParseFloat(value, 64); err == nil {
				config.MaxEnergyField = max
			}
		case "log_level":
			config.LogLevel = value
		case "verbose_output":
			config.VerboseOutput = (strings.ToLower(value) == "true")
		case "websocket_enabled":
			config.WebSocketEnabled = (strings.ToLower(value) == "true")
		case "prometheus_enabled":
			config.PrometheusEnabled = (strings.ToLower(value) == "true")
		case "cors_enabled":
			config.CORSEnabled = (strings.ToLower(value) == "true")
		case "rate_limiting_enabled":
			config.RateLimitingEnabled = (strings.ToLower(value) == "true")
		}
	}

	return config, scanner.Err()
}

// =============================================================================
// DATA STRUCTURES FOR API COMMUNICATION
// =============================================================================

// We define the API request structure for creating energy fields
type EnergyFieldRequest struct {
	InitialEnergyMeV  float64 `json:"initial_energy_mev"`
	DissipationRounds int     `json:"dissipation_rounds,omitempty"`
	FieldName         string  `json:"field_name,omitempty"`
	AutoDissipate     bool    `json:"auto_dissipate,omitempty"`
}

// We define the API response structure for energy field status
type EnergyFieldResponse struct {
	FieldID             string    `json:"field_id"`
	EnergyMeV           float64   `json:"energy_mev"`
	MemoryBytes         uint64    `json:"memory_bytes"`
	CPUCycles           uint64    `json:"cpu_cycles"`
	EntropyFactor       float64   `json:"entropy_factor"`
	DissipationRate     float64   `json:"dissipation_rate"`
	StabilityFactor     float64   `json:"stability_factor"`
	InteractionStrength float64   `json:"interaction_strength"`
	Active              bool      `json:"active"`
	TotalEnergyMeV      float64   `json:"total_energy_mev"`
	CreatedAt           time.Time `json:"created_at"`
	LastUpdated         time.Time `json:"last_updated"`
	Status              string    `json:"status"`
}

// We define system status response
type SystemStatusResponse struct {
	UptimeSeconds        int64   `json:"uptime_seconds"`
	TotalFissionEvents   uint64  `json:"total_fission_events"`
	TotalEnergySimulated float64 `json:"total_energy_simulated_mev"`
	ActiveEnergyFields   int     `json:"active_energy_fields"`
        PeakMemoryUsage      uint64  `json:"peak_memory_usage_bytes"`
        AverageCalcTime      float64 `json:"average_calculation_time_microseconds"`
        TotalCalculations    uint64  `json:"total_calculations"`
        SimulationRunning    bool    `json:"simulation_running"`
        CPUUsagePercent      float64 `json:"cpu_usage_percent"`
        MemoryUsagePercent   float64 `json:"memory_usage_percent"`
        EstimatedPower       float64 `json:"estimated_power_mev"`
        PortalDurationRemain int     `json:"portal_duration_remaining_seconds"`
}

// =============================================================================
// API SERVER IMPLEMENTATION
// =============================================================================

// We implement the main API server structure
type TernaryFissionAPIServer struct {
	config            *Config
	server            *http.Server
	router            *mux.Router
	websocketUpgrader websocket.Upgrader
	activeConnections map[string]*websocket.Conn
	connectionsMutex  sync.RWMutex
	startTime         time.Time

	// Performance metrics
	requestCounter      *prometheus.CounterVec
	responseTime        *prometheus.HistogramVec
	reactorActiveFields prometheus.Gauge
	reactorTotalEnergy  prometheus.Gauge

	// Reactor communication
	reactorClient *http.Client

	// System control
	shutdownChan chan os.Signal
	ctx          context.Context
	cancelFunc   context.CancelFunc
}

// We initialize the API server with configuration
func NewTernaryFissionAPIServer(config *Config) *TernaryFissionAPIServer {
	ctx, cancel := context.WithCancel(context.Background())

	server := &TernaryFissionAPIServer{
		config:            config,
		router:            mux.NewRouter(),
		activeConnections: make(map[string]*websocket.Conn),
		reactorClient:     &http.Client{Timeout: time.Duration(config.APITimeout) * time.Second},
		shutdownChan:      make(chan os.Signal, 1),
		ctx:               ctx,
		cancelFunc:        cancel,
		startTime:         time.Now(),
	}

	// We configure WebSocket upgrader with proper settings
	server.websocketUpgrader = websocket.Upgrader{
		CheckOrigin: func(r *http.Request) bool {
			return true
		},
		ReadBufferSize:  config.WebSocketBufferSize,
		WriteBufferSize: config.WebSocketBufferSize,
	}

	// We initialize Prometheus metrics for monitoring
	if config.PrometheusEnabled {
		server.initializeMetrics()
		server.startReactorStatusPolling()
	}

	// We set up HTTP routes
	server.setupRoutes()

	// We configure the HTTP server
	server.server = &http.Server{
		Addr:         fmt.Sprintf("%s:%d", config.APIHost, config.APIPort),
		Handler:      server.router,
		ReadTimeout:  time.Duration(config.APITimeout) * time.Second,
		WriteTimeout: time.Duration(config.APITimeout) * time.Second,
		IdleTimeout:  120 * time.Second,
	}

	// We set up graceful shutdown handling
	signal.Notify(server.shutdownChan, syscall.SIGINT, syscall.SIGTERM)

	log.Printf("Ternary Fission API Server initialized on port %d", config.APIPort)
	return server
}

// We initialize Prometheus metrics for performance monitoring
func (s *TernaryFissionAPIServer) initializeMetrics() {
	s.requestCounter = prometheus.NewCounterVec(
		prometheus.CounterOpts{
			Name: "ternary_fission_api_requests_total",
			Help: "Total number of API requests processed",
		},
		[]string{"endpoint", "method", "status"},
	)

	s.responseTime = prometheus.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "ternary_fission_api_response_time_seconds",
			Help:    "Response time of API requests",
			Buckets: prometheus.DefBuckets,
		},
		[]string{"endpoint", "method"},
	)

	s.reactorActiveFields = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Name: "reactor_active_fields",
			Help: "Number of active reactor energy fields",
		},
	)

	s.reactorTotalEnergy = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Name: "reactor_total_energy_mev",
			Help: "Total reactor energy in MeV",
		},
	)

	// We register metrics with Prometheus
	prometheus.MustRegister(s.requestCounter)
	prometheus.MustRegister(s.responseTime)
	prometheus.MustRegister(s.reactorActiveFields)
	prometheus.MustRegister(s.reactorTotalEnergy)
}

// We periodically poll the reactor status and update Prometheus metrics
func (s *TernaryFissionAPIServer) startReactorStatusPolling() {
	ticker := time.NewTicker(time.Duration(s.config.StatusPollInterval) * time.Second)
	go func() {
		defer ticker.Stop()
		for {
			select {
			case <-ticker.C:
				s.updateReactorMetrics()
			case <-s.ctx.Done():
				return
			}
		}
	}()
}

// We query the reactor status endpoint and update our gauges
func (s *TernaryFissionAPIServer) updateReactorMetrics() {
	resp, err := s.reactorClient.Get(fmt.Sprintf("%s/api/v1/status", s.config.ReactorBaseURL))
	if err != nil {
		return
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return
	}
	var status SystemStatusResponse
	if err := json.NewDecoder(resp.Body).Decode(&status); err != nil {
		return
	}
	s.reactorActiveFields.Set(float64(status.ActiveEnergyFields))
	s.reactorTotalEnergy.Set(status.TotalEnergySimulated)
}

// We set up all HTTP routes and middleware with proper error handling
func (s *TernaryFissionAPIServer) setupRoutes() {
	// We add middleware for logging and metrics
	s.router.Use(s.loggingMiddleware)
	if s.config.PrometheusEnabled {
		s.router.Use(s.metricsMiddleware)
	}
	if s.config.CORSEnabled {
		s.router.Use(s.corsMiddleware)
	}

    s.router.HandleFunc("/api/v1/portal/trigger", s.triggerPortalSimulation).Methods("PUT")

        // We serve the enhanced web dashboard at root - FIXED routing
	s.router.PathPrefix("/").HandlerFunc(s.routeHandler).Methods("GET")

	log.Println("API routes configured successfully with enhanced dashboard")
}

// We handle all routing with proper dashboard serving - FIXED
func (s *TernaryFissionAPIServer) routeHandler(w http.ResponseWriter, r *http.Request) {
	path := r.URL.Path

	// We route API calls to appropriate handlers
	if strings.HasPrefix(path, "/api/v1/") {
		s.handleAPIRoutes(w, r)
		return
	}

	// We serve the dashboard for root and dashboard paths
	if path == "/" || path == "/dashboard" {
		s.serveDashboard(w, r)
		return
	}

	// We serve 404 for unknown paths
	http.NotFound(w, r)
}

// We handle API routes properly
func (s *TernaryFissionAPIServer) handleAPIRoutes(w http.ResponseWriter, r *http.Request) {
	path := strings.TrimPrefix(r.URL.Path, "/api/v1")

	switch {
	case path == "/health":
		s.healthCheck(w, r)
	case path == "/status":
		s.getSystemStatus(w, r)
	case path == "/energy-fields" && r.Method == "GET":
		s.listEnergyFields(w, r)
	case path == "/energy-fields" && r.Method == "POST":
		s.createEnergyField(w, r)
	case strings.HasPrefix(path, "/energy-fields/") && r.Method == "GET":
		s.getEnergyField(w, r)
	case strings.HasPrefix(path, "/energy-fields/") && r.Method == "DELETE":
		s.deleteEnergyField(w, r)
	case strings.HasPrefix(path, "/energy-fields/") && strings.HasSuffix(path, "/dissipate"):
		s.dissipateEnergyField(w, r)
	case path == "/metrics" && s.config.PrometheusEnabled:
		promhttp.Handler().ServeHTTP(w, r)
	case path == "/ws/monitor" && s.config.WebSocketEnabled:
		s.handleWebSocketConnection(w, r)
	default:
		s.writeErrorResponse(w, http.StatusNotFound, "API endpoint not found")
	}
}

// =============================================================================
// ENHANCED WEB DASHBOARD IMPLEMENTATION
// =============================================================================

// We define the comprehensive dashboard template with immersive content
const enhancedDashboardHTML = `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Ternary Fission Energy Emulation System - Beyond The Horizon Labs</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        * {
            margin: 0;
            padding: 1px;
            box-sizing: border-box;
        }

        body {
            font-family: Verdana, Arial, sans-serif;
            font-size: 12px;
            background: linear-gradient(135deg, #0f0f23 0%, #1a1a2e 50%, #16213e 100%);
            min-height: 100vh;
            color: #e8e8e8;
            line-height: 1.4;
        }

        .container {
            max-width: 1400px;
            margin: 0 auto;
            padding: 20px;
        }

        .hero-header {
            background: linear-gradient(135deg, rgba(0,150,255,0.1) 0%, rgba(150,0,255,0.1) 100%);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 12px;
            padding: 30px;
            margin-bottom: 30px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.3);
            backdrop-filter: blur(10px);
            text-align: center;
        }

        .hero-header h1 {
            font-size: 16px;
            color: #00d4ff;
            margin-bottom: 10px;
            text-shadow: 0 0 10px rgba(0,212,255,0.5);
        }

        .hero-header .subtitle {
            font-size: 14px;
            color: #64b5f6;
            margin-bottom: 15px;
        }

        .hero-header .description {
            font-size: 12px;
            color: #90caf9;
            max-width: 800px;
            margin: 0 auto 20px;
        }

        .hero-header .author-info {
            font-size: 12px;
            color: #81c784;
            border-top: 1px solid rgba(255,255,255,0.1);
            padding-top: 15px;
            margin-top: 15px;
        }

        .physics-info {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }

        .info-card {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 8px;
            padding: 20px;
            box-shadow: 0 4px 16px rgba(0,0,0,0.2);
            backdrop-filter: blur(5px);
        }

        .info-card h2 {
            font-size: 14px;
            color: #ffd54f;
            margin-bottom: 15px;
            border-bottom: 2px solid rgba(255,213,79,0.3);
            padding-bottom: 8px;
        }

        .info-card p {
            margin-bottom: 10px;
            color: #e0e0e0;
        }

        .info-card .formula {
            font-family: 'Courier New', monospace;
            background: rgba(0,0,0,0.3);
            padding: 8px;
            border-radius: 4px;
            margin: 10px 0;
            color: #81c784;
            border-left: 3px solid #4caf50;
        }

        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }

        .status-card {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 8px;
            padding: 20px;
            box-shadow: 0 4px 16px rgba(0,0,0,0.2);
            backdrop-filter: blur(5px);
        }

        .status-card h2 {
            font-size: 14px;
            color: #ff6b6b;
            margin-bottom: 15px;
            border-bottom: 2px solid rgba(255,107,107,0.3);
            padding-bottom: 8px;
        }

        .metric {
            display: flex;
            justify-content: space-between;
            margin: 10px 0;
            padding: 8px;
            background: rgba(255,255,255,0.02);
            border-radius: 4px;
            border-left: 3px solid transparent;
        }

        .metric.active { border-left-color: #4caf50; }
        .metric.warning { border-left-color: #ff9800; }
        .metric.critical { border-left-color: #f44336; }

        .metric-label {
            font-weight: bold;
            color: #b0bec5;
        }

        .metric-value {
            color: #64b5f6;
            font-weight: bold;
        }

        .energy-visualization {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 30px;
            box-shadow: 0 4px 16px rgba(0,0,0,0.2);
        }

        .energy-visualization h2 {
            font-size: 14px;
            color: #9c27b0;
            margin-bottom: 15px;
            text-align: center;
        }

        .energy-bar-container {
            background: rgba(0,0,0,0.3);
            height: 30px;
            border-radius: 15px;
            margin: 15px 0;
            overflow: hidden;
            position: relative;
        }

        .energy-bar {
            height: 100%;
            background: linear-gradient(90deg, #4caf50, #8bc34a, #cddc39);
            border-radius: 15px;
            transition: width 0.5s ease;
            box-shadow: 0 0 20px rgba(76,175,80,0.4);
        }

        .energy-bar-label {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            font-size: 11px;
            font-weight: bold;
            color: #fff;
            text-shadow: 1px 1px 2px rgba(0,0,0,0.7);
        }

        .field-creator {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 30px;
            box-shadow: 0 4px 16px rgba(0,0,0,0.2);
        }

        .field-creator h2 {
            font-size: 14px;
            color: #e91e63;
            margin-bottom: 15px;
            text-align: center;
        }

        .portal-trigger {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 30px;
            box-shadow: 0 4px 16px rgba(0,0,0,0.2);
        }

        .portal-trigger h2 {
            font-size: 14px;
            color: #2196f3;
            margin-bottom: 15px;
            text-align: center;
        }

        .form-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }

        .form-group label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
            color: #90caf9;
            font-size: 12px;
        }

        .form-group input, .form-group select {
            width: 100%;
            padding: 8px;
            border: 1px solid rgba(255,255,255,0.2);
            border-radius: 4px;
            font-size: 12px;
            background: rgba(0,0,0,0.3);
            color: #e8e8e8;
        }

        .form-group input:focus, .form-group select:focus {
            border-color: #64b5f6;
            outline: none;
            box-shadow: 0 0 10px rgba(100,181,246,0.3);
        }

        .submit-btn {
            background: linear-gradient(135deg, #e91e63, #ad1457);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 12px;
            font-weight: bold;
            transition: all 0.3s ease;
            width: 100%;
        }

        .submit-btn:hover {
            background: linear-gradient(135deg, #ad1457, #880e4f);
            box-shadow: 0 4px 16px rgba(233,30,99,0.4);
        }

        .api-documentation {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 30px;
            box-shadow: 0 4px 16px rgba(0,0,0,0.2);
        }

        .api-documentation h2 {
            font-size: 14px;
            color: #03dac6;
            margin-bottom: 15px;
            text-align: center;
        }

        .endpoints-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 15px;
        }

        .endpoint {
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 6px;
            padding: 15px;
            background: rgba(0,0,0,0.2);
        }

        .endpoint-method {
            display: inline-block;
            padding: 4px 8px;
            border-radius: 3px;
            font-size: 10px;
            font-weight: bold;
            margin-right: 10px;
        }

        .method-get { background: #4caf50; color: white; }
        .method-post { background: #2196f3; color: white; }
        .method-delete { background: #f44336; color: white; }

        .endpoint-url {
            font-family: 'Courier New', monospace;
            font-size: 11px;
            color: #81c784;
        }

        .endpoint-desc {
            margin-top: 8px;
            font-size: 11px;
            color: #b0bec5;
        }

        .test-button {
            background: linear-gradient(135deg, #9c27b0, #673ab7);
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 11px;
            margin-top: 8px;
            transition: all 0.3s ease;
        }

        .test-button:hover {
            background: linear-gradient(135deg, #673ab7, #3f51b5);
            box-shadow: 0 2px 8px rgba(156,39,176,0.4);
        }

        .response-area {
            margin-top: 15px;
            padding: 15px;
            background: rgba(0,0,0,0.4);
            color: #e8e8e8;
            border-radius: 6px;
            font-family: 'Courier New', monospace;
            font-size: 11px;
            max-height: 300px;
            overflow-y: auto;
            border: 1px solid rgba(255,255,255,0.1);
            display: none;
        }

        .chart-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin: 30px 0;
        }

        .chart-card {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 8px;
            padding: 20px;
            box-shadow: 0 4px 16px rgba(0,0,0,0.2);
            backdrop-filter: blur(5px);
        }

        .footer {
            text-align: center;
            padding: 20px;
            margin-top: 40px;
            border-top: 1px solid rgba(255,255,255,0.1);
            color: #757575;
            font-size: 11px;
        }

        @media (max-width: 768px) {
            .container { padding: 10px; }
            .physics-info { grid-template-columns: 1fr; }
            .status-grid { grid-template-columns: 1fr; }
            .endpoints-grid { grid-template-columns: 1fr; }
            .form-grid { grid-template-columns: 1fr; }
        }

        /* We add pulsing animation for active elements */
        @keyframes pulse {
            0% { box-shadow: 0 0 0 0 rgba(100,181,246,0.4); }
            70% { box-shadow: 0 0 0 10px rgba(100,181,246,0); }
            100% { box-shadow: 0 0 0 0 rgba(100,181,246,0); }
        }

        .pulse { animation: pulse 2s infinite; }
    </style>
</head>
<body>
    <div class="container">
        <div class="hero-header">
            <h1>🔬 Ternary Fission Energy Emulation System</h1>
            <div class="subtitle">Advanced Nuclear Physics Simulation Platform</div>
            <div class="description">
                We have developed a revolutionary system that simulates ternary nuclear fission events using computational energy field mapping.
                Our system represents nuclear energy through memory allocation and CPU cycles, creating a unique approach to energy simulation
                that bridges theoretical physics with practical computing resources. We use base-3 mathematics for energy generation,
                base-8 electromagnetic field stabilization, and encryption-based energy dissipation modeling.
            </div>
            <div class="author-info">
                <strong>Beyond The Horizon Labs</strong> | <strong>Author:</strong> bthlops (David StJ) |
                <strong>Server:</strong> {{.ServerHost}}:{{.ServerPort}} |
                <strong>Status:</strong> <span id="server-status" class="pulse">Connected</span>
            </div>
        </div>

        <div class="physics-info">
            <div class="info-card">
                <h2>🧬 Ternary Fission Physics</h2>
                <p>Traditional nuclear fission splits atomic nuclei into two fragments. We simulate <strong>ternary fission</strong> - a rare process where nuclei split into three fragments, typically producing two fission fragments plus an alpha particle.</p>
                <div class="formula">E = Q₀ + Eₓ = (M_parent - M_frag1 - M_frag2 - M_α) × c²</div>
                <p>Our simulation uses realistic U-235 parameters with 6.5 MeV excitation energy, generating statistically accurate fragment mass distributions and energy releases of ~200 MeV per event.</p>
            </div>

            <div class="info-card">
                <h2>💾 Energy Field Mapping</h2>
                <p>We innovatively represent nuclear energy as computational resources:</p>
                <p><strong>Memory Mapping:</strong> 1 MeV = 1 MB allocated memory</p>
                <p><strong>CPU Mapping:</strong> 1 MeV = 1 billion CPU cycles consumed</p>
                <div class="formula">S = k × ln(W) - Entropy calculated from system microstates</div>
                <p>Energy dissipation occurs through encryption rounds, modeling exponential decay: E(t) = E₀ × e^(-λt)</p>
            </div>

            <div class="info-card">
                <h2>🔬 Research Applications</h2>
                <p><strong>Nuclear Physics Education:</strong> Interactive learning tool for understanding fission processes</p>
                <p><strong>Safety Analysis:</strong> Reactor safety simulations and risk assessment</p>
                <p><strong>Waste Management:</strong> Modeling fission product behavior and decay chains</p>
                <p><strong>Research Validation:</strong> Cross-checking experimental results with theoretical predictions</p>
            </div>

            <div class="info-card">
                <h2>🚀 Technical Innovation</h2>
                <p>Our system implements conservation laws (energy, momentum, mass, charge) with realistic tolerances. We use:</p>
                <p><strong>Watt Spectrum:</strong> For neutron energy distributions</p>
                <p><strong>Maxwell-Boltzmann:</strong> For thermal velocity modeling</p>
                <p><strong>Monte Carlo Methods:</strong> For statistical simulation accuracy</p>
                <div class="formula">p = √(E² - (mc²)²)/c - Relativistic momentum calculation</div>
            </div>
        </div>

        <div class="status-grid">
            <div class="status-card">
                <h2>🖥️ System Status</h2>
                <div class="metric active">
                    <span class="metric-label">System Uptime:</span>
                    <span class="metric-value" id="uptime">Loading...</span>
                </div>
                <div class="metric active">
                    <span class="metric-label">Active Energy Fields:</span>
                    <span class="metric-value" id="active-fields">Loading...</span>
                </div>
                <div class="metric active">
                    <span class="metric-label">Total Energy Simulated:</span>
                    <span class="metric-value" id="total-energy">Loading...</span>
                </div>
                <div class="metric active">
                    <span class="metric-label">Simulation Running:</span>
                    <span class="metric-value" id="simulation-running">Loading...</span>
                </div>
            </div>

            <div class="status-card">
                <h2>📊 Performance Metrics</h2>
                <div class="metric">
                    <span class="metric-label">CPU Usage:</span>
                    <span class="metric-value" id="cpu-usage">Loading...</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Memory Usage:</span>
                    <span class="metric-value" id="memory-usage">Loading...</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Peak Memory Usage:</span>
                    <span class="metric-value" id="peak-memory">Loading...</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Avg Calculation Time:</span>
                    <span class="metric-value" id="avg-calc-time">Loading...</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Event Rate:</span>
                    <span class="metric-value" id="event-rate">Loading...</span>
                </div>
            </div>

            <div class="status-card">
                <h2>🔬 Physics Parameters</h2>
                <div class="metric">
                    <span class="metric-label">Parent Nucleus:</span>
                    <span class="metric-value">U-{{.Config.ParentMass}}</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Excitation Energy:</span>
                    <span class="metric-value">{{.Config.ExcitationEnergy}} MeV</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Events Per Second:</span>
                    <span class="metric-value">{{.Config.EventsPerSecond}}</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Max Energy Field:</span>
                    <span class="metric-value">{{.Config.MaxEnergyField}} MeV</span>
                </div>
            </div>
        </div>

        <div class="chart-grid">
            <div class="chart-card">
                <canvas id="energyChart"></canvas>
            </div>
            <div class="chart-card">
                <canvas id="resourceChart"></canvas>
            </div>
            <div class="chart-card">
                <canvas id="eventChart"></canvas>
            </div>
        </div>

        <div class="energy-visualization">
            <h2>⚡ Real-Time Energy Field Visualization</h2>
            <div class="energy-bar-container">
                <div class="energy-bar" id="energy-bar" style="width: 0%"></div>
                <div class="energy-bar-label" id="energy-bar-label">0 MeV</div>
            </div>
            <p style="text-align: center; color: #90caf9; font-size: 11px; margin-top: 10px;">
                Energy field intensity visualization - updates in real-time based on active simulations
            </p>
        </div>

        <div class="field-creator">
            <h2>🎛️ Create Energy Field</h2>
            <p style="text-align: center; color: #b0bec5; margin-bottom: 20px; font-size: 11px;">
                Generate a new energy field for nuclear physics simulation. We allocate computational resources
                proportional to the energy level and simulate realistic dissipation through encryption operations.
            </p>
            <form id="energy-field-form">
                <div class="form-grid">
                    <div class="form-group">
                        <label for="energy-level">Initial Energy (MeV):</label>
                        <input type="number" id="energy-level" min="0.1" max="{{.Config.MaxEnergyField}}" step="0.1" value="100" required>
                    </div>
                    <div class="form-group">
                        <label for="field-name">Field Name:</label>
                        <input type="text" id="field-name" placeholder="Energy Field #1">
                    </div>
                    <div class="form-group">
                        <label for="auto-dissipate">Auto Dissipate:</label>
                        <select id="auto-dissipate">
                            <option value="true">Yes (Recommended)</option>
                            <option value="false">No</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="dissipation-rounds">Dissipation Rounds:</label>
                        <input type="number" id="dissipation-rounds" min="1" max="256" value="15">
                    </div>
                </div>
                <button type="submit" class="submit-btn">🚀 Create Energy Field</button>
            </form>
            <div id="field-response" class="response-area"></div>
        </div>

        <div class="portal-trigger">
            <h2>🌀 Trigger Portal</h2>
            <p style="text-align: center; color: #b0bec5; margin-bottom: 20px; font-size: 11px;">
                Initiate a transient portal event with specified duration and power level.
            </p>
            <form id="portal-trigger-form">
                <div class="form-grid">
                    <div class="form-group">
                        <label for="portal-duration">Duration (minutes):</label>
                        <input type="number" id="portal-duration" min="1" value="15" required>
                    </div>
                    <div class="form-group">
                        <label for="portal-power">Power Level:</label>
                        <input type="number" id="portal-power" min="0.1" step="0.1" value="1" required>
                    </div>
                </div>
                <button type="submit" class="submit-btn">🌀 Trigger Portal</button>
            </form>
            <div id="portal-response" class="response-area"></div>
        </div>

        <div class="api-documentation">
            <h2>📡 API Documentation & Testing Interface</h2>
            <p style="text-align: center; color: #b0bec5; margin-bottom: 20px; font-size: 11px;">
                Complete RESTful API for programmatic access to the Ternary Fission simulation system.
                We provide endpoints for energy field management, system monitoring, and real-time WebSocket updates.
            </p>
            <div class="endpoints-grid">
                <div class="endpoint">
                    <span class="endpoint-method method-get">GET</span>
                    <span class="endpoint-url">/api/v1/status</span>
                    <div class="endpoint-desc">Comprehensive system status including performance metrics, active fields, and simulation statistics</div>
                    <button class="test-button" onclick="testEndpoint('/api/v1/status', 'GET')">🧪 Test</button>
                </div>

                <div class="endpoint">
                    <span class="endpoint-method method-get">GET</span>
                    <span class="endpoint-url">/api/v1/health</span>
                    <div class="endpoint-desc">Basic health check endpoint for monitoring system availability</div>
                    <button class="test-button" onclick="testEndpoint('/api/v1/health', 'GET')">🧪 Test</button>
                </div>

                <div class="endpoint">
                    <span class="endpoint-method method-get">GET</span>
                    <span class="endpoint-url">/api/v1/energy-fields</span>
                    <div class="endpoint-desc">List all active energy fields with detailed status information</div>
                    <button class="test-button" onclick="testEndpoint('/api/v1/energy-fields', 'GET')">🧪 Test</button>
                </div>

                <div class="endpoint">
                    <span class="endpoint-method method-post">POST</span>
                    <span class="endpoint-url">/api/v1/energy-fields</span>
                    <div class="endpoint-desc">Create new energy field with specified parameters (energy level, dissipation settings)</div>
                    <button class="test-button" onclick="showCreateForm()">🧪 Test</button>
                </div>

                <div class="endpoint">
                    <span class="endpoint-method method-get">GET</span>
                    <span class="endpoint-url">/api/v1/energy-fields/{id}</span>
                    <div class="endpoint-desc">Retrieve specific energy field details by field ID</div>
                    <button class="test-button" onclick="testWithId('energy-fields')">🧪 Test</button>
                </div>

                <div class="endpoint">
                    <span class="endpoint-method method-delete">DELETE</span>
                    <span class="endpoint-url">/api/v1/energy-fields/{id}</span>
                    <div class="endpoint-desc">Safely terminate and cleanup specified energy field</div>
                    <button class="test-button" onclick="deleteWithId('energy-fields')">🧪 Test</button>
                </div>

                <div class="endpoint">
                    <span class="endpoint-method method-post">POST</span>
                    <span class="endpoint-url">/api/v1/energy-fields/{id}/dissipate</span>
                    <div class="endpoint-desc">Dissipate existing energy field through encryption rounds</div>
                    <button class="test-button" onclick="dissipateWithId()">🧪 Test</button>
                </div>

                <div class="endpoint">
                    <span class="endpoint-method method-get">GET</span>
                    <span class="endpoint-url">/api/v1/metrics</span>
                    <div class="endpoint-desc">Prometheus-compatible metrics for monitoring and alerting systems</div>
                    <button class="test-button" onclick="testEndpoint('/api/v1/metrics', 'GET')">🧪 Test</button>
                </div>

                <div class="endpoint">
                    <span class="endpoint-method method-get">GET</span>
                    <span class="endpoint-url">/api/v1/ws/monitor</span>
                    <div class="endpoint-desc">WebSocket endpoint for real-time monitoring and live updates</div>
                    <button class="test-button" onclick="testWebSocket()">🧪 Test</button>
                </div>
            </div>
        </div>

        <div class="footer">
            <p><strong>Ternary Fission Energy Emulation System</strong> | Beyond The Horizon Labs</p>
            <p>Advanced nuclear physics simulation with computational energy field mapping</p>
            <p><strong>Author:</strong> bthlops (David StJ) | <strong>Contact:</strong> davestj@gmail.com</p>
            <p><strong>Research Focus:</strong> Ternary nuclear fission, energy field theory, computational physics simulation</p>
        </div>
    </div>

    <script>
        // We implement comprehensive dashboard functionality
        let statusUpdateInterval;
        let energyBarAnimation;
        let energyChart;
        let resourceChart;
        let eventChart;
        let lastEventCount = 0;
        let lastEventTime = Date.now();

        // We initialize the dashboard when page loads
        document.addEventListener('DOMContentLoaded', function() {
            console.log('🚀 Ternary Fission Dashboard Loading...');
            initializeCharts();
            updateSystemStatus();
            updateEnergyFields();
            startStatusUpdates();
            setupEnergyFieldForm();
            setupPortalTriggerForm();
            initializeEnergyVisualization();
            console.log('✅ Dashboard fully loaded and operational');
        });

        // We fetch and update system status with enhanced error handling
        function updateSystemStatus() {
            fetch('/api/v1/status')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok: ' + response.status);
                    }
                    return response.json();
                })
                .then(data => {
                    console.log('📊 Status update received:', data);

                    // We update all status displays
                    document.getElementById('uptime').textContent = formatUptime(data.uptime_seconds);
                    document.getElementById('total-energy').textContent = data.total_energy_simulated.toFixed(2) + ' MeV';
                    document.getElementById('simulation-running').textContent = data.simulation_running ? 'Active' : 'Idle';
                    document.getElementById('cpu-usage').textContent = data.cpu_usage_percent.toFixed(1) + '%';
                    document.getElementById('memory-usage').textContent = data.memory_usage_percent.toFixed(1) + '%';
                    document.getElementById('peak-memory').textContent = formatBytes(data.peak_memory_usage_bytes);
                    document.getElementById('avg-calc-time').textContent = data.average_calculation_time_microseconds.toFixed(1) + ' μs';
                    const now = new Date();
                    const label = now.toLocaleTimeString();
                    const deltaEvents = data.total_fission_events - lastEventCount;
                    const deltaTime = (now.getTime() - lastEventTime) / 1000;
                    const eventRate = deltaTime > 0 ? deltaEvents / deltaTime : 0;
                    lastEventCount = data.total_fission_events;
                    lastEventTime = now.getTime();
                    document.getElementById('event-rate').textContent = eventRate.toFixed(2) + ' events/s';

                    energyChart.data.labels.push(label);
                    energyChart.data.datasets[0].data.push(data.total_energy_simulated);
                    energyChart.update();

                    resourceChart.data.labels.push(label);
                    resourceChart.data.datasets[0].data.push(data.cpu_usage_percent);
                    resourceChart.data.datasets[1].data.push(data.memory_usage_percent);
                    resourceChart.update();

                    eventChart.data.labels.push(label);
                    eventChart.data.datasets[1].data.push(eventRate);

                    // We update connection status
                    const statusElement = document.getElementById('server-status');
                    statusElement.textContent = 'Connected';
                    statusElement.style.color = '#4caf50';
                    statusElement.classList.add('pulse');

                    // We update energy visualization
                    updateEnergyVisualization(data.total_energy_simulated);

                    // We update metric styling based on values
                    updateMetricStyling(data);
                })
                .catch(error => {
                    console.error('❌ Failed to fetch status:', error);
                    const statusElement = document.getElementById('server-status');
                    statusElement.textContent = 'Disconnected';
                    statusElement.style.color = '#f44336';
                    statusElement.classList.remove('pulse');
                });
        }

        // We start periodic status updates with configurable interval
        function startStatusUpdates() {
            statusUpdateInterval = setInterval(function() {
                updateSystemStatus();
                updateEnergyFields();
            }, 3000); // Update every 3 seconds
            console.log('⏰ Status updates started (3s interval)');
        }

        // We fetch energy field information and update charts
        function updateEnergyFields() {
            fetch('/api/v1/energy-fields')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok: ' + response.status);
                    }
                    return response.json();
                })
                .then(fields => {
                    const count = Array.isArray(fields) ? fields.length : 0;
                    document.getElementById('active-fields').textContent = count;
                    eventChart.data.datasets[0].data.push(count);
                    eventChart.update();
                })
                .catch(error => {
                    console.error('❌ Failed to fetch energy fields:', error);
                });
        }

        // We initialize Chart.js visualizations
        function initializeCharts() {
            const ctxEnergy = document.getElementById('energyChart').getContext('2d');
            energyChart = new Chart(ctxEnergy, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: 'Total Energy (MeV)',
                        borderColor: '#ffd54f',
                        backgroundColor: 'rgba(255,213,79,0.2)',
                        data: [],
                        tension: 0.3
                    }]
                },
                options: {
                    scales: { y: { beginAtZero: true } }
                }
            });

            const ctxResource = document.getElementById('resourceChart').getContext('2d');
            resourceChart = new Chart(ctxResource, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [
                        {
                            label: 'CPU Usage %',
                            borderColor: '#64b5f6',
                            backgroundColor: 'rgba(100,181,246,0.2)',
                            data: [],
                            tension: 0.3
                        },
                        {
                            label: 'Memory Usage %',
                            borderColor: '#81c784',
                            backgroundColor: 'rgba(129,199,132,0.2)',
                            data: [],
                            tension: 0.3
                        }
                    ]
                },
                options: {
                    scales: { y: { beginAtZero: true, max: 100 } }
                }
            });

            const ctxEvent = document.getElementById('eventChart').getContext('2d');
            eventChart = new Chart(ctxEvent, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [
                        {
                            label: 'Active Fields',
                            borderColor: '#f06292',
                            backgroundColor: 'rgba(240,98,146,0.2)',
                            data: [],
                            tension: 0.3
                        },
                        {
                            label: 'Event Rate (events/s)',
                            borderColor: '#ba68c8',
                            backgroundColor: 'rgba(186,104,200,0.2)',
                            data: [],
                            tension: 0.3,
                            yAxisID: 'y1'
                        }
                    ]
                },
                options: {
                    scales: {
                        y: { beginAtZero: true, position: 'left' },
                        y1: { beginAtZero: true, position: 'right', grid: { drawOnChartArea: false } }
                    }
                }
            });
        }

        // We set up the energy field creation form with validation
        function setupEnergyFieldForm() {
            const form = document.getElementById('energy-field-form');
            form.addEventListener('submit', function(e) {
                e.preventDefault();
                console.log('🔬 Creating new energy field...');

                const formData = {
                    initial_energy_mev: parseFloat(document.getElementById('energy-level').value),
                    field_name: document.getElementById('field-name').value || '',
                    auto_dissipate: document.getElementById('auto-dissipate').value === 'true',
                    dissipation_rounds: parseInt(document.getElementById('dissipation-rounds').value)
                };

                console.log('📝 Form data:', formData);

                fetch('/api/v1/energy-fields', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(formData)
                })
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Failed to create energy field: ' + response.status);
                    }
                    return response.json();
                })
                .then(data => {
                    console.log('✅ Energy field created:', data);
                    const responseArea = document.getElementById('field-response');
                    responseArea.innerHTML = '<strong>✅ Energy Field Created Successfully!</strong><br><pre>' +
                        JSON.stringify(data, null, 2) + '</pre>';
                    responseArea.style.display = 'block';
                    updateSystemStatus(); // Refresh status after creation
                })
                .catch(error => {
                    console.error('❌ Error creating energy field:', error);
                    const responseArea = document.getElementById('field-response');
                    responseArea.innerHTML = '<strong>❌ Error:</strong> ' + error.message;
                    responseArea.style.display = 'block';
                });
            });
        }

        // We set up the portal trigger form
        function setupPortalTriggerForm() {
            const form = document.getElementById('portal-trigger-form');
            form.addEventListener('submit', function(e) {
                e.preventDefault();
                console.log('🌌 Triggering portal...');

                const formData = {
                    duration_minutes: parseInt(document.getElementById('portal-duration').value),
                    power_level: parseFloat(document.getElementById('portal-power').value)
                };

                fetch('/api/v1/portal/trigger', {
                    method: 'PUT',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(formData)
                })
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Failed to trigger portal: ' + response.status);
                    }
                    return response.json();
                })
                .then(data => {
                    console.log('🌠 Portal triggered:', data);
                    const responseArea = document.getElementById('portal-response');
                    responseArea.innerHTML = '<strong>✅ Portal triggered successfully!</strong><br><pre>' +
                        JSON.stringify(data, null, 2) + '</pre>';
                    responseArea.style.display = 'block';
                })
                .catch(error => {
                    console.error('❌ Error triggering portal:', error);
                    const responseArea = document.getElementById('portal-response');
                    responseArea.innerHTML = '<strong>❌ Error:</strong> ' + error.message;
                    responseArea.style.display = 'block';
                });
            });
        }

        // We initialize energy visualization with animation
        function initializeEnergyVisualization() {
            console.log('⚡ Initializing energy visualization...');
            updateEnergyVisualization(0);
        }

        // We update energy bar visualization
        function updateEnergyVisualization(totalEnergy) {
            const maxEnergy = {{.Config.MaxEnergyField}};
            const percentage = Math.min((totalEnergy / maxEnergy) * 100, 100);

            const energyBar = document.getElementById('energy-bar');
            const energyLabel = document.getElementById('energy-bar-label');

            if (energyBar && energyLabel) {
                energyBar.style.width = percentage + '%';
                energyLabel.textContent = totalEnergy.toFixed(1) + ' MeV';

                // We add visual effects based on energy level
                if (percentage > 80) {
                    energyBar.style.background = 'linear-gradient(90deg, #ff5722, #ff9800, #ffc107)';
                } else if (percentage > 50) {
                    energyBar.style.background = 'linear-gradient(90deg, #ff9800, #ffc107, #ffeb3b)';
                } else {
                    energyBar.style.background = 'linear-gradient(90deg, #4caf50, #8bc34a, #cddc39)';
                }
            }
        }

        // We update metric styling based on values
        function updateMetricStyling(data) {
            const metrics = document.querySelectorAll('.metric');
            metrics.forEach(metric => {
                metric.classList.remove('active', 'warning', 'critical');

                const valueText = metric.querySelector('.metric-value').textContent;
                if (valueText.includes('%')) {
                    const value = parseFloat(valueText);
                    if (value > 80) metric.classList.add('critical');
                    else if (value > 60) metric.classList.add('warning');
                    else metric.classList.add('active');
                } else {
                    metric.classList.add('active');
                }
            });
        }

        // We test API endpoints with enhanced feedback
        function testEndpoint(url, method) {
            console.log('🧪 Testing endpoint:', method, url);

            fetch(url, { method: method })
                .then(response => {
                    if (!response.ok) {
                        throw new Error('HTTP ' + response.status + ': ' + response.statusText);
                    }
                    return response.text();
                })
                .then(data => {
                    console.log('✅ Test successful:', url);
                    alert('🎉 Test Successful!\n\nEndpoint: ' + method + ' ' + url + '\n\nResponse:\n' +
                          (data.length > 500 ? data.substring(0, 500) + '...\n[Response truncated]' : data));
                })
                .catch(error => {
                    console.error('❌ Test failed:', error);
                    alert('❌ Test Failed!\n\nEndpoint: ' + method + ' ' + url + '\n\nError: ' + error.message);
                });
        }

        // We test endpoints that require an ID
        function testWithId(endpoint) {
            const id = prompt('Enter ' + endpoint + ' ID:');
            if (id) {
                testEndpoint('/api/v1/' + endpoint + '/' + id, 'GET');
            }
        }

        // We delete resources by ID with confirmation
        function deleteWithId(endpoint) {
            const id = prompt('Enter ' + endpoint + ' ID to delete:');
            if (id && confirm('⚠️  Are you sure you want to delete this ' + endpoint + '?\n\nThis action cannot be undone!')) {
                console.log('🗑️ Deleting:', endpoint, id);

                fetch('/api/v1/' + endpoint + '/' + id, { method: 'DELETE' })
                    .then(response => {
                        if (!response.ok) {
                            throw new Error('HTTP ' + response.status + ': ' + response.statusText);
                        }
                        return response.json();
                    })
                    .then(data => {
                        console.log('✅ Delete successful:', data);
                        alert('✅ Delete Successful!\n\n' + JSON.stringify(data, null, 2));
                        updateSystemStatus();
                    })
                    .catch(error => {
                        console.error('❌ Delete failed:', error);
                        alert('❌ Delete Failed!\n\nError: ' + error.message);
                    });
            }
        }

        // We dissipate energy fields by ID with configurable rounds
        function dissipateWithId() {
            const id = prompt('Enter energy-fields ID to dissipate:');
            if (id) {
                const roundsInput = prompt('Enter dissipation rounds:', '1');
                const rounds = roundsInput ? parseInt(roundsInput) : 0;
                const payload = rounds > 0 ? { dissipation_rounds: rounds } : {};

                fetch('/api/v1/energy-fields/' + id + '/dissipate', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                })
                    .then(response => {
                        if (!response.ok) {
                            throw new Error('HTTP ' + response.status + ': ' + response.statusText);
                        }
                        return response.json();
                    })
                    .then(data => {
                        console.log('✅ Dissipation successful:', data);
                        alert('✅ Dissipation Successful!\n\n' + JSON.stringify(data, null, 2));
                        updateSystemStatus();
                    })
                    .catch(error => {
                        console.error('❌ Dissipation failed:', error);
                        alert('❌ Dissipation Failed!\n\nError: ' + error.message);
                    });
            }
        }

        // We show create form
        function showCreateForm() {
            document.querySelector('.field-creator').scrollIntoView({ behavior: 'smooth' });
            document.getElementById('energy-level').focus();
        }

        // We test WebSocket connection
        function testWebSocket() {
            console.log('🔌 Testing WebSocket connection...');

            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = protocol + '//' + window.location.host + '/api/v1/ws/monitor';

            try {
                const ws = new WebSocket(wsUrl);

                ws.onopen = function() {
                    console.log('✅ WebSocket connected');
                    alert('🎉 WebSocket Test Successful!\n\nConnected to: ' + wsUrl + '\n\nListening for real-time updates...');
                };

                ws.onmessage = function(event) {
                    console.log('📨 WebSocket message:', event.data);
                };

                ws.onerror = function(error) {
                    console.error('❌ WebSocket error:', error);
                    alert('❌ WebSocket Test Failed!\n\nError connecting to: ' + wsUrl);
                };

                // We close connection after 5 seconds for testing
                setTimeout(() => {
                    ws.close();
                    console.log('🔌 WebSocket test connection closed');
                }, 5000);

            } catch (error) {
                console.error('❌ WebSocket test failed:', error);
                alert('❌ WebSocket Test Failed!\n\nError: ' + error.message);
            }
        }

        // We format uptime in human readable format
        function formatUptime(seconds) {
            const days = Math.floor(seconds / 86400);
            const hours = Math.floor((seconds % 86400) / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const remainingSeconds = seconds % 60;

            if (days > 0) {
                return days + 'd ' + hours + 'h ' + minutes + 'm';
            } else if (hours > 0) {
                return hours + 'h ' + minutes + 'm';
            } else if (minutes > 0) {
                return minutes + 'm ' + remainingSeconds.toFixed(0) + 's';
            } else {
                return remainingSeconds.toFixed(1) + 's';
            }
        }

        // We format bytes in human readable format
        function formatBytes(bytes) {
            const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
            if (bytes === 0) return '0 B';
            const i = Math.floor(Math.log(bytes) / Math.log(1024));
            return (bytes / Math.pow(1024, i)).toFixed(1) + ' ' + sizes[i];
        }

        // We add keyboard shortcuts for power users
        document.addEventListener('keydown', function(e) {
            if (e.ctrlKey || e.metaKey) {
                switch(e.key) {
                    case 'r':
                        e.preventDefault();
                        updateSystemStatus();
                        console.log('🔄 Manual status refresh triggered');
                        break;
                    case 'e':
                        e.preventDefault();
                        showCreateForm();
                        break;
                }
            }
        });

        console.log('🎛️ Dashboard keyboard shortcuts enabled: Ctrl+R (refresh), Ctrl+E (create field)');
    </script>
</body>
</html>`

// We serve the enhanced web dashboard with proper error handling
func (s *TernaryFissionAPIServer) serveDashboard(w http.ResponseWriter, r *http.Request) {
	tmpl, err := template.New("dashboard").Parse(enhancedDashboardHTML)
	if err != nil {
		log.Printf("Template parsing error: %v", err)
		http.Error(w, "Template parsing error", http.StatusInternalServerError)
		return
	}

	data := struct {
		ServerHost string
		ServerPort int
		Config     *Config
	}{
		ServerHost: s.config.APIHost,
		ServerPort: s.config.APIPort,
		Config:     s.config,
	}

	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
	w.Header().Set("Pragma", "no-cache")
	w.Header().Set("Expires", "0")

	if err := tmpl.Execute(w, data); err != nil {
		log.Printf("Template execution error: %v", err)
		http.Error(w, "Template execution error", http.StatusInternalServerError)
		return
	}

	log.Printf("Dashboard served successfully to %s", r.RemoteAddr)
}

// =============================================================================
// API ENDPOINT IMPLEMENTATIONS
// =============================================================================

// We implement the energy field creation endpoint
func (s *TernaryFissionAPIServer) createEnergyField(w http.ResponseWriter, r *http.Request) {
	body, err := io.ReadAll(r.Body)
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadRequest, "Invalid request body")
		return
	}

	req, err := http.NewRequest("POST", fmt.Sprintf("%s/api/v1/energy-fields", s.config.ReactorBaseURL), bytes.NewReader(body))
	if err != nil {
		s.writeErrorResponse(w, http.StatusInternalServerError, "Failed to build reactor request")
		return
	}
	req.Header.Set("Content-Type", "application/json")

	resp, err := s.reactorClient.Do(req)
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadGateway, "Failed to contact reactor")
		return
	}
	defer resp.Body.Close()

	w.Header().Set("Content-Type", resp.Header.Get("Content-Type"))
	w.WriteHeader(resp.StatusCode)
	io.Copy(w, resp.Body)
}

// We implement the system status endpoint
func (s *TernaryFissionAPIServer) getSystemStatus(w http.ResponseWriter, r *http.Request) {
	resp, err := s.reactorClient.Get(fmt.Sprintf("%s/api/v1/status", s.config.ReactorBaseURL))
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadGateway, "Failed to contact reactor")
		return
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(resp.Body)
		s.writeErrorResponse(w, resp.StatusCode, string(body))
		return
	}

	var status SystemStatusResponse
	if err := json.NewDecoder(resp.Body).Decode(&status); err != nil {
		s.writeErrorResponse(w, http.StatusInternalServerError, "Invalid reactor response")
		return
	}

	if s.config.PrometheusEnabled {
		s.reactorActiveFields.Set(float64(status.ActiveEnergyFields))
		s.reactorTotalEnergy.Set(status.TotalEnergySimulated)
	}

	s.writeJSONResponse(w, http.StatusOK, status)
}

// We list all energy fields
func (s *TernaryFissionAPIServer) listEnergyFields(w http.ResponseWriter, r *http.Request) {
	resp, err := s.reactorClient.Get(fmt.Sprintf("%s/api/v1/energy-fields", s.config.ReactorBaseURL))
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadGateway, "Failed to contact reactor")
		return
	}
	defer resp.Body.Close()

	w.Header().Set("Content-Type", resp.Header.Get("Content-Type"))
	w.WriteHeader(resp.StatusCode)
	io.Copy(w, resp.Body)
}

// We get a specific energy field
func (s *TernaryFissionAPIServer) getEnergyField(w http.ResponseWriter, r *http.Request) {
	pathParts := strings.Split(r.URL.Path, "/")
	if len(pathParts) < 4 {
		s.writeErrorResponse(w, http.StatusBadRequest, "Invalid field ID")
		return
	}
	fieldID := pathParts[len(pathParts)-1]

	resp, err := s.reactorClient.Get(fmt.Sprintf("%s/api/v1/energy-fields/%s", s.config.ReactorBaseURL, fieldID))
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadGateway, "Failed to contact reactor")
		return
	}
	defer resp.Body.Close()

	w.Header().Set("Content-Type", resp.Header.Get("Content-Type"))
	w.WriteHeader(resp.StatusCode)
	io.Copy(w, resp.Body)
}

// We delete an energy field
func (s *TernaryFissionAPIServer) deleteEnergyField(w http.ResponseWriter, r *http.Request) {
	pathParts := strings.Split(r.URL.Path, "/")
	if len(pathParts) < 4 {
		s.writeErrorResponse(w, http.StatusBadRequest, "Invalid field ID")
		return
	}
	fieldID := pathParts[len(pathParts)-1]

	req, err := http.NewRequest("DELETE", fmt.Sprintf("%s/api/v1/energy-fields/%s", s.config.ReactorBaseURL, fieldID), nil)
	if err != nil {
		s.writeErrorResponse(w, http.StatusInternalServerError, "Failed to build reactor request")
		return
	}

	resp, err := s.reactorClient.Do(req)
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadGateway, "Failed to contact reactor")
		return
	}
	defer resp.Body.Close()

	w.Header().Set("Content-Type", resp.Header.Get("Content-Type"))
	w.WriteHeader(resp.StatusCode)
	io.Copy(w, resp.Body)
}

// We handle WebSocket connections for real-time monitoring
func (s *TernaryFissionAPIServer) handleWebSocketConnection(w http.ResponseWriter, r *http.Request) {
	conn, err := s.websocketUpgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("WebSocket upgrade failed: %v", err)
		return
	}
	defer conn.Close()

	log.Printf("WebSocket client connected: %s", r.RemoteAddr)

	ticker := time.NewTicker(time.Duration(s.config.WebSocketPingInterval) * time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			resp, err := s.reactorClient.Get(fmt.Sprintf("%s/api/v1/status", s.config.ReactorBaseURL))
			if err != nil {
				log.Printf("WebSocket status fetch failed: %v", err)
				return
			}
			var status SystemStatusResponse
			if err := json.NewDecoder(resp.Body).Decode(&status); err != nil {
				resp.Body.Close()
				log.Printf("WebSocket status decode failed: %v", err)
				return
			}
			resp.Body.Close()

			if err := conn.WriteJSON(status); err != nil {
				log.Printf("WebSocket write failed: %v", err)
				return
			}
		case <-s.ctx.Done():
			return
		}
	}
}

// We dissipate an energy field by forwarding the request to the reactor
func (s *TernaryFissionAPIServer) dissipateEnergyField(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost && r.Method != http.MethodPut {
		s.writeErrorResponse(w, http.StatusMethodNotAllowed, "Method not allowed - use POST or PUT")
		return
	}

	parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
	if len(parts) < 5 {
		s.writeErrorResponse(w, http.StatusBadRequest, "Invalid field ID")
		return
	}
	fieldID := parts[len(parts)-2]

	body, err := io.ReadAll(r.Body)
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadRequest, "Invalid request body")
		return
	}

	req, err := http.NewRequest(r.Method, fmt.Sprintf("%s/api/v1/energy-fields/%s/dissipate", s.config.ReactorBaseURL, fieldID), bytes.NewReader(body))
	if err != nil {
		s.writeErrorResponse(w, http.StatusInternalServerError, "Failed to build reactor request")
		return
	}
	if len(body) > 0 {
		req.Header.Set("Content-Type", r.Header.Get("Content-Type"))
	}

	resp, err := s.reactorClient.Do(req)
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadGateway, "Failed to contact reactor")
		return
	}
	defer resp.Body.Close()

        w.Header().Set("Content-Type", resp.Header.Get("Content-Type"))
        w.WriteHeader(resp.StatusCode)
        io.Copy(w, resp.Body)
}

// We trigger portal simulations by forwarding the request to the reactor
func (s *TernaryFissionAPIServer) triggerPortalSimulation(w http.ResponseWriter, r *http.Request) {
    var req struct {
        DurationSeconds int     `json:"duration_seconds"`
        PowerLevelMEV   float64 `json:"power_level_mev"`
    }

    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        s.writeErrorResponse(w, http.StatusBadRequest, "Invalid request body")
        return
    }
    if req.DurationSeconds == 0 {
        req.DurationSeconds = 900
    } else if req.DurationSeconds < 0 {
        s.writeErrorResponse(w, http.StatusBadRequest, "Duration must be non-negative")
        return
    }

    body, err := json.Marshal(req)
    if err != nil {
        s.writeErrorResponse(w, http.StatusInternalServerError, "Failed to encode request")
        return
    }

    reactorURL := s.config.ReactorBaseURL + "/api/v1/portal/trigger"
    reactorReq, err := http.NewRequest(http.MethodPut, reactorURL, bytes.NewReader(body))
    if err != nil {
        s.writeErrorResponse(w, http.StatusInternalServerError, "Failed to build reactor request")
        return
    }
    reactorReq.Header.Set("Content-Type", "application/json")

    resp, err := s.reactorClient.Do(reactorReq)
    if err != nil {
        s.writeErrorResponse(w, http.StatusBadGateway, "Failed to contact reactor")
        return
    }
    defer resp.Body.Close()

    w.Header().Set("Content-Type", resp.Header.Get("Content-Type"))
    w.WriteHeader(resp.StatusCode)
    if _, err := io.Copy(w, resp.Body); err != nil {
        log.Printf("Failed to forward reactor response: %v", err)
    }
}

// =============================================================================
// UTILITY METHODS
// =============================================================================

// We provide various utility methods
func (s *TernaryFissionAPIServer) writeJSONResponse(w http.ResponseWriter, status int, data interface{}) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	if err := json.NewEncoder(w).Encode(data); err != nil {
		log.Printf("JSON encoding error: %v", err)
	}
}

func (s *TernaryFissionAPIServer) writeErrorResponse(w http.ResponseWriter, status int, message string) {
	s.writeJSONResponse(w, status, map[string]string{
		"error":     message,
		"timestamp": time.Now().Format(time.RFC3339),
	})
}

// Middleware implementations
func (s *TernaryFissionAPIServer) loggingMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		next.ServeHTTP(w, r)
		duration := time.Since(start)
		if s.config.VerboseOutput {
			log.Printf("%s %s %v from %s", r.Method, r.URL.Path, duration, r.RemoteAddr)
		}
	})
}

func (s *TernaryFissionAPIServer) metricsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		next.ServeHTTP(w, r)
		duration := time.Since(start).Seconds()
		if s.responseTime != nil {
			s.responseTime.WithLabelValues(r.URL.Path, r.Method).Observe(duration)
			s.requestCounter.WithLabelValues(r.URL.Path, r.Method, "200").Inc()
		}
	})
}

func (s *TernaryFissionAPIServer) corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")

		if r.Method == "OPTIONS" {
			w.WriteHeader(http.StatusOK)
			return
		}

		next.ServeHTTP(w, r)
	})
}

func (s *TernaryFissionAPIServer) healthCheck(w http.ResponseWriter, r *http.Request) {
	uptime := time.Since(s.startTime)

	resp, err := s.reactorClient.Get(fmt.Sprintf("%s/api/v1/status", s.config.ReactorBaseURL))
	if err != nil {
		s.writeErrorResponse(w, http.StatusBadGateway, "Failed to contact reactor")
		return
	}
	var status SystemStatusResponse
	if err := json.NewDecoder(resp.Body).Decode(&status); err != nil {
		resp.Body.Close()
		s.writeErrorResponse(w, http.StatusInternalServerError, "Invalid reactor response")
		return
	}
	resp.Body.Close()

	health := map[string]interface{}{
		"status":               "healthy",
		"timestamp":            time.Now().Format(time.RFC3339),
		"uptime_seconds":       int64(uptime.Seconds()),
		"active_energy_fields": status.ActiveEnergyFields,
		"version":              Version,
	}

	s.writeJSONResponse(w, http.StatusOK, health)
}

// We start the API server
func (s *TernaryFissionAPIServer) Start() error {
	go func() {
		<-s.shutdownChan
		log.Println("Shutdown signal received, stopping server...")

		ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
		defer cancel()

		if err := s.server.Shutdown(ctx); err != nil {
			log.Printf("Server shutdown error: %v", err)
		}

		s.cancelFunc()
	}()

	log.Printf("🚀 Starting Ternary Fission API Server on %s", s.server.Addr)
	return s.server.ListenAndServe()
}

// =============================================================================
// MAIN APPLICATION ENTRY POINT
// =============================================================================

func main() {
	var (
		configFile = flag.String("config", "configs/ternary_fission.conf", "Configuration file path")
		port       = flag.Int("port", 0, "Override API port (0 = use config file)")
		help       = flag.Bool("help", false, "Show help message")
	)
	flag.Parse()

	if *help {
		fmt.Printf("Ternary Fission Energy Emulation API Server\n")
		fmt.Printf("Author: bthlops (David StJ)\n\n")
		fmt.Printf("Usage: %s [options]\n\n", os.Args[0])
		fmt.Printf("Options:\n")
		flag.PrintDefaults()
		fmt.Printf("\nExamples:\n")
		fmt.Printf("  %s -config configs/ternary_fission.conf\n", os.Args[0])
		fmt.Printf("  %s -port 8238\n", os.Args[0])
		fmt.Printf("  %s -config custom.conf -port 9000\n", os.Args[0])
		fmt.Printf("\nAccess Points:\n")
		fmt.Printf("  Web Dashboard: http://localhost:8238/\n")
		fmt.Printf("  API Documentation: http://localhost:8238/api/v1\n")
		fmt.Printf("  Health Check: http://localhost:8238/api/v1/health\n")
		fmt.Printf("  System Status: http://localhost:8238/api/v1/status\n")
		return
	}

	// We parse the configuration file
	config, err := parseConfigFile(*configFile)
	if err != nil {
		log.Printf("Warning: Failed to parse config file %s: %v", *configFile, err)
		log.Printf("Using default configuration")
		config = defaultConfig()
	} else {
		log.Printf("Loaded configuration from %s", *configFile)
	}

	// We override port if specified on command line
	if *port > 0 {
		config.APIPort = *port
		log.Printf("Port overridden to %d", *port)
	}

	server := NewTernaryFissionAPIServer(config)

	log.Println("=== Ternary Fission Energy Emulation API Server ===")
	log.Printf("Author: bthlops (David StJ)")
	log.Printf("Version: %s", Version)
	log.Printf("Starting server on port %d", config.APIPort)
	log.Printf("🌐 Web Dashboard: http://localhost:%d/", config.APIPort)
	log.Printf("📡 API Documentation: http://localhost:%d/api/v1", config.APIPort)
	if config.WebSocketEnabled {
		log.Printf("🔌 WebSocket monitoring: ws://localhost:%d/api/v1/ws/monitor", config.APIPort)
	}
	if config.PrometheusEnabled {
		log.Printf("📊 Prometheus metrics: http://localhost:%d/api/v1/metrics", config.APIPort)
	}

	if err := server.Start(); err != nil && err != http.ErrServerClosed {
		log.Fatalf("Server failed to start: %v", err)
	}

	log.Println("Server shutdown complete")
}
