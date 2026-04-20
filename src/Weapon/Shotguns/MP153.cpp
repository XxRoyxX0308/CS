#include "Weapon/Shotguns/MP153.hpp"

namespace Weapon {

void MP153::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Shotguns/mp153/scene.gltf";
    m_WeaponScale    = glm::vec3(0.018f);
    m_WeaponOffset   = glm::vec3(0.2f, -0.2f, 0.46f);

    m_FireRate       = 1.5f;    // pump-action
    m_RecoilStrength = 3.0f;
    m_RecoilRecovery = 6.0f;
    m_MagSize        = 5;
    m_ReloadTime     = 2.5f;
    m_BulletRange    = 40.0f;
    m_Damage         = 80.0f;
    m_Price          = 1800;

    // Spread: minSpread, maxSpread, moveRate, fireIncrement, jumpPenalty, recoveryRate, crouchMult
    m_Spread.Configure(2.0f, 6.0f, 2.5f, 1.0f, 3.0f, 4.5f, 2.5f);
}

} // namespace Weapon
