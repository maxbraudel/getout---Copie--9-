cmake_minimum_required(VERSION 3.11)
cmake_policy(SET CMP0072 NEW)

project(SORBETCOCO)

# Set the folder where the executable will be created
set(OUTPUT_BIN_FOLDER ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Libraries

# ---Add GL---
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
set(ALL_LIBRARIES ${OPENGL_LIBRARIES})

# ---Add GLFW---
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/glfw)
set(ALL_LIBRARIES ${ALL_LIBRARIES} glfw)

# ---Add glad---
add_library(glad third_party/glad/src/glad.c)
include_directories(third_party/glad/include)
set(ALL_LIBRARIES ${ALL_LIBRARIES} glad)

# ---Add glbasimac---
add_subdirectory(third_party/glbasimac)
set(ALL_LIBRARIES ${ALL_LIBRARIES} glbasimac)
include_directories(third_party/glbasimac/)
set(CMAKE_COLOR_MAKEFILE ON)

# ---Add Taskflow---
include_directories(third_party/taskflow)

# ---Add magic_enum---
include_directories(third_party/magic_enum)

# Main executable
add_executable(${PROJECT_NAME} src/main.cpp src/map.cpp src/terrainGeneration.cpp src/elementsOnMap.cpp src/player.cpp src/camera.cpp src/globals.cpp src/collision.cpp src/debug.cpp src/entities.cpp src/entityBehaviors.cpp src/pathfinding.cpp src/inputs.cpp src/threading.cpp src/asyncPathfinding.cpp src/crashDebug.cpp src/enumDefinitions.cpp)

# Find threading library
find_package(Threads REQUIRED)
set(ALL_LIBRARIES ${ALL_LIBRARIES} Threads::Threads)

# Add Windows-specific debugging libraries for crash handling
if(WIN32)
    set(ALL_LIBRARIES ${ALL_LIBRARIES} dbghelp psapi)
endif()

target_link_libraries(${PROJECT_NAME} ${ALL_LIBRARIES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
