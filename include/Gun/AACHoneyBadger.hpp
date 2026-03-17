#ifndef CS_GUN_AAC_HONEY_BADGER_HPP
#define CS_GUN_AAC_HONEY_BADGER_HPP

#include "Gun/Gun.hpp"

namespace Gun {

/**
 * @brief AAC Honey Badger — suppressed assault rifle.
 *
 * High fire rate, moderate recoil, 30-round magazine.
 */
class AACHoneyBadger : public Gun {
protected:
    void Configure() override;
};

} // namespace Gun

#endif // CS_GUN_AAC_HONEY_BADGER_HPP
