#include "Weapon/Rifles/AXL47.hpp"

namespace Weapon {

void AXL47::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Rifles/axl-47/scene.gltf";
    m_WeaponScale    = glm::vec3(0.25f);
    m_WeaponOffset   = glm::vec3(0.23f, -0.20f, 0.5f);

    m_FireRate       = 10.0f;   // ~600 RPM
    m_RecoilStrength = 1.5f;
    m_RecoilRecovery = 7.0f;
    m_MagSize        = 30;
    m_ReloadTime     = 2.0f;
    m_BulletRange    = 200.0f;
    m_Damage         = 38.0f;
    m_Price          = 2500;
}

} // namespace Weapon
