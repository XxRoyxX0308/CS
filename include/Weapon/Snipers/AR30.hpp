#ifndef CS_WEAPON_SNIPERS_AR30_HPP
#define CS_WEAPON_SNIPERS_AR30_HPP

#include "Weapon/Weapon.hpp"

namespace Weapon {

class AR30 : public Weapon {
protected:
    void Configure() override;
};

} // namespace Weapon

#endif // CS_WEAPON_SNIPERS_AR30_HPP
