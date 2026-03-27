#include "App/InputManager.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

namespace App {

void InputManager::LockCursor() {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    m_CursorLocked = true;
}

void InputManager::UnlockCursor() {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    m_CursorLocked = false;
}

void InputManager::ToggleCursor() {
    if (m_CursorLocked) {
        UnlockCursor();
    } else {
        LockCursor();
    }
}

void InputManager::ProcessMouseLook(Core3D::Camera& camera) {
    if (!m_CursorLocked) return;

    int xrel = 0, yrel = 0;
    SDL_GetRelativeMouseState(&xrel, &yrel);

    if (xrel != 0 || yrel != 0) {
        camera.ProcessMouseMovement(
            static_cast<float>(xrel),
            static_cast<float>(-yrel)
        );
    }
}

Network::InputState InputManager::SampleInput(const Core3D::Camera& camera,
                                               const glm::vec3& position,
                                               bool isWalking,
                                               bool isOnGround) const {
    Network::InputState input;
    input.keys = 0;

    if (Util::Input::IsKeyPressed(Util::Keycode::W)) input.keys |= Network::INPUT_W;
    if (Util::Input::IsKeyPressed(Util::Keycode::S)) input.keys |= Network::INPUT_S;
    if (Util::Input::IsKeyPressed(Util::Keycode::A)) input.keys |= Network::INPUT_A;
    if (Util::Input::IsKeyPressed(Util::Keycode::D)) input.keys |= Network::INPUT_D;
    if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) input.keys |= Network::INPUT_JUMP;
    if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) input.keys |= Network::INPUT_FIRE;
    if (Util::Input::IsKeyDown(Util::Keycode::R)) input.keys |= Network::INPUT_RELOAD;

    input.yaw = camera.GetYaw();
    input.pitch = camera.GetPitch();
    input.position = position;

    input.flags = 0;
    if (isWalking) input.flags |= Network::FLAG_IS_WALKING;
    if (isOnGround) input.flags |= Network::FLAG_ON_GROUND;

    return input;
}

bool InputManager::IsExitRequested() const {
    return Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit();
}

bool InputManager::IsTabPressed() const {
    return Util::Input::IsKeyDown(Util::Keycode::TAB);
}

bool InputManager::IsToggleModelPressed() const {
    return Util::Input::IsKeyDown(Util::Keycode::V);
}

bool InputManager::IsSwitchCharacterPressed() const {
    return Util::Input::IsKeyDown(Util::Keycode::C);
}

} // namespace App
