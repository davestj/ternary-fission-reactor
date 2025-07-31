# Testing Guide - Ternary Fission Docker System

**Author:** bthlops (David StJ)  
**Date:** July 31, 2025  
**Purpose:** Quick testing and verification commands for Docker builds and deployments

## Quick Test Commands

### 1. Build and Test
```bash
# Make scripts executable
chmod +x scripts/*.sh

# Build all variants
./scripts/docker-build.sh build-all

# Test all built images
./scripts/test-docker.sh test-all
```

### 2. Single Image Testing
```bash
# Test default image
./scripts/test-docker.sh test

# Test specific variant
./scripts/test-docker.sh test ternary-fission:ubuntu24
```

### 3. Daemon Mode with External Access
```bash
# Start daemon on port 8080
./scripts/test-docker.sh daemon

# Start on custom port
./scripts/test-docker.sh daemon ternary-fission:latest 8081

# Check status
./scripts/test-docker.sh status
```

### 4. API Verification Commands
```bash
# Health check
curl http://localhost:8080/api/v1/health

# System status
curl http://localhost:8080/api/v1/status | jq

# Metrics
curl http://localhost:8080/api/v1/metrics

# Create energy field
curl -X POST http://localhost:8080/api/v1/energy-fields \
  -H "Content-Type: application/json" \
  -d '{"initial_energy_mev": 100.0, "auto_dissipate": true}'

# List energy fields
curl http://localhost:8080/api/v1/energy-fields | jq
```

### 5. Docker Compose Testing
```bash
# Start full stack
docker-compose up -d

# Check services
docker-compose ps

# View logs
docker-compose logs -f ternary-fission-app

# Test external access
curl http://localhost:8080/api/v1/health

# Test internal network (from another container)
docker exec -it ternary-fission-emulator curl http://localhost:8080/api/v1/status
```

### 6. Simulation Engine Testing
```bash
# Test C++ engine directly
docker run --rm ternary-fission:latest ternary-fission --help

# Run sample simulation
docker run --rm ternary-fission:latest ternary-fission -n 5 -p 235 -e 6.5

# Interactive mode
docker run -it --rm ternary-fission:latest ternary-fission -x
```

## Smoke Tests Checklist

- [ ] `ternary-fission --help` works
- [ ] `api-server --help` works  
- [ ] Container starts successfully
- [ ] Health endpoint responds (200)
- [ ] Status endpoint returns JSON
- [ ] Metrics endpoint accessible
- [ ] C++ simulation runs
- [ ] Internal network communication works
- [ ] External port forwarding works

## Internal Network Communication

### Container-to-Container (Docker Compose)
```bash
# API server accessible from other services
curl http://ternary-fission-app:8080/api/v1/health

# From within the network
docker exec redis curl http://ternary-fission-app:8080/api/v1/status
```

### Standalone Container Network
```bash
# Create custom network
docker network create ternary-net

# Run API server on network
docker run -d --name api --network ternary-net -p 8080:8080 ternary-fission:latest

# Run client on same network  
docker run --rm --network ternary-net ternary-fission:latest \
  sh -c "curl http://api:8080/api/v1/health"
```

## Advanced Testing

### Load Testing
```bash
# Multiple concurrent requests
for i in {1..10}; do
  curl -s http://localhost:8080/api/v1/status > /dev/null &
done
wait
```

### Performance Testing
```bash
# Time API responses
time curl -s http://localhost:8080/api/v1/status > /dev/null

# Monitor resource usage
docker stats ternary-fission-emulator
```

### WebSocket Testing (if enabled)
```bash
# Install wscat: npm install -g wscat
wscat -c ws://localhost:8080/api/v1/ws/monitor
```

## Troubleshooting

### Container Not Starting
```bash
# Check logs
docker logs ternary-fission-emulator

# Debug with interactive shell
docker run -it --rm ternary-fission:latest /bin/bash
```

### Port Issues
```bash
# Check port usage
netstat -tulpn | grep 8080
lsof -i :8080

# Use different port
./scripts/test-docker.sh daemon ternary-fission:latest 8081
```

### Network Issues
```bash
# Check networks
docker network ls

# Inspect network
docker network inspect ternary-fission-network

# Check container connectivity
docker exec ternary-fission-emulator ping google.com
```

## Cleanup Commands
```bash
# Stop and remove test containers
./scripts/test-docker.sh stop

# Clean up docker-compose
docker-compose down

# Remove all project images
docker images ternary-fission --format "{{.ID}}" | xargs docker rmi

# Full cleanup
docker system prune -f
```
