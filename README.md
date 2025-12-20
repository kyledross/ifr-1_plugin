# IFR-1 Flex Flight Controller Plugin for X-Plane 12 on Linux
Copyright 2025 Kyle D. Ross
See LICENSE for details.

## Disclaimer

This software is not affiliated with, endorsed by, or supported by Octavi GmbH or Laminar Research. All trademarks are the property of their respective owners and are used herein for reference only.

## What is this?

**IFR-1 Flex** is a native X-Plane plugin that integrates the Octavi IFR-1 hardware controller with X-Plane 12 on Linux. 

Unlike other plugins, IFR-1 Flex is designed for maximum flexibility. It uses a JSON-based configuration system that allows you to map hardware events (knob rotations, button presses) to any X-Plane command or dataref. This means you can easily customize the controller for any aircraft, even those with complex custom systems, without writing any code.

### Why not Windows?
There are already well-established plugins for Windows that provide similar functionality. See the IFR-1's webpage at https://www.octavi.net/ for more information.

## Your support is appreciated
Writing good quality software takes time and tools. The source code for this is openly available for anyone. If you find this plugin helpful, please consider supporting the project at https://buymeacoffee.com/kyledross. Any contribution to help defray costs is greatly appreciated.

## Features

- **Native Performance**: Written in C++ as a native X-Plane plugin for low latency and high reliability.
- **Flexible JSON Configuration**: Define your own button mappings and LED logic in simple JSON files.
- **Per-Aircraft Support**: The plugin automatically loads the correct configuration based on the aircraft you are flying.
- **Shifted States**: Supports a "shifted" state (toggled via long-press on the inner knob) to double the available controls in each mode.
- **Dynamic LED Feedback**: LEDs are driven by aircraft state (datarefs), allowing for accurate status displays even for third-party aircraft.
- **Expandable**: Add support for any new aircraft by simply dropping a new JSON file into the `configs` folder.

## Requirements

### Software
- X-Plane 12.x on Linux

### Hardware
- Octavi IFR-1 USB controller

## Quick Start (Recommended)

The easiest way to install the plugin is to use the provided installation script.

### 1. Download the Plugin
Download the latest release from the [GitHub Releases](https://github.com/kyledross/ifr1flex/releases) page. Download the `ifr1flex_Linux_Install.tar.gz` (or similarly named) archive.

### 2. Unpack the Plugin
Extract the downloaded archive to a temporary directory.

```bash
tar -xzf ifr1flex_Linux_Install.tar.gz
```

### 3. Install the Plugin
Disconnect your IFR-1 device, then run the installation script from the extracted directory:

```bash
./install.sh
```

The script will:
- Locate your X-Plane installation automatically.
- Copy the plugin and default configurations to the correct location.
- Set up necessary `udev` rules so the plugin can communicate with the hardware without root permissions.

### 4. Start X-Plane
After the installation is finished, connect your IFR-1 device and launch X-Plane 12. The plugin will automatically detect your hardware and load the appropriate configuration for your aircraft.

## Updates
To update to a newer version, simply download the new release and run the `install.sh` script again. It will replace the existing plugin and update the default configurations.

## Aircraft Support

The plugin comes with pre-configured support for several aircraft:

- **General Aviation (G1000)**: Optimized for aircraft like the Cessna 172 G1000 and Cirrus SR22.
- **General Aviation (G430/G530)**: For aircraft using standard Garmin 430/530 units.
- **Beechcraft King Air/Baron**: Tailored for the default Beechcraft models.
- **McDonnell-Douglas MD-80**: Basic controls for the MD-80 series.
- **Piper Cub**: Simple controls for the classic Cub.
- **Schleicher ASK 21**: Glider-specific configuration.
- **Generic Fallback**: A broad configuration that works with most standard X-Plane aircraft.

### Adding New Aircraft
You can add support for any aircraft by creating a new JSON file in the `Resources/plugins/ifr1flex/configs/` directory. For detailed instructions on how to create these configurations, see the [Aircraft Configuration Guide](documentation/aircraft_configs.md).

## Functionality

While the specific behavior of each button and knob is defined by the JSON configuration, most default configs follow a standard layout:

- **Mode Selection**: Press mode buttons to choose the mode that you will be working with (COM1, NAV1, FMS1, etc.).
- **Shifted Modes**: Long-press the **inner knob** (for ~0.5s) to toggle the shifted state. For example, in COM1 mode, shifting will take you to HDG mode.
- **Autopilot Controls**: The buttons across the bottom and right are generally mapped to autopilot and FMS functions.
- **LEDs**: The LEDs at the bottom show the status of the corresponding autopilot modes (e.g., AP engaged, ALT hold, etc.).

## Troubleshooting

### Device Not Found
- Verify the device is connected by running `lsusb | grep 04d8:e6d6`.
- Ensure the `udev` rules were installed correctly by the `install.sh` script.
- Check X-Plane's `Log.txt` for messages from the `ifr1flex` plugin.

### Plugin Not Loading
- Ensure you are running X-Plane 12 on Linux.
- Check `Log.txt` for any error messages during plugin initialization.
- Verify the plugin has been installed to `Resources/plugins/ifr1flex/lin_x64/ifr1flex.xpl`.

## Support
This plugin is a hobby project. If you encounter an issue or have a feature request, please open an issue on the [GitHub Issues](https://github.com/kyledross/ifr1flex/issues) page.

Contributions are welcome! If you create a great configuration for a new aircraft, please consider submitting a Pull Request.

## License
See the accompanying `LICENSE` file for details.

