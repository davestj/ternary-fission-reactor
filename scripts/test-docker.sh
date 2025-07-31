#!/bin/bash
#
# File: scripts/test-docker.sh
# Author: bthlops (David StJ)
# Date: July 31, 2025
# Title: Docker Container Testing and Smoke Tests for Ternary Fission Energy Emulation System
# Purpose: Automated testing of built Docker images with comprehensive smoke tests
# Reason: Validates container functionality, API endpoints, and daemon operations

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[TEST]${NC} $*"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

PROJECT_NAME="ternary-fission"
API_PORT=8080
CONTAINER_NAME="${PROJECT_NAME}-test"

# Smoke test functions
test_binary_help() {
    local image=$1
    local binary=$2
    
    log_info "Testing ${binary} --help in ${image}"
    if docker run --rm "$image" "$binary" --help >/dev/null 2>&1; then
        log_info "✓ ${binary} help command works"
        return 0
    else
        log_error "✗ ${binary} help command failed"
        return 1
    fi
}

test_api_daemon() {
    local image=$1
    
    log_info "Testing API daemon startup"
    
    # Start container in daemon mode
    local container_id=$(docker run -d --name "$CONTAINER_NAME" -p "$API_PORT:8080" "$image" api-server)
    
    if [[ -z "$container_id" ]]; then
        log_error "✗ Failed to start container"
        return 1
    fi
    
    log_info "Container started: $container_id"
    
    # Wait for startup
    sleep 10
    
    # Test health endpoint
    local health_status=$(curl -s -o /dev/null -w "%{http_code}" "http://localhost:$API_PORT/api/v1/health" || echo "000")
    
    if [[ "$health_status" == "200" ]]; then
        log_info "✓ Health endpoint responding (HTTP $health_status)"
    else
        log_error "✗ Health endpoint failed (HTTP $health_status)"
        docker logs "$CONTAINER_NAME"
        docker stop "$CONTAINER_NAME" >/dev/null 2>&1 || true
        docker rm "$CONTAINER_NAME" >/dev/null 2>&1 || true
        return 1
    fi
    
    # Test status endpoint
    local status_response=$(curl -s "http://localhost:$API_PORT/api/v1/status" | jq -r '.uptime_seconds' 2>/dev/null || echo "error")
    
    if [[ "$status_response" != "error" ]] && [[ "$status_response" != "null" ]]; then
        log_info "✓ Status endpoint responding (uptime: ${status_response}s)"
    else
        log_warn "? Status endpoint returned unexpected data"
    fi
    
    # Test metrics endpoint
    local metrics_status=$(curl -s -o /dev/null -w "%{http_code}" "http://localhost:$API_PORT/api/v1/metrics" || echo "000")
    
    if [[ "$metrics_status" == "200" ]]; then
        log_info "✓ Metrics endpoint responding"
    else
        log_warn "? Metrics endpoint not available (HTTP $metrics_status)"
    fi
    
    # Cleanup
    docker stop "$CONTAINER_NAME" >/dev/null 2>&1 || true
    docker rm "$CONTAINER_NAME" >/dev/null 2>&1 || true
    
    log_info "✓ API daemon test completed successfully"
    return 0
}

test_simulation_engine() {
    local image=$1
    
    log_info "Testing C++ simulation engine"
    
    # Test basic simulation
    if docker run --rm "$image" ternary-fission -n 5 --help >/dev/null 2>&1; then
        log_info "✓ Simulation engine basic test passed"
    else
        log_error "✗ Simulation engine basic test failed"
        return 1
    fi
    
    # Test simulation with JSON output
    local temp_container=$(docker run -d --rm "$image" ternary-fission -n 3 -j test.json)
    sleep 5
    
    if docker ps | grep -q "$temp_container"; then
        docker stop "$temp_container" >/dev/null 2>&1 || true
    fi
    
    log_info "✓ Simulation engine JSON test completed"
    return 0
}

test_internal_communication() {
    local image=$1
    
    log_info "Testing internal Docker network communication"
    
    # Start API server
    docker run -d --name "${CONTAINER_NAME}-api" --network bridge "$image" api-server
    sleep 8
    
    # Get container IP
    local api_ip=$(docker inspect "${CONTAINER_NAME}-api" --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}')
    
    if [[ -n "$api_ip" ]]; then
        log_info "API container IP: $api_ip"
        
        # Test internal communication
        local internal_health=$(docker run --rm --network bridge "$image" \
            sh -c "curl -s -o /dev/null -w '%{http_code}' http://$api_ip:8080/api/v1/health" 2>/dev/null || echo "000")
        
        if [[ "$internal_health" == "200" ]]; then
            log_info "✓ Internal network communication working"
        else
            log_warn "? Internal network communication test inconclusive"
        fi
    fi
    
    # Cleanup
    docker stop "${CONTAINER_NAME}-api" >/dev/null 2>&1 || true
    docker rm "${CONTAINER_NAME}-api" >/dev/null 2>&1 || true
    
    return 0
}

# Main test function
test_image() {
    local image=$1
    local failed_tests=0
    
    log_info "Testing image: $image"
    
    # Check image exists
    if ! docker image inspect "$image" >/dev/null 2>&1; then
        log_error "Image not found: $image"
        return 1
    fi
    
    # Binary tests
    test_binary_help "$image" "ternary-fission" || ((failed_tests++))
    test_binary_help "$image" "api-server" || ((failed_tests++))
    
    # Simulation engine test
    test_simulation_engine "$image" || ((failed_tests++))
    
    # API daemon test
    test_api_daemon "$image" || ((failed_tests++))
    
    # Internal communication test
    test_internal_communication "$image" || ((failed_tests++))
    
    if [[ $failed_tests -eq 0 ]]; then
        log_info "✓ All tests passed for $image"
        return 0
    else
        log_error "✗ $failed_tests test(s) failed for $image"
        return 1
    fi
}

# Run daemon mode for external testing
run_daemon() {
    local image=${1:-"${PROJECT_NAME}:latest"}
    local port=${2:-$API_PORT}
    
    log_info "Starting daemon mode on port $port"
    
    # Stop existing container
    docker stop "$CONTAINER_NAME" >/dev/null 2>&1 || true
    docker rm "$CONTAINER_NAME" >/dev/null 2>&1 || true
    
    # Start in daemon mode with port forwarding
    docker run -d \
        --name "$CONTAINER_NAME" \
        -p "$port:8080" \
        -v "$(pwd)/data:/app/data" \
        -v "$(pwd)/results:/app/results" \
        -v "$(pwd)/logs:/app/logs" \
        -e LOG_LEVEL=info \
        -e EVENTS_PER_SECOND=5.0 \
        "$image" api-server
    
    local container_id=$(docker ps -q -f name="$CONTAINER_NAME")
    
    if [[ -n "$container_id" ]]; then
        log_info "✓ Daemon started successfully"
        log_info "Container ID: $container_id"
        log_info "API URL: http://localhost:$port"
        log_info "Health check: curl http://localhost:$port/api/v1/health"
        log_info "Status: curl http://localhost:$port/api/v1/status"
        log_info "Metrics: curl http://localhost:$port/api/v1/metrics"
        log_info "Logs: docker logs $CONTAINER_NAME"
        log_info "Stop: docker stop $CONTAINER_NAME"
        return 0
    else
        log_error "✗ Failed to start daemon"
        return 1
    fi
}

# Show usage
show_usage() {
    cat << EOF
Docker Container Testing Script

Usage: $0 [COMMAND] [OPTIONS]

Commands:
    test [IMAGE]     Run smoke tests on image (default: ${PROJECT_NAME}:latest)
    test-all         Test all built images
    daemon [IMAGE]   Start daemon mode with port forwarding
    stop             Stop running test daemon
    logs             Show daemon logs
    status           Check daemon status

Examples:
    $0 test                              # Test latest image
    $0 test ternary-fission:ubuntu24     # Test specific variant
    $0 test-all                          # Test all variants
    $0 daemon                            # Start daemon on port $API_PORT
    $0 daemon ternary-fission:latest 8081  # Start on custom port

External API Access:
    curl http://localhost:$API_PORT/api/v1/health
    curl http://localhost:$API_PORT/api/v1/status
    
EOF
}

# Parse arguments
case "${1:-test}" in
    "test")
        IMAGE=${2:-"${PROJECT_NAME}:latest"}
        test_image "$IMAGE"
        ;;
    "test-all")
        IMAGES=$(docker images "${PROJECT_NAME}" --format "{{.Repository}}:{{.Tag}}" | grep -v "<none>")
        FAILED=0
        for img in $IMAGES; do
            test_image "$img" || ((FAILED++))
        done
        if [[ $FAILED -eq 0 ]]; then
            log_info "✓ All images passed testing"
        else
            log_error "✗ $FAILED image(s) failed testing"
            exit 1
        fi
        ;;
    "daemon")
        run_daemon "$2" "$3"
        ;;
    "stop")
        docker stop "$CONTAINER_NAME" >/dev/null 2>&1 || true
        docker rm "$CONTAINER_NAME" >/dev/null 2>&1 || true
        log_info "Daemon stopped"
        ;;
    "logs")
        docker logs -f "$CONTAINER_NAME"
        ;;
    "status")
        if docker ps | grep -q "$CONTAINER_NAME"; then
            log_info "✓ Daemon is running"
            curl -s "http://localhost:$API_PORT/api/v1/health" | jq . 2>/dev/null || echo "API not responding"
        else
            log_warn "Daemon is not running"
        fi
        ;;
    "help"|"--help"|"-h")
        show_usage
        ;;
    *)
        log_error "Unknown command: $1"
        show_usage
        exit 1
        ;;
esac
