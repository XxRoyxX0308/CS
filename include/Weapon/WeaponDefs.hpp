#ifndef CS_WEAPON_WEAPONDEFS_HPP
#define CS_WEAPON_WEAPONDEFS_HPP

#include "Weapon/Weapon.hpp"

// ── Knives ──
#include "Weapon/Knives/CombatKnife.hpp"
#include "Weapon/Knives/VictorinoxKnife.hpp"
// ── Pistols ──
#include "Weapon/Pistols/M1895.hpp"
#include "Weapon/Pistols/SigScorpion.hpp"
// ── Assault Rifles ──
#include "Weapon/Rifles/AXL47.hpp"
#include "Weapon/Rifles/HoneyBadger.hpp"
// ── SMGs ──
#include "Weapon/SMGs/MP5K.hpp"
#include "Weapon/SMGs/UZI.hpp"
// ── Snipers ──
#include "Weapon/Snipers/Axis2.hpp"
#include "Weapon/Snipers/ar30.hpp"
// ── Shotguns ──
#include "Weapon/Shotguns/MP153.hpp"
#include "Weapon/Shotguns/DoubleDeuce.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Weapon {

/**
 * @brief Weapon categories for the buy menu.
 */
enum class WeaponCategory {
    Knife,
    Pistol,
    Rifle,
    SMG,
    Sniper,
    Shotgun
};

/**
 * @brief Static info about a purchasable weapon for the buy menu.
 */
struct WeaponInfo {
    std::string name;
    WeaponCategory category;
    int price;
    std::string modelPath;         ///< GLTF model path (for third-person sync)
    glm::vec3 modelScale;          ///< Model scale (for third-person sync)
    std::function<std::unique_ptr<Weapon>()> factory;
};

/**
 * @brief Get the display name for a weapon category.
 */
inline const char* GetCategoryName(WeaponCategory cat) {
    switch (cat) {
        case WeaponCategory::Knife:   return "Knife";
        case WeaponCategory::Pistol:  return "Pistol";
        case WeaponCategory::Rifle:   return "Rifle";
        case WeaponCategory::SMG:     return "SMG";
        case WeaponCategory::Sniper:  return "Sniper";
        case WeaponCategory::Shotgun: return "Shotgun";
        default:                      return "Unknown";
    }
}

/**
 * @brief Build a WeaponInfo by reading modelPath and modelScale directly
 *        from the weapon's Configure(), ensuring a single source of truth.
 */
template <typename T>
inline WeaponInfo MakeWeaponInfo(std::string name, WeaponCategory category, int price) {
    T tmp;
    tmp.ApplyConfig();
    return WeaponInfo{
        std::move(name),
        category,
        price,
        tmp.GetModelPath(),
        tmp.GetWeaponScale(),
        []() { return std::make_unique<T>(); }
    };
}

/**
 * @brief Get the full list of purchasable weapons.
 */
inline const std::vector<WeaponInfo>& GetWeaponRegistry() {
    static const std::vector<WeaponInfo> registry = {
        // ── Knives ──
        MakeWeaponInfo<CombatKnife>    ("Combat Knife",    WeaponCategory::Knife,   200),
        MakeWeaponInfo<VictorinoxKnife>("Victorinox Knife",WeaponCategory::Knife,     0),
        // ── Pistols ──
        MakeWeaponInfo<M1895>          ("M1895 Revolver",  WeaponCategory::Pistol,  500),
        MakeWeaponInfo<SigScorpion>    ("SIG Scorpion",    WeaponCategory::Pistol,  700),
        // ── Assault Rifles ──
        MakeWeaponInfo<AXL47>          ("AXL-47",          WeaponCategory::Rifle,  2500),
        MakeWeaponInfo<HoneyBadger>    ("Honey Badger",    WeaponCategory::Rifle,  2700),
        // ── SMGs ──
        MakeWeaponInfo<MP5K>           ("MP5K",            WeaponCategory::SMG,    1500),
        MakeWeaponInfo<UZI>            ("UZI",             WeaponCategory::SMG,    1700),
        // ── Snipers ──
        MakeWeaponInfo<Axis2>          ("Axis-2",          WeaponCategory::Sniper, 4750),
        MakeWeaponInfo<AR30>           ("AR30",            WeaponCategory::Sniper, 5000),
        // ── Shotguns ──
        MakeWeaponInfo<MP153>          ("MP-153",          WeaponCategory::Shotgun,1800),
        MakeWeaponInfo<DoubleDeuce>    ("Double Deuce",    WeaponCategory::Shotgun,1200),
    };
    return registry;
}

/**
 * @brief All weapon categories in display order.
 */
inline const std::vector<WeaponCategory>& GetAllCategories() {
    static const std::vector<WeaponCategory> cats = {
        WeaponCategory::Knife,
        WeaponCategory::Pistol,
        WeaponCategory::Rifle,
        WeaponCategory::SMG,
        WeaponCategory::Sniper,
        WeaponCategory::Shotgun
    };
    return cats;
}

} // namespace Weapon

#endif // CS_WEAPON_WEAPONDEFS_HPP
