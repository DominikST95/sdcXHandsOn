# Current Target
set( CURRENT_TARGET_NAME ORTableDemoConsumer)
# Add this for better project structure after cmake generation
project(${CURRENT_TARGET_NAME})

message(STATUS "Adding Target ${CURRENT_TARGET_NAME}...")
add_executable(${CURRENT_TARGET_NAME} "")

# Variables for better handling
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR})


# Add the sources to the target
target_sources(${CURRENT_TARGET_NAME}
    PRIVATE
        # Source Files
        ${SRC_DIR}/main.cpp
        #...
        # Headers
        #...
)

# Make sure this include dir can be found inside the project to include
target_include_directories(${CURRENT_TARGET_NAME} PUBLIC ${SRC_DIR})
# Additional include directories

# Link every dependency we need to build this
target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE sdcX::ConsumerAPI)
target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE sdcX::SDCCore)


# build
set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
                        LINKER_LANGUAGE CXX
)

# Copy required dependencies and resources to the runtime output directory
include(copy_resources)
copy_resources (${CMAKE_BINARY_DIR}/bin)
include(copy_shared_dependencies)
copy_shared_dependencies (${CMAKE_BINARY_DIR}/bin)