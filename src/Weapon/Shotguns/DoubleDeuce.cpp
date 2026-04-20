#include "Weapon/Shotguns/DoubleDeuce.hpp"

namespace Weapon {

void DoubleDeuce::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Shotguns/stolzer__son_double_deuce/scene.gltf";
    m_WeaponScale    = glm::vec3(0.014f);
    m_WeaponOffset   = glm::vec3(0.2f, -0.15f, 0.52f);

    m_FireRate       = 3.0f;    // double-barrel, fast follow-up
    m_RecoilStrength = 3.5f;
    m_RecoilRecovery = 5.0f;
    m_MagSize        = 2;
    m_ReloadTime     = 2.0f;
    m_BulletRange    = 30.0f;
    m_Damage         = 100.0f;
    m_Price          = 1200;

    // Spread: minSpread, maxSpread, moveRate, fireIncrement, jumpPenalty, recoveryRate, crouchMult
    m_Spread.Configure(2.5f, 7.0f, 3.0f, 1.2f, 3.5f, 4.5f, 3.0f);
}

} // namespace Weapon
