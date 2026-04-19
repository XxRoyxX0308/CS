#include "Weapon/Snipers/AR30.hpp"

namespace Weapon {

void AR30::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Snipers/ar30/scene.gltf";
    m_WeaponScale    = glm::vec3(0.024f);
    m_WeaponOffset   = glm::vec3(0.22f, -0.24f, 0.6f);

    m_FireRate       = 1.2f;    // bolt-action, slower
    m_RecoilStrength = 4.0f;
    m_RecoilRecovery = 4.5f;
    m_MagSize        = 5;
    m_ReloadTime     = 2.5f;
    m_BulletRange    = 350.0f;
    m_Damage         = 100.0f;
    m_Price          = 5000;

    // Spread: minSpread, maxSpread, moveRate, fireIncrement, jumpPenalty, recoveryRate, crouchMult
    m_Spread.Configure(0.05f, 7.0f, 6.0f, 3.0f, 6.0f, 3.8f, 5.0f);
}

} // namespace Weapon
