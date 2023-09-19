#Minimum Cmake Version
cmake_minimum_required(VERSION 3.10)

################################################################################
# Using own CMake Modules
################################################################################
get_filename_component(PROJECT_CMAKE_MODULES ${CMAKE_CURRENT_LIST_DIR}/tools/cmake/Modules REALPATH)
message(STATUS "Adding ${PROJECT_CMAKE_MODULES} to Modules Path...")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_CMAKE_MODULES})
################################################################################

################################################################################
# C++ standard
################################################################################
message(STATUS "Setting CXX_STANDARD...")
set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ Standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
################################################################################


################################################################################
# Main Project
################################################################################
set(PROJECT_NAME "HelloSDC") # in case you need it in a config file
project(${PROJECT_NAME})
################################################################################

################################################################################
# Check Compiler Version
# Min Requirements chosen based on C++14
################################################################################
include(checkRequiredCompilerCXX14)
################################################################################


################################################################################
# Targets
# Overwrite
################################################################################
set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "Config Types")
# None specified?
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type specified, setting to Debug.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Debug as default" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Release" "Debug")
endif()
################################################################################


################################################################################
# RPATH Settings
# NOTE: Set the RPath to the "Current Directory". (Portability)
################################################################################
include(reset_RPATH)
################################################################################

################################################################################
# Import sdcX
################################################################################
# Set the path where we installed the sdcX
set(SDCX_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/external/sdcX" CACHE STRING "sdcX install dir")
# Set the following variable to help find_package(... CONFIG) in finding the sdcX
set(CMAKE_PREFIX_PATH ${SDCX_INSTALL_PATH}/lib/cmake/sdcX)
find_package(sdcX CONFIG REQUIRED)
target_link_libraries(sdcX::SDCCore INTERFACE sdcX::Logging)
################################################################################

# Proceed to Examples
add_subdirectory(libs)

# ...
