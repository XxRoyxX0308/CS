#ifndef CONFIG_H
#define CONFIG_H

#include "Util/Logger.hpp"
#include "pch.hpp" // IWYU pragma: export

namespace Core {
class Context;
}

// clang-format off
/**
    * To use, place a config.json file either in the current directory (.) or
    * the parent directory (..). If both directories contain the file, the one
    * in the current directory will be read.
    *
    * Example directory structure:
    *
    * @code{.md}
    * ├── build/
    * │   ├── YOUR_GAME
    * │   └── config.json <- place here
    * └── config.json <- or here
    * @endcode
    *
    * The config.json file can contain any number of configurations, each in
    * the following format:
    *
    * @code{.json}
    * {
    *   "title": "<string, default is 'Practice-Tools-for-Simple-Design'>",
    *   "window_pos_x": <int, default is SDL_WINDOWPOS_UNDEFINED (wherever the OS places the window)>,
    *   "window_pos_y": <int, default is SDL_WINDOWPOS_UNDEFINED (wherever the OS places the window)>,
    *   "window_width": <int, default is 1280>,
    *   "window_height": <int, default is 720>,
    *   "default_log_level": <int in range [0, 5], default is 2 (Util::Logger::Level::INFO)>,
    *   "fps_cap": <int, default is 60>
    * }
    * @endcode
    *
    * If a key is not present in the config.json, the default value will be
    * used.
    */
// clang-format on
struct PTSD_Config {
public:
    static std::string TITLE;

    static int WINDOW_POS_X;

    static int WINDOW_POS_Y;
    static unsigned int WINDOW_WIDTH;

    static unsigned int WINDOW_HEIGHT;
    static Util::Logger::Level DEFAULT_LOG_LEVEL;

    /**
     * @brief FPS limit
     *
     * Set value to 0 to turn off FPS cap
     */
    static unsigned int FPS_CAP;

    // ====== 3D Configuration ======

    /** @brief Default camera field of view in degrees */
    static float DEFAULT_FOV;

    /** @brief Near clip plane for 3D perspective projection */
    static float NEAR_CLIP_3D;

    /** @brief Far clip plane for 3D perspective projection */
    static float FAR_CLIP_3D;

    /** @brief Shadow map texture resolution (square) */
    static int SHADOW_MAP_SIZE;

    /** @brief Enable PBR rendering pipeline */
    static bool ENABLE_PBR;

    /** @brief Enable shadow mapping */
    static bool ENABLE_SHADOWS;

    /** @brief Maximum number of point lights */
    static int MAX_POINT_LIGHTS;

    /** @brief Maximum number of spot lights */
    static int MAX_SPOT_LIGHTS;

private:
    friend class Core::Context;
    static void Init();
}; // namespace PTSD_Config

#endif
