#include "Weapon/Pistols/M1895.hpp"

namespace Weapon {

void M1895::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Pistols/m1895/scene.gltf";
    m_WeaponScale    = glm::vec3(0.006f);
    m_WeaponOffset   = glm::vec3(0.18f, -0.18f, 0.5f);

    m_FireRate       = 4.0f;    // revolver — slower
    m_RecoilStrength = 1.2f;
    m_RecoilRecovery = 10.0f;
    m_MagSize        = 7;
    m_ReloadTime     = 2.0f;
    m_BulletRange    = 80.0f;
    m_Damage         = 40.0f;
    m_Price          = 500;

    // Spread: minSpread, maxSpread, moveRate, fireIncrement, jumpPenalty, recoveryRate, crouchMult
    m_Spread.Configure(0.4f, 4.5f, 3.0f, 0.8f, 3.0f, 5.0f, 2.5f);
}

} // namespace Weapon
