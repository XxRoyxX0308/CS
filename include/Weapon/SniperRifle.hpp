#ifndef CS_WEAPON_SNIPER_RIFLE_HPP
#define CS_WEAPON_SNIPER_RIFLE_HPP

#include "Weapon/Weapon.hpp"

namespace Weapon {

/**
 * @brief Sniper Rifle — semi-automatic marksman rifle.
 *
 * Lower fire rate, higher recoil per shot, 20-round magazine.
 */
class SniperRifle : public Weapon {
protected:
    void Configure() override;
};

} // namespace Weapon

#endif // CS_WEAPON_SNIPER_RIFLE_HPP
