#include "Weapon/SMGs/UZI.hpp"

namespace Weapon {

void UZI::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/SMGs/uzi/scene.gltf";
    m_WeaponScale    = glm::vec3(0.2f);
    m_WeaponOffset   = glm::vec3(0.22f, -0.20f, 0.45f);

    m_FireRate       = 16.0f;   // ~780 RPM
    m_RecoilStrength = 1.6f;
    m_RecoilRecovery = 9.0f;
    m_MagSize        = 25;
    m_ReloadTime     = 1.6f;
    m_BulletRange    = 130.0f;
    m_Damage         = 22.0f;
    m_Price          = 1700;

    // Spread: minSpread, maxSpread, moveRate, fireIncrement, jumpPenalty, recoveryRate, crouchMult
    m_Spread.Configure(0.9f, 6.5f, 3.5f, 0.35f, 3.5f, 6.5f, 2.0f);
}

} // namespace Weapon
