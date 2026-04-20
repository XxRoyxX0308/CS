#include "Weapon/Rifles/HoneyBadger.hpp"

namespace Weapon {

void HoneyBadger::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Rifles/honey_badger/scene.gltf";
    m_WeaponScale    = glm::vec3(0.025f);
    m_WeaponOffset   = glm::vec3(0.23f, -0.25f, 0.5f);

    m_FireRate       = 12.0f;   // ~800 RPM, suppressed
    m_RecoilStrength = 1.2f;
    m_RecoilRecovery = 8.0f;
    m_MagSize        = 30;
    m_ReloadTime     = 1.8f;
    m_BulletRange    = 200.0f;
    m_Damage         = 35.0f;
    m_Price          = 2700;

    // Spread: minSpread, maxSpread, moveRate, fireIncrement, jumpPenalty, recoveryRate, crouchMult
    m_Spread.Configure(0.2f, 4.5f, 3.5f, 0.7f, 3.5f, 5.0f, 2.5f);
}

} // namespace Weapon
