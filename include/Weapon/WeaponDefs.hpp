#ifndef CS_WEAPON_WEAPONDEFS_HPP
#define CS_WEAPON_WEAPONDEFS_HPP

#include "Weapon/Weapon.hpp"

// ── Blades ──
#include "Weapon/Blades/CombatKnife.hpp"
#include "Weapon/Blades/VictorinoxKnife.hpp"
// ── Pistols ──
#include "Weapon/Pistols/M1895.hpp"
#include "Weapon/Pistols/SigScorpion.hpp"
// ── Assault Rifles ──
#include "Weapon/Assaults/AXL47.hpp"
#include "Weapon/Assaults/HoneyBadger.hpp"
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
 * @brief Get the full list of purchasable weapons.
 */
inline const std::vector<WeaponInfo>& GetWeaponRegistry() {
    static const std::string base = std::string(ASSETS_DIR) + "/weapons/";
    static const std::vector<WeaponInfo> registry = {
        // ── Blades ──
        { "Combat Knife",      WeaponCategory::Knife,   200,
          base + "Blades/combat_knife/scene.gltf",                glm::vec3(0.15f),
          []() { return std::make_unique<CombatKnife>(); } },
        { "Victorinox Knife",  WeaponCategory::Knife,   0,
          base + "Blades/victorinox_multitool_knife/scene.gltf",  glm::vec3(0.12f),
          []() { return std::make_unique<VictorinoxKnife>(); } },

        // ── Pistols ──
        { "M1895 Revolver",    WeaponCategory::Pistol,  500,
          base + "Pistols/m1895/scene.gltf",                      glm::vec3(0.12f),
          []() { return std::make_unique<M1895>(); } },
        { "SIG Scorpion",      WeaponCategory::Pistol,  700,
          base + "Pistols/sig_scorpion/scene.gltf",               glm::vec3(0.12f),
          []() { return std::make_unique<SigScorpion>(); } },

        // ── Assault Rifles ──
        { "AXL-47",            WeaponCategory::Rifle,   2500,
          base + "Assaults/axl-47/scene.gltf",                    glm::vec3(0.025f),
          []() { return std::make_unique<AXL47>(); } },
        { "Honey Badger",      WeaponCategory::Rifle,   2700,
          base + "Assaults/honey_badger/scene.gltf",              glm::vec3(0.025f),
          []() { return std::make_unique<HoneyBadger>(); } },

        // ── SMGs ──
        { "MP5K",              WeaponCategory::SMG,     1500,
          base + "SMGs/mp5k/scene.gltf",                          glm::vec3(0.08f),
          []() { return std::make_unique<MP5K>(); } },
        { "UZI",              WeaponCategory::SMG,     1700,
          base + "SMGs/uzi/scene.gltf",                          glm::vec3(0.08f),
          []() { return std::make_unique<UZI>(); } },

        // ── Snipers ──
        { "Axis-2",            WeaponCategory::Sniper,  4750,
          base + "Snipers/axis_2/scene.gltf",                     glm::vec3(0.15f),
          []() { return std::make_unique<Axis2>(); } },
        { "AR30",            WeaponCategory::Sniper,  5000,
          base + "Snipers/ar30/scene.gltf",                     glm::vec3(0.15f),
          []() { return std::make_unique<AR30>(); } },

        // ── Shotguns ──
        { "MP-153",            WeaponCategory::Shotgun, 1800,
          base + "Shotguns/mp153/scene.gltf",                     glm::vec3(0.10f),
          []() { return std::make_unique<MP153>(); } },
        { "Double Deuce",      WeaponCategory::Shotgun, 1200,
          base + "Shotguns/stolzer__son_double_deuce/scene.gltf", glm::vec3(0.10f),
          []() { return std::make_unique<DoubleDeuce>(); } },
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
