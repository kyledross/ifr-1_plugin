# Ubuntu 24.04 LTS build environment for X-Plane plugin
FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libudev-dev \
    libusb-1.0-0-dev \
    python3 \
    python3-pil \
    libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

# Create workspace directory
WORKDIR /workspace

# Set default command
CMD ["/bin/bash"]
