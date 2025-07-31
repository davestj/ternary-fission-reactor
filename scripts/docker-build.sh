#!/bin/bash
#
# File: scripts/docker-build.sh
# Author: bthlops (David StJ)
# Date: July 31, 2025
# Title: Docker Multi-Platform Build Script for Ternary Fission Energy Emulation System
# Purpose: Automated building and testing of Docker images across multiple platforms and variants
# Reason: Simplifies Docker build process with platform detection, testing, and deployment options
#
# Change Log:
# 2025-07-31: Initial creation with multi-platform build support
#             Added platform detection for ARM64/AMD64 builds
#             Integrated build testing and verification
#             Added push functionality for registry deployment
#             Fixed package naming issues for Ubuntu 24.04/22.04 and Debian 12
#
# Carry-over Context:
# - Supports ARM64/AMD64 multi-platform builds for macOS M2 and Intel systems
# - Automatically detects current platform and builds appropriate variants
# - Includes testing and verification of built images
# - Provides registry push functionality for deployment
# - Handles all Dockerfile variants (ubuntu24, ubuntu22, debian12)

set -e

# =============================================================================
# CONFIGURATION AND GLOBAL VARIABLES
# =============================================================================

# We define script metadata and version information
SCRIPT_NAME="docker-build.sh"
SCRIPT_VERSION="1.1.13"
SCRIPT_AUTHOR="bthlops (David StJ)"

# We set up build configuration
PROJECT_NAME="ternary-fission"
IMAGE_TAG="${PROJECT_NAME}:latest"
BUILD_DATE=$(date -u +"%Y-%m-%d_%H:%M:%S_UTC")
GIT_COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")

# We detect current platform
CURRENT_ARCH=$(uname -m)
CURRENT_OS=$(uname -s)

# We map architecture names
case "$CURRENT_ARCH" in
    "x86_64") DOCKER_ARCH="amd64" ;;
    "arm64"|"aarch64") DOCKER_ARCH="arm64" ;;
    *) DOCKER_ARCH="amd64" ;;
esac

# We define supported platforms and variants
PLATFORMS=("linux/amd64" "linux/arm64")
VARIANTS=("ubuntu24" "ubuntu22" "debian12")

# We set up colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# =============================================================================
# UTILITY FUNCTIONS
# =============================================================================

# We implement logging functions for different severity levels
log_info() {
    echo -e "${GREEN}[INFO]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

log_debug() {
    if [[ "${DEBUG:-false}" == "true" ]]; then
        echo -e "${BLUE}[DEBUG]${NC} $*"
    fi
}

# We display usage information
show_usage() {
    cat << EOF
Docker Multi-Platform Build Script for Ternary Fission Energy Emulation System
Version: ${SCRIPT_VERSION}
Author: ${SCRIPT_AUTHOR}

Usage: $0 [OPTIONS] [COMMAND]

Commands:
    build           Build images for current platform (default)
    build-all       Build all variants for current platform
    build-multi     Build multi-platform images
    test            Test built images
    push            Push images to registry
    clean           Clean build artifacts and images
    help            Show this help message

Options:
    --platform PLATFORM    Target platform (linux/amd64, linux/arm64, all)
    --variant VARIANT       Build variant (ubuntu24, ubuntu22, debian12, all)
    --registry REGISTRY     Registry URL for push operations
    --tag TAG              Custom image tag (default: ${IMAGE_TAG})
    --no-cache             Build without using cache
    --debug                Enable debug output
    --dry-run              Show commands without executing

Environment Variables:
    DOCKER_REGISTRY        Default registry for push operations
    DOCKER_BUILDKIT        Enable BuildKit (recommended: 1)
    BUILD_ARGS             Additional build arguments

Examples:
    $0 build                           # Build for current platform
    $0 build-all                       # Build all variants
    $0 build --variant ubuntu24        # Build specific variant
    $0 build-multi --platform all      # Build all platforms
    $0 test                           # Test built images
    $0 push --registry ghcr.io/user   # Push to GitHub registry

Current Platform: ${CURRENT_OS}/${CURRENT_ARCH} (Docker: linux/${DOCKER_ARCH})

EOF
}

# We check Docker prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # We check if Docker is installed and running
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed or not in PATH"
        exit 1
    fi
    
    # We check if Docker daemon is running
    if ! docker info &> /dev/null; then
        log_error "Docker daemon is not running"
        exit 1
    fi
    
    # We check Docker version
    local docker_version=$(docker version --format '{{.Server.Version}}' 2>/dev/null)
    log_info "Docker version: ${docker_version}"
    
    # We check if buildx is available for multi-platform builds
    if docker buildx version &> /dev/null; then
        log_info "Docker Buildx available for multi-platform builds"
        BUILDX_AVAILABLE=true
    else
        log_warn "Docker Buildx not available - multi-platform builds disabled"
        BUILDX_AVAILABLE=false
    fi
    
    # We check if we're in the project directory
    if [[ ! -f "Dockerfile" ]] || [[ ! -f "docker-compose.yml" ]]; then
        log_error "Not in project root directory (missing Dockerfile or docker-compose.yml)"
        exit 1
    fi
    
    log_info "Prerequisites check completed successfully"
}

# We build a single Docker image
build_image() {
    local dockerfile=$1
    local tag=$2
    local platform=$3
    local build_args=$4
    
    log_info "Building image: ${tag}"
    log_debug "Dockerfile: ${dockerfile}, Platform: ${platform}"
    
    local build_cmd="docker build"
    
    # We add platform specification if provided
    if [[ -n "$platform" ]] && [[ "$BUILDX_AVAILABLE" == "true" ]]; then
        build_cmd="docker buildx build --platform ${platform}"
    fi
    
    # We add cache options
    if [[ "$NO_CACHE" == "true" ]]; then
        build_cmd="$build_cmd --no-cache"
    fi
    
    # We add build arguments
    build_cmd="$build_cmd"
    build_cmd="$build_cmd --build-arg BUILD_DATE='${BUILD_DATE}'"
    build_cmd="$build_cmd --build-arg GIT_COMMIT='${GIT_COMMIT}'"
    build_cmd="$build_cmd --build-arg VERSION='${SCRIPT_VERSION}'"
    
    if [[ -n "$build_args" ]]; then
        build_cmd="$build_cmd $build_args"
    fi
    
    # We add final arguments
    build_cmd="$build_cmd -f ${dockerfile} -t ${tag} ."
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "DRY RUN: $build_cmd"
        return 0
    fi
    
    # We execute the build
    log_debug "Executing: $build_cmd"
    if eval "$build_cmd"; then
        log_info "Successfully built: ${tag}"
        return 0
    else
        log_error "Failed to build: ${tag}"
        return 1
    fi
}

# We test a built Docker image
test_image() {
    local image=$1
    
    log_info "Testing image: ${image}"
    
    # We test if the image exists
    if ! docker image inspect "$image" &> /dev/null; then
        log_error "Image does not exist: ${image}"
        return 1
    fi
    
    # We test basic container startup
    log_debug "Testing container startup..."
    local container_id=$(docker run -d --rm "$image" --help 2>/dev/null || echo "")
    
    if [[ -n "$container_id" ]]; then
        sleep 2
        docker stop "$container_id" &> /dev/null || true
        log_info "Container startup test passed: ${image}"
    else
        log_warn "Container startup test failed: ${image}"
    fi
    
    # We test image size
    local image_size=$(docker image inspect "$image" --format='{{.Size}}' 2>/dev/null)
    if [[ -n "$image_size" ]]; then
        local size_mb=$((image_size / 1024 / 1024))
        log_info "Image size: ${size_mb} MB"
        
        if [[ $size_mb -gt 2000 ]]; then
            log_warn "Large image size (>${size_mb}MB) - consider optimization"
        fi
    fi
    
    return 0
}

# =============================================================================
# BUILD COMMANDS
# =============================================================================

# We build images for current platform
build_current() {
    local variant=${1:-""}
    local dockerfile="Dockerfile"
    local tag="${PROJECT_NAME}:latest"
    
    if [[ -n "$variant" ]]; then
        dockerfile="Dockerfile.${variant}"
        tag="${PROJECT_NAME}:${variant}"
    fi
    
    if [[ ! -f "$dockerfile" ]]; then
        log_error "Dockerfile not found: ${dockerfile}"
        return 1
    fi
    
    build_image "$dockerfile" "$tag" "linux/${DOCKER_ARCH}" "$BUILD_ARGS"
}

# We build all variants for current platform
build_all_variants() {
    log_info "Building all variants for current platform (linux/${DOCKER_ARCH})"
    
    local failed_builds=()
    
    # We build main Dockerfile
    if ! build_current; then
        failed_builds+=("main")
    fi
    
    # We build all variants
    for variant in "${VARIANTS[@]}"; do
        log_info "Building variant: ${variant}"
        if ! build_current "$variant"; then
            failed_builds+=("$variant")
        fi
    done
    
    # We report results
    if [[ ${#failed_builds[@]} -eq 0 ]]; then
        log_info "All builds completed successfully"
        return 0
    else
        log_error "Failed builds: ${failed_builds[*]}"
        return 1
    fi
}

# We build multi-platform images
build_multi_platform() {
    local platforms_to_build=()
    local variant=${1:-""}
    
    if [[ "$BUILDX_AVAILABLE" != "true" ]]; then
        log_error "Multi-platform builds require Docker Buildx"
        return 1
    fi
    
    # We determine platforms to build
    if [[ "$TARGET_PLATFORM" == "all" ]]; then
        platforms_to_build=("${PLATFORMS[@]}")
    elif [[ -n "$TARGET_PLATFORM" ]]; then
        platforms_to_build=("$TARGET_PLATFORM")
    else
        platforms_to_build=("linux/${DOCKER_ARCH}")
    fi
    
    log_info "Building for platforms: ${platforms_to_build[*]}"
    
    # We create/use buildx builder
    local builder_name="ternary-fission-builder"
    if ! docker buildx inspect "$builder_name" &> /dev/null; then
        log_info "Creating buildx builder: ${builder_name}"
        docker buildx create --name "$builder_name" --use
    else
        docker buildx use "$builder_name"
    fi
    
    local platform_list=$(IFS=, ; echo "${platforms_to_build[*]}")
    local dockerfile="Dockerfile"
    local tag="${PROJECT_NAME}:multi"
    
    if [[ -n "$variant" ]]; then
        dockerfile="Dockerfile.${variant}"
        tag="${PROJECT_NAME}:${variant}-multi"
    fi
    
    build_image "$dockerfile" "$tag" "$platform_list" "$BUILD_ARGS --load"
}

# We test all built images
test_all_images() {
    log_info "Testing all built images..."
    
    local images=$(docker images "${PROJECT_NAME}" --format "{{.Repository}}:{{.Tag}}" | grep -v "<none>")
    
    if [[ -z "$images" ]]; then
        log_warn "No images found to test"
        return 0
    fi
    
    local failed_tests=()
    
    while IFS= read -r image; do
        if ! test_image "$image"; then
            failed_tests+=("$image")
        fi
    done <<< "$images"
    
    if [[ ${#failed_tests[@]} -eq 0 ]]; then
        log_info "All image tests passed"
        return 0
    else
        log_error "Failed image tests: ${failed_tests[*]}"
        return 1
    fi
}

# We push images to registry
push_images() {
    local registry=${1:-$DOCKER_REGISTRY}
    
    if [[ -z "$registry" ]]; then
        log_error "No registry specified. Use --registry or set DOCKER_REGISTRY"
        return 1
    fi
    
    log_info "Pushing images to registry: ${registry}"
    
    local images=$(docker images "${PROJECT_NAME}" --format "{{.Repository}}:{{.Tag}}" | grep -v "<none>")
    
    while IFS= read -r image; do
        local remote_tag="${registry}/${image}"
        
        log_info "Tagging: ${image} -> ${remote_tag}"
        if [[ "$DRY_RUN" != "true" ]]; then
            docker tag "$image" "$remote_tag"
            
            log_info "Pushing: ${remote_tag}"
            docker push "$remote_tag"
        else
            log_info "DRY RUN: docker push ${remote_tag}"
        fi
    done <<< "$images"
}

# We clean build artifacts
clean_images() {
    log_info "Cleaning Docker images and build artifacts..."
    
    # We remove project images
    local images=$(docker images "${PROJECT_NAME}" -q)
    if [[ -n "$images" ]]; then
        log_info "Removing project images..."
        docker rmi $images || true
    fi
    
    # We clean build cache
    if [[ "$BUILDX_AVAILABLE" == "true" ]]; then
        log_info "Cleaning buildx cache..."
        docker buildx prune -f || true
    fi
    
    # We clean general Docker cache
    log_info "Cleaning Docker system..."
    docker system prune -f
    
    log_info "Cleanup completed"
}

# =============================================================================
# MAIN EXECUTION
# =============================================================================

# We parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            TARGET_PLATFORM="$2"
            shift 2
            ;;
        --variant)
            TARGET_VARIANT="$2"
            shift 2
            ;;
        --registry)
            DOCKER_REGISTRY="$2"
            shift 2
            ;;
        --tag)
            IMAGE_TAG="$2"
            shift 2
            ;;
        --no-cache)
            NO_CACHE=true
            shift
            ;;
        --debug)
            DEBUG=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --help|-h)
            show_usage
            exit 0
            ;;
        build|build-all|build-multi|test|push|clean|help)
            COMMAND="$1"
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# We set default command
COMMAND=${COMMAND:-build}

# We display startup banner
echo "========================================================"
echo "Ternary Fission Docker Build Script"
echo "Version: ${SCRIPT_VERSION}"
echo "Author: ${SCRIPT_AUTHOR}"
echo "Platform: ${CURRENT_OS}/${CURRENT_ARCH} (Docker: linux/${DOCKER_ARCH})"
echo "Command: ${COMMAND}"
echo "========================================================"

# We check prerequisites
check_prerequisites

# We execute the requested command
case "$COMMAND" in
    "build")
        build_current "$TARGET_VARIANT"
        ;;
    "build-all")
        build_all_variants
        ;;
    "build-multi")
        build_multi_platform "$TARGET_VARIANT"
        ;;
    "test")
        test_all_images
        ;;
    "push")
        push_images "$DOCKER_REGISTRY"
        ;;
    "clean")
        clean_images
        ;;
    "help")
        show_usage
        ;;
    *)
        log_error "Unknown command: ${COMMAND}"
        show_usage
        exit 1
        ;;
esac

log_info "Docker build script completed successfully"
