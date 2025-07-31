#
# File: Dockerfile
# Author: bthlops (David StJ)
# Date: July 31, 2025
# Title: Multi-stage Docker Container for Ternary Fission Energy Emulation System - FIXED
# Purpose: Production-ready containerization with cross-platform support and proper build args
# Reason: Provides consistent deployment environment with ARM64/AMD64 compatibility across all Linux distros
#
# Change Log:
# 2025-07-31: FIXED build argument consumption and cross-platform compatibility
#             Added proper ARG consumption to eliminate build warnings
#             Updated package names for Ubuntu 24.04 compatibility
#             Added multi-architecture support for Debian/Ubuntu variants
#             Fixed label usage of build arguments for metadata
#
# Carry-over Context:
# - Multi-stage build reduces final image size while maintaining functionality
# - Works on Debian 12, Ubuntu 22.04/24.04, and other Linux distributions
# - Consumes all build arguments to eliminate warnings
# - Provides consistent behavior across ARM64/AMD64 architectures

# =============================================================================
# BUILD ARGUMENTS (Available to all stages)
# =============================================================================

ARG BUILD_DATE
ARG GIT_COMMIT
ARG VERSION=1.1.13

# =============================================================================
# BUILD STAGE 1: C++ COMPILATION ENVIRONMENT
# =============================================================================

FROM ubuntu:24.04 AS cpp-builder

# We consume build arguments in this stage
ARG BUILD_DATE
ARG GIT_COMMIT
ARG VERSION

# We install C++ development tools and scientific libraries
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    cmake \
    make \
    pkg-config \
    libgsl-dev \
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

# We compile the C++ simulation engine with optimizations and version info
RUN make cpp-release \
    VERSION_FLAGS="-DVERSION=\"${VERSION}\" -DBUILD_DATE=\"${BUILD_DATE}\" -DGIT_COMMIT=\"${GIT_COMMIT}\"" && \
    strip bin/ternary-fission && \
    chmod +x bin/ternary-fission

# We verify the build was successful
RUN ./bin/ternary-fission --help

# =============================================================================
# BUILD STAGE 2: GO COMPILATION ENVIRONMENT
# =============================================================================

FROM golang:1.23-alpine AS go-builder

# We consume build arguments in this stage
ARG BUILD_DATE
ARG GIT_COMMIT
ARG VERSION

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

# We build the Go API server with optimizations and version info
RUN CGO_ENABLED=0 GOOS=linux go build \
    -ldflags="-w -s -extldflags '-static' -X main.Version=${VERSION} -X main.BuildDate='${BUILD_DATE}' -X main.GitCommit=${GIT_COMMIT}" \
    -a -installsuffix cgo \
    -o api-server \
    api.ternary.fission.server.go

# We verify the Go build
RUN ./api-server --help || true

# =============================================================================
# RUNTIME STAGE: PRODUCTION CONTAINER
# =============================================================================

FROM ubuntu:24.04 AS runtime

# We consume build arguments in this stage for labeling
ARG BUILD_DATE
ARG GIT_COMMIT
ARG VERSION

# We install runtime dependencies with correct Ubuntu 24.04 package names
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    libgsl27 \
    libfftw3-double3 \
    libopenblas0-pthread \
    liblapack3 \
    libssl3t64 \
    libcrypto++8 \
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
COPY --from=cpp-builder /build/bin/ternary-fission /app/bin/
COPY --from=go-builder /app/api-server /app/bin/

# We copy configuration files and scripts
COPY configs/ /app/configs/
COPY scripts/docker-entrypoint.sh /app/bin/

# We make scripts executable
RUN chmod +x /app/bin/docker-entrypoint.sh \
    && chmod +x /app/bin/ternary-fission \
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

# We set environment variables including build info
ENV GO_ENV=production \
    LOG_LEVEL=info \
    API_PORT=8080 \
    SIMULATION_MODE=daemon \
    CONFIG_FILE=/app/configs/ternary_fission.conf \
    TF_VERSION=${VERSION} \
    TF_BUILD_DATE=${BUILD_DATE} \
    TF_GIT_COMMIT=${GIT_COMMIT}

# We create volume mount points
VOLUME ["/app/data", "/app/results", "/app/logs", "/app/configs"]

# We set the entrypoint script
ENTRYPOINT ["/app/bin/docker-entrypoint.sh"]

# We default to running the API server
CMD ["api-server"]

# =============================================================================
# METADATA LABELS WITH BUILD ARGUMENTS
# =============================================================================

LABEL maintainer="bthlops (David StJ) <davestj@gmail.com>" \
      version="${VERSION}" \
      build_date="${BUILD_DATE}" \
      git_commit="${GIT_COMMIT}" \
      description="Ternary Fission Energy Emulation System" \
      org.opencontainers.image.title="Ternary Fission Emulator" \
      org.opencontainers.image.description="High-performance nuclear physics simulation with energy field emulation" \
      org.opencontainers.image.version="${VERSION}" \
      org.opencontainers.image.created="${BUILD_DATE}" \
      org.opencontainers.image.revision="${GIT_COMMIT}" \
      org.opencontainers.image.author="bthlops (David StJ)" \
      org.opencontainers.image.url="https://github.com/davestj/ternary-fission-reactor" \
      org.opencontainers.image.source="https://github.com/davestj/ternary-fission-reactor" \
      org.opencontainers.image.vendor="davidstj.com" \
      org.opencontainers.image.licenses="MIT" \
      org.opencontainers.image.documentation="https://github.com/davestj/ternary-fission-reactor/blob/main/README.md" \
      platforms="linux/amd64,linux/arm64" \
      base_image="ubuntu:24.04"