#include "Gun/AACHoneyBadger.hpp"

namespace Gun {

void AACHoneyBadger::Configure() {
    m_ModelPath      = std::string(ASSETS_DIR) + "/guns/aac_honey_badger/scene.gltf";
    m_GunScale       = glm::vec3(0.15f);
    m_GunOffset      = glm::vec3(0.25f, -0.2f, 0.5f);

    m_FireRate       = 12.0f;   // 800 RPM equivalent
    m_RecoilStrength = 1.2f;    // moderate recoil
    m_RecoilRecovery = 8.0f;
    m_MagSize        = 30;
    m_ReloadTime     = 1.8f;
    m_BulletRange    = 200.0f;
}

} // namespace Gun
