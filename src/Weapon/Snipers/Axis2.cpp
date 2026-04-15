#include "Weapon/Snipers/Axis2.hpp"

namespace Weapon {

void Axis2::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Snipers/axis_2/scene.gltf";
    m_WeaponScale    = glm::vec3(0.034f);
    m_WeaponOffset   = glm::vec3(0.22f, -0.16f, 0.52f);

    m_FireRate       = 1.5f;    // bolt-action
    m_RecoilStrength = 3.5f;
    m_RecoilRecovery = 5.0f;
    m_MagSize        = 4;
    m_ReloadTime     = 3.0f;
    m_BulletRange    = 300.0f;
    m_Damage         = 90.0f;
    m_Price          = 4750;
}

} // namespace Weapon
