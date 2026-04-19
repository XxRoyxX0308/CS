#include "Weapon/Knives/VictorinoxKnife.hpp"

namespace Weapon {

void VictorinoxKnife::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/weapons/Knives/victorinox_multitool_knife/scene.gltf";
    m_WeaponScale    = glm::vec3(0.002f);
    m_WeaponOffset   = glm::vec3(0.2f, -0.2f, 0.4f);

    m_FireRate       = 2.5f;
    m_RecoilStrength = 0.0f;
    m_RecoilRecovery = 0.0f;
    m_MagSize        = 1;
    m_ReloadTime     = 0.0f;
    m_BulletRange    = 2.0f;
    m_Damage         = 40.0f;
    m_Price          = 0;       // free starter knife

    // Knives have no spread
    m_Spread.Configure(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

} // namespace Weapon
