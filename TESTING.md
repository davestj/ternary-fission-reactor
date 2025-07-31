# Testing Guide - Ternary Fission Docker System

**Author:** bthlops (David StJ)  
**Date:** July 31, 2025  
**Purpose:** Complete testing and verification procedures for Docker builds and deployments  
**Reason:** Updated guide with confirmed working Docker Compose stack and verified API endpoints

**Change Log:**
- 2025-07-31: Updated with verified working Docker Compose deployment
- 2025-07-31: Added confirmed API endpoint testing procedures
- 2025-07-31: Updated container testing commands with working configurations
- 2025-07-31: Added real-world port mappings and verified response codes

**Carry-over Context:**
- Docker deployment now fully operational with confirmed multi-stage builds
- All primary API endpoints verified working including health, status, and metrics
- Container orchestration tested with proper graceful shutdown
- Web dashboard confirmed functional with educational content
- Next focus: comprehensive API testing and WebSocket validation

## 🚀 Quick Verification Commands (All Working)

### 1. Build and Start System
```bash
# Clone repository
git clone https://github.com/davestj/ternary-fission-reactor.git
cd ternary-fission-reactor

# Start Docker Compose stack (confirmed working)
docker-compose up -d

# Verify containers are running
docker-compose ps
```

### 2. Test All Working Endpoints
```bash
# Health check (confirmed 200 OK)
curl -f http://localhost:8080/api/v1/health

# System status (confirmed JSON response)
curl -s http://localhost:8080/api/v1/status | jq

# Prometheus metrics (confirmed working)
curl -s http://localhost:8080/api/v1/metrics | head -20

# Web dashboard (confirmed HTML response)
curl -s http://localhost:8080/ | grep "Ternary Fission"
```

### 3. Container Health Verification
```bash
# Check container status
docker-compose ps | grep "Up"

# Verify processes inside container
docker-compose exec ternary-fission-app ps aux

# Test C++ engine in container
docker-compose exec ternary-fission-app /app/bin/ternary-fission --help

# Check container resource usage
docker stats ternary-fission-emulator --no-stream
```

## ✅ Verified Working Features

### Container Infrastructure
- [x] **Multi-stage Docker builds** - Working correctly
- [x] **Container startup** - Proper initialization sequence
- [x] **Port mapping** - 8080:8080 confirmed functional
- [x] **Volume mounts** - Data persistence working
- [x] **Graceful shutdown** - SIGTERM handling verified
- [x] **Health checks** - Container health monitoring active

### API Server Functionality
- [x] **Go API Server** - Compiles and runs successfully
- [x] **Web Dashboard** - Educational interface accessible
- [x] **Health Endpoint** - `/api/v1/health` returns 200 OK
- [x] **Status Endpoint** - `/api/v1/status` returns JSON data
- [x] **Metrics Endpoint** - `/api/v1/metrics` Prometheus format
- [x] **Configuration Loading** - Config file parsing working
- [x] **CORS Support** - Cross-origin requests handled

### Physics Engine
- [x] **C++ Compilation** - GCC 13+ builds successful
- [x] **Command Line Interface** - All options functional
- [x] **Multi-threading** - Concurrent processing working
- [x] **JSON Output** - Statistics export functional
- [x] **Conservation Laws** - Physics validation active

## 🧪 Comprehensive Testing Procedures

### Smoke Test Suite (5 minutes)
```bash
#!/bin/bash
# smoke_test.sh - Quick verification of all systems

echo "=== Ternary Fission Smoke Test ==="

# 1. Container health
docker-compose ps | grep -q "Up" && echo "✅ Container: HEALTHY" || echo "❌ Container: FAILED"

# 2. API endpoints
curl -sf http://localhost:8080/api/v1/health >/dev/null && echo "✅ Health: PASS" || echo "❌ Health: FAIL"
curl -sf http://localhost:8080/api/v1/status >/dev/null && echo "✅ Status: PASS" || echo "❌ Status: FAIL"
curl -sf http://localhost:8080/api/v1/metrics >/dev/null && echo "✅ Metrics: PASS" || echo "❌ Metrics: FAIL"
curl -sf http://localhost:8080/ | grep -q "Ternary" && echo "✅ Dashboard: PASS" || echo "❌ Dashboard: FAIL"

# 3. Physics engine
docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 5 -p 235 >/dev/null && echo "✅ Physics: PASS" || echo "❌ Physics: FAIL"

echo "=== Smoke Test Complete ==="
```

### API Endpoint Testing
```bash
# Test all confirmed working endpoints
echo "Testing API endpoints..."

# Health check (should return 200)
echo -n "Health check: "
curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/api/v1/health

# Status endpoint (should return JSON)
echo -n "Status endpoint: "
curl -s http://localhost:8080/api/v1/status | jq -e '.uptime_seconds' >/dev/null && echo "JSON OK" || echo "JSON FAIL"

# Metrics endpoint (should return Prometheus format)
echo -n "Metrics endpoint: "
curl -s http://localhost:8080/api/v1/metrics | grep -q "ternary_fission" && echo "PROMETHEUS OK" || echo "PROMETHEUS FAIL"

# Dashboard (should return HTML)
echo -n "Dashboard: "
curl -s http://localhost:8080/ | grep -q "DOCTYPE html" && echo "HTML OK" || echo "HTML FAIL"
```

### Performance Testing
```bash
# Load test health endpoint
echo "Load testing health endpoint..."
ab -n 1000 -c 10 http://localhost:8080/api/v1/health

# Stress test status endpoint
echo "Testing status endpoint performance..."
time for i in {1..100}; do curl -s http://localhost:8080/api/v1/status >/dev/null; done

# Container resource monitoring during load
docker stats ternary-fission-emulator --no-stream
```

### Physics Engine Testing
```bash
# Test C++ engine functionality
echo "Testing physics engine..."

# Basic simulation test
docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 10 -p 235 -e 6.5

# Multi-threaded test
docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 100 -t 4 -j /tmp/test.json

# Continuous mode test (5 seconds)
timeout 10s docker-compose exec ternary-fission-app /app/bin/ternary-fission -c -d 5 -r 10

# Interactive mode test
echo -e "event 3\nstats\nexit\n" | docker-compose exec -T ternary-fission-app /app/bin/ternary-fission -x
```

## 🔍 Advanced Testing Scenarios

### Container Network Testing
```bash
# Test internal container networking
docker network ls | grep ternary

# Test container-to-container communication
docker-compose exec ternary-fission-app curl -s http://localhost:8080/api/v1/health

# Test external connectivity from container
docker-compose exec ternary-fission-app ping -c 3 google.com
```

### Configuration Testing
```bash
# Test custom configuration loading
docker-compose exec ternary-fission-app cat /app/configs/ternary_fission.conf

# Test environment variable overrides
docker-compose exec ternary-fission-app env | grep TERNARY

# Test configuration parsing
docker-compose logs ternary-fission-app | grep "configuration"
```

### Memory and Resource Testing
```bash
# Monitor memory usage during simulation
docker stats ternary-fission-emulator &
STATS_PID=$!

# Run memory-intensive simulation
docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 10000 -t 8 -j /tmp/memory_test.json

# Stop monitoring
kill $STATS_PID

# Check for memory leaks
docker-compose exec ternary-fission-app free -h
```

### API Stress Testing
```bash
# Concurrent API requests
echo "Running concurrent API tests..."

# Multiple health checks
for i in {1..10}; do
  curl -s http://localhost:8080/api/v1/health &
done
wait

# Status endpoint stress test
for i in {1..5}; do
  curl -s http://localhost:8080/api/v1/status | jq .cpu_usage_percent &
done
wait

# Metrics endpoint load test
for i in {1..3}; do
  curl -s http://localhost:8080/api/v1/metrics | wc -l &
done
wait
```

## 🚨 Troubleshooting Guide

### Container Issues
```bash
# Container won't start
docker-compose ps
docker-compose logs ternary-fission-app

# Port conflicts
netstat -tulpn | grep 8080
lsof -i :8080

# Permission issues
docker-compose exec ternary-fission-app whoami
docker-compose exec ternary-fission-app ls -la /app/bin/

# Container restart
docker-compose restart ternary-fission-app
```

### API Issues
```bash
# API not responding
curl -v http://localhost:8080/api/v1/health

# Check API server process
docker-compose exec ternary-fission-app ps aux | grep api-server

# Review API logs
docker-compose logs ternary-fission-app | grep -i error

# Test with different endpoints
curl http://localhost:8080/api/v1/status
curl http://localhost:8080/
```

### Performance Issues
```bash
# High CPU usage
docker stats ternary-fission-emulator

# Memory consumption
docker-compose exec ternary-fission-app free -h

# Disk space
docker-compose exec ternary-fission-app df -h

# Network connectivity
docker-compose exec ternary-fission-app netstat -tuln
```

## 📊 Test Results Validation

### Expected Response Codes
| Endpoint | Method | Expected Code | Content Type |
|----------|--------|---------------|--------------|
| `/api/v1/health` | GET | 200 | application/json |
| `/api/v1/status` | GET | 200 | application/json |
| `/api/v1/metrics` | GET | 200 | text/plain |
| `/` | GET | 200 | text/html |

### Performance Benchmarks
- **Health endpoint**: < 5ms response time
- **Status endpoint**: < 50ms response time
- **Metrics endpoint**: < 100ms response time
- **Container startup**: < 30 seconds
- **Physics simulation**: > 100 events/second

### Resource Usage Limits
- **Memory usage**: < 1GB under normal load
- **CPU usage**: < 50% with 4 cores
- **Disk usage**: < 100MB for logs
- **Network**: < 10MB/hour typical usage

## 🔄 Continuous Testing

### Automated Test Script
```bash
#!/bin/bash
# continuous_test.sh - Run tests every 5 minutes

while true; do
  echo "$(date): Running automated tests..."
  
  # Quick health check
  curl -sf http://localhost:8080/api/v1/health >/dev/null
  if [ $? -eq 0 ]; then
    echo "$(date): ✅ Health check PASS"
  else
    echo "$(date): ❌ Health check FAIL - System may need attention"
  fi
  
  # Container status
  docker-compose ps | grep -q "Up"
  if [ $? -eq 0 ]; then
    echo "$(date): ✅ Container status HEALTHY"
  else
    echo "$(date): ❌ Container status UNHEALTHY"
  fi
  
  sleep 300  # Wait 5 minutes
done
```

### Integration with CI/CD
```yaml
# .github/workflows/docker-test.yml
name: Docker Integration Tests
on: [push, pull_request]

jobs:
  docker-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Start Docker Compose
        run: docker-compose up -d
      - name: Wait for services
        run: sleep 30
      - name: Test health endpoint
        run: curl -f http://localhost:8080/api/v1/health
      - name: Test status endpoint
        run: curl -s http://localhost:8080/api/v1/status | jq
      - name: Test metrics endpoint
        run: curl -s http://localhost:8080/api/v1/metrics | grep ternary_fission
      - name: Test physics engine
        run: docker-compose exec -T ternary-fission-app /app/bin/ternary-fission -n 5
      - name: Cleanup
        run: docker-compose down
```

## 📈 Monitoring and Alerting

### Health Monitoring
```bash
# Set up health monitoring
watch -n 10 'curl -s http://localhost:8080/api/v1/health | jq'

# Monitor system metrics
watch -n 5 'curl -s http://localhost:8080/api/v1/status | jq ".cpu_usage_percent, .memory_usage_percent"'

# Container resource monitoring
watch -n 5 'docker stats ternary-fission-emulator --no-stream'
```

### Log Analysis
```bash
# Monitor application logs
docker-compose logs -f --tail=50 ternary-fission-app

# Search for errors
docker-compose logs ternary-fission-app | grep -i error

# Performance analysis
docker-compose logs ternary-fission-app | grep "response time"
```

## 🧹 Cleanup Commands

### Standard Cleanup
```bash
# Stop all services
docker-compose down

# Remove containers and networks
docker-compose down --remove-orphans

# Clean up volumes (CAUTION: removes data)
docker-compose down --volumes
```

### Deep Cleanup
```bash
# Remove all project images
docker images ternary-fission* --format "{{.ID}}" | xargs -r docker rmi

# Clean unused Docker resources
docker system prune -f

# Full Docker cleanup (CAUTION: affects all Docker resources)
docker system prune -af --volumes
```

---

**Testing Status**: Docker deployment fully functional, all primary API endpoints verified working, comprehensive testing framework operational and ready for extensive validation.