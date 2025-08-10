# Environment Configuration Reference

**Author:** bthlops (David StJ)
**Date:** July 31, 2025
**Title:** Environment variable reference for runtime, build metadata, and configuration overrides
**Purpose:** Document environment variables with defaults and examples for CLI and Docker
**Reason:** Provide consistent configuration reference
**Change Log:**
- 2025-07-31: Initial creation

## Runtime Variables

| Variable | Default | Options | CLI Example | Docker Example |
|----------|---------|---------|-------------|----------------|
| `GO_ENV` | `production` | `development`, `production`, `testing` | `GO_ENV=development ./bin/ternary-api` | `docker run -e GO_ENV=development ternary-fission` |
| `LOG_LEVEL` | `info` | `debug`, `info`, `warn`, `error` | `LOG_LEVEL=debug ./bin/ternary-api` | `docker run -e LOG_LEVEL=debug ternary-fission` |
| `API_PORT` | `8080` | any valid port | `API_PORT=9090 ./bin/ternary-api` | `docker run -e API_PORT=9090 -p 9090:9090 ternary-fission` |
| `CONFIG_FILE` | `configs/ternary_fission.conf` | path to config file | `CONFIG_FILE=./configs/custom.conf ./bin/ternary-api` | `docker run -e CONFIG_FILE=/app/configs/custom.conf -v $(pwd)/configs/custom.conf:/app/configs/custom.conf ternary-fission` |
| `SIMULATION_MODE` | `daemon` | `batch`, `continuous`, `daemon` | `SIMULATION_MODE=batch ./bin/ternary-api` | `docker run -e SIMULATION_MODE=continuous ternary-fission` |
| `DEBUG` | `false` (auto `true` in development/testing) | `true`, `false` | `DEBUG=true ./bin/ternary-api` | `docker run -e DEBUG=true ternary-fission` |

## Build and Metadata Variables

| Variable | Default | Purpose | CLI Example | Docker Example |
|----------|---------|---------|-------------|----------------|
| `TF_VERSION` | `2.0.0-rc1` | application version | `TF_VERSION=local make go-build` | `docker build --build-arg VERSION=2.0.0-rc1 .` |
| `TF_BUILD_DATE` | build time | ISO-8601 build timestamp | `TF_BUILD_DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ") make go-build` | `docker build --build-arg BUILD_DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ") .` |
| `TF_GIT_COMMIT` | current commit | git revision for build | `TF_GIT_COMMIT=$(git rev-parse HEAD) make go-build` | `docker build --build-arg GIT_COMMIT=$(git rev-parse HEAD) .` |
| `DEBIAN_FRONTEND` | `noninteractive` | apt interaction mode | `DEBIAN_FRONTEND=noninteractive sudo apt-get update` | `docker build --build-arg DEBIAN_FRONTEND=noninteractive .` |

## Configuration Overrides

Any configuration file parameter can be overridden by setting an environment variable with the `TERNARY_` prefix followed by the upper-cased parameter name.

Examples:

```bash
# CLI
export TERNARY_EVENTS_PER_SECOND=10
./bin/ternary-fission

# Docker
docker run -e TERNARY_EVENTS_PER_SECOND=10 ternary-fission
```
