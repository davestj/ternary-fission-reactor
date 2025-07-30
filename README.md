# Ternary Fission Energy Emulation System

[![Build Status](https://github.com/davestj/ternary-fission-reactor/workflows/Build%20and%20Test/badge.svg)](https://github.com/davestj/ternary-fission-reactor/actions)
[![Release](https://img.shields.io/github/v/release/davestj/ternary-fission-reactor?include_prereleases)](https://github.com/davestj/ternary-fission-reactor/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![Go](https://img.shields.io/badge/Go-1.23+-00ADD8.svg)](https://golang.org/)
[![Docker](https://img.shields.io/badge/Docker-Ready-2496ED.svg)](https://hub.docker.com/)

[![GitHub issues](https://img.shields.io/github/issues/davestj/ternary-fission-reactor)](https://github.com/davestj/ternary-fission-reactor/issues)
[![GitHub forks](https://img.shields.io/github/forks/davestj/ternary-fission-reactor)](https://github.com/davestj/ternary-fission-reactor/network)
[![GitHub stars](https://img.shields.io/github/stars/davestj/ternary-fission-reactor)](https://github.com/davestj/ternary-fission-reactor/stargazers)
[![Contributors](https://img.shields.io/github/contributors/davestj/ternary-fission-reactor)](https://github.com/davestj/ternary-fission-reactor/graphs/contributors)

**Author:** bthlops (David StJ)  
**Version:** 1.1.1-alpha  
**Last Updated:** July 30, 2025  

> High-performance ternary nuclear fission simulation with C++ physics engine, Go REST API, and Docker deployment. Maps nuclear energy to computational resources with real-time monitoring and conservation law verification.

## 🚀 Quick Start

```bash
# Clone and build
git clone https://github.com/davestj/ternary-fission-reactor.git
cd ternary-fission-reactor
make all

# Run API server
./bin/ternary-api -config configs/ternary_fission.conf -port 8080

# Test endpoints
curl http://localhost:8080/api/v1/health
```

## 📊 Project Stats

![GitHub repo size](https://img.shields.io/github/repo-size/davestj/ternary-fission-reactor)
![Lines of code](https://img.shields.io/tokei/lines/github/davestj/ternary-fission-reactor)
![GitHub last commit](https://img.shields.io/github/last-commit/davestj/ternary-fission-reactor)

**Latest Release:** [Download v1.1.1-alpha](https://github.com/davestj/ternary-fission-reactor/releases/latest)
- Ubuntu 24.04 x64 binaries
- Complete source code
- Docker images
- Documentation

## ⚡ Key Features

- **High-Performance C++ Core**: Optimized physics simulation engine with conservation law verification
- **Concurrent Go API**: RESTful server with WebSocket real-time monitoring
- **Novel Energy Mapping**: 1 MeV = 1MB memory + 1B CPU cycles
- **Encryption-Based Dissipation**: Energy decay through cryptographic operations
- **Production Ready**: Docker deployment, Prometheus metrics, comprehensive testing

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                  Client Applications                    │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │ Web Dashboard│  │   API Clients│  │  Monitoring     │  │
│  │   (Browser) │  │ (curl/Postman)│  │   Tools         │  │
│  └─────────────┘  └─────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────┘
                           │
                    HTTP/WebSocket
                           │
┌─────────────────────────────────────────────────────────┐
│                 Go API Server Layer                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │ REST API    │  │  WebSocket  │  │   Metrics &     │  │
│  │ Endpoints   │  │   Handler   │  │   Monitoring    │  │
│  └─────────────┘  └─────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────┘
                           │
                      CGO/IPC
                           │
┌─────────────────────────────────────────────────────────┐
│              C++ Simulation Engine Core                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │ Ternary     │  │ Energy Field│  │   Physics       │  │
│  │ Fission     │  │ Management  │  │ Calculations    │  │
│  │ Simulator   │  │   System    │  │   & Validation  │  │
│  └─────────────┘  └─────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## 🔬 Physics Implementation

### Ternary Fission Modeling

Simulates U-235 splitting into three fragments:
- **Light Fragment** (~95 AMU, ~30% kinetic energy)
- **Heavy Fragment** (~140 AMU, ~67% kinetic energy)  
- **Alpha Particle** (4 AMU, ~3% kinetic energy)

**Conservation Laws:**
- Energy: Q = (M_parent - ΣM_fragments) × c²
- Momentum: Σp_initial = Σp_final (3D vector sum)

### Energy Field Representation

- **Memory Mapping**: 1 MeV = 1 MB allocated memory
- **CPU Mapping**: 1 MeV = 1 billion CPU cycles
- **Entropy**: S = k × ln(W) using system microstates
- **Dissipation**: E(t) = E₀ × e^(-λt) via encryption rounds

## 📦 Installation

### Prerequisites

**Ubuntu 24.04 LTS:**
```bash
sudo apt update && sudo apt install -y \
  build-essential gcc g++ cmake make \
  libssl-dev libcrypto++-dev \
  git docker.io docker-compose
```

**Go 1.23+:**
```bash
wget https://go.dev/dl/go1.23.3.linux-amd64.tar.gz
sudo tar -C /usr/local -xzf go1.23.3.linux-amd64.tar.gz
export PATH=/usr/local/go/bin:$PATH
```

### Build from Source

```bash
git clone https://github.com/davestj/ternary-fission-reactor.git
cd ternary-fission-reactor
make dev-setup  # Initialize environment  
make all        # Build all components
make test       # Run test suite
```

### Docker Deployment

```bash
make docker-build
make docker-run
```

## 🚀 Usage Examples

### Energy Field Creation

```bash
curl -X POST http://localhost:8080/api/v1/energy-fields \
  -H "Content-Type: application/json" \
  -d '{
    "initial_energy_mev": 50.0,
    "auto_dissipate": true,
    "dissipation_rounds": 10
  }'
```

### Ternary Fission Simulation

```bash
curl -X POST http://localhost:8080/api/v1/simulate/fission \
  -H "Content-Type: application/json" \
  -d '{
    "parent_mass": 235.0,
    "excitation_energy": 6.5,
    "number_of_events": 1
  }'
```

### Real-Time Monitoring

```javascript
const ws = new WebSocket('ws://localhost:8080/api/v1/ws/monitor');
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  console.log('Energy field update:', data);
};
```

## 📋 API Reference

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/v1/energy-fields` | POST | Create energy field |
| `/api/v1/energy-fields` | GET | List all fields |
| `/api/v1/energy-fields/{id}` | GET | Get specific field |
| `/api/v1/energy-fields/{id}/dissipate` | POST | Dissipate field |
| `/api/v1/simulate/fission` | POST | Simulate fission events |
| `/api/v1/status` | GET | System status |
| `/api/v1/health` | GET | Health check |
| `/api/v1/metrics` | GET | Prometheus metrics |

## 🧪 Testing & Quality

```bash
make test-cpp       # C++ unit tests
make test-go        # Go tests  
make benchmark      # Performance tests
make analyze-cpp    # Static analysis
make security-scan  # Security scanning
```

**Performance Targets:**
- 1000+ fission events/second (8-core system)
- <100MB per 1000 energy fields
- <50ms API response time
- 60fps WebSocket updates

## 🐳 Docker Configuration

```yaml
version: '3.8'
services:
  ternary-fission:
    image: ternary-fission:latest
    ports:
      - "8080:8080"
    environment:
      - GO_ENV=production
    volumes:
      - ./configs:/app/configs
```

## 🛠️ Development

### Code Standards
- **C++17** with GCC 13+ compatibility
- **Go 1.23+** with effective Go guidelines
- Production-grade error handling
- Comprehensive documentation

### Contributing
1. Fork repository
2. Create feature branch: `git checkout -b feature/name`
3. Make changes and test: `make test`
4. Submit pull request

## 📈 Monitoring

**Prometheus Metrics:**
- `ternary_fission_active_energy_fields`
- `ternary_fission_api_requests_total`
- `ternary_fission_response_time_seconds`

**Grafana Dashboard:** Available in `/monitoring/grafana/`

## 🔧 Troubleshooting

**Common Issues:**
- Port conflicts: Use `-port` flag to change port
- Memory errors: Check available RAM with `free -h`
- SSL errors: Install `libssl-dev libcrypto++-dev`

## 📄 License

MIT License - see [LICENSE](LICENSE) file.

## 🤝 Support & Contact

**Author:** bthlops (David StJ)  
**Email:** davestj@gmail.com  
**GitHub:** [@davestj](https://github.com/davestj)  
**Profile:** [davidstj.com](https://www.davidstj.com)

[![GitHub issues](https://img.shields.io/github/issues/davestj/ternary-fission-reactor)](https://github.com/davestj/ternary-fussion-reactor/issues/new)

---

> **"Advanced nuclear physics simulation meeting production engineering standards"**
