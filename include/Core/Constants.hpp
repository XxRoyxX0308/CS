#ifndef CS_CORE_CONSTANTS_HPP
#define CS_CORE_CONSTANTS_HPP

namespace CS {
namespace Constants {

// ── Physics Constants ─────────────────────────────────────────────────────

constexpr float GRAVITY = 9.8f;              // Gravity acceleration (m/s²)
constexpr float PLAYER_HEIGHT = 1.7f;        // Player eye height (meters)
constexpr float PLAYER_RADIUS = 0.5f;        // Collision capsule radius (meters)
constexpr float SKIN_WIDTH = 0.005f;         // Collision skin width (meters)
constexpr float JUMP_SPEED = 5.0f;           // Jump initial velocity (m/s)
constexpr float FALLBACK_FLOOR_Y = -50.0f;   // Fallback floor height

// ── Player Constants ──────────────────────────────────────────────────────

constexpr float DEFAULT_HEALTH = 100.0f;     // Default player health
constexpr float DEFAULT_MAX_HEALTH = 100.0f; // Default max health
constexpr float PLAYER_MOVE_SPEED = 5.0f;    // Movement speed (m/s)
constexpr float MOUSE_SENSITIVITY = 0.1f;   // Mouse sensitivity

// ── Network Constants ─────────────────────────────────────────────────────

constexpr int MAX_PLAYERS = 8;               // Maximum players in game
constexpr float NETWORK_TICK_RATE = 20.0f;   // Network updates per second (Hz)
constexpr float INTERPOLATION_DELAY = 0.1f;  // Interpolation delay (seconds)
constexpr int STATE_BUFFER_SIZE = 32;        // Network state buffer size

// ── Combat Constants ──────────────────────────────────────────────────────

constexpr float DEFAULT_BULLET_RANGE = 200.0f; // Default bullet range (meters)
constexpr float DEFAULT_DAMAGE = 25.0f;        // Default weapon damage

// ── Effect Constants ──────────────────────────────────────────────────────

constexpr int MAX_BULLET_HOLES = 100;          // Maximum bullet hole decals
constexpr float BULLET_HOLE_LIFETIME = 10.0f;  // Bullet hole lifetime (seconds)
constexpr float BULLET_HOLE_FADE = 2.0f;       // Fade duration (seconds)

} // namespace Constants
} // namespace CS

#endif // CS_CORE_CONSTANTS_HPP
