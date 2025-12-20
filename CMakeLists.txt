cmake_minimum_required(VERSION 3.25)
project(ifr1flex)

# 1. C++23 Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# 2. External Libraries (Google Test & JSON)
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

FetchContent_Declare(
  hidapi
  URL https://github.com/libusb/hidapi/archive/refs/tags/hidapi-0.14.0.tar.gz
)

# 2.1 hidapi configuration
set(HIDAPI_BUILD_HIDTEST OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
if(UNIX AND NOT APPLE)
    set(HIDAPI_WITH_LIBUSB OFF CACHE BOOL "" FORCE)
    set(HIDAPI_WITH_HIDRAW ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest json hidapi)

enable_testing()

# 3. X-Plane SDK Path
set(XPLANE_SDK_PATH "${CMAKE_SOURCE_DIR}/SDK" CACHE PATH "Path to X-Plane SDK")

# 4. Include SDK Headers and JSON
# These will be handled per-target to be more robust
# include_directories(${XPLANE_SDK_PATH}/CHeaders/XPLM)
# include_directories(${XPLANE_SDK_PATH}/CHeaders/Widgets)
# include_directories(src/core)

# 5. Definitions
add_definitions(-DAPL=0 -DIBM=0 -DLIN=1)
add_definitions(-DXPLM200 -DXPLM210 -DXPLM300 -DXPLM301 -DXPLM302 -DXPLM303 -DXPLM400)

# 6. Core Logic Library (Testable)
add_library(ifr1flex_core STATIC
        src/core/ConfigManager.cpp
        src/core/EventProcessor.cpp
        src/core/XPlaneSDK_Actual.cpp
        src/core/HIDManager.cpp
        src/core/OutputProcessor.cpp
        src/core/DeviceHandler.cpp
        src/core/ConditionEvaluator.cpp
)
target_include_directories(ifr1flex_core PUBLIC 
    src/core
    ${XPLANE_SDK_PATH}/CHeaders/XPLM
    ${XPLANE_SDK_PATH}/CHeaders/Widgets
    ${hidapi_SOURCE_DIR}
)
target_link_libraries(ifr1flex_core PUBLIC 
    nlohmann_json::nlohmann_json
    hidapi::hidapi
)

# 7. Create Library (Plugin Target)
add_library(ifr1flex SHARED src/plugin_main.cpp)
target_link_libraries(ifr1flex PRIVATE ifr1flex_core)
# ifr1flex needs SDK headers too, which it gets from ifr1flex_core PUBLIC includes

# 8. Static Linking
target_link_options(ifr1flex PRIVATE -static-libgcc -static-libstdc++)

# 9. Output Formatting
set_target_properties(ifr1flex PROPERTIES PREFIX "")
set_target_properties(ifr1flex PROPERTIES SUFFIX ".xpl")

# 10. Unit Tests
add_executable(ifr1flex_tests
        tests/ConfigManager_test.cpp
        tests/EventProcessor_test.cpp
        tests/OutputProcessor_test.cpp
        tests/DeviceHandler_test.cpp
        tests/ConditionalAction_test.cpp
        tests/Failing_test.cpp
)
target_include_directories(ifr1flex_tests PRIVATE tests)
target_link_libraries(ifr1flex_tests PRIVATE
        ifr1flex_core
        gtest_main
        gmock
)
add_test(NAME ifr1flex_tests COMMAND ifr1flex_tests)

# 11. (Optional) Deploy to Simulator
# install(TARGETS ifr1flex LIBRARY DESTINATION "/home/kyle/X-Plane 12/Resources/plugins/ifr1flex/lin_x64")
