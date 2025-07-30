#
# File: Dockerfile
# Author: bthlops (David StJ)
# Date: July 29, 2025
# Title: Multi-stage Docker Container for Ternary Fission Energy Emulation System
# Purpose: Production-ready containerization of C++ simulation engine and Go API server
# Reason: Provides consistent deployment environment with optimized build process
# 
# Change Log:
# 2025-07-29: Initial creation with multi-stage build for optimized image size
# 2025-07-29: Integrated C++ compilation with scientific libraries and dependencies
# 2025-07-29: Added Go API server build with proper dependency management
# 2025-07-29: Configured runtime environment with security and performance optimizations
# 2025-07-29: Added health checks and monitoring capabilities for production deployment
#
# Carry-over Context:
# - Multi-stage build reduces final image size while maintaining full functionality
# - Scientific libraries (GSL, FFTW, OpenSSL) support physics calculations
# - Runtime user isolation provides security best practices
# - Health checks enable proper container orchestration and monitoring
# - Volume mounts allow persistent data storage and configuration management

# =============================================================================
# BUILD STAGE 1: C++ COMPILATION ENVIRONMENT
# =============================================================================

FROM debian:12-slim AS cpp-builder

# We install C++ development tools and scientific libraries
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    cmake \
    make \
    pkg-config \
    libgsl-dev \
    libgslcblas0-dev \
    libeigen3-dev \
    libfftw3-dev \
    libopenblas-dev \
    liblapack-dev \
    libssl-dev \
    libcrypto++-dev \
    libboost-all-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# We set up the build environment
WORKDIR /build

# We copy source code and build configuration
COPY include/ include/
COPY src/cpp/ src/cpp/
COPY Makefile .

# We compile the C++ simulation engine with optimizations
RUN make cpp-release && \
    strip bin/ternary-engine && \
    chmod +x bin/ternary-engine

# We verify the build was successful
RUN ./bin/ternary-engine --version

# =============================================================================
# BUILD STAGE 2: GO COMPILATION ENVIRONMENT  
# =============================================================================

FROM golang:1.23-alpine AS go-builder

# We install build dependencies
RUN apk add --no-cache git ca-certificates tzdata

# We set up the Go build environment
WORKDIR /app

# We copy Go module files first for better caching
COPY src/go/go.mod src/go/go.sum ./

# We download Go dependencies
RUN go mod download && go mod verify

# We copy Go source code
COPY src/go/ .

# We build the Go API server with optimizations
RUN CGO_ENABLED=0 GOOS=linux go build \
    -ldflags='-w -s -extldflags "-static"' \
    -a -installsuffix cgo \
    -o api-server \
    api.ternary.fission.server.go

# We verify the Go build
RUN ./api-server --help || true

# =============================================================================
# RUNTIME STAGE: PRODUCTION CONTAINER
# =============================================================================

FROM debian:12-slim AS runtime

# We install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libgsl27 \
    libgslcblas0 \
    libfftw3-3 \
    libopenblas0 \
    liblapack3 \
    libssl3 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# We create a non-root user for security
RUN groupadd -r bthlops && useradd -r -g bthlops -s /bin/false bthlops

# We set up application directories
WORKDIR /app

# We create necessary directories with proper permissions
RUN mkdir -p \
    /app/bin \
    /app/configs \
    /app/data \
    /app/results \
    /app/logs \
    /var/log/ternary-fission \
    && chown -R bthlops:bthlops /app \
    && chown -R bthlops:bthlops /var/log/ternary-fission

# We copy compiled binaries from build stages
COPY --from=cpp-builder /build/bin/ternary-engine /app/bin/
COPY --from=go-builder /app/api-server /app/bin/

# We copy configuration files and scripts
COPY configs/ /app/configs/
COPY scripts/docker-entrypoint.sh /app/bin/
COPY web/static/ /app/web/static/

# We make scripts executable
RUN chmod +x /app/bin/docker-entrypoint.sh \
    && chmod +x /app/bin/ternary-engine \
    && chmod +x /app/bin/api-server

# We set proper ownership
RUN chown -R bthlops:bthlops /app

# We expose the API server port
EXPOSE 8080

# We set up health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/api/v1/health || exit 1

# We configure resource limits and security
USER bthlops

# We set environment variables
ENV GO_ENV=production \
    LOG_LEVEL=info \
    API_PORT=8080 \
    SIMULATION_MODE=daemon \
    CONFIG_FILE=/app/configs/ternary_fission.conf

# We create volume mount points
VOLUME ["/app/data", "/app/results", "/app/logs", "/app/configs"]

# We set the entrypoint script
ENTRYPOINT ["/app/bin/docker-entrypoint.sh"]

# We default to running the API server
CMD ["api-server"]

# =============================================================================
# METADATA LABELS FOR CONTAINER MANAGEMENT
# =============================================================================

LABEL maintainer="bthlops (David StJ) <davestj@gmail.com>" \
      version="1.0.0" \
      description="Ternary Fission Energy Emulation System" \
      org.opencontainers.image.title="Ternary Fission Emulator" \
      org.opencontainers.image.description="High-performance nuclear physics simulation with energy field emulation" \
      org.opencontainers.image.version="1.0.0" \
      org.opencontainers.image.author="bthlops (David StJ)" \
      org.opencontainers.image.url="https://github.com/davestj/ternary-fission-emulator" \
      org.opencontainers.image.source="https://github.com/davestj/ternary-fission-emulator" \
      org.opencontainers.image.vendor="davidstj.com" \
      org.opencontainers.image.licenses="MIT" \
      org.opencontainers.image.created="2025-07-29" \
      org.opencontainers.image.documentation="https://github.com/davestj/ternary-fission-emulator/blob/main/README.md"
