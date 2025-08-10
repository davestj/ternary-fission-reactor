# Ternary Fission Reactor - Complete Usage Guide

**Author:** bthlops (David StJ)  
**Date:** July 31, 2025  
**Title:** Complete usage guide for ternary fission simulation with working Docker deployment  
**Purpose:** Comprehensive examples from basic to extreme simulation scenarios including verified Docker workflows
**Reason:** Updated guide reflecting current working status with Docker Compose operational and Go API server confirmed functional

**Change Log:**
- 2025-07-31: Updated with working Docker Compose deployment and verified API endpoints
- 2025-07-31: Added Docker testing procedures and container-based workflows
- 2025-07-31: Confirmed web dashboard access and REST API functionality
- 2025-07-31: Updated port configurations to reflect actual runtime behavior

**Carry-over Context:**
- Docker deployment now fully functional with multi-stage builds working correctly
- Go API server operational with dashboard, health checks, and metrics endpoints verified
- Container orchestration tested with proper graceful shutdown capabilities
- All examples updated to reflect current working configuration

**Release Candidate:** v2.0.0-rc1 — see [ARCH.md](ARCH.md) for architecture and [NEXT-STEPS.md](NEXT-STEPS.md) for roadmap milestones

## 🚀 Quick Start (Docker - Recommended)

### Fastest Way to Get Started
```bash
# Clone and start with Docker
git clone https://github.com/davestj/ternary-fission-reactor.git
cd ternary-fission-reactor

# Start the complete system
docker-compose up -d

# Access web dashboard (confirmed working)
open http://localhost:8080

# Test API health (verified endpoint)
curl http://localhost:8080/api/v1/health
```

### Verify System is Working
```bash
# Check container status
docker-compose ps

# View real-time logs
docker-compose logs -f ternary-fission-app

# Test all main endpoints
curl http://localhost:8080/api/v1/health    # Health check
curl http://localhost:8080/api/v1/status    # System status  
curl http://localhost:8080/api/v1/metrics   # Prometheus metrics

# Access web dashboard
curl http://localhost:8080/                 # Educational interface
```

### Static Files and Streaming
`web_root` serves static assets while `media_root` provides optional streaming content.

```bash
curl http://localhost:8333/index.html     # Fetch static test page
curl http://localhost:8333/stream         # Stream media content
```

## 🔧 Dependencies

For native builds, install the header-only [cpp-httplib](https://github.com/yhirose/cpp-httplib) library:

```bash
# Debian/Ubuntu
sudo apt-get install libhttplib-dev

# macOS
brew install cpp-httplib
```

The project also vendors the header at `third_party/cpp-httplib/httplib.h` if package installation isn't possible.

## 📊 Working API Endpoints (Verified)

Based on confirmed testing, these endpoints are operational:

| Endpoint | Method | Status | Description |
|----------|--------|--------|-------------|
| `/` | GET | ✅ Working | Web dashboard with project information |
| `/api/v1/health` | GET | ✅ Working | Health check (returns 200 OK) |
| `/api/v1/status` | GET | ✅ Working | System status JSON response |
| `/api/v1/metrics` | GET | ✅ Working | Prometheus metrics endpoint |
| `/api/v1/energy-fields` | POST/GET | ⚠️ Testing | Energy field management |
| `/api/v1/simulate/fission` | POST | ⚠️ Testing | Fission simulation trigger |
| `/api/v1/ws/monitor` | WS | ⚠️ Testing | WebSocket monitoring |

## 🐳 Docker-Based Workflows

### Container Management
```bash
# Start services
docker-compose up -d

# Stop services
docker-compose down

# Rebuild and start
docker-compose up --build

# View service status
docker-compose ps

# Scale services (if needed)
docker-compose up -d --scale ternary-fission-app=2
```

### Development with Docker
```bash
# Development mode with live logs
docker-compose up

# Background mode for development
docker-compose up -d && docker-compose logs -f ternary-fission-app

# Restart just the main service
docker-compose restart ternary-fission-app

# Execute commands in running container
docker-compose exec ternary-fission-app /bin/bash
```

### Container Testing and Debugging
```bash
# Test C++ physics engine in container
docker-compose exec ternary-fission-app /app/bin/ternary-fission --help

# Run simulation in container
docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 10 -p 235 -e 6.5

# Check container resource usage
docker stats ternary-fission-emulator

# Inspect container configuration
docker inspect ternary-fission-emulator
```

## 🖥️ Native Command Line Interface

### Quick Reference
```bash
# Basic help
./ternary-fission -h

# Start API server natively
./ternary-api -config configs/ternary_fission.conf -port 8080
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-h, --help` | Show help message | - |
| `-p, --parent <mass>` | Parent nucleus mass (AMU) | 235.0 |
| `-e, --excitation <MeV>` | Excitation energy | 6.5 |
| `-n, --events <N>` | Number of events | 10 |
| `-t, --threads <N>` | Worker threads | auto |
| `-c, --continuous` | Continuous mode | disabled |
| `-d, --duration <sec>` | Duration (continuous) | 10 |
| `-r, --rate <N>` | Events per second | 10 |
| `-j, --json [file]` | JSON output | simulation_stats.json |
| `-x, --repl` | Interactive mode | disabled |
| `-l, --logdir <path>` | Log directory | ./logs |

### Basic Usage Examples

#### Simple Simulation
```bash
# Default U-235 simulation (10 events)
./ternary-fission

# U-238 with 100 events
./ternary-fission -n 100 -p 238

# Custom excitation energy
./ternary-fission -n 50 -p 235 -e 7.5
```

#### JSON Output
```bash
# Save statistics to JSON
./ternary-fission -n 1000 -j results.json

# Custom JSON filename
./ternary-fission -n 500 -j experiment_data.json
```

## 🧪 Medium Intensity Simulations

### Multi-threaded Processing
```bash
# Use 8 threads for 5000 events
./ternary-fission -n 5000 -t 8 -p 238 -e 7.1

# Maximum CPU utilization
./ternary-fission -n 10000 -t 16 -p 239 -e 8.0 -j high_intensity.json
```

### Continuous Mode
```bash
# Run for 60 seconds at 25 events/sec
./ternary-fission -c -d 60 -r 25 -t 8

# High rate continuous simulation
./ternary-fission -c -d 120 -r 50 -p 238 -e 7.5 -t 12
```

### Research-Grade Simulations
```bash
# Large dataset generation
./ternary-fission -n 50000 -p 235 -e 6.5 -t 16 -j research_u235.json

# Comparative isotope study
./ternary-fission -n 25000 -p 233 -e 6.0 -t 8 -j u233_data.json
./ternary-fission -n 25000 -p 235 -e 6.5 -t 8 -j u235_data.json  
./ternary-fission -n 25000 -p 238 -e 7.1 -t 8 -j u238_data.json
```

## ⚡ Extreme Intensity Simulations

### Maximum Performance
```bash
# Extreme simulation: 100K events, 32 threads, high energy
./ternary-fission -n 100000 -t 32 -p 238 -e 9.5 -j extreme_simulation.json

# Continuous extreme: 10 minutes at maximum rate
./ternary-fission -c -d 600 -r 100 -t 32 -p 239 -e 10.0 -j extreme_continuous.json

# Research cluster simulation
./ternary-fission -n 1000000 -t 64 -p 238 -e 8.5 -j million_events.json
```

### High-Energy Physics Scenarios
```bash
# Very high excitation energy simulation
./ternary-fission -n 50000 -p 238 -e 12.0 -t 24 -j high_energy.json

# Super-heavy element simulation
./ternary-fission -n 25000 -p 252 -e 15.0 -t 16 -j superheavy.json

# Extreme energy with maximum threads
./ternary-fission -n 200000 -p 240 -e 20.0 -t 48 -c -d 1800 -r 75 -j extreme_energy.json
```

### Production Batch Processing
```bash
# 24-hour continuous simulation
./ternary-fission -c -d 86400 -r 30 -t 24 -p 238 -e 7.8 -j daily_production.json

# High-throughput data generation
./ternary-fission -n 5000000 -t 64 -p 235 -e 6.5 -j production_dataset.json
```

## 🔄 Interactive Mode Examples

### REPL Commands
```bash
# Start interactive mode
./ternary-fission -x

# Commands in REPL:
> event 1        # Simulate 1 event
> event 10       # Simulate 10 events  
> status         # Show system status
> stats          # Show JSON statistics
> json output.json # Save stats to file
> exit           # Quit
```

### Interactive Workflows
```bash
# Interactive exploration
./ternary-fission -x
> event 5                    # Test with 5 events
> stats                      # Check results
> event 100                  # Scale up to 100
> json initial_test.json     # Save results
> event 1000                 # Production run
> json final_results.json    # Save final data
> exit
```

## 🌐 API Server Usage (Working)

### Starting the Server

#### Docker Method (Recommended)
```bash
# Start with Docker Compose (confirmed working)
docker-compose up -d

# Check server status
curl http://localhost:8080/api/v1/health

# Access web dashboard
open http://localhost:8080
```

#### Native Method
```bash
# Basic API server
./ternary-api -config configs/ternary_fission.conf -port 8080

# Custom configuration
./ternary-api -config custom.conf -port 9090 -log-level debug

# Production deployment
./ternary-api -config production.conf -port 8080 > api.log 2>&1 &
```

### API Endpoints Testing (Verified Working)
```bash
# Test server health (confirmed 200 OK)
curl http://localhost:8080/api/v1/health

# Get system status (confirmed JSON response)
curl http://localhost:8080/api/v1/status | jq

# Get Prometheus metrics (confirmed working)
curl http://localhost:8080/api/v1/metrics

# Access web dashboard (confirmed working)
curl http://localhost:8080/

# Create energy field (endpoint exists, testing in progress)
curl -X POST http://localhost:8080/api/v1/energy-fields \
  -H "Content-Type: application/json" \
  -d '{"initial_energy_mev": 500.0, "auto_dissipate": true}'

# WebSocket monitoring (endpoint exists, testing in progress)
# wscat -c ws://localhost:8080/api/v1/ws/monitor
```

## 🔧 Performance Optimization

### Memory Management
```bash
# Monitor memory usage during simulation
./ternary-fission -n 100000 -t 16 -l /tmp/logs &
watch -n 1 'ps aux | grep ternary-fission'

# Large simulation with memory tracking
time ./ternary-fission -n 500000 -t 32 -j memory_test.json

# Docker memory monitoring
docker stats ternary-fission-emulator
```

### CPU Optimization
```bash
# Test optimal thread count
for threads in 4 8 16 24 32; do
  echo "Testing $threads threads"
  time ./ternary-fission -n 10000 -t $threads -j "test_${threads}t.json"
done

# CPU affinity for extreme performance
taskset -c 0-31 ./ternary-fission -n 1000000 -t 32 -j affinity_test.json

# Docker CPU monitoring
docker exec ternary-fission-emulator htop
```

## 📊 Benchmarking Examples

### Performance Testing
```bash
# Standard benchmark
./ternary-fission -n 10000 -t 8 -j benchmark_standard.json

# Speed test various configurations
time ./ternary-fission -n 50000 -t 16 -p 235    # U-235
time ./ternary-fission -n 50000 -t 16 -p 238    # U-238  
time ./ternary-fission -n 50000 -t 16 -p 239    # Pu-239

# Continuous throughput test
./ternary-fission -c -d 300 -r 100 -t 32 -j throughput_test.json
```

### Docker Performance Testing
```bash
# Container performance benchmark
docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 10000 -t 8 -j /tmp/docker_benchmark.json

# Compare native vs container performance
time ./ternary-fission -n 10000 -t 8 -j native_benchmark.json
time docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 10000 -t 8 -j /tmp/container_benchmark.json

# API server performance testing
ab -n 1000 -c 10 http://localhost:8080/api/v1/health
```

### Scaling Analysis
```bash
# Event count scaling
for events in 1000 5000 10000 50000 100000; do
  time ./ternary-fission -n $events -t 16 -j "scale_${events}.json"
done

# Thread scaling analysis
for threads in 1 2 4 8 16 32; do
  time ./ternary-fission -n 25000 -t $threads -j "thread_${threads}.json"
done
```

## 🏭 Production Workflows

### Data Collection Pipeline
```bash
#!/bin/bash
# production_run.sh - Automated data collection

# Morning collection (8 hours) using Docker
docker-compose exec -d ternary-fission-app /app/bin/ternary-fission -c -d 28800 -r 25 -t 16 -j /app/data/morning_$(date +%Y%m%d).json

# Wait and collect evening data
sleep 28800
docker-compose exec -d ternary-fission-app /app/bin/ternary-fission -c -d 28800 -r 25 -t 16 -j /app/data/evening_$(date +%Y%m%d).json
```

### Batch Processing with Docker
```bash
#!/bin/bash
# batch_isotopes_docker.sh - Multi-isotope analysis in containers

isotopes=(233 235 238 239 241)
energies=(6.0 6.5 7.1 7.8 8.2)

for i in "${!isotopes[@]}"; do
  echo "Processing U-${isotopes[$i]} in container"
  docker-compose exec ternary-fission-app /app/bin/ternary-fission \
    -n 100000 -p ${isotopes[$i]} -e ${energies[$i]} \
    -t 24 -j "/app/results/batch_u${isotopes[$i]}.json"
done
```

## 🚨 Error Handling and Troubleshooting

### Docker Troubleshooting
```bash
# Check Docker service status
docker-compose ps

# View real-time logs
docker-compose logs -f ternary-fission-app

# Restart services
docker-compose restart ternary-fission-app

# Rebuild and restart
docker-compose down && docker-compose up --build

# Debug container issues
docker-compose exec ternary-fission-app /bin/bash
```

### Debug Mode
```bash
# Verbose logging
./ternary-fission -n 100 -l /tmp/debug_logs

# Check log output
tail -f /tmp/debug_logs/event.log

# Memory debugging
valgrind ./ternary-fission -n 10 --help

# Docker container debugging
docker-compose exec ternary-fission-app bash -c "ps aux && free -h && df -h"
```

### Performance Issues
```bash
# Check system resources
htop                    # Monitor CPU/memory
iotop                   # Monitor disk I/O

# Docker resource monitoring
docker stats ternary-fission-emulator

# Test with reduced load
./ternary-fission -n 1000 -t 4 -r 5    # Reduced intensity

# Container resource limits
docker-compose exec ternary-fission-app bash -c "cat /proc/cpuinfo | grep processor | wc -l"
```

## 🔗 Integration Examples

### Docker Integration (Primary Method)
```bash
# Run simulation in existing container
docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 1000 -j /app/data/results.json

# API server already running in container  
curl http://localhost:8080/api/v1/status

# Mount external data directory
docker-compose -f docker-compose.yml -f docker-compose.override.yml up -d
```

### Kubernetes Integration (Future)
```bash
# Batch job (not yet tested)
kubectl create job ternary-sim --image=ternary-fission \
  -- ./ternary-fission -n 100000 -t 16 -j /results/batch.json

# Service deployment (not yet tested)
kubectl create deployment ternary-api --image=ternary-fission \
  -- ./ternary-api -config /config/production.conf
```

## ⚙️ Advanced Configuration

### Docker Configuration
```bash
# Override default configuration
cp configs/ternary_fission.conf configs/custom.conf
# Edit configs/custom.conf as needed

# Use custom configuration with Docker
docker-compose exec ternary-fission-app /app/bin/ternary-api -config /app/configs/custom.conf -port 8080
```

### Environment Variables
```bash
# Set environment overrides for Docker
export TERNARY_THREADS=32
export TERNARY_LOG_LEVEL=debug
docker-compose up -d

# Native environment variables
export TERNARY_THREADS=32
export TERNARY_LOG_LEVEL=debug
./ternary-fission -n 50000
```

## ✅ Validation and Verification

### Conservation Law Checking
```bash
# Check conservation laws (native)
./ternary-fission -n 1000 -j conservation_test.json
grep "Conservation:" logs/event.log | grep "FAIL"

# Check conservation laws (Docker)
docker-compose exec ternary-fission-app /app/bin/ternary-fission -n 1000 -j /app/data/conservation_test.json

# Physics validation
./ternary-fission -n 10000 -p 235 -e 6.5 -j validation.json
```

### API Validation (Working Endpoints)
```bash
# Validate all working endpoints
curl -s http://localhost:8080/api/v1/health | grep -q "healthy" && echo "Health check: PASS"
curl -s http://localhost:8080/api/v1/status | jq -e '.uptime_seconds' && echo "Status endpoint: PASS"
curl -s http://localhost:8080/api/v1/metrics | grep -q "ternary_fission" && echo "Metrics endpoint: PASS"
curl -s http://localhost:8080/ | grep -q "Ternary Fission" && echo "Dashboard: PASS"
```

### Docker System Validation
```bash
# Validate Docker system health
docker-compose ps | grep "Up" && echo "Container status: HEALTHY"
docker-compose exec ternary-fission-app /app/bin/ternary-fission --help | grep -q "Usage:" && echo "C++ engine: WORKING"
curl -s http://localhost:8080/api/v1/health && echo "API connectivity: WORKING"
```

## 📈 Monitoring and Observability

### Real-time Monitoring
```bash
# Monitor Docker containers
watch docker stats ternary-fission-emulator

# Monitor API performance
watch -n 5 'curl -s http://localhost:8080/api/v1/status | jq .cpu_usage_percent'

# Monitor application logs
docker-compose logs -f --tail=100 ternary-fission-app
```

### Metrics Collection
```bash
# Collect Prometheus metrics
curl http://localhost:8080/api/v1/metrics > metrics_$(date +%Y%m%d_%H%M%S).txt

# System resource monitoring
docker exec ternary-fission-emulator ps aux > process_snapshot.txt
docker exec ternary-fission-emulator free -h > memory_snapshot.txt
```

---

**For questions or support:**
- Email: davestj@gmail.com
- GitHub: https://github.com/davestj/ternary-fission-reactor
- Website: https://davestj.com
- Stargate Framework: https://davestj.com/bthl/stargate_framework/

**Current Status**: Docker deployment confirmed working, Go API server operational with verified endpoints, comprehensive testing framework ready for validation.