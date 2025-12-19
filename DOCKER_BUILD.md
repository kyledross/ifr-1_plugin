# Docker Build System

This document describes the Docker-based build system for creating portable, statically-linked builds of the ifr1flex Plugin.

## Overview

The Docker build system creates a portable plugin binary that works across different Linux distributions by:

1. Building on Ubuntu 24.04 LTS (officially supported by X-Plane)
2. Statically linking dependencies (via `FetchContent` and specific CMake flags)
3. Following the "build old, run new" principle for maximum compatibility

## Files

- `Dockerfile` - Defines the Ubuntu 24.04 build environment
- `docker-build.sh` - Build script that manages Docker image and container
- `.dockerignore` - Excludes unnecessary files from Docker build context

## Usage

### Prerequisites

- Docker installed and running
- Your user added to the `docker` group

### Building the Plugin

```bash
./docker-build.sh
```

Output: `docker-output/lin.xpl`

## How It Works

### Build Environment (Dockerfile)

The Docker image includes:
- Ubuntu 24.04 LTS base
- GCC C++23 compatible compiler
- CMake build system
- All necessary build tools and libraries (`libudev-dev`, etc.)

### Build Process (docker-build.sh)

1. **Image Management**: Builds the Docker image if it doesn't exist or if changes are detected.
2. **Directory Setup**: Creates `docker-build/` and `docker-output/` directories.
3. **Volume Mounts**:
   - `/source` - Source code (read-only)
   - `/build` - CMake build directory
   - `/output` - Final plugin output
4. **Build Execution**: Runs CMake and builds the `ifr1flex` target.
5. **Output**: Copies `ifr1flex.xpl` to `docker-output/lin.xpl`.

## CI/CD Integration

This Docker build system is designed to be leveraged by GitHub Actions for automated releases.

## Verification

Check dependencies of the built plugin:

```bash
ldd docker-output/lin.xpl
```

It should only show standard system libraries.
