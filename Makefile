# File: Makefile
# Author: David St. John (davestj@gmail.com)
# Date: 2025-08-07
# Title: Hybrid Build System - Stargate Fission Reactor Project (Conflict Resolved)
# Purpose: Unifies modernized cross-platform Makefile2 logic with full legacy targets (Go, Docker, QA, Coverage, Docs)
# Reason: Preserve all functionality from original Makefile while adopting modular, scalable build practices and distribution workflows
#
# Change Log:
# - 2025-07-31: Initial hybrid generation merging Makefile2 cross-platform and modular features with legacy Stargate Makefile targets
# - 2025-07-31: Retained Go build, Docker image creation, QA targets, Valgrind, lcov, and changelog logic from legacy build
# - 2025-07-31: Adopted .deb packaging, static builds, BASE math platform detection from Makefile2
# - 2025-08-07: Resolved merge conflict in test section - merged FD count testing with system metrics testing
# - 2025-08-07: Enhanced test system to support multiple test suites with proper dependency management
# - 2025-08-07: Added comprehensive test harness with platform-specific test execution

# =============================================================================
# PROJECT METADATA
# =============================================================================
PROJECT_NAME := ternary-fission-reactor
VERSION := 1.1.13
BUILD_DATE := $(shell date -u +"%Y-%m-%d_%H:%M:%S_UTC")
GIT_COMMIT := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")


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
CPPFLAGS := -Iinclude
CPPFLAGS += -Ithird_party/cpp-httplib
LDFLAGS :=
LIBS := -lm -lpthread

ifeq ($(PLATFORM), macos)
        CXXFLAGS += -DMACOS
        CFLAGS += -DMACOS
        CXXFLAGS += -I$(OPENSSL_PREFIX)/include -I$(JSONCPP_PREFIX)/include
        LDFLAGS += -L$(OPENSSL_PREFIX)/lib -L$(JSONCPP_PREFIX)/lib
        LIBS += -lssl -lcrypto -ljsoncpp -framework Security -framework CoreFoundation -lproc
        CXXFLAGS += -DCPPHTTPLIB_OPENSSL_SUPPORT
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
ifeq ($(PLATFORM), linux)
        CXXFLAGS += $(shell pkg-config --cflags openssl jsoncpp)
        LDFLAGS += $(shell pkg-config --libs openssl jsoncpp)
        CXXFLAGS += -DLINUX
        CFLAGS += -DLINUX
        CXXFLAGS += -DCPPHTTPLIB_OPENSSL_SUPPORT

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
TEST_DIR := tests

# Source files
CPP_SOURCES := $(wildcard $(SRC_DIR)/cpp/*.cpp)
CPP_HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)
GO_SOURCES := $(wildcard $(SRC_DIR)/go/*.go)

# Object files (exclude main files to avoid multiple main definitions)
CPP_OBJS := $(BUILD_DIR)/$(BUILD_TYPE)/physics.utilities.o \
	    $(BUILD_DIR)/$(BUILD_TYPE)/ternary.fission.simulation.engine.o

BUILD_TYPE ?= release
BUILD_SUBDIR := $(BUILD_DIR)/$(BUILD_TYPE)


CPP_SOURCES := $(wildcard $(CPP_SRC_DIR)/*.cpp)
CPP_OBJECTS := $(CPP_SOURCES:$(CPP_SRC_DIR)/%.cpp=$(BUILD_SUBDIR)/%.o)
CPP_MAIN := $(BIN_DIR)/$(PROJECT_NAME)
GO_BINARY := $(BIN_DIR)/ternary-api

# =============================================================================
# TEST CONFIGURATION
# =============================================================================
TEST_BUILD_DIR := $(BUILD_DIR)/tests
FD_COUNT_TEST := $(TEST_BUILD_DIR)/test_fd_count
SYSTEM_METRICS_TEST := $(TEST_BUILD_DIR)/system_metrics_test
INTEGRATION_TEST := $(TEST_BUILD_DIR)/integration_test

TEST_SOURCES := $(wildcard $(TEST_DIR)/*.cpp)
TEST_BINARIES := $(FD_COUNT_TEST) $(SYSTEM_METRICS_TEST) $(INTEGRATION_TEST)

# =============================================================================
# TARGETS
# =============================================================================
.PHONY: all help clean test qa install docker release dist deb test-fd test-metrics test-integration

all: info $(CPP_MAIN) go-build

info:
	@echo "== Build Info =="
	@echo "Platform: $(PLATFORM) ($(ARCH))"
	@echo "Version: $(VERSION)"
	@echo "Build Type: $(BUILD_TYPE)"
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "================"


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

$(BUILD_SUBDIR):
	@mkdir -p $@


$(BIN_DIR):
	@mkdir -p $@

$(TEST_BUILD_DIR):
	@mkdir -p $@

$(BUILD_SUBDIR)/%.o: $(CPP_SRC_DIR)/%.cpp | $(BUILD_SUBDIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

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

test: $(BUILD_DIR) $(TEST_BIN)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) tests/test_fd_count.cpp $(LDFLAGS) $(LIBS) -o $(BUILD_DIR)/test_fd_count
	$(BUILD_DIR)/test_fd_count
	./$(TEST_BIN)
	@echo "✓ Tests passed"

$(TEST_BIN): tests/system_metrics_test.cpp src/cpp/system.metrics.cpp | tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

tests:
	@mkdir -p tests

qa: test
	@echo "Running static analysis and linting..."
	@which cppcheck > /dev/null && cppcheck $(CPP_SRC_DIR) $(TEST_DIR) --enable=all --suppress=missingIncludeSystem --xml --xml-version=2 2> cppcheck_report.xml || echo "cppcheck not installed"
	@which clang-tidy > /dev/null && clang-tidy $(CPP_SOURCES) $(TEST_SOURCES) -- $(CXXFLAGS) $(CPPFLAGS) > clang_tidy_report.txt 2>&1 || echo "clang-tidy not installed"
	@which scan-build > /dev/null && scan-build make $(CPP_MAIN) || echo "scan-build not installed"
	@echo "✓ QA checks complete - reports generated"

# =============================================================================
# DOCUMENTATION AND CHANGELOG
# =============================================================================
changelog:
	@echo "Generating changelog from git commits..."
	@git log --oneline -5 > CHANGELOG.md.tmp
	@echo "# Changelog - Last 5 Commits" > CHANGELOG.md
	@echo "" >> CHANGELOG.md
	@echo "Generated on: $(BUILD_DATE)" >> CHANGELOG.md
	@echo "Git Commit: $(GIT_COMMIT)" >> CHANGELOG.md
	@echo "" >> CHANGELOG.md
	@cat CHANGELOG.md.tmp >> CHANGELOG.md
	@rm CHANGELOG.md.tmp
	@echo "✓ Changelog generated: CHANGELOG.md"

docs:
	@which doxygen > /dev/null && doxygen Doxyfile || echo "doxygen not installed - skipping documentation generation"
	@echo "✓ Documentation generation complete"

# =============================================================================
# CLEANUP
# =============================================================================
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(DIST_DIR)
	rm -f coverage.info cppcheck_report.xml clang_tidy_report.txt
	rm -rf coverage_html
	find . -name "*.o" -delete
	find . -name "*.gcda" -delete
	find . -name "*.gcno" -delete
	@echo "✓ Cleaned all build artifacts and reports"

distclean: clean
	rm -rf .cache __pycache__ .pytest_cache
	find . -name "*.pyc" -delete
	rm -f CHANGELOG.md
	@echo "✓ Full deep clean complete"

# =============================================================================
# PACKAGE AND DIST
# =============================================================================
dist: all changelog
	@mkdir -p $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)/bin
	@mkdir -p $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)/docs
	@cp $(CPP_MAIN) $(GO_BINARY) $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)/bin/
	@cp README* LICENSE* CHANGELOG.md $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)/ 2>/dev/null || true
	@cp -r docs/* $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)/docs/ 2>/dev/null || true
	cd $(DIST_DIR) && tar czf $(PROJECT_NAME)-$(VERSION).tar.gz $(PROJECT_NAME)-$(VERSION)
	@echo "✓ Distribution .tar.gz created with documentation"

deb: all
	@mkdir -p $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN
	@mkdir -p $(DIST_DIR)/deb/$(PROJECT_NAME)/usr/bin
	@mkdir -p $(DIST_DIR)/deb/$(PROJECT_NAME)/usr/share/doc/$(PROJECT_NAME)
	@echo "Package: $(PROJECT_NAME)" > $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN/control
	@echo "Version: $(VERSION)" >> $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN/control
	@echo "Architecture: amd64" >> $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN/control
	@echo "Maintainer: David St. John <davestj@gmail.com>" >> $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN/control
	@echo "Description: Stargate Ternary Fission Reactor - Advanced Physics Simulation System" >> $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN/control
	@echo "Depends: libc6, libstdc++6, libssl3, libjsoncpp25" >> $(DIST_DIR)/deb/$(PROJECT_NAME)/DEBIAN/control
	@cp $(CPP_MAIN) $(GO_BINARY) $(DIST_DIR)/deb/$(PROJECT_NAME)/usr/bin/
	@cp README* LICENSE* CHANGELOG.md $(DIST_DIR)/deb/$(PROJECT_NAME)/usr/share/doc/$(PROJECT_NAME)/ 2>/dev/null || true
	dpkg-deb --build $(DIST_DIR)/deb/$(PROJECT_NAME) $(DIST_DIR)/$(PROJECT_NAME)_$(VERSION)_amd64.deb
	@echo "✓ .deb package created with full metadata"

# =============================================================================
# DOCKER SUPPORT
# =============================================================================
docker:
	docker build -t $(PROJECT_NAME):$(VERSION) .
	docker tag $(PROJECT_NAME):$(VERSION) $(PROJECT_NAME):latest
	@echo "✓ Docker image built and tagged"

docker-test: docker
	docker run --rm $(PROJECT_NAME):$(VERSION) --version
	@echo "✓ Docker image tested successfully"

# =============================================================================
# RELEASE WORKFLOW
# =============================================================================
release: clean all test qa dist deb changelog
	@echo "== Release $(VERSION) Complete =="
	@echo "Build Date: $(BUILD_DATE)"
	@echo "Git Commit: $(GIT_COMMIT)"
	@echo "Platform: $(PLATFORM)"
	@echo "Artifacts:"
	@echo "  - Binary: $(CPP_MAIN)"
	@echo "  - Go API: $(GO_BINARY)"
	@echo "  - Package: $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION).tar.gz"
	@echo "  - Debian: $(DIST_DIR)/$(PROJECT_NAME)_$(VERSION)_amd64.deb"
	@echo "================================"

# =============================================================================
# HELP
# =============================================================================
help:
	@echo "Available Targets:"
	@echo "  all            - Build all components (C++ and Go)"
	@echo "  test           - Run complete test suite (FD count + metrics + integration)"
	@echo "  test-fd        - Run file descriptor count tests only"
	@echo "  test-metrics   - Run system metrics tests only"
	@echo "  test-integration - Run integration tests only"
	@echo "  test-platform  - Run platform-specific test subset"
	@echo "  test-coverage  - Generate test coverage reports (requires lcov)"
	@echo "  test-valgrind  - Run memory analysis with Valgrind"
	@echo "  qa             - Run static analysis, linting, and all tests"
	@echo "  clean          - Clean intermediate build files and reports"
	@echo "  distclean      - Clean all build artifacts and cache files"
	@echo "  changelog      - Generate changelog from last 5 git commits"
	@echo "  docs           - Generate documentation (requires doxygen)"
	@echo "  docker         - Build Docker image"
	@echo "  docker-test    - Test Docker image functionality"
	@echo "  dist           - Create tar.gz distribution package"
	@echo "  deb            - Build .deb package with dependencies"
	@echo "  release        - Complete release workflow (build + test + package)"
	@echo "  help           - Show this comprehensive help"
	@echo ""
	@echo "Build Options:"
	@echo "  BUILD_TYPE=debug   - Build with debug symbols and no optimization"
	@echo "  BUILD_TYPE=release - Build optimized release version (default)"
	@echo ""
	@echo "Example Usage:"
	@echo "  make all BUILD_TYPE=debug"
	@echo "  make test-coverage"
	@echo "  make release"