#ifndef CS_APP_INPUTMANAGER_HPP
#define CS_APP_INPUTMANAGER_HPP

#include "Network/Types.hpp"
#include "Core3D/Camera.hpp"

#include <SDL.h>

namespace App {

/**
 * @brief Manages input handling, cursor state, and input sampling for network.
 */
class InputManager {
public:
    InputManager() = default;

    /** @brief Lock cursor for FPS mode. */
    void LockCursor();

    /** @brief Unlock cursor for menu mode. */
    void UnlockCursor();

    /** @brief Toggle cursor lock state. */
    void ToggleCursor();

    /** @brief Check if cursor is locked. */
    bool IsCursorLocked() const { return m_CursorLocked; }

    /** @brief Process mouse movement for camera look (only when cursor locked). */
    void ProcessMouseLook(Core3D::Camera& camera);

    /**
     * @brief Sample current input state for network transmission.
     * @param camera Camera for yaw/pitch values.
     * @param position Current player position.
     * @param isWalking Whether player is walking.
     * @param isOnGround Whether player is on ground.
     */
    Network::InputState SampleInput(const Core3D::Camera& camera,
                                    const glm::vec3& position,
                                    bool isWalking,
                                    bool isOnGround) const;

    /** @brief Check if exit was requested (ESC or window close). */
    bool IsExitRequested() const;

    /** @brief Check if TAB was pressed (for debug panel toggle). */
    bool IsTabPressed() const;

    /** @brief Check if V was pressed (model visibility toggle). */
    bool IsToggleModelPressed() const;

    /** @brief Check if C was pressed (character switch). */
    bool IsSwitchCharacterPressed() const;

    /** @brief Check if B was pressed (buy menu toggle). */
    bool IsBuyMenuPressed() const;

private:
    bool m_CursorLocked = false;
};

} // namespace App

#endif // CS_APP_INPUTMANAGER_HPP
