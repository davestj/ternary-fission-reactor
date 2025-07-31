#!/bin/bash
# install_dependencies.sh - Cross-Platform Dependency Installation
# Author: David St. John (davestj@gmail.com)
# Date: 2025-07-31
# Purpose: Install build dependencies for Ternary Fission Reactor project
# Version: 1.1.13
#
# Change Log:
# - 2025-07-31: Initial cross-platform dependency installation script
# - 2025-07-31: Added jsoncpp and OpenSSL installation for both platforms
# - 2025-07-31: Added development tools installation (cppcheck, clang-tidy)
# - 2025-07-31: Added package verification and error handling
#
# Usage:
# ./install_dependencies.sh
# sudo ./install_dependencies.sh  (if needed for Linux)

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Platform detection
detect_platform() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macos"
        ARCH=$(uname -m)
        log_info "Detected macOS ($ARCH)"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="linux"
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            DISTRO=$ID
            DISTRO_VERSION=$VERSION_ID
            log_info "Detected Linux: $PRETTY_NAME"
        else
            DISTRO="unknown"
            log_warn "Could not detect Linux distribution"
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
install_macos_deps() {
    log_info "Installing macOS dependencies..."
    
    # Check if Homebrew is installed
    if ! command_exists brew; then
        log_error "Homebrew is not installed. Please install it first:"
        echo "curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh | bash"
        exit 1
    fi
    
    log_info "Updating Homebrew..."
    brew update || log_warn "Failed to update Homebrew"
    
    # Install required packages
    local packages=(
        "jsoncpp"
        "openssl@3"
        "cppcheck"
        "clang-format"
        "pkg-config"
    )
    
    for package in "${packages[@]}"; do
        log_info "Installing $package..."
        if brew list "$package" &>/dev/null; then
            log_info "$package is already installed"
        else
            brew install "$package" || {
                log_error "Failed to install $package"
                exit 1
            }
        fi
    done
    
    # Verify installations
    verify_macos_installation
}

# Install Linux dependencies
install_linux_deps() {
    log_info "Installing Linux dependencies..."
    
    case $DISTRO in
        ubuntu|debian)
            install_debian_deps
            ;;
        fedora|centos|rhel)
            install_redhat_deps
            ;;
        *)
            log_error "Unsupported Linux distribution: $DISTRO"
            exit 1
            ;;
    esac
    
    # Verify installations
    verify_linux_installation
}

# Install Debian/Ubuntu dependencies
install_debian_deps() {
    log_info "Installing Debian/Ubuntu packages..."
    
    # Update package list
    log_info "Updating package list..."
    sudo apt-get update || {
        log_error "Failed to update package list"
        exit 1
    }
    
    # Install build essentials
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
        log_info "Installing $package..."
        sudo apt-get install -y "$package" || {
            log_error "Failed to install $package"
            exit 1
        }
    done
}

# Install Red Hat based dependencies
install_redhat_deps() {
    log_info "Installing Red Hat based packages..."
    
    local package_manager
    if command_exists dnf; then
        package_manager="dnf"
    elif command_exists yum; then
        package_manager="yum"
    else
        log_error "No suitable package manager found (dnf/yum)"
        exit 1
    fi
    
    local packages=(
        "gcc-c++"
        "jsoncpp-devel"
        "openssl-devel"
        "pkg-config"
        "cppcheck"
        "clang-tools-extra"
        "git"
        "cmake"
        "rpm-build"
    )
    
    for package in "${packages[@]}"; do
        log_info "Installing $package..."
        sudo $package_manager install -y "$package" || {
            log_warn "Failed to install $package (might not be available)"
        }
    done
}

# Verify macOS installation
verify_macos_installation() {
    log_info "Verifying macOS installation..."
    
    # Check Homebrew packages
    local homebrew_prefix=$(brew --prefix)
    
    # Check jsoncpp
    if [ -f "$homebrew_prefix/opt/jsoncpp/include/json/json.h" ]; then
        log_info "✓ JsonCpp header found at: $homebrew_prefix/opt/jsoncpp/include/json/json.h"
    else
        log_error "✗ JsonCpp header not found"
        exit 1
    fi
    
    # Check OpenSSL
    if [ -f "$homebrew_prefix/opt/openssl@3/include/openssl/ssl.h" ]; then
        log_info "✓ OpenSSL header found at: $homebrew_prefix/opt/openssl@3/include/openssl/ssl.h"
    else
        log_error "✗ OpenSSL header not found"
        exit 1
    fi
    
    # Check pkg-config files
    if pkg-config --exists jsoncpp; then
        log_info "✓ JsonCpp pkg-config found: $(pkg-config --modversion jsoncpp)"
    else
        log_warn "JsonCpp pkg-config not found"
    fi
}

# Verify Linux installation
verify_linux_installation() {
    log_info "Verifying Linux installation..."
    
    # Check system headers
    if [ -f "/usr/include/json/json.h" ] || pkg-config --exists jsoncpp; then
        if pkg-config --exists jsoncpp; then
            log_info "✓ JsonCpp found: $(pkg-config --modversion jsoncpp)"
        else
            log_info "✓ JsonCpp header found at: /usr/include/json/json.h"
        fi
    else
        log_error "✗ JsonCpp not found"
        exit 1
    fi
    
    # Check OpenSSL
    if pkg-config --exists openssl; then
        log_info "✓ OpenSSL found: $(pkg-config --modversion openssl)"
    else
        log_error "✗ OpenSSL not found"
        exit 1
    fi
    
    # Check build tools
    if command_exists g++; then
        log_info "✓ g++ found: $(g++ --version | head -n1)"
    else
        log_error "✗ g++ not found"
        exit 1
    fi
}

# Test compilation
test_compilation() {
    log_info "Testing compilation with a simple test program..."
    
    # Create temporary test file
    local test_file="/tmp/json_test.cpp"
    cat > "$test_file" << 'EOF'
#include <json/json.h>
#include <openssl/ssl.h>
#include <iostream>

int main() {
    Json::Value root;
    root["test"] = "success";
    
    SSL_library_init();
    
    std::cout << "✓ JsonCpp and OpenSSL headers compiled successfully" << std::endl;
    std::cout << "JsonCpp test value: " << root["test"].asString() << std::endl;
    
    return 0;
}
EOF
    
    # Compile test
    local compile_cmd
    if [[ "$PLATFORM" == "macos" ]]; then
        local homebrew_prefix=$(brew --prefix)
        compile_cmd="g++ -std=c++17 -I$homebrew_prefix/opt/jsoncpp/include -I$homebrew_prefix/opt/openssl@3/include -L$homebrew_prefix/opt/jsoncpp/lib -L$homebrew_prefix/opt/openssl@3/lib -ljsoncpp -lssl -lcrypto $test_file -o /tmp/json_test"
    else
        compile_cmd="g++ -std=c++17 $(pkg-config --cflags --libs jsoncpp openssl) $test_file -o /tmp/json_test"
    fi
    
    log_info "Compiling test program..."
    if eval "$compile_cmd"; then
        log_info "✓ Test compilation successful"
        
        # Run test
        if /tmp/json_test; then
            log_info "✓ Test execution successful"
        else
            log_error "✗ Test execution failed"
            exit 1
        fi
        
        # Cleanup
        rm -f /tmp/json_test /tmp/json_test.cpp
    else
        log_error "✗ Test compilation failed"
        log_error "Command: $compile_cmd"
        exit 1
    fi
}

# Main installation function
main() {
    log_info "Ternary Fission Reactor - Dependency Installation"
    log_info "================================================"
    
    detect_platform
    
    case $PLATFORM in
        macos)
            install_macos_deps
            ;;
        linux)
            install_linux_deps
            ;;
        *)
            log_error "Unsupported platform: $PLATFORM"
            exit 1
            ;;
    esac
    
    # Test compilation
    test_compilation
    
    log_info "================================================"
    log_info "✓ All dependencies installed successfully!"
    log_info ""
    log_info "You can now build the project with:"
    log_info "  make clean"
    log_info "  make all"
    log_info ""
    log_info "For static builds (recommended for distribution):"
    log_info "  make static"
    log_info ""
    log_info "To create a Debian package:"
    log_info "  make deb"
}

# Run main function
main "$@"
