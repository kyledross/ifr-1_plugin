# Project Guidelines

## Reference Project
- The `IFR-1_Native` project (located at `../IFR-1_Native`) is the primary reference for this project.
- Use it to understand the existing logic, HID communication with the Octavi IFR-1, and aircraft configuration patterns.
- The current project (`ifr1flex`) aims to be a more flexible version of the original plugin.

## Architecture and Testing
- **Unit Testing**: Google Test and Google Mock are used for unit testing.
- **Dependency Injection**: Use interfaces (e.g., `IXPlaneSDK`) for X-Plane SDK interactions to allow mocking in tests.
- **Core Logic**: Core logic is implemented in a separate static library (`ifr1flex_core`) to be linked by both the plugin and the test runner.
- **CMake Configuration**: Prefer target-based configuration (`target_include_directories`, `target_link_libraries`) over global commands (`include_directories`) to ensure better IDE integration and modularity.
- **JSON Library**: `nlohmann/json` is used for parsing aircraft configurations.
- **Project Structure**:
    - `src/core`: Contains the main logic and interfaces.
    - `src/plugin_main.cpp`: Entry point for the X-Plane plugin.
    - `tests`: Contains unit tests.
    - `configs`: Contains JSON configuration files for different aircraft.
