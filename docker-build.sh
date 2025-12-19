#!/bin/bash
# Build script for ifr1flex Plugin using Docker
# This creates a portable, statically-linked build on Ubuntu 24.04
set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="ifr1flex-builder"
CONTAINER_NAME="ifr1flex-build-temp"

echo "=========================================="
echo "ifr1flex Plugin - Docker Build"
echo "=========================================="
echo ""

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH"
    exit 1
fi

# Check if user is in docker group (or running as root)
if ! docker ps &> /dev/null; then
    echo "Error: Cannot connect to Docker daemon."
    echo "You may need to:"
    echo "  1. Log out and back in (for group membership to take effect)"
    echo "  2. Run 'newgrp docker' in this shell"
    echo "  3. Or run this script with sudo (not recommended)"
    exit 1
fi

# Build Docker image if it doesn't exist or if --rebuild is passed
echo "Building Docker image: $IMAGE_NAME"
# Always rebuild to ensure Dockerfile changes are picked up
docker build -t "$IMAGE_NAME" "$SCRIPT_DIR"
echo ""

# Clean up any existing container with the same name
docker rm -f "$CONTAINER_NAME" 2>/dev/null || true

mkdir -p "$SCRIPT_DIR/docker-build"
mkdir -p "$SCRIPT_DIR/docker-output"

# Cleanup accidental in-source build artifacts that confuse CMake in Docker
if [ -f "$SCRIPT_DIR/CMakeCache.txt" ]; then
    echo "Found CMakeCache.txt in source root. Removing to prevent build conflicts..."
    rm "$SCRIPT_DIR/CMakeCache.txt"
fi
if [ -d "$SCRIPT_DIR/CMakeFiles" ]; then
    echo "Found CMakeFiles in source root. Removing..."
    rm -rf "$SCRIPT_DIR/CMakeFiles"
fi

echo "Running build in Docker container..."
echo ""

# Run the build in Docker
# - Mount source directory as read-only
# - Mount build directory for CMake artifacts
# - Mount output directory for final plugin
docker run --rm \
    --name "$CONTAINER_NAME" \
    -v "$SCRIPT_DIR:/source:ro" \
    -v "$SCRIPT_DIR/docker-build:/build" \
    -v "$SCRIPT_DIR/docker-output:/output" \
    "$IMAGE_NAME" \
    bash -c '
        set -e
        echo "Configuring build with CMake..."
        cd /build
        cmake /source -DCMAKE_BUILD_TYPE=Release
        echo ""
        echo "Building plugin..."
        cmake --build . --target ifr1flex -j$(nproc)
        echo ""
        echo "Copying output to /output..."
        cp -v ifr1flex.xpl /output/ifr1flex.xpl
        cp -v /source/install.sh /output/install.sh
        cp -v /source/LICENSE /output/LICENSE
        cp -rv /source/configs /output/configs
        echo ""
        echo "Build complete!"
        echo ""
        echo "Checking dependencies (should only show system libraries):"
        ldd /output/ifr1flex.xpl || true
    '

echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo ""
echo "Output location: $SCRIPT_DIR/docker-output/"
echo ""
echo "To install the plugin:"
echo "  cd \"$SCRIPT_DIR/docker-output\" && ./install.sh"
echo ""
