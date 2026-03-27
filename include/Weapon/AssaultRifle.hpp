#ifndef CS_WEAPON_ASSAULT_RIFLE_HPP
#define CS_WEAPON_ASSAULT_RIFLE_HPP

#include "Weapon/Weapon.hpp"

namespace Weapon {

/**
 * @brief Assault Rifle — suppressed automatic rifle.
 *
 * High fire rate, moderate recoil, 30-round magazine.
 */
class AssaultRifle : public Weapon {
protected:
    void Configure() override;
};

} // namespace Weapon

#endif // CS_WEAPON_ASSAULT_RIFLE_HPP
