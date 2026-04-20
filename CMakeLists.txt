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
    # App Module (Refactored)
    ${CS_SRC_DIR}/App/App.cpp
    ${CS_SRC_DIR}/App/StateManager.cpp
    ${CS_SRC_DIR}/App/InputManager.cpp
    ${CS_SRC_DIR}/App/UIManager.cpp
    ${CS_SRC_DIR}/App/CombatManager.cpp
    ${CS_SRC_DIR}/App/NetworkController.cpp
    ${CS_SRC_DIR}/App/GameManager.cpp
    # Physics
    ${CS_SRC_DIR}/Physics/CollisionMesh.cpp
    ${CS_SRC_DIR}/Physics/CapsuleCast.cpp
    ${CS_SRC_DIR}/Entity/Character.cpp
    ${CS_SRC_DIR}/Entity/CharacterModel.cpp
    ${CS_SRC_DIR}/Entity/Player.cpp
    ${CS_SRC_DIR}/Entity/RemotePlayer.cpp
    ${CS_SRC_DIR}/Entity/BotPlayer.cpp
    # Navigation
    ${CS_SRC_DIR}/Navigation/PathFinder.cpp
    ${CS_SRC_DIR}/Navigation/NavMesh.cpp
    ${CS_SRC_DIR}/Weapon/RayCast.cpp
    ${CS_SRC_DIR}/Weapon/Weapon.cpp
    ${CS_SRC_DIR}/Weapon/WeaponSpread.cpp
    ${CS_SRC_DIR}/Weapon/Knives/CombatKnife.cpp
    ${CS_SRC_DIR}/Weapon/Knives/VictorinoxKnife.cpp
    ${CS_SRC_DIR}/Weapon/Pistols/M1895.cpp
    ${CS_SRC_DIR}/Weapon/Pistols/SigScorpion.cpp
    ${CS_SRC_DIR}/Weapon/Rifles/AXL47.cpp
    ${CS_SRC_DIR}/Weapon/Rifles/HoneyBadger.cpp
    ${CS_SRC_DIR}/Weapon/SMGs/MP5K.cpp
    ${CS_SRC_DIR}/Weapon/SMGs/UZI.cpp
    ${CS_SRC_DIR}/Weapon/Snipers/Axis2.cpp
    ${CS_SRC_DIR}/Weapon/Snipers/AR30.cpp
    ${CS_SRC_DIR}/Weapon/Shotguns/MP153.cpp
    ${CS_SRC_DIR}/Weapon/Shotguns/DoubleDeuce.cpp
    ${CS_SRC_DIR}/Effects/BulletHole.cpp
    # Network
    ${CS_SRC_DIR}/Network/Socket.cpp
    ${CS_SRC_DIR}/Network/NetworkManager.cpp
    ${CS_SRC_DIR}/Network/Server/GameServer.cpp
    ${CS_SRC_DIR}/Network/Client/GameClient.cpp
    ${CS_SRC_DIR}/Network/Discovery/LANDiscovery.cpp
)
set(CS_INCLUDE_FILES
    # App Module (Refactored)
    ${CS_INCLUDE_DIR}/App/App.hpp
    ${CS_INCLUDE_DIR}/App/StateManager.hpp
    ${CS_INCLUDE_DIR}/App/InputManager.hpp
    ${CS_INCLUDE_DIR}/App/UIManager.hpp
    ${CS_INCLUDE_DIR}/App/CombatManager.hpp
    ${CS_INCLUDE_DIR}/App/NetworkController.hpp
    ${CS_INCLUDE_DIR}/App/GameManager.hpp
    # Core
    ${CS_INCLUDE_DIR}/Core/Types.hpp
    ${CS_INCLUDE_DIR}/Core/Constants.hpp
    ${CS_INCLUDE_DIR}/Physics/CollisionTypes.hpp
    ${CS_INCLUDE_DIR}/Physics/CollisionMesh.hpp
    ${CS_INCLUDE_DIR}/Physics/CapsuleCast.hpp
    ${CS_INCLUDE_DIR}/Entity/Character.hpp
    ${CS_INCLUDE_DIR}/Entity/CharacterModel.hpp
    ${CS_INCLUDE_DIR}/Entity/Player.hpp
    ${CS_INCLUDE_DIR}/Entity/RemotePlayer.hpp
    ${CS_INCLUDE_DIR}/Entity/BotPlayer.hpp
    # Navigation
    ${CS_INCLUDE_DIR}/Navigation/PathFinder.hpp
    ${CS_INCLUDE_DIR}/Navigation/NavMesh.hpp
    ${CS_INCLUDE_DIR}/Weapon/RayCast.hpp
    ${CS_INCLUDE_DIR}/Weapon/Weapon.hpp
    ${CS_INCLUDE_DIR}/Weapon/WeaponDefs.hpp
    ${CS_INCLUDE_DIR}/Weapon/WeaponSpread.hpp
    ${CS_INCLUDE_DIR}/Weapon/Knives/CombatKnife.hpp
    ${CS_INCLUDE_DIR}/Weapon/Knives/VictorinoxKnife.hpp
    ${CS_INCLUDE_DIR}/Weapon/Pistols/M1895.hpp
    ${CS_INCLUDE_DIR}/Weapon/Pistols/SigScorpion.hpp
    ${CS_INCLUDE_DIR}/Weapon/Rifles/AXL47.hpp
    ${CS_INCLUDE_DIR}/Weapon/Rifles/HoneyBadger.hpp
    ${CS_INCLUDE_DIR}/Weapon/SMGs/MP5K.hpp
    ${CS_INCLUDE_DIR}/Weapon/SMGs/UZI.hpp
    ${CS_INCLUDE_DIR}/Weapon/Snipers/Axis2.hpp
    ${CS_INCLUDE_DIR}/Weapon/Snipers/AR30.hpp
    ${CS_INCLUDE_DIR}/Weapon/Shotguns/MP153.hpp
    ${CS_INCLUDE_DIR}/Weapon/Shotguns/DoubleDeuce.hpp
    ${CS_INCLUDE_DIR}/Effects/BulletHole.hpp
    # Network
    ${CS_INCLUDE_DIR}/Network/Types.hpp
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
