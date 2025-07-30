# Ternary Fission Energy Emulation System

![Build Status](https://github.com/davestj/ternary-fission-reactor/actions/workflows/build-and-release.yml/badge.svg)
![Release](https://img.shields.io/github/v/release/davestj/ternary-fission-reactor?include_prereleases)
![License](https://img.shields.io/github/license/davestj/ternary-fission-reactor)
![Language](https://img.shields.io/badge/C%2B%2B-17-blue)
![Go Version](https://img.shields.io/badge/Go-1.23%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Ubuntu%2024.04-green)

**Author:** bthlops (David StJ)  
**Version:** 1.1.1-alpha  
**Last Updated:** July 30, 2025

High-performance ternary nuclear fission simulation with C++ physics engine, Go REST API, and Docker deployment. Maps nuclear energy to computational resources with real-time monitoring and conservation law verification.

## 🚀 Quick Start

### Development Build (dev/develop branch)
```bash
# Clone and build development version
git clone https://github.com/davestj/ternary-fission-reactor.git
cd ternary-fission-reactor
git checkout develop
make all

# Run API server
./bin/ternary-api -config configs/ternary_fission.conf -port 8080
```

### Production Release (master branch)
```bash
# Download latest production release
wget https://github.com/davestj/ternary-fission-reactor/releases/latest/download/ternary-fission-reactor-v1.1.1-prod.tar.gz
tar -xzf ternary-fission-reactor-v1.1.1-prod.tar.gz

# Install system-wide
wget https://github.com/davestj/ternary-fission-reactor/releases/latest/download/install.sh
sudo ./install.sh

# Start service
ternary-api -config /etc/ternary-fission/ternary_fission.conf
```

## 📦 Download Options

### Development Artifacts (dev/develop branch)
- `ternary-fission-reactor-VERSION-dev-release.tar.gz` - Development binaries (tar.gz)
- `ternary-fission-reactor-VERSION-dev-release.zip` - Development binaries (zip)
- `ternary-fission-reactor-VERSION-dev-debug.tar.gz` - Debug build with symbols
- `ternary-fission-reactor-VERSION-dev-source.tar.gz` - Complete source with tests

### Production Releases (master branch)
- `ternary-fission-reactor-VERSION-prod.tar.gz` - Production binaries (tar.gz)
- `ternary-fission-reactor-VERSION-prod.zip` - Production binaries (zip)
- `ternary-fission-reactor-VERSION-source.tar.gz` - Clean source archive
- `install.sh` - Automated system installer

## 🏗️ Build Pipeline

### Branch Strategy
- **develop/dev** → Development builds with debug symbols and test coverage
- **master/main** → Production releases with optimized binaries
- **feature/** → Pull request builds for testing
- **v*tags** → Tagged releases with semantic versioning

### Automated Builds
| Branch | Build Types | Artifacts | Retention | Release |
|--------|-------------|-----------|-----------|---------|
| develop/dev | release + debug | tar.gz + zip | 14 days | No |
| master/main | release only | tar.gz + zip | 90 days | Yes |
| PR | release + debug | tar.gz + zip | 7 days | No |
| Tags | release only | tar.gz + zip | Permanent | Yes |

### Version Strategy
- **Alpha** (dev): `1.1.1-alpha.X` - Development builds
- **Beta** (master PR): `1.1.1-beta.X` - Pre-release testing  
- **RC** (manual): `1.1.1-rc.X` - Release candidates
- **Release** (tags): `1.1.1` - Production releases

## 🛠️ Development

### System Requirements
- Ubuntu 24.04 LTS (or compatible)
- GCC 13+ with C++17 support
- Go 1.23+
- CMake 3.16+
- OpenSSL development libraries
- 4GB+ RAM recommended

### Dependencies Installation
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
  build-essential gcc-13 g++-13 cmake make \
  libssl-dev libcrypto++-dev libgsl-dev \
  libeigen3-dev libfftw3-dev libopenblas-dev \
  liblapack-dev libboost-all-dev
```

### Build System
```bash
# Complete build
make all

# Individual components
make cpp-build         # C++ physics engine
make go-build          # Go API server
make docker-build      # Docker images

# Testing
make test-cpp          # C++ unit tests
make test-go           # Go unit tests
make test-integration  # Integration tests

# Development
make clean             # Clean build artifacts
make dev-setup         # Setup development environment
make benchmark         # Performance benchmarks
```

### Development Workflow
```bash
# 1. Create feature branch
git checkout develop
git checkout -b feature/new-physics-model

# 2. Make changes and test
make all
make test

# 3. Create pull request to develop
git push origin feature/new-physics-model
# → Triggers development build with debug + release artifacts

# 4. Merge to develop
# → Creates development source archives and artifacts

# 5. Promote to production
git checkout master
git merge develop
git push origin master
# → Triggers production release with zip/tar.gz downloads
```

## 🏢 Architecture

### Core Components
```
┌─────────────────────────────────────────────────────────┐
│                  Client Layer                           │
│  Web UI, API Clients, Monitoring Dashboards            │
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
- `src/go/api.ternary.fission.server.go` - REST API server
- `include/physics.constants.definitions.h` - Physics constants
- `configs/ternary_fission.conf` - Configuration file
- `docker-compose.yml` - Container orchestration

## 🚀 Deployment

### Docker Deployment
```bash
# Development stack
make docker-run

# Production deployment
docker-compose -f docker-compose.yml up -d

# Monitor services
docker-compose logs -f ternary-fission-app
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

## 📊 API Documentation

### REST Endpoints
```bash
# System status
GET /api/v1/status
GET /api/v1/health

# Energy field management  
POST /api/v1/energy-fields
GET /api/v1/energy-fields
GET /api/v1/energy-fields/{id}
DELETE /api/v1/energy-fields/{id}

# Simulation control
POST /api/v1/simulate/fission
POST /api/v1/simulate/continuous
DELETE /api/v1/simulate/continuous

# Monitoring
GET /api/v1/metrics      # Prometheus metrics
WS /api/v1/ws/monitor    # WebSocket real-time
```

### Example Usage
```bash
# Start API server
ternary-api -port 8080 -config configs/ternary_fission.conf

# Create energy field
curl -X POST http://localhost:8080/api/v1/energy-fields \
  -H "Content-Type: application/json" \
  -d '{"initial_energy_mev": 235.0, "auto_dissipate": true}'

# Get system status
curl http://localhost:8080/api/v1/status | jq

# WebSocket monitoring
wscat -c ws://localhost:8080/api/v1/ws/monitor
```

## 🔬 Physics Simulation

### Ternary Fission Model
- **Base-3 Mathematics**: Optimized ternary nuclear fission for energy generation
- **Base-8 Mathematics**: Electromagnetic field stabilization for exotic matter containment
- **Energy Mapping**: 1 MeV = 1 MB memory allocation + 1 billion CPU cycles
- **Conservation Laws**: Energy, momentum, mass, and charge verification

### Energy Field Management
- Dynamic energy field creation and allocation
- Encryption-based energy dissipation simulation
- Memory pool optimization for performance
- Real-time monitoring of energy field states

### Performance Targets
- 1000+ fission events per second (8-core system)
- <100MB memory per 1000 active energy fields
- <50ms API response time
- 60fps WebSocket updates

## 📈 Monitoring

### Prometheus Metrics
```bash
# Available at http://localhost:8080/api/v1/metrics
ternary_fission_api_requests_total
ternary_fission_active_energy_fields
ternary_fission_total_energy_mev
ternary_fission_api_response_time_seconds
```

### Grafana Dashboards
- Real-time energy field visualization
- System performance metrics
- API request monitoring
- Physics simulation statistics

### Health Checks
```bash
# API health
curl http://localhost:8080/api/v1/health

# Container health  
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

# Performance benchmarks
make benchmark
```

### Test Data
- Physics validation with known nuclear data
- Conservation law verification
- Memory leak detection with valgrind
- Concurrent stress testing

## 📚 Documentation

### Technical Documentation
- [BUILD_CARRYOVER.md](BUILD_CARRYOVER.md) - Development context and roadmap
- [Stargate Framework](docs/stargate-framework/) - Theoretical physics foundation
- [API Documentation](docs/api/) - Complete REST API reference
- [Docker Guide](docs/docker/) - Container deployment guide

### Research Papers
- Energy field mapping methodology
- Ternary fission simulation accuracy
- Performance comparison studies
- Nuclear engineering applications

## 🤝 Contributing

### Development Process
1. Fork the repository
2. Create feature branch from `develop`
3. Make changes and add tests
4. Ensure all builds pass
5. Submit pull request to `develop`
6. Wait for review and automated testing
7. Merge triggers development artifacts
8. Promotion to `master` creates production release

### Code Standards
- C++17 standard with GCC 13+
- Go 1.23+ with proper error handling
- Comprehensive documentation in comments
- Thread-safe concurrent operations
- Production-grade error handling

### Testing Requirements
- Unit tests for all new functionality
- Integration tests for API endpoints
- Performance benchmarks for physics code
- Memory leak verification
- Conservation law validation

## 🔧 Troubleshooting

### Common Issues
```bash
# Build failures
make clean && make all

# Missing dependencies
sudo apt install -y build-essential libssl-dev

# Port conflicts
ternary-api -port 8081

# Permission issues
sudo make install
sudo systemctl start ternary-fission
```

### Debug Mode
```bash
# Debug build
make cpp-build BUILD_TYPE=debug

# Verbose logging
ternary-api -config configs/ternary_fission.conf -log-level debug

# Memory debugging
valgrind ./bin/ternary-fission --help
```

### Performance Issues
```bash
# Profile CPU usage
perf record ./bin/ternary-fission
perf report

# Memory profiling
valgrind --tool=massif ./bin/ternary-fission

# Check system resources
htop
iotop
```

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 👨‍💻 Author

**bthlops (David StJ)**
- Email: davestj@gmail.com
- GitHub: https://github.com/davestj
- Profile: https://www.davestj.com
- ORCID: https://orcid.org/0009-0000-5077-9751

## 🎯 Roadmap

### Phase 1: Enhanced Physics (Next 2-4 weeks)
- Advanced nuclear data integration
- Improved conservation algorithms
- Statistical physics enhancement

### Phase 2: Performance & Scalability (4-6 weeks)
- GPU acceleration with CUDA
- Distributed computing support
- Database integration

### Phase 3: Machine Learning (6-8 weeks)
- Predictive energy modeling
- Adaptive simulation parameters
- Anomaly detection

### Phase 4: Visualization (8-10 weeks)
- 3D energy field visualization
- React dashboard development
- VR/AR support

## 🔍 Related Projects

- [Stargate Framework](docs/stargate-framework/) - Theoretical foundation
- [Nuclear Data Libraries](https://www.nndc.bnl.gov/) - Reference data
- [MCNP Integration](docs/mcnp/) - Cross-validation tools
- [GEANT4 Interface](docs/geant4/) - Particle transport

---

**For technical support, feature requests, or collaboration opportunities, please open an issue or contact directly.**
