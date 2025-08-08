# File: Makefile
# Author: bthlops (David StJ)
# Date: July 31, 2025
# Title: Ternary Fission Energy Emulation System - Build Configuration - macOS Fixed
# Purpose: Comprehensive build system for C++ simulation engine and Go API server
# Reason: Provides automated compilation, testing, and deployment workflows with macOS support
#
# Change Log:
# 2025-07-29: Initial creation with multi-target build system
# 2025-07-29: Added Docker containerization support
# 2025-07-29: Integrated testing and documentation generation
# 2025-07-29: Added development environment setup
# 2025-07-29: Fixed compilation structure to use proper header includes
# 2025-07-31: FIXED macOS OpenSSL compilation issues with Homebrew path detection
#             Added OS detection for Darwin/Linux/Windows compatibility
#             Automatic detection of OpenSSL paths for ARM64 and x86_64 macOS
#
# Carry-over Context:
# - Supports multiple build configurations (debug, release, profile)
# - Includes automated testing and quality checks
# - Docker integration for containerized deployment
# - Development tools setup for new contributors
# - Now uses proper C++ compilation with headers and linking
# - Fixed macOS OpenSSL linking with Homebrew detection

# =============================================================================
# PLATFORM DETECTION AND CONFIGURATION
# =============================================================================

# We detect the operating system and architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# We set platform-specific variables
ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
    # We detect Homebrew installation paths for macOS
    ifeq ($(UNAME_M),arm64)
	HOMEBREW_PREFIX := /opt/homebrew
    else
	HOMEBREW_PREFIX := /usr/local
    endif
    # We check if OpenSSL is installed via Homebrew
    OPENSSL_PREFIX := $(HOMEBREW_PREFIX)/opt/openssl@3
    ifneq ($(wildcard $(OPENSSL_PREFIX)/include/openssl/evp.h),)
	OPENSSL_FOUND := true
	OPENSSL_INCLUDE := -I$(OPENSSL_PREFIX)/include
	OPENSSL_LIB := -L$(OPENSSL_PREFIX)/lib
    else
	# We fall back to system OpenSSL if Homebrew version not found
	OPENSSL_FOUND := false
	OPENSSL_INCLUDE :=
	OPENSSL_LIB :=
    endif
else ifeq ($(UNAME_S),Linux)
    PLATFORM := linux
    OPENSSL_FOUND := true
    OPENSSL_INCLUDE :=
    OPENSSL_LIB :=
else
    PLATFORM := windows
    OPENSSL_FOUND := false
    OPENSSL_INCLUDE :=
    OPENSSL_LIB :=
endif

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================

# Compiler settings
CXX := g++
CC := gcc
GO := go

# We set base compiler flags
COMMON_FLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Iinclude $(OPENSSL_INCLUDE)

# We configure linker flags based on platform
ifeq ($(PLATFORM),macos)
    LDFLAGS := -lm -lpthread $(OPENSSL_LIB) -lcrypto -lssl
    # We add macOS-specific flags
    COMMON_FLAGS += -DMACOS
else ifeq ($(PLATFORM),linux)
    LDFLAGS := -lm -lpthread -lcrypto -lssl
    COMMON_FLAGS += -DLINUX
else
    LDFLAGS := -lm -lpthread
    COMMON_FLAGS += -DWINDOWS
endif

# Build configurations
DEBUG_FLAGS := $(COMMON_FLAGS) -g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined
RELEASE_FLAGS := $(COMMON_FLAGS) -O3 -DNDEBUG -flto
PROFILE_FLAGS := $(COMMON_FLAGS) -O2 -pg -DPROFILE

# We adjust march flag for macOS ARM
ifeq ($(PLATFORM),macos)
    ifeq ($(UNAME_M),arm64)
	RELEASE_FLAGS := $(COMMON_FLAGS) -O3 -DNDEBUG -flto -mcpu=apple-m1
    else
	RELEASE_FLAGS := $(COMMON_FLAGS) -O3 -DNDEBUG -march=native -flto
    endif
else
    RELEASE_FLAGS := $(COMMON_FLAGS) -O3 -DNDEBUG -march=native -flto
endif

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
VERSION := 1.1.13
BUILD_DATE := $(shell date -u +"%Y-%m-%d_%H:%M:%S_UTC")
GIT_COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")

# Version flags
VERSION_FLAGS := -DVERSION=\"$(VERSION)\" -DBUILD_DATE=\"$(BUILD_DATE)\" -DGIT_COMMIT=\"$(GIT_COMMIT)\"

# =============================================================================
# PRIMARY TARGETS
# =============================================================================

.PHONY: all clean test install docker help check-openssl

# Default target with OpenSSL check
all: check-openssl cpp-build go-build
	@echo "Build complete!"

# We check for OpenSSL availability
check-openssl:
ifeq ($(PLATFORM),macos)
	@echo "Platform: macOS ($(UNAME_M))"
	@echo "Homebrew prefix: $(HOMEBREW_PREFIX)"
ifeq ($(OPENSSL_FOUND),true)
	@echo "✓ OpenSSL found at: $(OPENSSL_PREFIX)"
else
	@echo "✗ OpenSSL not found. Installing via Homebrew..."
	@echo "Run: brew install openssl@3"
	@echo "Or manually set OPENSSL_PREFIX environment variable"
	@exit 1
endif
else
	@echo "Platform: $(PLATFORM)"
	@echo "✓ Using system OpenSSL"
endif

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
ifeq ($(PLATFORM),macos)
	install -d /usr/local/bin
	install -m 755 $(CPP_MAIN) /usr/local/bin/
	install -m 755 $(GO_BINARY) /usr/local/bin/
else
	sudo install -d /usr/local/bin
	sudo install -m 755 $(CPP_MAIN) /usr/local/bin/
	sudo install -m 755 $(GO_BINARY) /usr/local/bin/
endif
	@echo "Installation complete"

# Build Docker images
docker: docker-build
	@echo "Docker images built"

# Display help
help:
	@echo "Ternary Fission Energy Emulation System - Build System"
	@echo "===================================================="
	@echo ""
	@echo "Platform: $(PLATFORM) ($(UNAME_M))"
ifeq ($(PLATFORM),macos)
	@echo "OpenSSL: $(if $(OPENSSL_FOUND),Found at $(OPENSSL_PREFIX),NOT FOUND)"
endif
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build everything (default)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  test         - Run all tests"
	@echo "  install      - Install binaries to system"
	@echo "  docker       - Build Docker images"
	@echo "  check-openssl - Check OpenSSL installation"
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
ifeq ($(PLATFORM),macos)
	@echo "macOS Setup Commands:"
	@echo "  brew install openssl@3  - Install OpenSSL via Homebrew"
	@echo "  brew install gcc        - Install GCC compiler"
	@echo "  brew install go         - Install Go compiler"
endif

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
	$(CXX) $(DEBUG_FLAGS) $(VERSION_FLAGS) $(SRC_DIR)/cpp/main.ternary.fission.application.cpp $(CPP_OBJS) \
	$(SRC_DIR)/cpp/daemon.ternary.fission.server.cpp $(SRC_DIR)/cpp/config.ternary.fission.server.cpp -o $@ $(LDFLAGS)
else ifeq ($(BUILD_TYPE),profile)
	@echo "Building C++ simulation engine (PROFILE)..."
	$(CXX) $(PROFILE_FLAGS) $(VERSION_FLAGS) $(SRC_DIR)/cpp/main.ternary.fission.application.cpp $(CPP_OBJS) \
	$(SRC_DIR)/cpp/daemon.ternary.fission.server.cpp $(SRC_DIR)/cpp/config.ternary.fission.server.cpp -o $@ $(LDFLAGS)
else
	@echo "Building C++ simulation engine (RELEASE)..."
	$(CXX) $(RELEASE_FLAGS) $(VERSION_FLAGS) $(SRC_DIR)/cpp/main.ternary.fission.application.cpp $(CPP_OBJS) \
	$(SRC_DIR)/cpp/daemon.ternary.fission.server.cpp $(SRC_DIR)/cpp/config.ternary.fission.server.cpp -o $@ $(LDFLAGS)
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

# We define CXXFLAGS based on build type for testing
ifeq ($(BUILD_TYPE),debug)
    TEST_CXXFLAGS := $(DEBUG_FLAGS)
else ifeq ($(BUILD_TYPE),profile)
    TEST_CXXFLAGS := $(PROFILE_FLAGS)
else
    TEST_CXXFLAGS := $(RELEASE_FLAGS)
endif

# C++ unit tests
cpp-test: cpp-build
	@echo "Running C++ unit tests..."
	@if [ -d "$(TEST_DIR)/cpp" ]; then \
		echo "Compiling C++ tests..."; \
		$(CXX) $(TEST_CXXFLAGS) $(VERSION_FLAGS) -o $(BIN_DIR)/test-physics $(TEST_DIR)/cpp/test_physics.cpp $(CPP_OBJS) $(LDFLAGS); \
		echo "Executing C++ tests..."; \
		$(BIN_DIR)/test-physics; \
	else \
		echo "Creating basic C++ test structure..."; \
		mkdir -p $(TEST_DIR)/cpp; \
		echo "// Basic test placeholder" > $(TEST_DIR)/cpp/test_physics.cpp; \
		echo "No C++ tests found - created placeholder in $(TEST_DIR)/cpp"; \
	fi

# Go tests
go-test:
	@echo "Running Go tests..."
	@if [ -d "$(SRC_DIR)/go" ]; then \
		cd $(SRC_DIR)/go && $(GO) test -v -race -coverprofile=coverage.out ./...; \
		echo "Go test coverage:"; \
		cd $(SRC_DIR)/go && $(GO) tool cover -func=coverage.out; \
	else \
		echo "No Go source directory found"; \
	fi

# Integration tests
integration-test: all
	@echo "Running integration tests..."
	@if [ -f "$(TEST_DIR)/integration/test_integration.sh" ]; then \
		chmod +x $(TEST_DIR)/integration/test_integration.sh; \
		bash $(TEST_DIR)/integration/test_integration.sh; \
	else \
		echo "Creating integration test structure..."; \
		mkdir -p $(TEST_DIR)/integration; \
		echo '#!/bin/bash' > $(TEST_DIR)/integration/test_integration.sh; \
		echo 'echo "Integration test placeholder"' >> $(TEST_DIR)/integration/test_integration.sh; \
		echo 'exit 0' >> $(TEST_DIR)/integration/test_integration.sh; \
		chmod +x $(TEST_DIR)/integration/test_integration.sh; \
		echo "Created integration test placeholder"; \
	fi

# Performance benchmarks
benchmark: cpp-build
	@echo "Running performance benchmarks..."
	@echo "Testing with 1000 events..."
	time $(CPP_MAIN) -n 1000 -j benchmark_1k.json
	@echo "Testing with 10000 events..."
	time $(CPP_MAIN) -n 10000 -j benchmark_10k.json
	@echo "Benchmark results saved to benchmark_*.json"

# Set up development environment
dev-setup:
	@echo "Setting up development environment..."
	@echo "Platform: $(PLATFORM)"
ifeq ($(PLATFORM),macos)
	@echo "Checking macOS dependencies..."
	@which brew > /dev/null || (echo "Error: Homebrew not found. Install from https://brew.sh" && exit 1)
	@which g++ > /dev/null || (echo "Installing GCC..." && brew install gcc)
	@which go > /dev/null || (echo "Installing Go..." && brew install go)
	@test -f $(OPENSSL_PREFIX)/include/openssl/evp.h || (echo "Installing OpenSSL..." && brew install openssl@3)
	@which docker > /dev/null || echo "Warning: Docker not found. Install Docker Desktop for Mac"
else ifeq ($(PLATFORM),linux)
	@echo "Checking Ubuntu/Debian dependencies..."
	@echo "Updating package lists..."
	@sudo apt update
	@echo "Installing core development tools..."
	@sudo apt install -y build-essential gcc-13 g++-13 cmake make pkg-config
	@echo "Installing scientific computing libraries..."
	@sudo apt install -y libssl-dev libcrypto++-dev libgsl-dev libeigen3-dev libfftw3-dev libopenblas-dev liblapack-dev libboost-all-dev
	@echo "Installing additional development tools..."
	@sudo apt install -y git curl wget valgrind cppcheck clang-format clang-tidy doxygen
	@which go > /dev/null || (echo "Installing Go..." && sudo apt install -y golang-go)
	@which docker > /dev/null || (echo "Installing Docker..." && sudo apt install -y docker.io && sudo usermod -aG docker $USER)
else
	@echo "Installing dependencies for Windows/Other..."
	@which g++ > /dev/null || (echo "Error: g++ not found. Please install GCC." && exit 1)
	@which go > /dev/null || (echo "Error: go not found. Please install Go." && exit 1)
	@which docker > /dev/null || echo "Warning: Docker not found. Docker targets will not work."
endif
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
	@echo "Running cppcheck on C++ code..."
	@which cppcheck > /dev/null && cppcheck --enable=all --suppress=missingIncludeSystem --xml --xml-version=2 $(SRC_DIR)/cpp $(INCLUDE_DIR) 2> static_analysis.xml || echo "cppcheck not found"
	@echo "Running clang-tidy on C++ code..."
	@which clang-tidy > /dev/null && clang-tidy $(SRC_DIR)/cpp/*.cpp -- $(TEST_CXXFLAGS) > clang_tidy_report.txt 2>&1 || echo "clang-tidy not found"
	@echo "Running Go vet..."
	@cd $(SRC_DIR)/go && $(GO) vet ./... > ../../go_vet_report.txt 2>&1
	@echo "Static analysis reports generated"

# Memory leak check with comprehensive testing
memcheck: cpp-debug
	@echo "Running memory leak check with Valgrind..."
	@which valgrind > /dev/null || (echo "Valgrind not found. Install with: sudo apt install valgrind (Linux) or brew install valgrind (macOS)" && exit 1)
	@echo "Testing basic functionality..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --xml=yes --xml-file=valgrind_basic.xml $(CPP_MAIN) -n 10 --help > valgrind_basic.log 2>&1
	@echo "Testing simulation with 100 events..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --xml=yes --xml-file=valgrind_simulation.xml $(CPP_MAIN) -n 100 -j valgrind_test.json > valgrind_simulation.log 2>&1
	@echo "Memory check reports saved to valgrind_*.xml and valgrind_*.log"

# Performance profiling with gprof
profile: cpp-profile
	@echo "Running performance profiling..."
	@echo "Executing profiling run with 1000 events..."
	$(CPP_MAIN) -n 1000 -j profile_test.json
	@which gprof > /dev/null && gprof $(CPP_MAIN) gmon.out > gprof_analysis.txt && echo "Profile analysis saved to gprof_analysis.txt" || echo "gprof not found"
	@echo "Running continuous simulation profile test..."
	timeout 10s $(CPP_MAIN) -c -d 5 -r 50 -j profile_continuous.json || true
	@which gprof > /dev/null && gprof $(CPP_MAIN) gmon.out > gprof_continuous.txt && echo "Continuous profile saved to gprof_continuous.txt" || echo "gprof not found"

# Code coverage analysis
coverage:
	@echo "Generating code coverage report..."
	@echo "Building with coverage instrumentation..."
	$(MAKE) clean
	$(MAKE) cpp-build BUILD_TYPE=debug RELEASE_FLAGS="$(DEBUG_FLAGS) --coverage"
	@echo "Running instrumented binary..."
	$(CPP_MAIN) -n 100 -j coverage_test.json
	$(CPP_MAIN) -c -d 2 -r 10 -j coverage_continuous.json || true
	@echo "Generating coverage reports..."
	@which gcov > /dev/null && for file in $(SRC_DIR)/cpp/*.cpp; do gcov $file; done || echo "gcov not found"
	@which lcov > /dev/null && lcov --capture --directory . --output-file coverage.info && genhtml coverage.info --output-directory coverage_html && echo "HTML coverage report generated in coverage_html/" || echo "lcov not found - install with: sudo apt install lcov"
	@echo "Coverage analysis complete"

# =============================================================================
# DOCKER TARGETS
# =============================================================================

# Build Docker image
docker-build:
	@echo "Building Docker image..."
	@test -f Dockerfile || (echo "Error: Dockerfile not found in project root" && exit 1)
	docker build -t ternary-fission:$(VERSION) -f Dockerfile .
	docker tag ternary-fission:$(VERSION) ternary-fission:latest
	@echo "Docker image built: ternary-fission:$(VERSION)"

# Run Docker container
docker-run: docker-build
	@echo "Running Docker container..."
	@docker stop ternary-fission 2>/dev/null || true
	@docker rm ternary-fission 2>/dev/null || true
	docker run -d --name ternary-fission \
		-p 8080:8080 \
		-v $(PWD)/data:/app/data \
		-v $(PWD)/results:/app/results \
		-v $(PWD)/logs:/app/logs \
		-v $(PWD)/configs:/app/configs:ro \
		ternary-fission:latest
	@echo "Container started. API available at http://localhost:8080"
	@echo "Check logs with: docker logs ternary-fission"

# Stop Docker container
docker-stop:
	@echo "Stopping Docker container..."
	@docker stop ternary-fission 2>/dev/null || echo "Container not running"
	@docker rm ternary-fission 2>/dev/null || echo "Container already removed"

# Push to registry
docker-push: docker-build
	@echo "Pushing Docker image to registry..."
	@read -p "Enter registry URL (e.g., ghcr.io/username): " registry; \
	if [ -n "$registry" ]; then \
		docker tag ternary-fission:$(VERSION) $registry/ternary-fission:$(VERSION); \
		docker tag ternary-fission:$(VERSION) $registry/ternary-fission:latest; \
		docker push $registry/ternary-fission:$(VERSION); \
		docker push $registry/ternary-fission:latest; \
		echo "Images pushed to $registry"; \
	else \
		echo "Registry URL required"; \
	fi

# Docker compose operations
docker-compose: docker-build
	@echo "Starting services with Docker Compose..."
	@test -f docker-compose.yml || (echo "Error: docker-compose.yml not found" && exit 1)
	docker-compose up -d
	@echo "Services started. Check status with: docker-compose ps"

# Stop docker compose
docker-compose-down:
	@echo "Stopping Docker Compose services..."
	@test -f docker-compose.yml && docker-compose down || echo "docker-compose.yml not found"

# Show Docker logs
docker-logs:
	@docker logs --follow ternary-fission 2>/dev/null || echo "Container not running"

# =============================================================================
# DISTRIBUTION TARGETS
# =============================================================================

# Create distribution package
dist: all docs
	@echo "Creating distribution package..."
	mkdir -p $(DIST_DIR)/ternary-fission-$(VERSION)
	@echo "Copying binaries..."
	cp -r $(BIN_DIR) $(DIST_DIR)/ternary-fission-$(VERSION)/
	@echo "Copying configuration files..."
	cp -r configs $(DIST_DIR)/ternary-fission-$(VERSION)/
	@echo "Copying documentation..."
	test -d $(DOC_DIR) && cp -r $(DOC_DIR) $(DIST_DIR)/ternary-fission-$(VERSION)/ || mkdir -p $(DIST_DIR)/ternary-fission-$(VERSION)/$(DOC_DIR)
	@echo "Copying project files..."
	cp README.md $(DIST_DIR)/ternary-fission-$(VERSION)/ 2>/dev/null || echo "README.md not found"
	cp LICENSE $(DIST_DIR)/ternary-fission-$(VERSION)/ 2>/dev/null || echo "LICENSE not found"
	cp HOW-TO.md $(DIST_DIR)/ternary-fission-$(VERSION)/ 2>/dev/null || echo "HOW-TO.md not found"
	cp Makefile $(DIST_DIR)/ternary-fission-$(VERSION)/ 2>/dev/null || echo "Makefile not found"
	@echo "Creating changelog..."
	$(MAKE) changelog > $(DIST_DIR)/ternary-fission-$(VERSION)/CHANGELOG.md 2>/dev/null || echo "No git history for changelog"
	@echo "Creating tar.gz package..."
	tar -czf $(DIST_DIR)/ternary-fission-$(VERSION).tar.gz -C $(DIST_DIR) ternary-fission-$(VERSION)
	@echo "Creating zip package..."
	cd $(DIST_DIR) && zip -r ternary-fission-$(VERSION).zip ternary-fission-$(VERSION)/
	@echo "Distribution packages created:"
	@echo "  $(DIST_DIR)/ternary-fission-$(VERSION).tar.gz"
	@echo "  $(DIST_DIR)/ternary-fission-$(VERSION).zip"

# Create Debian package (Linux only)
deb-package: all
ifeq ($(PLATFORM),linux)
	@echo "Creating Debian package..."
	mkdir -p $(DIST_DIR)/debian/DEBIAN
	mkdir -p $(DIST_DIR)/debian/usr/local/bin
	mkdir -p $(DIST_DIR)/debian/etc/ternary-fission
	mkdir -p $(DIST_DIR)/debian/usr/share/doc/ternary-fission
	@echo "Copying binaries..."
	cp $(BIN_DIR)/* $(DIST_DIR)/debian/usr/local/bin/
	@echo "Copying configuration..."
	cp -r configs/* $(DIST_DIR)/debian/etc/ternary-fission/
	@echo "Creating control file..."
	echo "Package: ternary-fission" > $(DIST_DIR)/debian/DEBIAN/control
	echo "Version: $(VERSION)" >> $(DIST_DIR)/debian/DEBIAN/control
	echo "Architecture: amd64" >> $(DIST_DIR)/debian/DEBIAN/control
	echo "Maintainer: bthlops <davestj@gmail.com>" >> $(DIST_DIR)/debian/DEBIAN/control
	echo "Description: Ternary Fission Energy Emulation System" >> $(DIST_DIR)/debian/DEBIAN/control
	echo "Homepage: https://github.com/davestj/ternary-fission-reactor" >> $(DIST_DIR)/debian/DEBIAN/control
	echo "Depends: libssl3, libgsl27, libboost-all-dev" >> $(DIST_DIR)/debian/DEBIAN/control
	@echo "Building package..."
	dpkg-deb --build $(DIST_DIR)/debian $(DIST_DIR)/ternary-fission_$(VERSION)_amd64.deb
	@echo "Debian package created: $(DIST_DIR)/ternary-fission_$(VERSION)_amd64.deb"
else
	@echo "Debian packages can only be created on Linux"
endif

# Create RPM package (Linux only)
rpm-package: all
ifeq ($(PLATFORM),linux)
	@echo "Creating RPM package..."
	@which rpmbuild > /dev/null || (echo "rpmbuild not found. Install with: sudo apt install rpm" && exit 1)
	mkdir -p $(DIST_DIR)/rpm/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
	@echo "Creating RPM spec file..."
	echo "Name: ternary-fission" > $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "Version: $(VERSION)" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "Release: 1" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "Summary: Ternary Fission Energy Emulation System" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "License: MIT" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "URL: https://github.com/davestj/ternary-fission-reactor" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "%description" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "High-performance ternary nuclear fission simulation with energy field mapping" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "%files" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "/usr/local/bin/ternary-fission" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	echo "/usr/local/bin/ternary-api" >> $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	tar -czf $(DIST_DIR)/rpm/SOURCES/ternary-fission-$(VERSION).tar.gz -C $(DIST_DIR) ternary-fission-$(VERSION)
	rpmbuild --define "_topdir $(PWD)/$(DIST_DIR)/rpm" -bb $(DIST_DIR)/rpm/SPECS/ternary-fission.spec
	@echo "RPM package created in $(DIST_DIR)/rpm/RPMS/"
else
	@echo "RPM packages can only be created on Linux"
endif

# =============================================================================
# UTILITY TARGETS
# =============================================================================

# Show build information
info:
	@echo "Build Information:"
	@echo "  Platform: $(PLATFORM) ($(UNAME_M))"
	@echo "  Version: $(VERSION)"
	@echo "  Build Date: $(BUILD_DATE)"
	@echo "  Git Commit: $(GIT_COMMIT)"
	@echo "  Build Type: $(BUILD_TYPE)"
	@echo "  CXX: $(CXX)"
	@echo "  CXXFLAGS: $(TEST_CXXFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
ifeq ($(PLATFORM),macos)
	@echo "  Homebrew: $(HOMEBREW_PREFIX)"
	@echo "  OpenSSL: $(if $(OPENSSL_FOUND),$(OPENSSL_PREFIX),NOT FOUND)"
endif

# Count lines of code
loc:
	@echo "Lines of code:"
	@echo "  C++ source:"
	@find $(SRC_DIR)/cpp $(INCLUDE_DIR) -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1
	@echo "  Go source:"
	@find $(SRC_DIR)/go -name "*.go" | xargs wc -l | tail -1
	@echo "  Total:"
	@find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.h" -o -name "*.go" | xargs wc -l | tail -1

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@which $(CXX) > /dev/null && echo "✓ C++ compiler: $(CXX)" || echo "✗ C++ compiler not found"
	@which $(GO) > /dev/null && echo "✓ Go compiler: $(GO)" || echo "✗ Go compiler not found"
	@which docker > /dev/null && echo "✓ Docker" || echo "✗ Docker not found"
	@which make > /dev/null && echo "✓ Make" || echo "✗ Make not found"
	@which git > /dev/null && echo "✓ Git" || echo "✗ Git not found"
ifeq ($(PLATFORM),macos)
	@test -f $(OPENSSL_PREFIX)/include/openssl/evp.h && echo "✓ OpenSSL: $(OPENSSL_PREFIX)" || echo "✗ OpenSSL not found - run 'brew install openssl@3'"
	@which brew > /dev/null && echo "✓ Homebrew" || echo "✗ Homebrew not found"
else
	@pkg-config --exists openssl && echo "✓ OpenSSL" || echo "✗ OpenSSL not found"
endif

# Generate changelog from git commits (last 5 commits)
changelog:
	@echo "# Changelog"
	@echo ""
	@echo "Generated: $(BUILD_DATE)"
	@echo "Version: $(VERSION)"
	@echo ""
	@if [ -d .git ]; then \
		echo "## Recent Changes (Last 5 Commits)"; \
		echo ""; \
		git log --oneline -5 --pretty=format:"- %h: %s (%an, %ad)" --date=short 2>/dev/null || echo "No git history available"; \
		echo ""; \
		echo ""; \
		echo "## Detailed Changes"; \
		echo ""; \
		git log -5 --pretty=format:"### %h: %s%n**Author:** %an <%ae>%n**Date:** %ad%n%n%b%n" --date=short 2>/dev/null || echo "No git history available"; \
	else \
		echo "No git repository found"; \
	fi

# Create release tag and package
release: all test static-analysis
	@echo "Creating release $(VERSION)..."
	@if [ -d .git ]; then \
		echo "Tagging release..."; \
		git tag -a v$(VERSION) -m "Release version $(VERSION)"; \
		echo "Release tagged: v$(VERSION)"; \
	else \
		echo "No git repository - skipping tag"; \
	fi
	@echo "Creating distribution packages..."
	$(MAKE) dist
	@echo "Release $(VERSION) complete"

# Create GitHub release package
github-release: release
	@echo "Preparing GitHub release package..."
	@echo "Creating release notes..."
	$(MAKE) changelog > $(DIST_DIR)/RELEASE_NOTES_$(VERSION).md
	@echo "Release package ready for GitHub:"
	@echo "  Tag: v$(VERSION)"
	@echo "  Assets: $(DIST_DIR)/ternary-fission-$(VERSION).tar.gz"
	@echo "  Assets: $(DIST_DIR)/ternary-fission-$(VERSION).zip"
	@echo "  Notes: $(DIST_DIR)/RELEASE_NOTES_$(VERSION).md"

# Git commit with automated message
git-commit:
	@echo "Staging changes..."
	@git add -A
	@echo "Enter commit message (or press Enter for auto-generated):"
	@read -r message; \
	if [ -z "$message" ]; then \
		message="Build $(VERSION) - $(BUILD_DATE)"; \
	fi; \
	git commit -m "$message" || echo "Nothing to commit"
	@echo "Commit complete"

# Git commit log analysis
git-log:
	@echo "Git Commit Log Analysis"
	@echo "======================="
	@if [ -d .git ]; then \
		echo "Last 10 commits:"; \
		git log --oneline -10; \
		echo ""; \
		echo "Commit statistics:"; \
		echo "  Total commits: $(git rev-list --count HEAD 2>/dev/null || echo 0)"; \
		echo "  Contributors: $(git shortlog -sn | wc -l)"; \
		echo "  First commit: $(git log --reverse --pretty=format:'%ad' --date=short | head -1 2>/dev/null || echo 'Unknown')"; \
		echo "  Last commit: $(git log -1 --pretty=format:'%ad' --date=short 2>/dev/null || echo 'Unknown')"; \
		echo ""; \
		echo "Top contributors:"; \
		git shortlog -sn | head -5 2>/dev/null || echo "No contributors found"; \
	else \
		echo "No git repository found"; \
	fi

# Version bump utilities
version-patch:
	@echo "Current version: $(VERSION)"
	@echo "Bumping patch version..."
	# This would require a more complex version parsing system
	@echo "Manual version update required in Makefile"

version-minor:
	@echo "Current version: $(VERSION)"
	@echo "Bumping minor version..."
	@echo "Manual version update required in Makefile"

version-major:
	@echo "Current version: $(VERSION)"
	@echo "Bumping major version..."
	@echo "Manual version update required in Makefile"

# System resource monitoring
monitor:
	@echo "System Resource Monitor"
	@echo "======================"
	@echo "CPU Usage:"
	@top -l 1 -n 0 | grep "CPU usage" 2>/dev/null || echo "CPU info not available"
	@echo ""
	@echo "Memory Usage:"
	@free -h 2>/dev/null || vm_stat | head -5 2>/dev/null || echo "Memory info not available"
	@echo ""
	@echo "Disk Usage:"
	@df -h . 2>/dev/null || echo "Disk info not available"
	@echo ""
	@echo "Build artifacts:"
	@du -sh $(BUILD_DIR) $(BIN_DIR) $(DIST_DIR) 2>/dev/null || echo "No build artifacts"

# Clean everything including git ignored files
deep-clean: clean
	@echo "Performing deep clean..."
	@git clean -fdx 2>/dev/null || echo "Not a git repository or git not available"
	@echo "Deep clean complete"

# =============================================================================
# MACOS-SPECIFIC TARGETS
# =============================================================================

# Install macOS dependencies
macos-deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing macOS dependencies via Homebrew..."
	brew install openssl@3 gcc go
	@echo "Dependencies installed successfully"
else
	@echo "This target is only for macOS"
endif

# =============================================================================
# PHONY TARGETS DECLARATION
# =============================================================================

.PHONY: all clean test install docker help check-openssl check-deps
.PHONY: cpp-build cpp-debug cpp-release cpp-profile go-build go-mod-init
.PHONY: cpp-test go-test integration-test benchmark
.PHONY: dev-setup format lint docs
.PHONY: static-analysis memcheck profile coverage
.PHONY: docker-build docker-run docker-stop docker-push docker-compose docker-compose-down docker-logs
.PHONY: dist deb-package rpm-package
.PHONY: info loc macos-deps monitor deep-clean
.PHONY: changelog release github-release git-commit git-log
.PHONY: version-patch version-minor version-major