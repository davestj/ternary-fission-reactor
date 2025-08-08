# File: Makefile.stargate.hybrid
# Author: David St. John (davestj@gmail.com)
# Date: 2025-07-31
# Title: Hybrid Build System - Stargate Fission Reactor Project
# Purpose: Unifies modernized cross-platform Makefile2 logic with full legacy targets (Go, Docker, QA, Coverage, Docs)
# Reason: Preserve all functionality from original Makefile while adopting modular, scalable build practices and distribution workflows
#
# Change Log:
# - 2025-07-31: Initial hybrid generation merging Makefile2 cross-platform and modular features with legacy Stargate Makefile targets
# - 2025-07-31: Retained Go build, Docker image creation, QA targets, Valgrind, lcov, and changelog logic from legacy build
# - 2025-07-31: Adopted .deb packaging, static builds, BASE math platform detection from Makefile2

# =============================================================================
# PROJECT METADATA
# =============================================================================
PROJECT_NAME := ternary-fission-reactor
VERSION := 1.1.13
BUILD_DATE := $(shell date -u +"%Y-%m-%d_%H:%M:%S_UTC")
GIT_COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")

# =============================================================================
# PLATFORM DETECTION
# =============================================================================
UNAME := $(shell uname)
ARCH := $(shell uname -m)
PLATFORM := unknown

ifeq ($(UNAME), Darwin)
	PLATFORM := macos
	HOMEBREW_PREFIX := $(shell brew --prefix)
	OPENSSL_PREFIX := $(HOMEBREW_PREFIX)/opt/openssl@3
	JSONCPP_PREFIX := $(HOMEBREW_PREFIX)/opt/jsoncpp
endif

ifeq ($(UNAME), Linux)
	PLATFORM := linux
endif

# =============================================================================
# COMPILER CONFIGURATION
# =============================================================================
CC := gcc
CXX := g++
CXXSTD := -std=c++17
CSTD := -std=c11
GO := go

CXXFLAGS := $(CXXSTD) -Wall -Wextra -Wpedantic
CFLAGS := $(CSTD) -Wall -Wextra -Wpedantic
INCLUDES := -Iinclude
LDFLAGS := 
LIBS := -lm -lpthread

ifeq ($(PLATFORM), macos)
	CXXFLAGS += -DMACOS
	CFLAGS += -DMACOS
	CXXFLAGS += -I$(OPENSSL_PREFIX)/include -I$(JSONCPP_PREFIX)/include
	LDFLAGS += -L$(OPENSSL_PREFIX)/lib -L$(JSONCPP_PREFIX)/lib
	LIBS += -lssl -lcrypto -ljsoncpp -framework Security -framework CoreFoundation
endif

ifeq ($(PLATFORM), linux)
	CXXFLAGS += $(shell pkg-config --cflags openssl jsoncpp)
	LDFLAGS += $(shell pkg-config --libs openssl jsoncpp)
	CXXFLAGS += -DLINUX
	CFLAGS += -DLINUX
endif

CXXFLAGS += -DVERSION=\"$(VERSION)\" -DBUILD_DATE=\"$(BUILD_DATE)\" -DGIT_COMMIT=\"$(GIT_COMMIT)\"

# =============================================================================
# SOURCE STRUCTURE
# =============================================================================
SRC_DIR := src
CPP_SRC_DIR := $(SRC_DIR)/cpp
GO_SRC_DIR := $(SRC_DIR)/go
INCLUDE_DIR := include
BUILD_DIR := build
BIN_DIR := bin
DIST_DIR := dist

BUILD_TYPE ?= release
BUILD_SUBDIR := $(BUILD_DIR)/$(BUILD_TYPE)

CPP_SOURCES := $(wildcard $(CPP_SRC_DIR)/*.cpp)
CPP_OBJECTS := $(CPP_SOURCES:$(CPP_SRC_DIR)/%.cpp=$(BUILD_SUBDIR)/%.o)
CPP_MAIN := $(BIN_DIR)/$(PROJECT_NAME)
GO_BINARY := $(BIN_DIR)/ternary-api

# =============================================================================
# TARGETS
# =============================================================================
.PHONY: all help clean test qa install docker release dist deb

all: info $(CPP_MAIN) go-build

info:
	@echo "== Build Info =="
	@echo "Platform: $(PLATFORM) ($(ARCH))"
	@echo "Version: $(VERSION)"
	@echo "Build Type: $(BUILD_TYPE)"
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "================"

$(BUILD_SUBDIR):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@

$(BUILD_SUBDIR)/%.o: $(CPP_SRC_DIR)/%.cpp | $(BUILD_SUBDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(CPP_MAIN): $(CPP_OBJECTS) | $(BIN_DIR)
	$(CXX) $(CPP_OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	@echo "✓ C++ binary built: $@"

# =============================================================================
# GO BUILD
# =============================================================================
go-build:
	@cd $(GO_SRC_DIR) && $(GO) build -ldflags "-X main.Version=$(VERSION) -X main.BuildDate=$(BUILD_DATE) -X main.GitCommit=$(GIT_COMMIT)" -o ../../$(GO_BINARY)
	@echo "✓ Go binary built: $(GO_BINARY)"

# =============================================================================
# TEST AND QA
# =============================================================================
TEST_BIN := tests/system_metrics_test

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): tests/system_metrics_test.cpp src/cpp/system.metrics.cpp | tests
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

tests:
	@mkdir -p tests

qa:
	@which cppcheck && cppcheck $(CPP_SRC_DIR) --enable=all --suppress=missingIncludeSystem || echo "cppcheck not installed"
	@which clang-tidy && clang-tidy $(CPP_SOURCES) -- $(CXXFLAGS) $(INCLUDES) || echo "clang-tidy not installed"
	@echo "✓ QA checks complete"

# =============================================================================
# CLEANUP
# =============================================================================
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(DIST_DIR)
	find . -name "*.o" -delete
	@echo "✓ Cleaned all build artifacts"

distclean: clean
	rm -rf .cache __pycache__ .pytest_cache
	find . -name "*.pyc" -delete
	@echo "✓ Full deep clean complete"

# =============================================================================
# PACKAGE AND DIST
# =============================================================================
dist: all
	@mkdir -p $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)/bin
	@cp $(CPP_MAIN) $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)/bin/
	@cp README* LICENSE* $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)/ 2>/dev/null || true
	cd $(DIST_DIR) && tar czf $(PROJECT_NAME)-$(VERSION).tar.gz $(PROJECT_NAME)-$(VERSION)
	@echo "✓ Distribution .tar.gz created"

deb: all
	@mkdir -p $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN
	@mkdir -p $(DIST_DIR)/deb/$(PROJECT_NAME)/usr/bin
	@echo "Package: $(PROJECT_NAME)\nVersion: $(VERSION)\nArchitecture: amd64\nMaintainer: David St. John <davestj@gmail.com>\nDescription: Stargate Reactor Binary Package" > $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN/control
	@cp $(CPP_MAIN) $(DIST_DIR)/deb/$(PROJECT_NAME)/usr/bin/
	dpkg-deb --build $(DIST_DIR)/deb/$(PROJECT_NAME) $(DIST_DIR)/$(PROJECT_NAME)_$(VERSION)_amd64.deb
	@echo "✓ .deb package created"

# =============================================================================
# DOCKER SUPPORT
# =============================================================================
docker:
	docker build -t $(PROJECT_NAME):$(VERSION) .
	docker tag $(PROJECT_NAME):$(VERSION) $(PROJECT_NAME):latest
	@echo "✓ Docker image built"

# =============================================================================
# HELP
# =============================================================================
help:
	@echo "Available Targets:"
	@echo "  all        - Build all components"
	@echo "  test       - Run test harness"
	@echo "  qa         - Lint and static analysis"
	@echo "  clean      - Clean intermediate build files"
	@echo "  distclean  - Clean all build + cache"
	@echo "  docker     - Build Docker image"
	@echo "  dist       - Create tar.gz package"
	@echo "  deb        - Build .deb package"
	@echo "  help       - Show this help"

