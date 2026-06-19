# CS — Counter-Strike Inspired FPS Game

A multiplayer first-person shooter inspired by Counter-Strike, built on top of a custom 3D version of PTSD Engine.

## Overview

CS is a fully-featured FPS game project that recreates many core systems found in modern multiplayer shooters, including:

* Client-Server Networking
* Real-time Multiplayer Synchronization
* AI Bots
* NavMesh Pathfinding
* Skeletal Animation
* Weapon & Combat System
* Physics & Collision Detection
* Voice Chat
* Custom 3D Rendering Framework

The project was developed as an Object-Oriented Programming final project and serves as a complete demonstration of game engine architecture, networking, AI, rendering, and gameplay systems.

---

## Features

### Multiplayer Networking

* Host Authoritative Server Architecture
* Client-Server Communication
* State Synchronization
* Remote Player Interpolation
* Packet Serialization
* Real-time Multiplayer Gameplay

### FPS Gameplay

* First-Person Camera
* Character Movement
* Jumping & Gravity
* Weapon System
* Raycast Hit Detection
* Health & Damage System
* Death Handling

### Artificial Intelligence

* Finite State Machine (FSM)
* NavMesh Navigation
* A* Pathfinding
* Enemy Detection
* Patrol & Chase Behaviors
* Combat Decision Making

### Physics System

* Capsule Collider
* Character Controller
* Ground Detection
* Wall Collision
* Sliding Movement
* Map Collision Handling

### Rendering System

* Custom 3D PTSD Engine
* Scene Graph
* Mesh Rendering
* Model Loading
* Skeletal Animation
* Shadow Mapping
* Decal Rendering
* Dynamic Lighting

### Audio System

* Spatial Audio
* Weapon Sound Effects
* Footstep Audio
* Voice Chat Support

### Debug Tools

* Runtime Debug Panel
* FPS Monitoring
* Camera Information
* Physics Visualization
* Network Debugging

---

## Technology Stack

| Component     | Technology            |
| ------------- | --------------------- |
| Language      | C++17                 |
| Graphics API  | OpenGL                |
| Window System | SDL2                  |
| Math Library  | GLM                   |
| GUI           | ImGui                 |
| Asset Import  | Assimp                |
| Build System  | CMake                 |
| Networking    | Custom TCP/UDP Layer  |
| AI            | FSM + A* Pathfinding  |
| Rendering     | Custom 3D PTSD Engine |

---

## Controls

| Key           | Action            |
| ------------- | ----------------- |
| W / A / S / D | Move              |
| Mouse         | Look Around       |
| Left Click    | Fire              |
| Space         | Jump              |
| TAB           | Toggle Debug Mode |
| ESC           | Exit              |

---

## Highlights

### Custom 3D PTSD Engine

The original PTSD framework was designed as a 2D engine. This project extends it into a complete 3D rendering framework by implementing:

* 3D Transform System
* Camera System
* Scene Graph
* Skeletal Animation
* Model Loading
* Lighting & Shadows
* Rendering Pipeline

### NavMesh & A*

AI navigation is based on a NavMesh representation of the map combined with the A* pathfinding algorithm, allowing bots to navigate complex environments efficiently.

### Multiplayer Architecture

A host-authoritative architecture ensures consistent game state across all clients while reducing synchronization issues and preventing invalid client-side actions.

---

## Development Notes

Approximately 99% of the codebase was initially generated with GitHub Copilot assistance.

The primary development effort focused on:

* System integration
* Architecture refinement
* Bug fixing
* Collision system improvements
* Multiplayer synchronization debugging

The most challenging task was resolving player-to-map collision issues, including wall clipping, corner sticking, stair traversal problems, and movement stability.

---

## License

See [LICENSE](LICENSE) for details.

## Asset Credits

Gun Models:
https://sketchfab.com/TastyTony/models

Character Models:
https://sketchfab.com/erhanmatur/collections/csgo-character-229341bc3fb84b4d931318208d9634e2
