#!/bin/bash
# setup_build_environment.sh - Complete Build Environment Setup
# Author: David St. John (davestj@gmail.com)
# Date: 2025-07-31
# Purpose: Complete solution for Ternary Fission Reactor build environment
# Version: 1.1.13
#
# Change Log:
# - 2025-07-31: Complete build environment setup with dependency resolution
# - 2025-07-31: Fixed jsoncpp include path issues for both macOS and Ubuntu
# - 2025-07-31: Added static linking configuration for .deb distribution
# - 2025-07-31: Integrated Makefile updates and testing procedures
#
# Usage:
# chmod +x setup_build_environment.sh
# ./setup_build_environment.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging functions
log_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Platform detection
detect_platform() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macos"
        ARCH=$(uname -m)
        log_info "Detected: macOS ($ARCH)"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="linux"
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            DISTRO=$ID
            DISTRO_VERSION=$VERSION_ID
            log_info "Detected: $PRETTY_NAME"
        else
            DISTRO="unknown"
        fi
    else
        log_error "Unsupported platform: $OSTYPE"
        exit 1
    fi
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install macOS dependencies
install_macos_dependencies() {
    log_header "Installing macOS Dependencies"
    
    if ! command_exists brew; then
        log_error "Homebrew not found. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    log_info "Updating Homebrew..."
    brew update
    
    local packages=("jsoncpp" "openssl@3" "cppcheck" "clang-format" "pkg-config")
    
    for package in "${packages[@]}"; do
        if brew list "$package" &>/dev/null; then
            log_info "✓ $package already installed"
        else
            log_info "Installing $package..."
            brew install "$package"
        fi
    done
    
    # Verify installations
    local homebrew_prefix=$(brew --prefix)
    
    if [ -f "$homebrew_prefix/opt/jsoncpp/include/json/json.h" ]; then
        log_success "✓ JsonCpp found: $homebrew_prefix/opt/jsoncpp/include/json/json.h"
    else
        log_error "JsonCpp installation failed"
        exit 1
    fi
    
    if [ -f "$homebrew_prefix/opt/openssl@3/include/openssl/ssl.h" ]; then
        log_success "✓ OpenSSL found: $homebrew_prefix/opt/openssl@3/include/openssl/ssl.h"
    else
        log_error "OpenSSL installation failed"
        exit 1
    fi
}

# Install Ubuntu/Debian dependencies
install_ubuntu_dependencies() {
    log_header "Installing Ubuntu/Debian Dependencies"
    
    log_info "Updating package list..."
    sudo apt-get update
    
    local packages=(
        "build-essential"
        "libjsoncpp-dev"
        "libssl-dev"
        "pkg-config"
        "cppcheck"
        "clang-tidy"
        "clang-format"
        "git"
        "cmake"
        "dpkg-dev"
    )
    
    for package in "${packages[@]}"; do
        if dpkg -l | grep -q "^ii  $package "; then
            log_info "✓ $package already installed"
        else
            log_info "Installing $package..."
            sudo apt-get install -y "$package"
        fi
    done
    
    # Verify installations
    if pkg-config --exists jsoncpp; then
        log_success "✓ JsonCpp found: $(pkg-config --modversion jsoncpp)"
        log_info "  Include path: $(pkg-config --cflags jsoncpp)"
        log_info "  Library path: $(pkg-config --libs jsoncpp)"
    else
        log_error "JsonCpp installation verification failed"
        exit 1
    fi
    
    if pkg-config --exists openssl; then
        log_success "✓ OpenSSL found: $(pkg-config --modversion openssl)"
    else
        log_error "OpenSSL installation verification failed"
        exit 1
    fi
}

# Create test program to verify compilation
create_test_program() {
    log_header "Creating Test Program"
    
    cat > test_compilation.cpp << 'EOF'
/**
 * test_compilation.cpp - Compilation verification test
 * Author: David St. John (davestj@gmail.com)
 * Date: 2025-07-31
 * Purpose: Verify that all dependencies compile correctly
 */

#include <json/json.h>
#include <openssl/ssl.h>
#include <iostream>
#include <string>

int main() {
    std::cout << "Testing JSON library..." << std::endl;
    
    // Test JsonCpp
    Json::Value root;
    root["message"] = "JsonCpp working correctly";
    root["version"] = "test";
    root["timestamp"] = "2025-07-31";
    
    Json::StreamWriterBuilder builder;
    std::string json_string = Json::writeString(builder, root);
    std::cout << "✓ JsonCpp test: " << json_string << std::endl;
    
    // Test OpenSSL
    std::cout << "Testing OpenSSL library..." << std::endl;
    SSL_load_error_strings();
    SSL_library_init();
    std::cout << "✓ OpenSSL initialized successfully" << std::endl;
    
    std::cout << "✓ All libraries working correctly!" << std::endl;
    return 0;
}
EOF
    
    log_info "Test program created: test_compilation.cpp"
}

# Test compilation with current system
test_compilation() {
    log_header "Testing Compilation"
    
    create_test_program
    
    local compile_cmd
    if [[ "$PLATFORM" == "macos" ]]; then
        local homebrew_prefix=$(brew --prefix)
        compile_cmd="g++ -std=c++17 -Wall -Wextra -I$homebrew_prefix/opt/jsoncpp/include -I$homebrew_prefix/opt/openssl@3/include -L$homebrew_prefix/opt/jsoncpp/lib -L$homebrew_prefix/opt/openssl@3/lib -ljsoncpp -lssl -lcrypto test_compilation.cpp -o test_compilation"
    else
        compile_cmd="g++ -std=c++17 -Wall -Wextra $(pkg-config --cflags --libs jsoncpp openssl) test_compilation.cpp -o test_compilation"
    fi
    
    log_info "Compile command: $compile_cmd"
    
    if eval "$compile_cmd"; then
        log_success "✓ Compilation successful"
        
        if ./test_compilation; then
            log_success "✓ Test execution successful"
        else
            log_error "Test execution failed"
            exit 1
        fi
        
        # Cleanup
        rm -f test_compilation test_compilation.cpp
    else
        log_error "Compilation failed"
        log_error "This indicates a problem with the build environment"
        exit 1
    fi
}

# Backup existing Makefile
backup_makefile() {
    if [ -f "Makefile" ]; then
        log_info "Backing up existing Makefile to Makefile.backup"
        cp Makefile Makefile.backup
    fi
}

# Update project Makefile
update_makefile() {
    log_header "Updating Project Makefile"
    
    backup_makefile
    
    log_info "The updated Makefile has been provided as an artifact"
    log_info "Replace your current Makefile with the new one to resolve the include path issues"
    log_info ""
    log_info "Key changes in the new Makefile:"
    log_info "  - Proper jsoncpp include paths for both macOS and Ubuntu"
    log_info "  - Static linking support for .deb distribution"
    log_info "  - Enhanced cross-platform support"
    log_info "  - Automatic dependency detection and verification"
    log_info "  - Debian package building capabilities"
}

# Test project build
test_project_build() {
    log_header "Testing Project Build"
    
    if [ ! -f "Makefile" ]; then
        log_error "Makefile not found. Please ensure you've replaced your Makefile with the updated version."
        return 1
    fi
    
    log_info "Testing make deps..."
    if make deps; then
        log_success "✓ Dependencies verification passed"
    else
        log_warn "Dependencies verification had issues, but continuing..."
    fi
    
    log_info "Testing make clean..."
    if make clean; then
        log_success "✓ Clean target works"
    else
        log_error "Clean target failed"
        return 1
    fi
    
    log_info "Testing make all..."
    if make all; then
        log_success "✓ Build successful!"
    else
        log_error "Build failed - check your source files"
        return 1
    fi
    
    log_info "Testing static build..."
    if make static; then
        log_success "✓ Static build successful!"
    else
        log_warn "Static build failed, but regular build worked"
    fi
}

# Create build directories
create_build_structure() {
    log_header "Creating Build Structure"
    
    local dirs=("build" "bin" "dist" "include" "src/cpp" "src/go" "debian")
    
    for dir in "${dirs[@]}"; do
        if [ ! -d "$dir" ]; then
            mkdir -p "$dir"
            log_info "Created directory: $dir"
        else
            log_info "Directory exists: $dir"
        fi
    done
}

# Create debian package files
create_debian_files() {
    log_header "Creating Debian Package Files"
    
    mkdir -p debian
    
    # The debian/control file content is provided as a separate artifact
    log_info "Debian control file template has been provided as an artifact"
    log_info "Place it at: debian/control"
    
    # Create changelog
    cat > debian/changelog << EOF
ternary-fission-reactor (1.1.13-1) unstable; urgency=medium

  * Initial Debian package release
  * Added JsonCpp and OpenSSL integration
  * Implemented static linking for distribution
  * Cross-platform build system support

 -- David St. John <davestj@gmail.com>  $(date -R)
EOF
    
    # Create rules file
    cat > debian/rules << 'EOF'
#!/usr/bin/make -f

%:
	dh $@

override_dh_auto_build:
	$(MAKE) static

override_dh_auto_install:
	mkdir -p debian/ternary-fission-reactor/usr/bin
	cp bin/ternary-fission-reactor debian/ternary-fission-reactor/usr/bin/

override_dh_auto_clean:
	$(MAKE) clean
EOF
    
    chmod +x debian/rules
    
    log_success "✓ Debian package files created"
}

# Final verification and instructions
final_verification() {
    log_header "Final Verification and Instructions"
    
    log_success "Build environment setup complete!"
    log_info ""
    log_info "Next steps:"
    log_info "1. Replace your Makefile with the updated version (provided as artifact)"
    log_info "2. Place the debian/control file in the debian/ directory"
    log_info "3. Ensure your source files are in the correct locations:"
    log_info "   - C++ sources: src/cpp/*.cpp"
    log_info "   - Headers: include/*.h"
    log_info ""
    log_info "Build commands:"
    log_info "  make clean      # Clean build artifacts"
    log_info "  make all        # Build all targets"
    log_info "  make static     # Build with static linking"
    log_info "  make deb        # Create Debian package"
    log_info "  make test       # Run tests"
    log_info "  make qa         # Quality assurance checks"
    log_info ""
    log_info "Platform-specific notes:"
    if [[ "$PLATFORM" == "macos" ]]; then
        log_info "  - JsonCpp: $(brew --prefix jsoncpp)"
        log_info "  - OpenSSL: $(brew --prefix openssl@3)"
    else
        log_info "  - JsonCpp: $(pkg-config --modversion jsoncpp)"
        log_info "  - OpenSSL: $(pkg-config --modversion openssl)"
    fi
    log_info ""
    log_success "Your build environment is ready!"
}

# Main execution
main() {
    log_header "Ternary Fission Reactor - Build Environment Setup"
    
    detect_platform
    create_build_structure
    
    case $PLATFORM in
        macos)
            install_macos_dependencies
            ;;
        linux)
            install_ubuntu_dependencies
            ;;
        *)
            log_error "Platform $PLATFORM not supported"
            exit 1
            ;;
    esac
    
    test_compilation
    update_makefile
    create_debian_files
    
    # Only test project build if source files exist
    if [ -d "src/cpp" ] && [ "$(ls -A src/cpp)" ]; then
        test_project_build
    else
        log_info "No source files found in src/cpp - skipping project build test"
    fi
    
    final_verification
}

# Handle script interruption
trap 'log_error "Setup interrupted"; exit 1' INT TERM

# Run main function
main "$@"
