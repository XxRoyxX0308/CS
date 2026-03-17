#include "Gun/SiliconeGun.hpp"

namespace Gun {

void SiliconeGun::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/guns/silicone_gun/scene.gltf";
    m_GunScale       = glm::vec3(0.12f);
    m_GunOffset      = glm::vec3(0.2f, -0.18f, 0.4f);

    m_FireRate       = 6.0f;    // semi-auto pistol
    m_RecoilStrength = 0.8f;    // low recoil
    m_RecoilRecovery = 10.0f;
    m_MagSize        = 12;
    m_ReloadTime     = 1.2f;
    m_BulletRange    = 100.0f;
}

} // namespace Gun
