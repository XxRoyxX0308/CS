#ifndef CS_GUN_SILICONE_GUN_HPP
#define CS_GUN_SILICONE_GUN_HPP

#include "Gun/Gun.hpp"

namespace Gun {

/**
 * @brief Silicone Gun — sidearm / pistol.
 *
 * Medium fire rate, low recoil, 12-round magazine, fast reload.
 */
class SiliconeGun : public Gun {
protected:
    void Configure() override;
};

} // namespace Gun

#endif // CS_GUN_SILICONE_GUN_HPP
