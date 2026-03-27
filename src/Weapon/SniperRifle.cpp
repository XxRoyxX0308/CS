#include "Weapon/SniperRifle.hpp"

namespace Weapon {

void SniperRifle::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/guns/sks/scene.gltf";
    m_WeaponScale    = glm::vec3(0.15f);
    m_WeaponOffset   = glm::vec3(0.25f, -0.2f, 0.5f);

    m_FireRate       = 4.0f;    // semi-auto, ~240 RPM
    m_RecoilStrength = 2.5f;    // high recoil per shot
    m_RecoilRecovery = 6.0f;
    m_MagSize        = 20;
    m_ReloadTime     = 2.2f;
    m_BulletRange    = 250.0f;
}

} // namespace Weapon
