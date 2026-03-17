#ifndef CS_GUN_SKS_HPP
#define CS_GUN_SKS_HPP

#include "Gun/Gun.hpp"

namespace Gun {

/**
 * @brief SKS — semi-automatic rifle.
 *
 * Lower fire rate, higher recoil per shot, 20-round magazine.
 */
class SKS : public Gun {
protected:
    void Configure() override;
};

} // namespace Gun

#endif // CS_GUN_SKS_HPP
