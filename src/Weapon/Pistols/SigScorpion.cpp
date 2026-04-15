#include "Weapon/Pistols/SigScorpion.hpp"

namespace Weapon {

void SigScorpion::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Pistols/sig_scorpion/scene.gltf";
    m_WeaponScale    = glm::vec3(0.015f);
    m_WeaponOffset   = glm::vec3(0.18f, -0.18f, 0.5f);

    m_FireRate       = 6.0f;    // semi-auto pistol
    m_RecoilStrength = 0.8f;
    m_RecoilRecovery = 10.0f;
    m_MagSize        = 12;
    m_ReloadTime     = 1.2f;
    m_BulletRange    = 100.0f;
    m_Damage         = 30.0f;
    m_Price          = 700;
}

} // namespace Weapon
