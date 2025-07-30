# File: Makefile
# Author: bthlops (David StJ)
# Date: July 29, 2025
# Title: Ternary Fission Energy Emulation System - Build Configuration
# Purpose: Comprehensive build system for C++ simulation engine and Go API server
# Reason: Provides automated compilation, testing, and deployment workflows
#
# Change Log:
# 2025-07-29: Initial creation with multi-target build system
# 2025-07-29: Added Docker containerization support
# 2025-07-29: Integrated testing and documentation generation
# 2025-07-29: Added development environment setup
# 2025-07-29: Fixed compilation structure to use proper header includes
#             Separate compilation of object files prevents multiple definitions
#
# Carry-over Context:
# - Supports multiple build configurations (debug, release, profile)
# - Includes automated testing and quality checks
# - Docker integration for containerized deployment
# - Development tools setup for new contributors
# - Now uses proper C++ compilation with headers and linking

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================

# Compiler settings
CXX := g++
CC := gcc
GO := go

# Common flags
COMMON_FLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS := -lm -lpthread -lcrypto -lssl

# Build configurations
DEBUG_FLAGS := $(COMMON_FLAGS) -g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined
RELEASE_FLAGS := $(COMMON_FLAGS) -O3 -DNDEBUG -march=native -flto
PROFILE_FLAGS := $(COMMON_FLAGS) -O2 -pg -DPROFILE

# Default configuration
BUILD_TYPE ?= release

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
INCLUDE_DIR := include
TEST_DIR := tests
DOC_DIR := docs
DIST_DIR := dist

# Source files
CPP_SOURCES := $(wildcard $(SRC_DIR)/cpp/*.cpp)
CPP_HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)
GO_SOURCES := $(wildcard $(SRC_DIR)/go/*.go)

# Object files (exclude main files to avoid multiple main definitions)
CPP_OBJS := $(BUILD_DIR)/$(BUILD_TYPE)/physics.utilities.o \
            $(BUILD_DIR)/$(BUILD_TYPE)/ternary.fission.simulation.engine.o

# Executables
CPP_MAIN := $(BIN_DIR)/ternary-fission
GO_BINARY := $(BIN_DIR)/ternary-api

# Version information
VERSION := 1.1.11
BUILD_DATE := $(shell date -u +"%Y-%m-%d_%H:%M:%S_UTC")
GIT_COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")

# Version flags
VERSION_FLAGS := -DVERSION=\"$(VERSION)\" -DBUILD_DATE=\"$(BUILD_DATE)\" -DGIT_COMMIT=\"$(GIT_COMMIT)\"

# =============================================================================
# PRIMARY TARGETS
# =============================================================================

.PHONY: all clean test install docker help

# Default target
all: cpp-build go-build
	@echo "Build complete!"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)/*
	rm -rf $(BIN_DIR)/*
	rm -rf $(DIST_DIR)/
	find . -name "*.o" -delete
	find . -name "*.a" -delete
	find . -name "gmon.out" -delete
	cd $(SRC_DIR)/go && $(GO) clean -cache -testcache -modcache 2>/dev/null || true
	@echo "Build artifacts cleaned"

# Run tests
test: cpp-test go-test
	@echo "All tests completed"

# Install binaries
install: all
	@echo "Installing binaries..."
	install -d /usr/local/bin
	install -m 755 $(CPP_MAIN) /usr/local/bin/
	install -m 755 $(GO_BINARY) /usr/local/bin/
	@echo "Installation complete"

# Build Docker images
docker: docker-build
	@echo "Docker images built"

# Display help
help:
	@echo "Ternary Fission Energy Emulation System - Build System"
	@echo "===================================================="
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build everything (default)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  test         - Run all tests"
	@echo "  install      - Install binaries to system"
	@echo "  docker       - Build Docker images"
	@echo ""
	@echo "Component targets:"
	@echo "  cpp-build    - Build C++ simulation engine"
	@echo "  go-build     - Build Go API server"
	@echo "  cpp-test     - Run C++ unit tests"
	@echo "  go-test      - Run Go tests"
	@echo ""
	@echo "Build configurations:"
	@echo "  make BUILD_TYPE=debug    - Debug build with sanitizers"
	@echo "  make BUILD_TYPE=release  - Optimized release build (default)"
	@echo "  make BUILD_TYPE=profile  - Build with profiling support"
	@echo ""
	@echo "Development targets:"
	@echo "  dev-setup    - Set up development environment"
	@echo "  format       - Format source code"
	@echo "  lint         - Run linters"
	@echo "  docs         - Generate documentation"
	@echo ""
	@echo "Docker targets:"
	@echo "  docker-build - Build Docker images"
	@echo "  docker-run   - Run Docker containers"
	@echo "  docker-push  - Push images to registry"

# =============================================================================
# C++ BUILD TARGETS
# =============================================================================

# Create build directory
$(BUILD_DIR)/$(BUILD_TYPE):
	mkdir -p $(BUILD_DIR)/$(BUILD_TYPE)

# Compile physics utilities
$(BUILD_DIR)/$(BUILD_TYPE)/physics.utilities.o: $(SRC_DIR)/cpp/physics.utilities.cpp $(CPP_HEADERS) | $(BUILD_DIR)/$(BUILD_TYPE)
	@echo "Compiling physics utilities..."
ifeq ($(BUILD_TYPE),debug)
	$(CXX) $(DEBUG_FLAGS) $(VERSION_FLAGS) -c $< -o $@
else ifeq ($(BUILD_TYPE),profile)
	$(CXX) $(PROFILE_FLAGS) $(VERSION_FLAGS) -c $< -o $@
else
	$(CXX) $(RELEASE_FLAGS) $(VERSION_FLAGS) -c $< -o $@
endif

# Compile simulation engine
$(BUILD_DIR)/$(BUILD_TYPE)/ternary.fission.simulation.engine.o: $(SRC_DIR)/cpp/ternary.fission.simulation.engine.cpp $(CPP_HEADERS) | $(BUILD_DIR)/$(BUILD_TYPE)
	@echo "Compiling simulation engine..."
ifeq ($(BUILD_TYPE),debug)
	$(CXX) $(DEBUG_FLAGS) $(VERSION_FLAGS) -c $< -o $@
else ifeq ($(BUILD_TYPE),profile)
	$(CXX) $(PROFILE_FLAGS) $(VERSION_FLAGS) -c $< -o $@
else
	$(CXX) $(RELEASE_FLAGS) $(VERSION_FLAGS) -c $< -o $@
endif

# Link main executable
$(CPP_MAIN): $(CPP_OBJS) $(SRC_DIR)/cpp/main.ternary.fission.application.cpp | $(BIN_DIR)
ifeq ($(BUILD_TYPE),debug)
	@echo "Building C++ simulation engine (DEBUG)..."
	$(CXX) $(DEBUG_FLAGS) $(VERSION_FLAGS) $(SRC_DIR)/cpp/main.ternary.fission.application.cpp $(CPP_OBJS) -o $@ $(LDFLAGS)
else ifeq ($(BUILD_TYPE),profile)
	@echo "Building C++ simulation engine (PROFILE)..."
	$(CXX) $(PROFILE_FLAGS) $(VERSION_FLAGS) $(SRC_DIR)/cpp/main.ternary.fission.application.cpp $(CPP_OBJS) -o $@ $(LDFLAGS)
else
	@echo "Building C++ simulation engine (RELEASE)..."
	$(CXX) $(RELEASE_FLAGS) $(VERSION_FLAGS) $(SRC_DIR)/cpp/main.ternary.fission.application.cpp $(CPP_OBJS) -o $@ $(LDFLAGS)
endif
	@echo "C++ build complete: $@"

# Create bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# C++ build target
cpp-build: $(CPP_MAIN)

# Separate targets for different build types
cpp-debug:
	$(MAKE) cpp-build BUILD_TYPE=debug

cpp-release:
	$(MAKE) cpp-build BUILD_TYPE=release

cpp-profile:
	$(MAKE) cpp-build BUILD_TYPE=profile

# =============================================================================
# GO BUILD TARGETS
# =============================================================================

# Go build flags
GO_BUILD_FLAGS := -ldflags "-X main.Version=$(VERSION) -X main.BuildDate='$(BUILD_DATE)' -X main.GitCommit=$(GIT_COMMIT)"

# Build Go API server
$(GO_BINARY): $(GO_SOURCES)
	@echo "Building Go API server..."
	cd $(SRC_DIR)/go && $(GO) build $(GO_BUILD_FLAGS) -o ../../$(GO_BINARY) api.ternary.fission.server.go
	@echo "Go build complete: $(GO_BINARY)"

# Go build target
go-build: $(GO_BINARY)

# Go module initialization
go-mod-init:
	cd $(SRC_DIR)/go && $(GO) mod init ternary-fission-api || true
	cd $(SRC_DIR)/go && $(GO) mod tidy

# =============================================================================
# TESTING TARGETS
# =============================================================================

# C++ unit tests
cpp-test: cpp-build
	@echo "Running C++ unit tests..."
	@if [ -d "$(TEST_DIR)/cpp" ]; then \
		$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/test-physics $(TEST_DIR)/cpp/test_physics.cpp $(CPP_OBJS) $(LDFLAGS); \
		$(BIN_DIR)/test-physics; \
	else \
		echo "No C++ tests found in $(TEST_DIR)/cpp"; \
	fi

# Go tests
go-test:
	@echo "Running Go tests..."
	cd $(SRC_DIR)/go && $(GO) test -v ./...

# Integration tests
integration-test: all
	@echo "Running integration tests..."
	@if [ -f "$(TEST_DIR)/integration/test_integration.sh" ]; then \
		bash $(TEST_DIR)/integration/test_integration.sh; \
	else \
		echo "No integration tests found"; \
	fi

# =============================================================================
# DEVELOPMENT TARGETS
# =============================================================================

# Set up development environment
dev-setup:
	@echo "Setting up development environment..."
	@echo "Installing dependencies..."
	@which g++ > /dev/null || (echo "Error: g++ not found. Please install GCC." && exit 1)
	@which go > /dev/null || (echo "Error: go not found. Please install Go." && exit 1)
	@which docker > /dev/null || echo "Warning: Docker not found. Docker targets will not work."
	@echo "Creating directory structure..."
	mkdir -p $(BUILD_DIR) $(BIN_DIR) $(TEST_DIR) $(DOC_DIR) $(DIST_DIR)
	mkdir -p configs data results logs
	@echo "Initializing Go modules..."
	$(MAKE) go-mod-init
	@echo "Development environment ready!"

# Format source code
format:
	@echo "Formatting C++ code..."
	find $(SRC_DIR)/cpp $(INCLUDE_DIR) -name "*.cpp" -o -name "*.h" | xargs clang-format -i -style=file
	@echo "Formatting Go code..."
	cd $(SRC_DIR)/go && $(GO) fmt ./...

# Run linters
lint:
	@echo "Linting C++ code..."
	@which cppcheck > /dev/null && cppcheck --enable=all --suppress=missingIncludeSystem $(SRC_DIR)/cpp $(INCLUDE_DIR) || echo "cppcheck not found"
	@echo "Linting Go code..."
	cd $(SRC_DIR)/go && $(GO) vet ./...
	@which golint > /dev/null && cd $(SRC_DIR)/go && golint ./... || echo "golint not found"

# Generate documentation
docs:
	@echo "Generating documentation..."
	@which doxygen > /dev/null && doxygen Doxyfile || echo "Doxygen not found"
	@cd $(SRC_DIR)/go && $(GO) doc -all > ../../$(DOC_DIR)/go-api-docs.txt

# =============================================================================
# QUALITY ASSURANCE TARGETS
# =============================================================================

# Static analysis
static-analysis:
	@echo "Running static analysis..."
	@which clang-tidy > /dev/null && clang-tidy $(SRC_DIR)/cpp/*.cpp -- $(CXXFLAGS) || echo "clang-tidy not found"

# Memory leak check
memcheck: cpp-debug
	@echo "Running memory leak check..."
	@which valgrind > /dev/null && valgrind --leak-check=full --show-leak-kinds=all $(CPP_MAIN) -n 10 || echo "valgrind not found"

# Performance profiling
profile: cpp-profile
	@echo "Running performance profiling..."
	$(CPP_MAIN) -n 1000
	@which gprof > /dev/null && gprof $(CPP_MAIN) gmon.out > profile.txt && echo "Profile saved to profile.txt" || echo "gprof not found"

# Code coverage
coverage:
	@echo "Generating code coverage report..."
	$(MAKE) cpp-build BUILD_TYPE=debug CXXFLAGS="$(DEBUG_FLAGS) --coverage"
	$(CPP_MAIN) -n 100
	@which gcov > /dev/null && gcov $(SRC_DIR)/cpp/*.cpp || echo "gcov not found"

# =============================================================================
# DOCKER TARGETS
# =============================================================================

# Build Docker image
docker-build:
	@echo "Building Docker image..."
	docker build -t ternary-fission:$(VERSION) -f docker/Dockerfile .
	docker tag ternary-fission:$(VERSION) ternary-fission:latest

# Run Docker container
docker-run:
	@echo "Running Docker container..."
	docker run -d --name ternary-fission \
		-p 8080:8080 \
		-v $(PWD)/data:/app/data \
		-v $(PWD)/results:/app/results \
		ternary-fission:latest

# Push to registry
docker-push:
	@echo "Pushing Docker image to registry..."
	@read -p "Enter registry URL: " registry; \
	docker tag ternary-fission:$(VERSION) $$registry/ternary-fission:$(VERSION); \
	docker push $$registry/ternary-fission:$(VERSION)

# Docker compose
docker-compose:
	@echo "Starting services with Docker Compose..."
	docker-compose -f docker/docker-compose.yml up -d

# =============================================================================
# DISTRIBUTION TARGETS
# =============================================================================

# Create distribution package
dist: all docs
	@echo "Creating distribution package..."
	mkdir -p $(DIST_DIR)/ternary-fission-$(VERSION)
	cp -r $(BIN_DIR) $(DIST_DIR)/ternary-fission-$(VERSION)/
	cp -r configs $(DIST_DIR)/ternary-fission-$(VERSION)/
	cp -r $(DOC_DIR) $(DIST_DIR)/ternary-fission-$(VERSION)/
	cp README.md LICENSE $(DIST_DIR)/ternary-fission-$(VERSION)/ 2>/dev/null || true
	tar -czf $(DIST_DIR)/ternary-fission-$(VERSION).tar.gz -C $(DIST_DIR) ternary-fission-$(VERSION)
	@echo "Distribution package created: $(DIST_DIR)/ternary-fission-$(VERSION).tar.gz"

# Create Debian package
deb-package:
	@echo "Creating Debian package..."
	mkdir -p $(DIST_DIR)/debian/DEBIAN
	mkdir -p $(DIST_DIR)/debian/usr/local/bin
	cp $(BIN_DIR)/* $(DIST_DIR)/debian/usr/local/bin/
	echo "Package: ternary-fission" > $(DIST_DIR)/debian/DEBIAN/control
	echo "Version: $(VERSION)" >> $(DIST_DIR)/debian/DEBIAN/control
	echo "Architecture: amd64" >> $(DIST_DIR)/debian/DEBIAN/control
	echo "Maintainer: bthlops <davestj@gmail.com>" >> $(DIST_DIR)/debian/DEBIAN/control
	echo "Description: Ternary Fission Energy Emulation System" >> $(DIST_DIR)/debian/DEBIAN/control
	dpkg-deb --build $(DIST_DIR)/debian $(DIST_DIR)/ternary-fission_$(VERSION)_amd64.deb

# =============================================================================
# UTILITY TARGETS
# =============================================================================

# Show build information
info:
	@echo "Build Information:"
	@echo "  Version: $(VERSION)"
	@echo "  Build Date: $(BUILD_DATE)"
	@echo "  Git Commit: $(GIT_COMMIT)"
	@echo "  Build Type: $(BUILD_TYPE)"
	@echo "  CXX: $(CXX)"
	@echo "  CXXFLAGS: $(CXXFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"

# Count lines of code
loc:
	@echo "Lines of code:"
	@echo "  C++ source:"
	@find $(SRC_DIR)/cpp $(INCLUDE_DIR) -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1
	@echo "  Go source:"
	@find $(SRC_DIR)/go -name "*.go" | xargs wc -l | tail -1

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@which $(CXX) > /dev/null && echo "✓ C++ compiler: $(CXX)" || echo "✗ C++ compiler not found"
	@which $(GO) > /dev/null && echo "✓ Go compiler: $(GO)" || echo "✗ Go compiler not found"
	@which docker > /dev/null && echo "✓ Docker" || echo "✗ Docker not found"
	@which make > /dev/null && echo "✓ Make" || echo "✗ Make not found"
	@pkg-config --exists openssl && echo "✓ OpenSSL" || echo "✗ OpenSSL not found"

# =============================================================================
# PHONY TARGETS DECLARATION
# =============================================================================

.PHONY: cpp-build cpp-debug cpp-release cpp-profile go-build go-mod-init
.PHONY: cpp-test go-test integration-test
.PHONY: dev-setup format lint docs
.PHONY: static-analysis memcheck profile coverage
.PHONY: docker-build docker-run docker-push docker-compose
.PHONY: dist deb-package
.PHONY: info loc check-deps
