#include "Weapon/Blades/CombatKnife.hpp"

namespace Weapon {

void CombatKnife::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Blades/combat_knife/scene.gltf";
    m_WeaponScale    = glm::vec3(0.05f);
    m_WeaponOffset   = glm::vec3(0.2f, -0.2f, 0.4f);

    m_FireRate       = 2.0f;    // stab rate
    m_RecoilStrength = 0.0f;
    m_RecoilRecovery = 0.0f;
    m_MagSize        = 1;       // melee — infinite use, 1 "ammo"
    m_ReloadTime     = 0.0f;
    m_BulletRange    = 2.0f;    // melee range
    m_Damage         = 55.0f;
    m_Price          = 200;
}

} // namespace Weapon
