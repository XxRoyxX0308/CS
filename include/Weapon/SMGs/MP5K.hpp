#ifndef CS_WEAPON_SMGS_MP5K_HPP
#define CS_WEAPON_SMGS_MP5K_HPP

#include "Weapon/Weapon.hpp"

namespace Weapon {

class MP5K : public Weapon {
protected:
    void Configure() override;
};

} // namespace Weapon

#endif // CS_WEAPON_SMGS_MP5K_HPP
