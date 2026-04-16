#ifndef CS_WEAPON_KNIVES_COMBATKNIFE_HPP
#define CS_WEAPON_KNIVES_COMBATKNIFE_HPP

#include "Weapon/Weapon.hpp"

namespace Weapon {

class CombatKnife : public Weapon {
protected:
    void Configure() override;
};

} // namespace Weapon

#endif // CS_WEAPON_KNIVES_COMBATKNIFE_HPP
