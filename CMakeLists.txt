cmake_minimum_required(VERSION 3.16)
project(CS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    set(TARGET_COMPILE_OPTIONS /W4)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/build)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/build)
    add_compile_options(/FS)
else()
    set(TARGET_COMPILE_OPTIONS -Wall -Wextra -pedantic)
endif()

# ── Pull in PTSD as a subdirectory ──────────────────────────────────────────
add_subdirectory(PTSD)

# ── CS source files ─────────────────────────────────────────────────────────
set(CS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CS_SRC_FILES
    ${CS_SRC_DIR}/main.cpp
    ${CS_SRC_DIR}/App.cpp
    ${CS_SRC_DIR}/Collision/CollisionMesh.cpp
    ${CS_SRC_DIR}/Collision/CapsuleCast.cpp
    ${CS_SRC_DIR}/Characters/Character.cpp
    ${CS_SRC_DIR}/Characters/CharacterModel.cpp
    ${CS_SRC_DIR}/Characters/Player.cpp
    ${CS_SRC_DIR}/Characters/RemotePlayer.cpp
    ${CS_SRC_DIR}/Gun/RayCast.cpp
    ${CS_SRC_DIR}/Gun/Gun.cpp
    ${CS_SRC_DIR}/Gun/AACHoneyBadger.cpp
    ${CS_SRC_DIR}/Gun/SKS.cpp
    ${CS_SRC_DIR}/Gun/SiliconeGun.cpp
    ${CS_SRC_DIR}/Effects/BulletHole.cpp
    # Network
    ${CS_SRC_DIR}/Network/Socket.cpp
    ${CS_SRC_DIR}/Network/NetworkManager.cpp
    ${CS_SRC_DIR}/Network/Server/GameServer.cpp
    ${CS_SRC_DIR}/Network/Client/GameClient.cpp
    ${CS_SRC_DIR}/Network/Discovery/LANDiscovery.cpp
)
set(CS_INCLUDE_FILES
    ${CS_INCLUDE_DIR}/App.hpp
    ${CS_INCLUDE_DIR}/Collision/CollisionTypes.hpp
    ${CS_INCLUDE_DIR}/Collision/CollisionMesh.hpp
    ${CS_INCLUDE_DIR}/Collision/CapsuleCast.hpp
    ${CS_INCLUDE_DIR}/Characters/Character.hpp
    ${CS_INCLUDE_DIR}/Characters/CharacterModel.hpp
    ${CS_INCLUDE_DIR}/Characters/Player.hpp
    ${CS_INCLUDE_DIR}/Characters/RemotePlayer.hpp
    ${CS_INCLUDE_DIR}/Gun/RayCast.hpp
    ${CS_INCLUDE_DIR}/Gun/Gun.hpp
    ${CS_INCLUDE_DIR}/Gun/AACHoneyBadger.hpp
    ${CS_INCLUDE_DIR}/Gun/SKS.hpp
    ${CS_INCLUDE_DIR}/Gun/SiliconeGun.hpp
    ${CS_INCLUDE_DIR}/Effects/BulletHole.hpp
    # Network
    ${CS_INCLUDE_DIR}/Network/NetworkTypes.hpp
    ${CS_INCLUDE_DIR}/Network/Packet.hpp
    ${CS_INCLUDE_DIR}/Network/Socket.hpp
    ${CS_INCLUDE_DIR}/Network/NetworkManager.hpp
    ${CS_INCLUDE_DIR}/Network/Server/GameServer.hpp
    ${CS_INCLUDE_DIR}/Network/Client/GameClient.hpp
    ${CS_INCLUDE_DIR}/Network/Client/Interpolation.hpp
    ${CS_INCLUDE_DIR}/Network/Discovery/LANDiscovery.hpp
)

# ── Executable ──────────────────────────────────────────────────────────────
add_executable(CS
    ${CS_SRC_FILES}
    ${CS_INCLUDE_FILES}
)

target_link_libraries(CS
    SDL2::SDL2main
    PTSD
)

# PTSD propagates DEPENDENCY_INCLUDE_DIRS and DEPENDENCY_LINK_LIBRARIES
# via PARENT_SCOPE when it is not the top-level project.
target_include_directories(CS SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PTSD/include
    ${DEPENDENCY_INCLUDE_DIRS}
)

target_include_directories(CS PRIVATE
    ${CS_INCLUDE_DIR}
)

target_compile_definitions(CS PRIVATE
    ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

# Prevent Windows min/max macros from interfering with std library
if(WIN32)
    target_compile_definitions(CS PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

target_compile_options(CS PRIVATE
    ${TARGET_COMPILE_OPTIONS}
)
