# Ternary Fission Energy Emulation System

![Build Status](https://github.com/davestj/ternary-fission-reactor/actions/workflows/build-and-release.yml/badge.svg)
![Release](https://img.shields.io/github/v/release/davestj/ternary-fission-reactor?include_prereleases)
![License](https://img.shields.io/github/license/davestj/ternary-fission-reactor)
![Language](https://img.shields.io/badge/C%2B%2B-17-blue)
![Go Version](https://img.shields.io/badge/Go-1.23%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Ubuntu%2024.04-green)
![v2.0.0-rc1](https://img.shields.io/badge/Version-2.0.0--rc1-orange)

**Author:** bthlops (David StJ)  
**Version:** 1.1.15-alpha (Latest: [Releases](https://github.com/davestj/ternary-fission-reactor/releases))  
**Last Updated:** July 31, 2025  
**Repository:** https://github.com/davestj/ternary-fission-reactor

High-performance ternary nuclear fission simulation with C++ physics engine, Go REST API, and Docker deployment. Maps nuclear energy to computational resources with real-time monitoring and conservation law verification using multi-base mathematical frameworks.


## Overview

The system emulates ternary nuclear fission to explore how computational resources map to energy production. It pairs a high-speed C++ physics core with a Go-based API and web dashboard for monitoring and interaction.

## Version 2.0.0-rc1 Highlights

- Distributed daemon architecture bridging C++ and Go components
- Docker-first deployment with real-time monitoring stack
- Kubernetes-ready configuration and release workflows
- See [ARCH.md](ARCH.md), [HOW-TO.md](HOW-TO.md), [docs/BENCHMARKING.md](docs/BENCHMARKING.md), and [NEXT-STEPS.md](NEXT-STEPS.md) for RC milestone details

## Architecture

See [ARCH.md](ARCH.md) for a complete description of the physics engine, API server, monitoring stack, and container layout.

## Build Prerequisites

- Ubuntu 24.04 with build-essential
- GCC 13+ and C++17 libraries
- Go 1.23+
- Optional: Docker and Docker Compose for containerized runs

## Build and Test

```bash
make cpp-build       # build C++ components
make go-build        # build Go components
make cpp-test        # run C++ unit tests
make static-analysis # run static analysis
```

## Configuration Examples

```ini
# configs/ternary_fission.conf
parent_mass=235.0
excitation_energy=6.5
web_root=webroot
media_streaming_enabled=false
```

## Environment Variables

See [docs/ENVIRONMENT.md](docs/ENVIRONMENT.md) for default values, valid options, and usage examples.

## Running the Daemon and Dashboard

```bash
# Run API daemon
./bin/ternary-api -config configs/ternary_fission.conf &

# Access web dashboard
open http://localhost:8080
```

## Clustering & Load Balancing

The daemon layer scales horizontally using an eight-node ring topology. Nodes are numbered `node0` through `node7`, each maintaining a persistent connection to its immediate neighbors. Workloads are sharded with `shard = hash(task_id) % 8` and routed to the node owning that shard.

- Each node owns one primary shard and serves as standby for its predecessor's shard
- Neighbor heartbeats detect failures and trigger automatic shard reassignment
- Recovery reintegrates the node and rebalances shards across the ring

This strategy provides even load distribution while ensuring any single node failure is absorbed by the remaining ring without service interruption.

## 📊 Current Development Status (v1.1.15-alpha)

### ✅ Fully Working Components
- **C++ Physics Engine**: Fully functional ternary fission simulation with conservation law verification
- **Go REST API Server**: **WORKING** - Compiles, runs, serves dashboard and API endpoints
- **Docker Deployment**: **WORKING** - Multi-stage builds, proper health checks, graceful shutdown
- **Docker Compose Stack**: **WORKING** - Tested with ARM64/AMD64 compatibility
- **Web Dashboard**: **WORKING** - Immersive educational interface with project information
- **Multi-Base Mathematics**: Base-3, Base-5, Base-8, Base-17 implementations complete
- **CLI Interface**: Complete command-line tool with all options and interactive mode
- **Build System**: Automated CI/CD with GitHub Actions, cross-platform compatibility

### ✅ Operational Endpoints
- **Web Dashboard**: `http://localhost:8080/` - Educational project interface
- **Health Check**: `http://localhost:8080/api/v1/health` - System health monitoring
- **API Status**: `http://localhost:8080/api/v1/status` - Real-time system metrics
- **Prometheus Metrics**: `http://localhost:8080/api/v1/metrics` - Performance monitoring
- **API Documentation**: `http://localhost:8080/api/v1/` - REST API reference

### Static Files and Media Streaming
- **Static Files** (`web_root`): `http://localhost:8333/index.html` - Files served from the configurable web root
- **Media Stream** (`media_root`): `http://localhost:8333/stream` - Icecast stream when `media_streaming_enabled` is true

```bash
curl http://localhost:8333/index.html      # Fetch static test page
curl http://localhost:8333/stream          # Stream media content
```

### Web Dashboard Structure
- **Atoms** (`web/atoms`): buttons, inputs, and shared styles
- **Molecules** (`web/molecules`): login form and status cards
- **Organisms** (`web/organisms`): navigation bar and metrics panel
- **Templates** (`web/templates`): layout shells
- **Pages** (`web/pages`): entry points `index.html` and `login.html`

Set `web_root=webroot` so the C++ HTTP server serves the `webroot/` directory.
Open `http://localhost:8333/login.html` to authenticate and reach the dashboard. The metrics panel polls `/api/v1/metrics` and management controls send commands such as `POST /api/v1/control/shutdown`.

### ⚠️ In Testing/Validation Phase
- **WebSocket Monitoring**: Basic implementation, real-time validation ongoing
- **Energy Field Management**: Core functionality present, comprehensive testing needed
- **Cross-Platform Testing**: Ubuntu 24.04 verified, other distributions pending
- **Performance Optimization**: Memory pool improvements and load testing required
- **Media Streaming Control**: Start and stop Icecast streams via `/api/v1/stream/start` and `/api/v1/stream/stop`

### 🎯 Updated Release Timeline
- **v1.1.25**: Complete API testing → **v1.2.1-beta** (Full feature completion)
- **v1.2.100**: Performance optimization → **v1.3.1** Gold Release
- **Current Focus**: Comprehensive API testing and Kubernetes integration

## Related Documentation

- [HOW-TO.md](HOW-TO.md) - Complete usage guide with examples from basic to extreme simulations
- [docs/MATH_PRIMER.md](docs/MATH_PRIMER.md) - Base-3/5/8/17 arithmetic for energy distribution
- [docs/BENCHMARKING.md](docs/BENCHMARKING.md) - Benchmark presets for repeatable runs
- [TESTING.md](TESTING.md) - Docker testing procedures and verification commands
- [NEXT-STEPS.md](NEXT-STEPS.md) - Development roadmap and technical debt analysis
- [BUILD_CARRYOVER.md](BUILD_CARRYOVER.md) - Development context and architecture overview
- [RELEASE.md](RELEASE.md) - Commit impact summary

## 🚀 Quick Start

### Docker Deployment (Recommended)
```bash
# Clone repository
git clone https://github.com/davestj/ternary-fission-reactor.git
cd ternary-fission-reactor

# Start with Docker Compose
docker-compose up -d

# Access web dashboard
open http://localhost:8080

# Check system status
curl http://localhost:8080/api/v1/health
```

### Native Development Build
```bash
# Ubuntu/Debian dependencies
sudo apt update && sudo apt install -y \
  build-essential gcc-13 g++-13 cmake make \
  libssl-dev libcrypto++-dev libgsl-dev \
  libeigen3-dev libfftw3-dev libopenblas-dev \
  liblapack-dev libboost-all-dev \
  libhttplib-dev

# macOS dependencies
brew install jsoncpp openssl@3 cpp-httplib

# Build all components
make all

# Build only C++ components
make cpp-build

# The cpp-httplib header is vendored under `third_party/cpp-httplib/httplib.h` if package managers are unavailable.

# Run API server
./bin/ternary-api -config configs/ternary_fission.conf -port 8080

# Run physics simulation
./bin/ternary-fission -n 1000 -p 235 -e 6.5 -j results.json
```

### Production Release Installation
```bash
# Download latest production release
wget https://github.com/davestj/ternary-fission-reactor/releases/latest/download/ternary-fission-reactor-v1.1.1-prod.tar.gz
tar -xzf ternary-fission-reactor-v1.1.1-prod.tar.gz

# Install system-wide
wget https://github.com/davestj/ternary-fission-reactor/releases/latest/download/install.sh
sudo ./install.sh
```

## 📦 Download Options

### Development Artifacts (dev branch)
- `ternary-fission-reactor-VERSION-dev-release.tar.gz` - Development binaries (tar.gz)
- `ternary-fission-reactor-VERSION-dev-release.zip` - Development binaries (zip)
- `ternary-fission-reactor-VERSION-dev-debug.tar.gz` - Debug build with symbols
- `ternary-fission-reactor-VERSION-dev-source.tar.gz` - Complete source with tests

### Production Releases (master branch)
- `ternary-fission-reactor-VERSION-prod.tar.gz` - Production binaries (tar.gz)
- `ternary-fission-reactor-VERSION-prod.zip` - Production binaries (zip)
- `ternary-fission-reactor-VERSION-source.tar.gz` - Clean source archive
- `install.sh` - Automated system installer

## Release Strategy

- See [docs/RELEASE_STRATEGY.md](docs/RELEASE_STRATEGY.md) for full details.

- **dev branch**: push-only CI for rapid iteration
- **future branch**: CI on pushes and pull requests for staging releases
- **master branch**: production releases after `dev` merges
- **Promotion**: release candidates cut from `dev`, promoted to beta on `future`, then merged to `master`
- **Packages**: Debian and Ubuntu `.deb` artifacts published for each production release

## 🏗️ Build Pipeline

### Branch Strategy
- **dev** → Development builds with debug symbols and test coverage
- **future** → Staging branch with push/PR CI
- **master** → Production releases with optimized binaries
- **feature/** → Pull request builds for testing
- **v*tags** → Tagged releases with semantic versioning

### Automated Builds
| Branch | Build Types | Artifacts | Retention | Release |
|--------|-------------|-----------|-----------|---------|
| dev | release + debug | tar.gz + zip | 14 days | No |
| future | release + debug | tar.gz + zip | 14 days | No |
| master | release only | tar.gz + zip + deb | 90 days | Yes |
| PR | release + debug | tar.gz + zip | 7 days | No |
| Tags | release only | tar.gz + zip | Permanent | Yes |

### Version Strategy
- **Alpha** (dev): `1.1.1-alpha.X` - Development builds
- **Beta** (future): `1.1.1-beta.X` - Pre-release testing
- **RC** (future): `1.1.1-rc.X` - Release candidates
- **Release** (tags): `1.1.1` - Production releases

## 🏢 Architecture

### Core Components
```
┌─────────────────────────────────────────────────────────┐
│                  Client Layer                           │
│  Web Dashboard, API Clients, Monitoring Dashboards     │
└─────────────────────────────────────────────────────────┘
                           │
                    HTTP/WebSocket API
                           │
┌─────────────────────────────────────────────────────────┐
│                 Go API Server                           │
│  REST Endpoints, WebSocket, Metrics Collection         │
└─────────────────────────────────────────────────────────┘
                           │  
                      CGO/IPC Interface
                           │
┌─────────────────────────────────────────────────────────┐
│              C++ Simulation Engine                      │
│  Physics Calculations, Energy Field Management         │
└─────────────────────────────────────────────────────────┘
```

### Key Files
- `src/cpp/main.ternary.fission.application.cpp` - Main physics engine
- `src/cpp/ternary.fission.simulation.engine.cpp` - Core simulation logic
- `src/go/api.ternary.fission.server.go` - REST API server with dashboard
- `include/physics.constants.definitions.h` - Physics constants
- `configs/ternary_fission.conf` - Configuration file
- `docker-compose.yml` - Container orchestration

## 🚀 Deployment

### Docker Deployment (Working)
```bash
# Start full stack
docker-compose up -d

# Check service status
docker-compose ps

# View application logs
docker-compose logs -f ternary-fission-app

# Stop services
docker-compose down
```

### Native Installation
```bash
# System-wide installation (requires sudo)
sudo make install

# User installation
make install PREFIX=$HOME/.local

# Service management
systemctl start ternary-fission
systemctl enable ternary-fission
```

### Configuration
```bash
# Edit configuration
sudo nano /etc/ternary-fission/ternary_fission.conf

# Key settings
api_port=8080
events_per_second=5.0
max_energy_field=1000.0
log_level=info
```

### SSL Certificate Management
```bash
# Generate a self-signed certificate for local testing
./scripts/ssl-manager.sh --cn localhost --days 365
```

## 📊 API Documentation

### Working REST Endpoints
```bash
# Web Dashboard
GET /                    # Educational project interface

# System monitoring
GET /api/v1/health       # Health check (200 OK confirmed)
GET /api/v1/status       # System status with JSON response
GET /api/v1/metrics      # Prometheus metrics (working)

# Energy field management  
POST /api/v1/energy-fields
GET /api/v1/energy-fields
GET /api/v1/energy-fields/{id}
DELETE /api/v1/energy-fields/{id}

# Simulation control
POST /api/v1/simulate/fission
POST /api/v1/simulate/continuous
DELETE /api/v1/simulate/continuous

# Real-time monitoring
WS /api/v1/ws/monitor    # WebSocket real-time updates
```

### Verified API Usage
```bash
# Start API server (confirmed working)
docker-compose up -d

# Test health endpoint (200 OK response)
curl http://localhost:8080/api/v1/health

# Get system status (JSON response confirmed)
curl http://localhost:8080/api/v1/status | jq

# Access Prometheus metrics (working)
curl http://localhost:8080/api/v1/metrics

# Create energy field
curl -X POST http://localhost:8080/api/v1/energy-fields \
  -H "Content-Type: application/json" \
  -d '{"initial_energy_mev": 235.0, "auto_dissipate": true}'
```

## 🔬 Physics Simulation

### Multi-Base Mathematical Framework
- **Base-3 Mathematics**: Optimized ternary nuclear fission for sustainable energy generation
- **Base-5 Mathematics**: Real-time geospatial navigation for planetary and interstellar coordinate systems
- **Base-8 Mathematics**: Electromagnetic field stabilization for exotic matter containment
- **Base-17 Mathematics**: Temporal and multiverse navigation for precise traversal across time and alternate realities

### Ternary Fission Model
- **Energy Mapping**: 1 MeV = 1 MB memory allocation + 1 billion CPU cycles
- **Conservation Laws**: Energy, momentum, mass, and charge verification
- **Wormhole Physics**: Spacetime manipulation through computational resource mapping
- **Quantum Field Theory**: Integration with general relativity for realistic simulations

### Energy Field Management
- Dynamic energy field creation and allocation using Base-3/Base-8 algorithms
- Encryption-based energy dissipation simulation modeling real physics
- Base-5 geospatial tracking for planetary coordinate systems
- Base-17 temporal navigation for timeline and multiverse calculations
- Memory pool optimization for performance
- Real-time monitoring of energy field states

### Performance Targets
- 1000+ fission events per second (8-core system)
- <100MB memory per 1000 active energy fields
- <50ms API response time
- 60fps WebSocket updates

## 📈 Monitoring

### Prometheus Metrics (Working)
```bash
# Available at http://localhost:8080/api/v1/metrics
ternary_fission_api_requests_total
reactor_active_fields
reactor_total_energy_mev
ternary_fission_api_response_time_seconds
```

### Grafana Dashboards
- Real-time energy field visualization
- System performance metrics
- API request monitoring
- Physics simulation statistics

### Health Checks (Verified)
```bash
# API health (confirmed working)
curl http://localhost:8080/api/v1/health

# Container health (working with proper status codes)
docker-compose ps
```

## 🧪 Testing

### Test Suites
```bash
# C++ tests
make test-cpp
./bin/ternary-fission --help

# Go tests with coverage
cd src/go && go test -v ./... -coverprofile=coverage.out

# Integration tests
make test-integration

# Docker testing (see TESTING.md)
./scripts/test-docker.sh test-all
```

### Test Data
- Physics validation with known nuclear data
- Conservation law verification
- Memory leak detection with valgrind
- Concurrent stress testing
- Docker deployment verification

## 📚 Documentation

### Technical Documentation
- **[HOW-TO.md](HOW-TO.md)** - Complete usage guide from basic to extreme simulations
- **[docs/BENCHMARKING.md](docs/BENCHMARKING.md)** - Benchmark presets for repeatable runs
- **[TESTING.md](TESTING.md)** - Docker testing procedures and API verification
- **[NEXT-STEPS.md](NEXT-STEPS.md)** - Development roadmap and architecture improvements
- **[BUILD_CARRYOVER.md](BUILD_CARRYOVER.md)** - Development context and system overview
- **[Stargate Framework](https://davestj.com/bthl/stargate_framework/)** - Theoretical physics foundation

### Research Papers
- Energy field mapping methodology
- Ternary fission simulation accuracy
- Performance comparison studies
- Nuclear engineering applications

## 🤝 Contributing

### Development Process
1. Fork the repository
2. Create feature branch from `dev`
3. Make changes and add tests
4. Ensure all builds pass (including Docker)
5. Submit pull request to `future`
6. Wait for review and automated testing
7. Merge into `dev` triggers development artifacts
8. Promotion from `dev` to `master` creates production release

### Code Standards
- C++17 standard with GCC 13+
- Go 1.23+ with proper error handling
- Comprehensive documentation in comments
- Thread-safe concurrent operations
- Production-grade error handling
- Docker deployment compatibility

### Testing Requirements
- Unit tests for all new functionality
- Integration tests for API endpoints
- Performance benchmarks for physics code
- Memory leak verification
- Conservation law validation
- Docker deployment testing

## 🔧 Troubleshooting

### Common Issues
```bash
# Build failures
make clean && make all

# Missing dependencies
sudo apt install -y build-essential libssl-dev libhttplib-dev

# Docker issues
docker-compose down && docker-compose up --build

# Port conflicts (API server uses port from config)
docker-compose ps  # Check actual port assignments
```

### Debug Mode
```bash
# Debug build
make cpp-build BUILD_TYPE=debug

# Verbose logging
./bin/ternary-api -config configs/ternary_fission.conf -log-level debug

# Docker logs
docker-compose logs -f ternary-fission-app
```

### Performance Issues
```bash
# Profile CPU usage
perf record ./bin/ternary-fission
perf report

# Memory profiling
valgrind --tool=massif ./bin/ternary-fission

# Docker resource monitoring
docker stats ternary-fission-emulator
```

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 👨‍💻 Author

**bthlops (David StJ)**
- Email: davestj@gmail.com
- GitHub: https://github.com/davestj
- Profile: https://davestj.com
- ORCID: https://orcid.org/0009-0000-5077-9751

## 🎯 Updated Roadmap

### Phase 1: API Testing & Validation (Next 2-3 weeks)
- Comprehensive API endpoint testing
- WebSocket functionality validation
- Performance optimization and load testing
- Cross-platform Docker verification

### Phase 2: Kubernetes Integration (3-4 weeks)
- K8s deployment manifests
- Service discovery and networking
- Horizontal pod autoscaling
- Production-ready configurations

### Phase 3: Enhanced Physics Features (4-6 weeks)
- Advanced nuclear data integration
- Complete base mathematics implementation
- GPU acceleration with CUDA
- Machine learning integration

### Phase 4: Production Hardening (6-8 weeks)
- Security audit and hardening
- Performance benchmarking
- Documentation completion
- Academic validation

## 🔍 Related Projects

- **[Stargate Framework](https://davestj.com/bthl/stargate_framework/)** - Theoretical foundation
- **[Nuclear Data Libraries](https://www.nndc.bnl.gov/)** - Reference data
- **[MCNP Integration](docs/mcnp/)** - Cross-validation tools
- **[GEANT4 Interface](docs/geant4/)** - Particle transport

---

**For technical support, feature requests, or collaboration opportunities, please open an issue or contact directly.**

**Current Status**: Docker deployment working, Go API server operational, comprehensive testing in progress.