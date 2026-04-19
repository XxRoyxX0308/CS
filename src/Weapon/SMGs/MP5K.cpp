#include "Weapon/SMGs/MP5K.hpp"

namespace Weapon {

void MP5K::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/SMGs/mp5k/scene.gltf";
    m_WeaponScale    = glm::vec3(0.09f);
    m_WeaponOffset   = glm::vec3(0.22f, -0.24f, 0.48f);

    m_FireRate       = 15.0f;   // ~900 RPM
    m_RecoilStrength = 1.5f;
    m_RecoilRecovery = 10.0f;
    m_MagSize        = 30;
    m_ReloadTime     = 1.5f;
    m_BulletRange    = 120.0f;
    m_Damage         = 20.0f;
    m_Price          = 1500;

    // Spread: minSpread, maxSpread, moveRate, fireIncrement, jumpPenalty, recoveryRate, crouchMult
    m_Spread.Configure(0.8f, 6.0f, 3.0f, 0.3f, 3.5f, 7.0f, 2.0f);
}

} // namespace Weapon
