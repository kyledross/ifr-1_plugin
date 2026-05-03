# Ubuntu 24.04 LTS build environment for X-Plane plugin
FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN for i in 1 2 3; do \
    apt-get update && apt-get install -y \
        clang \
        lld \
        cmake \
        git \
        pkg-config \
        libudev-dev \
        libusb-1.0-0-dev \
        python3 \
        python3-pil \
        libgl1-mesa-dev \
    && break || { \
        echo "apt-get attempt $i failed, retrying in 30s..."; \
        sleep 30; \
    }; \
done && rm -rf /var/lib/apt/lists/*

# Create workspace directory
WORKDIR /workspace

# Set default command
CMD ["/bin/bash"]
