cmake_minimum_required(VERSION 3.16)
project(CS2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    set(TARGET_COMPILE_OPTIONS /W4)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/build)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/build)
else()
    set(TARGET_COMPILE_OPTIONS -Wall -Wextra -pedantic)
endif()

# ── Pull in PTSD as a subdirectory ──────────────────────────────────────────
add_subdirectory(PTSD)

# ── CS2 source files ────────────────────────────────────────────────────────
set(CS2_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CS2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CS2_SRC_FILES
    ${CS2_SRC_DIR}/main.cpp
    ${CS2_SRC_DIR}/App.cpp
)
set(CS2_INCLUDE_FILES
    ${CS2_INCLUDE_DIR}/App.hpp
)

# ── Executable ──────────────────────────────────────────────────────────────
add_executable(CS2
    ${CS2_SRC_FILES}
    ${CS2_INCLUDE_FILES}
)

target_link_libraries(CS2
    SDL2::SDL2main
    PTSD
)

# PTSD propagates DEPENDENCY_INCLUDE_DIRS and DEPENDENCY_LINK_LIBRARIES
# via PARENT_SCOPE when it is not the top-level project.
target_include_directories(CS2 SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PTSD/include
    ${DEPENDENCY_INCLUDE_DIRS}
)

target_include_directories(CS2 PRIVATE
    ${CS2_INCLUDE_DIR}
)

target_compile_definitions(CS2 PRIVATE
    ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/PTSD/assets"
    RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources"
)

target_compile_options(CS2 PRIVATE
    ${TARGET_COMPILE_OPTIONS}
)
