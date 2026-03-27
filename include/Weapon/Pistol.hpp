#ifndef CS_WEAPON_PISTOL_HPP
#define CS_WEAPON_PISTOL_HPP

#include "Weapon/Weapon.hpp"

namespace Weapon {

/**
 * @brief Pistol — sidearm handgun.
 *
 * Medium fire rate, low recoil, 12-round magazine, fast reload.
 */
class Pistol : public Weapon {
protected:
    void Configure() override;
};

} // namespace Weapon

#endif // CS_WEAPON_PISTOL_HPP
