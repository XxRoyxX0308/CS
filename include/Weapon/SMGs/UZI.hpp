#ifndef CS_WEAPON_SMGS_UZI_HPP
#define CS_WEAPON_SMGS_UZI_HPP

#include "Weapon/Weapon.hpp"

namespace Weapon {

class UZI : public Weapon {
protected:
    void Configure() override;
};

} // namespace Weapon

#endif // CS_WEAPON_SMGS_UZI_HPP
