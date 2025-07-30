#!/bin/bash
#
# File: scripts/docker-entrypoint.sh
# Author: bthlops (David StJ)
# Date: July 29, 2025
# Title: Docker Container Entrypoint Script for Ternary Fission Energy Emulation
# Purpose: Container initialization and application startup orchestration
# Reason: Provides flexible container startup with proper initialization and configuration
# 
# Change Log:
# 2025-07-29: Initial creation with basic container startup logic
# 2025-07-29: Added configuration validation and environment variable processing
# 2025-07-29: Implemented health checks and service readiness verification
# 2025-07-29: Added graceful shutdown handling and signal forwarding
# 2025-07-29: Integrated logging configuration and debug output management
#
# Carry-over Context:
# - This script serves as the primary entrypoint for Docker containers
# - Environment variables override configuration file settings
# - Health checks ensure services are ready before accepting requests
# - Signal handling enables graceful shutdown of simulation processes
# - Debug mode provides detailed logging for troubleshooting

set -e

# =============================================================================
# GLOBAL VARIABLES AND CONFIGURATION
# =============================================================================

# We define script metadata and version information
SCRIPT_NAME="docker-entrypoint.sh"
SCRIPT_VERSION="1.0.0"
SCRIPT_AUTHOR="bthlops (David StJ)"

# We set default paths and configuration
APP_DIR="/app"
BIN_DIR="${APP_DIR}/bin"
CONFIG_DIR="${APP_DIR}/configs"
DATA_DIR="${APP_DIR}/data"
RESULTS_DIR="${APP_DIR}/results"
LOGS_DIR="${APP_DIR}/logs"

# We define default configuration values
DEFAULT_CONFIG_FILE="${CONFIG_DIR}/ternary_fission.conf"
DEFAULT_LOG_LEVEL="info"
DEFAULT_API_PORT="8080"
DEFAULT_SIMULATION_MODE="daemon"

# We set up signal handling for graceful shutdown
PID_FILE="/tmp/ternary_fission.pid"
SHUTDOWN_TIMEOUT=30

# =============================================================================
# UTILITY FUNCTIONS
# =============================================================================

# We implement logging functions for different severity levels
log_info() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [INFO] $*" >&1
}

log_warn() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [WARN] $*" >&2
}

log_error() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [ERROR] $*" >&2
}

log_debug() {
    if [[ "${DEBUG:-false}" == "true" ]]; then
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] [DEBUG] $*" >&2
    fi
}

# We display usage information for the entrypoint script
show_usage() {
    cat << EOF
Docker Entrypoint Script for Ternary Fission Energy Emulation System
Version: ${SCRIPT_VERSION}
Author: ${SCRIPT_AUTHOR}

Usage: $0 [COMMAND] [OPTIONS]

Commands:
    api-server          Start the Go API server (default)
    simulation-engine   Start the C++ simulation engine
    daemon             Start both services in daemon mode
    benchmark          Run performance benchmarks
    interactive        Start interactive shell
    help               Show this help message

Environment Variables:
    GO_ENV             Environment (development|production|testing)
    LOG_LEVEL          Logging level (debug|info|warn|error)
    API_PORT           API server port (default: 8080)
    CONFIG_FILE        Configuration file path
    DEBUG              Enable debug output (true|false)
    SIMULATION_MODE    Simulation mode (batch|continuous|daemon)

Examples:
    $0 api-server
    $0 simulation-engine --continuous --rate=5.0
    $0 daemon
    $0 benchmark --events=10000

EOF
}

# We validate that required directories exist and are writable
validate_directories() {
    log_debug "Validating application directories..."
    
    local required_dirs=("${CONFIG_DIR}" "${DATA_DIR}" "${RESULTS_DIR}" "${LOGS_DIR}")
    
    for dir in "${required_dirs[@]}"; do
        if [[ ! -d "${dir}" ]]; then
            log_info "Creating directory: ${dir}"
            mkdir -p "${dir}" || {
                log_error "Failed to create directory: ${dir}"
                exit 1
            }
        fi
        
        if [[ ! -w "${dir}" ]]; then
            log_error "Directory not writable: ${dir}"
            exit 1
        fi
    done
    
    log_debug "Directory validation completed successfully"
}

# We validate that configuration file exists and is readable
validate_configuration() {
    local config_file="${CONFIG_FILE:-${DEFAULT_CONFIG_FILE}}"
    
    log_debug "Validating configuration file: ${config_file}"
    
    if [[ ! -f "${config_file}" ]]; then
        log_warn "Configuration file not found: ${config_file}"
        log_info "Creating default configuration file..."
        
        cat > "${config_file}" << 'EOF'
# Default Ternary Fission Configuration
parent_mass=235.0
excitation_energy=6.5
number_of_events=1000
events_per_second=5.0
max_energy_field=500.0
log_level=info
api_port=8080
EOF
        
        log_info "Default configuration created: ${config_file}"
    fi
    
    if [[ ! -r "${config_file}" ]]; then
        log_error "Configuration file not readable: ${config_file}"
        exit 1
    fi
    
    log_debug "Configuration validation completed successfully"
}

# We process environment variables and update configuration
process_environment_variables() {
    log_debug "Processing environment variables..."
    
    # We export commonly used environment variables
    export GO_ENV="${GO_ENV:-production}"
    export LOG_LEVEL="${LOG_LEVEL:-${DEFAULT_LOG_LEVEL}}"
    export API_PORT="${API_PORT:-${DEFAULT_API_PORT}}"
    export CONFIG_FILE="${CONFIG_FILE:-${DEFAULT_CONFIG_FILE}}"
    export SIMULATION_MODE="${SIMULATION_MODE:-${DEFAULT_SIMULATION_MODE}}"
    
    # We set up logging configuration based on environment
    case "${GO_ENV}" in
        "development")
            export LOG_LEVEL="${LOG_LEVEL:-debug}"
            export DEBUG="${DEBUG:-true}"
            ;;
        "production")
            export LOG_LEVEL="${LOG_LEVEL:-info}"
            export DEBUG="${DEBUG:-false}"
            ;;
        "testing")
            export LOG_LEVEL="${LOG_LEVEL:-debug}"
            export DEBUG="${DEBUG:-true}"
            ;;
    esac
    
    log_debug "Environment: GO_ENV=${GO_ENV}, LOG_LEVEL=${LOG_LEVEL}, API_PORT=${API_PORT}"
    log_debug "Configuration: CONFIG_FILE=${CONFIG_FILE}, SIMULATION_MODE=${SIMULATION_MODE}"
}

# We check system health and service readiness
check_system_health() {
    log_debug "Performing system health checks..."
    
    # We check available memory
    local available_memory
    available_memory=$(free -m | awk '/^Mem:/{print $7}')
    
    if [[ ${available_memory} -lt 512 ]]; then
        log_warn "Low available memory: ${available_memory}MB (recommended: >512MB)"
    else
        log_debug "Available memory: ${available_memory}MB"
    fi
    
    # We check available disk space
    local available_disk
    available_disk=$(df "${DATA_DIR}" | awk 'NR==2{print $4}')
    
    if [[ ${available_disk} -lt 1048576 ]]; then  # 1GB in KB
        log_warn "Low available disk space: ${available_disk}KB (recommended: >1GB)"
    else
        log_debug "Available disk space: $((available_disk / 1024))MB"
    fi
    
    # We check that required binaries exist
    local required_binaries=("${BIN_DIR}/api-server" "${BIN_DIR}/ternary-engine")
    
    for binary in "${required_binaries[@]}"; do
        if [[ ! -x "${binary}" ]]; then
            log_error "Required binary not found or not executable: ${binary}"
            exit 1
        fi
    done
    
    log_debug "System health checks completed successfully"
}

# We wait for external services to become available
wait_for_services() {
    log_debug "Waiting for external services..."
    
    # We check if Prometheus is expected to be available
    if [[ "${PROMETHEUS_ENABLED:-true}" == "true" ]]; then
        local prometheus_host="${PROMETHEUS_HOST:-prometheus}"
        local prometheus_port="${PROMETHEUS_PORT:-9090}"
        
        log_debug "Checking Prometheus availability at ${prometheus_host}:${prometheus_port}"
        
        for i in {1..30}; do
            if timeout 2 bash -c "</dev/tcp/${prometheus_host}/${prometheus_port}" 2>/dev/null; then
                log_debug "Prometheus is available"
                break
            fi
            
            if [[ $i -eq 30 ]]; then
                log_warn "Prometheus not available after 30 attempts, continuing anyway..."
            else
                log_debug "Waiting for Prometheus... (attempt $i/30)"
                sleep 2
            fi
        done
    fi
    
    log_debug "Service dependency checks completed"
}

# We set up signal handlers for graceful shutdown
setup_signal_handlers() {
    log_debug "Setting up signal handlers..."
    
    # We handle SIGTERM and SIGINT for graceful shutdown
    trap 'handle_shutdown SIGTERM' TERM
    trap 'handle_shutdown SIGINT' INT
    trap 'handle_shutdown SIGQUIT' QUIT
    
    log_debug "Signal handlers configured successfully"
}

# We handle shutdown signals gracefully
handle_shutdown() {
    local signal=$1
    log_info "Received ${signal}, initiating graceful shutdown..."
    
    # We read the PID of the main application process
    if [[ -f "${PID_FILE}" ]]; then
        local main_pid
        main_pid=$(cat "${PID_FILE}")
        
        log_info "Sending ${signal} to main process (PID: ${main_pid})"
        kill -${signal#SIG} "${main_pid}" 2>/dev/null || true
        
        # We wait for the process to shutdown gracefully
        local timeout=${SHUTDOWN_TIMEOUT}
        while [[ $timeout -gt 0 ]] && kill -0 "${main_pid}" 2>/dev/null; do
            log_debug "Waiting for graceful shutdown... (${timeout}s remaining)"
            sleep 1
            ((timeout--))
        done
        
        # We force kill if graceful shutdown failed
        if kill -0 "${main_pid}" 2>/dev/null; then
            log_warn "Graceful shutdown timeout, forcing termination"
            kill -KILL "${main_pid}" 2>/dev/null || true
        fi
        
        rm -f "${PID_FILE}"
    fi
    
    log_info "Shutdown completed"
    exit 0
}

# =============================================================================
# APPLICATION STARTUP FUNCTIONS
# =============================================================================

# We start the Go API server
start_api_server() {
    log_info "Starting Ternary Fission API Server..."
    
    local api_args=()
    
    # We build command line arguments from environment variables
    api_args+=("--port=${API_PORT}")
    api_args+=("--config=${CONFIG_FILE}")
    api_args+=("--log-level=${LOG_LEVEL}")
    
    if [[ "${DEBUG:-false}" == "true" ]]; then
        api_args+=("--debug")
    fi
    
    if [[ "${GO_ENV}" == "development" ]]; then
        api_args+=("--cors-origin=*")
    fi
    
    log_debug "API server arguments: ${api_args[*]}"
    
    # We start the API server and capture its PID
    "${BIN_DIR}/api-server" "${api_args[@]}" &
    local api_pid=$!
    echo "${api_pid}" > "${PID_FILE}"
    
    log_info "API server started with PID: ${api_pid}"
    
    # We wait for the API server to become ready
    local ready=false
    for i in {1..30}; do
        if curl -sf "http://localhost:${API_PORT}/api/v1/health" >/dev/null 2>&1; then
            ready=true
            break
        fi
        log_debug "Waiting for API server to become ready... (attempt $i/30)"
        sleep 2
    done
    
    if [[ "${ready}" == "true" ]]; then
        log_info "API server is ready and accepting requests"
    else
        log_error "API server failed to become ready within timeout"
        exit 1
    fi
    
    # We wait for the process to complete
    wait "${api_pid}"
}

# We start the C++ simulation engine
start_simulation_engine() {
    log_info "Starting Ternary Fission Simulation Engine..."
    
    local engine_args=()
    
    # We build command line arguments
    engine_args+=("--config=${CONFIG_FILE}")
    
    case "${SIMULATION_MODE}" in
        "batch")
            engine_args+=("--events=${NUMBER_OF_EVENTS:-1000}")
            ;;
        "continuous")
            engine_args+=("--continuous")
            engine_args+=("--rate=${EVENTS_PER_SECOND:-5.0}")
            ;;
        "daemon")
            engine_args+=("--daemon")
            engine_args+=("--rate=${EVENTS_PER_SECOND:-5.0}")
            ;;
        "interactive")
            engine_args+=("--interactive")
            ;;
    esac
    
    if [[ "${LOG_LEVEL}" == "debug" ]]; then
        engine_args+=("--verbose")
    fi
    
    log_debug "Simulation engine arguments: ${engine_args[*]}"
    
    # We start the simulation engine
    "${BIN_DIR}/ternary-engine" "${engine_args[@]}" &
    local engine_pid=$!
    echo "${engine_pid}" > "${PID_FILE}"
    
    log_info "Simulation engine started with PID: ${engine_pid}"
    
    # We wait for the process to complete
    wait "${engine_pid}"
}

# We start both services in daemon mode
start_daemon_mode() {
    log_info "Starting Ternary Fission System in daemon mode..."
    
    # We start the simulation engine in background
    log_info "Starting simulation engine in background..."
    "${BIN_DIR}/ternary-engine" --daemon --config="${CONFIG_FILE}" --rate="${EVENTS_PER_SECOND:-5.0}" &
    local engine_pid=$!
    
    # We give the simulation engine time to initialize
    sleep 2
    
    # We start the API server in foreground
    log_info "Starting API server in foreground..."
    exec "${BIN_DIR}/api-server" --port="${API_PORT}" --config="${CONFIG_FILE}" --log-level="${LOG_LEVEL}"
}

# We run performance benchmarks
run_benchmark() {
    log_info "Running Ternary Fission Performance Benchmarks..."
    
    local benchmark_args=()
    benchmark_args+=("--benchmark")
    benchmark_args+=("--config=${CONFIG_FILE}")
    benchmark_args+=("--events=${BENCHMARK_EVENTS:-10000}")
    
    if [[ "${LOG_LEVEL}" == "debug" ]]; then
        benchmark_args+=("--verbose")
    fi
    
    log_debug "Benchmark arguments: ${benchmark_args[*]}"
    
    # We run the benchmarks
    "${BIN_DIR}/ternary-engine" "${benchmark_args[@]}"
    
    log_info "Benchmarks completed"
}

# We start interactive shell mode
start_interactive_shell() {
    log_info "Starting Ternary Fission Interactive Shell..."
    
    exec "${BIN_DIR}/ternary-engine" --interactive --config="${CONFIG_FILE}"
}

# =============================================================================
# MAIN ENTRYPOINT LOGIC
# =============================================================================

# We display startup banner
echo "==============================================="
echo "Ternary Fission Energy Emulation System"
echo "Docker Container Entrypoint"
echo "Version: ${SCRIPT_VERSION}"
echo "Author: ${SCRIPT_AUTHOR}"
echo "Date: $(date '+%Y-%m-%d %H:%M:%S')"
echo "==============================================="

# We perform initialization steps
log_info "Initializing container environment..."

validate_directories
process_environment_variables
validate_configuration
check_system_health
wait_for_services
setup_signal_handlers

log_info "Container initialization completed successfully"

# We determine the command to execute
COMMAND="${1:-api-server}"
shift || true

log_info "Executing command: ${COMMAND}"

# We execute the appropriate command
case "${COMMAND}" in
    "api-server")
        start_api_server "$@"
        ;;
    "simulation-engine")
        start_simulation_engine "$@"
        ;;
    "daemon")
        start_daemon_mode "$@"
        ;;
    "benchmark")
        run_benchmark "$@"
        ;;
    "interactive")
        start_interactive_shell "$@"
        ;;
    "help"|"--help"|"-h")
        show_usage
        exit 0
        ;;
    *)
        log_error "Unknown command: ${COMMAND}"
        show_usage
        exit 1
        ;;
esac

log_info "Command execution completed"
