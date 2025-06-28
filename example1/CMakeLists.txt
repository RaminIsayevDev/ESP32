# The following lines of cmake code are required for a component to be
# recognized by the build system. More detailed explanation is available at
# https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#cmake-project-api

# This specifies a minimal version of CMake required to build the project.
cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF's project build system. This macro handles setting up
# the build environment, finding components, and defining the project structure.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# This defines your project. The first argument is the project name.
# The second argument lists the components that make up your project.
# "main" is the default component where your application code resides.
project(example1 C) # Or 'C' if only C code
