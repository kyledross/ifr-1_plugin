#!/bin/bash
#
#   Copyright 2025 Kyle D. Ross
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# Setup script for IFR-1 Flight Controller Plugin
#

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to display success messages
success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Function to display info messages
info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Function to display warning messages
warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to display error messages
error_no_exit() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to display error messages and exit
error() {
    echo -e "${RED}[ERROR]${NC} $1"
    cleanup
    exit 1
}

# Function to clean up any changes made if an error occurs
cleanup() {
    if [ -n "$CLEANUP_ACTIONS" ]; then
        warning "Cleaning up changes due to error..."
        eval "$CLEANUP_ACTIONS"
        info "Cleanup completed."
    fi
}

# Trap errors and call cleanup
trap 'error "An unexpected error occurred. Exiting."' ERR

# Get the original user's home directory
ORIGINAL_USER=${SUDO_USER:-$(whoami)}
USER_HOME=$(eval echo "~$ORIGINAL_USER")

# Welcome message
echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE} IFR-1 Flight Controller Plugin Setup            ${NC}"
echo -e "${BLUE}                                            ${NC}"
echo -e "${BLUE} This will setup the IIFR-1 Flight Controller    ${NC}"
echo -e "${BLUE} plugin for use in X-Plane 12 in Linux.    ${NC}"
echo -e "${BLUE}=============================================${NC}"

echo "This software is not affiliated with, endorsed by, or supported by Octavi GmbH or Laminar Research."
echo "All trademarks are the property of their respective owners, and are used herein for reference only."
echo ""
info "Running as: $(whoami)"
info "User home directory: $USER_HOME"
echo ""

# Sponsor message (word-wrapped, green), then prompt to continue
{
    # Determine terminal width and choose a wrap width just under it
    COLS=$(tput cols 2>/dev/null)
    if [ -z "$COLS" ] || [ "$COLS" -lt 40 ]; then
        COLS=80
    fi
    WRAP_WIDTH=$((COLS - 2))

    echo -e "${GREEN}"
    cat <<'EOF' | fold -s -w "$WRAP_WIDTH"
Thank you for trying out this plugin for the IFR-1 controller and X-Plane 12.  If this tool enhances your flight sim experience, please consider supporting the project at https://buymeacoffee.com/kyledross.  Any contribution to help defray costs is greatly appreciated.

EOF
    echo -e "${NC}"
}

read -r -p "Press Enter to continue with the installation..." _

# Prompt user to disconnect the device
info "Please ensure the IFR-1 device is disconnected before proceeding."
read -r -p "Is the device disconnected? (y/n): " device_disconnected

if [[ ! "$device_disconnected" =~ ^[Yy]$ ]]; then
    error "Please disconnect the device and run the script again."
fi

success "Device check passed."

# Check glibc version
info "Checking system dependencies..."
REQUIRED_GLIBC="2.14"
SYSTEM_GLIBC=$(ldd --version | head -n1 | grep -oP '\d+\.\d+' | head -n1)

if [ -z "$SYSTEM_GLIBC" ]; then
    warning "Could not determine system glibc version. Proceeding anyway..."
else
    info "System glibc version: $SYSTEM_GLIBC"
    if printf '%s\n' "$REQUIRED_GLIBC" "$SYSTEM_GLIBC" | sort -V -C; then
        success "System glibc version is compatible (>= $REQUIRED_GLIBC)."
    else
        error_no_exit "System glibc version ($SYSTEM_GLIBC) is older than required ($REQUIRED_GLIBC)."
        error "Please upgrade your system or use a newer distribution."
    fi
fi

# Find X-Plane installation
XPLANE_INSTALL_FILE="$USER_HOME/.x-plane/x-plane_install_12.txt"

if [ ! -f "$XPLANE_INSTALL_FILE" ]; then
    error_no_exit "X-Plane installation file not found at $XPLANE_INSTALL_FILE. Please make sure X-Plane 12 is installed."
    error "If X-Plane is installed, run it and exit, then try this setup again."
fi

info "Finding X-Plane installation..."

# Find the latest X-Plane installation by creation date
XPLANE_ROOT=""
latest_time=0

# Disable the error trap temporarily for the loop reading the file
# This prevents bad lines or permission issues from killing the script
trap - ERR

while IFS= read -r line || [ -n "$line" ]; do
    # 2. Strip Windows carriage returns (\r)
    line="${line//$'\r'/}"

    # 3. Skip empty lines (Using 'if' instead of '&&' to avoid error trap triggers)
    if [ -z "$line" ]; then
        continue
    fi

    # 4. Check if directory exists
    if [ -d "$line" ]; then
        # 5. Run 'stat'. If it fails, we just ignore it (checking exit code explicitly)
        if dir_time=$(stat -c %Y "$line" 2>/dev/null); then
            if [ "$dir_time" -gt "$latest_time" ]; then
                latest_time=$dir_time
                XPLANE_ROOT="$line"
            fi
        fi
    fi
done < "$XPLANE_INSTALL_FILE"

# Re-enable the error trap
trap 'error "An unexpected error occurred. Exiting."' ERR

if [ -z "$XPLANE_ROOT" ]; then
    error "Could not find a valid X-Plane installation. Please check that X-Plane is installed and try this again."
fi

success "Found X-Plane installation at: $XPLANE_ROOT"

# Check if X-Plane is running
XPLANE_RUNNING=false
if pgrep -f "X-Plane-x86_64" > /dev/null; then
    info "X-Plane is currently running."
    info "Setup will continue, but you'll need to restart X-Plane after installation."
    XPLANE_RUNNING=true
fi

# Check if the plugin binary exists
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
PLUGIN_BINARY="$SCRIPT_DIR/ifr1flex.xpl"

if [ ! -f "$PLUGIN_BINARY" ]; then
    error_no_exit "Plugin binary not found at: $PLUGIN_BINARY"
    error "Installation package appears incomplete."
fi

success "Found plugin binary: $PLUGIN_BINARY"

# Verify plugin binary
info "Verifying plugin binary..."
if ! ldd "$PLUGIN_BINARY" > /dev/null 2>&1; then
    error "Plugin binary has broken dependencies or is incompatible with this system."
fi
success "Plugin binary verified."

# Install the plugin
PLUGIN_BASE_DIR="$XPLANE_ROOT/Resources/plugins/ifr1flex"
PLUGIN_DIR="$PLUGIN_BASE_DIR/lin_x64"
CONFIGS_TARGET_DIR="$PLUGIN_BASE_DIR/configs"
TARGET_PLUGIN="$PLUGIN_DIR/ifr1flex.xpl"

info "Installing IFR-1 Flight Controller Plugin to X-Plane..."

# Create the plugin directory
if mkdir -p "$PLUGIN_DIR"; then
    CLEANUP_ACTIONS="rm -rf '$PLUGIN_BASE_DIR'; $CLEANUP_ACTIONS"
    success "Created plugin directory: $PLUGIN_DIR"
else
    error "Failed to create plugin directory: $PLUGIN_DIR"
fi

# Copy the plugin binary
if cp "$PLUGIN_BINARY" "$TARGET_PLUGIN"; then
    success "Plugin binary copied to: $TARGET_PLUGIN"
else
    error "Failed to copy plugin binary to: $TARGET_PLUGIN"
fi

# Copy configurations
CONFIGS_SRC_DIR="$SCRIPT_DIR/configs"
if [ -d "$CONFIGS_SRC_DIR" ]; then
    info "Installing aircraft configurations..."
    if mkdir -p "$CONFIGS_TARGET_DIR"; then
        if cp "$CONFIGS_SRC_DIR"/*.json "$CONFIGS_TARGET_DIR/"; then
            success "Configurations copied to: $CONFIGS_TARGET_DIR"
        else
            warning "Failed to copy some configuration files."
        fi
    else
        error "Failed to create configs directory: $CONFIGS_TARGET_DIR"
    fi
else
    warning "Configs directory not found at $CONFIGS_SRC_DIR. No configurations installed."
fi

# Make sure the plugin is executable
if chmod +x "$TARGET_PLUGIN"; then
    success "Plugin binary set as executable."
else
    warning "Failed to set plugin binary as executable. X-Plane may not load it."
fi

success "IFR-1 Flight Controller Plugin successfully installed to X-Plane."

# Set up udev rules
UDEV_RULE_FILE="/etc/udev/rules.d/99-ifr1.rules"
UDEV_RULE_CONTENT='SUBSYSTEM=="hidraw", ATTRS{idProduct}=="e6d6", ATTRS{idVendor}=="04d8", MODE="0666"'
info "Setting up udev rules for IFR-1 device..."

# If any other file in /etc/udev/rules.d already defines rules for the IFR-1 device,
# skip installing our own rules.
existing_ifr_rules_file=""
if [ -d "/etc/udev/rules.d" ]; then
    while IFS= read -r f; do
        # Skip our target file; we only care about other files
        if [ "$f" = "$UDEV_RULE_FILE" ]; then
            continue
        fi
        # Must contain all three conditions. Be tolerant to a possible typo variant for idProduct.
        if grep -q 'SUBSYSTEM=="hidraw"' "$f" \
           && grep -q 'ATTRS{idVendor}=="04d8"' "$f" \
           && (grep -q 'ATTRS{idProduct}=="e6d6"' "$f"); then
            existing_ifr_rules_file="$f"
            break
        fi
    done < <(find /etc/udev/rules.d -maxdepth 1 -type f 2>/dev/null)
fi

if [ -n "$existing_ifr_rules_file" ]; then
    info "IFR-1 device rules are already installed"
else
    if [ ! -f "$UDEV_RULE_FILE" ]; then
        echo "$UDEV_RULE_CONTENT" | sudo tee "$UDEV_RULE_FILE" > /dev/null
        CLEANUP_ACTIONS="sudo rm -f '$UDEV_RULE_FILE'; $CLEANUP_ACTIONS"
        success "Created udev rule file: $UDEV_RULE_FILE"
    else
        info "Udev rule file already exists: $UDEV_RULE_FILE"
        
        # Check if the content is correct
        existing_content=$(cat "$UDEV_RULE_FILE")
        if [ "$existing_content" != "$UDEV_RULE_CONTENT" ]; then
            warning "Udev rule file exists but has different content."
            info "Expected: $UDEV_RULE_CONTENT"
            info "Found: $existing_content"
            read -r -p "Update the udev rule? (y/n): " update_udev
            if [[ "$update_udev" =~ ^[Yy]$ ]]; then
                echo "$UDEV_RULE_CONTENT" | sudo tee "$UDEV_RULE_FILE" > /dev/null
                success "Updated udev rule file: $UDEV_RULE_FILE"
            fi
        fi
    fi

    # Reload udev rules only if we changed/ensured our rules here
    info "Reloading udev rules..."
    sudo udevadm control --reload-rules
    sudo udevadm trigger
    success "Udev rules reloaded."
fi

# Final success message
echo -e "${GREEN}==================================================${NC}"
echo -e "${GREEN} IFR-1 Flight Controller Plugin Setup Complete    ${NC}"
echo -e "${GREEN}==================================================${NC}"

if [ "$XPLANE_RUNNING" = true ]; then
    info "You may now connect the IFR-1 device."
    info "X-Plane is running. Please restart X-Plane to load the plugin."
else
    info "You may now connect the IFR-1 device and start X-Plane."
fi

# Keep the window open for users running the script graphically
echo
info "Press Enter to finish the install."

# Prefer stdin when attached to a terminal; fall back to /dev/tty if available.
if [ -t 0 ]; then
    read -r _ || true
elif [ -e /dev/tty ]; then
    read -r _ < /dev/tty || true
fi

exit 0
