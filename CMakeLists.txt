# -------------------------------
# eVTOL Simulation Demo
# -------------------------------

cmake_minimum_required(VERSION 3.16)
project(
    evtol-sim
    VERSION 1.0.0
    LANGUAGES CXX
)

# -------------------------------
# Project Options
# -------------------------------

option(EVTOL_BUILD_TESTS "Build tests" OFF)

# -------------------------------
# Setup Compiler
# -------------------------------

# Set compiler options
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------
# Resolve library dependencies
# -------------------------------

add_subdirectory(thirdparty)

# -------------------------------
# Build Simulation Framework
# -------------------------------

add_library(sim src/sim.cpp src/sim.hpp)
target_link_libraries(sim PUBLIC nlohmann_json::nlohmann_json)
target_include_directories(sim PUBLIC src)

# -------------------------------
# Build Simulator
# -------------------------------

add_executable(evtol-sim src/main.cpp)
target_link_libraries(evtol-sim PRIVATE nlohmann_json::nlohmann_json fmt::fmt sim)

# -------------------------------
# Build Unit Tests
# -------------------------------

if(EVTOL_BUILD_TESTS)
    message(STATUS "evtol-sim: building with tests")
    enable_testing()
    add_subdirectory(tests)
endif()
