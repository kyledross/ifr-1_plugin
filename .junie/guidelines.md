# Project Guidelines

- Keep this document up-to-date when there are any architectural or functional changes to the project.

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
- **C++ Features**: Prefer C++-style language features over C-style ones. For example, use C++-style casts (`static_cast`, `reinterpret_cast`, etc.) instead of C-style casts.
- **Global Namespace**: Avoid bringing reserved identifiers into the global namespace. For example, do not use `using ::testing::_;` at the global scope, as `_` is reserved by the implementation in the global namespace.
- **Dataref Handling**: Always verify dataref types using `IXPlaneSDK::GetDataRefTypes()`. Use `GetDatai`/`SetDatai` for integer datarefs and `GetDataf`/`SetDataf` for float datarefs to ensure compatibility with X-Plane's strict typing (e.g., `XPLMGetDataf` on an integer dataref returns `0.0f`).

## Configuration and Device State
- **Flexible Mapping**: The plugin uses a JSON-based configuration system to map HID events to X-Plane commands and datarefs.
- **Modes and Shifted State**: Controls are organized by modes (COM1, NAV1, AP, etc.). A "shifted" state (toggled via long-press on the inner knob) allows for secondary mappings (e.g., HDG, BARO, CRS).
- **LED Logic**: LEDs are driven by range-based or bit-mask tests defined in the JSON configuration, evaluated by `OutputProcessor`.

## Project Structure
    - `src/core`: Contains the main logic and interfaces.
    - `src/plugin_main.cpp`: Entry point for the X-Plane plugin.
    - `tests`: Contains unit tests.
    - `configs`: Contains JSON configuration files for different aircraft.
