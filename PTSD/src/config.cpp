#include "config.hpp"
#include "SDL_image.h"

#include <fstream>

std::string PTSD_Config::TITLE = "Practice-Tools-for-Simple-Design";

int PTSD_Config::WINDOW_POS_X = SDL_WINDOWPOS_UNDEFINED;
int PTSD_Config::WINDOW_POS_Y = SDL_WINDOWPOS_UNDEFINED;

unsigned int PTSD_Config::WINDOW_WIDTH = 1280;
unsigned int PTSD_Config::WINDOW_HEIGHT = 720;

Util::Logger::Level PTSD_Config::DEFAULT_LOG_LEVEL = Util::Logger::Level::INFO;

unsigned int PTSD_Config::FPS_CAP = 60;

// 3D defaults
float PTSD_Config::DEFAULT_FOV = 45.0f;
float PTSD_Config::NEAR_CLIP_3D = 0.1f;
float PTSD_Config::FAR_CLIP_3D = 1000.0f;
int PTSD_Config::SHADOW_MAP_SIZE = 2048;
bool PTSD_Config::ENABLE_PBR = true;
bool PTSD_Config::ENABLE_SHADOWS = true;
int PTSD_Config::MAX_POINT_LIGHTS = 16;
int PTSD_Config::MAX_SPOT_LIGHTS = 8;

template <typename T>
inline static void AssignValueFromConfigFile(const nlohmann::json &j,
                                             const std::string_view key,
                                             T &value) {
    if (j.contains(key.data())) {
        value = j[key.data()].get<T>();
    }
}

void PTSD_Config::Init() {
    nlohmann::json j;
    std::ifstream file;
    if (std::filesystem::exists("config.json")) {
        file = std::ifstream("config.json");
    } else if (std::filesystem::exists("../config.json")) {
        file = std::ifstream("../config.json");
    } else {
        LOG_WARN("config.json not found, using default configurations.");
        return;
    }
    file >> j;
    file.close();
    AssignValueFromConfigFile(j, "title", TITLE);
    AssignValueFromConfigFile(j, "window_pos_x", WINDOW_POS_X);
    AssignValueFromConfigFile(j, "window_pos_y", WINDOW_POS_Y);
    AssignValueFromConfigFile(j, "window_width", WINDOW_WIDTH);
    AssignValueFromConfigFile(j, "window_height", WINDOW_HEIGHT);
    AssignValueFromConfigFile(j, "default_log_level", DEFAULT_LOG_LEVEL);
    AssignValueFromConfigFile(j, "fps_cap", FPS_CAP);
    
    // 3D config
    AssignValueFromConfigFile(j, "default_fov", DEFAULT_FOV);
    AssignValueFromConfigFile(j, "near_clip_3d", NEAR_CLIP_3D);
    AssignValueFromConfigFile(j, "far_clip_3d", FAR_CLIP_3D);
    AssignValueFromConfigFile(j, "shadow_map_size", SHADOW_MAP_SIZE);
    AssignValueFromConfigFile(j, "enable_pbr", ENABLE_PBR);
    AssignValueFromConfigFile(j, "enable_shadows", ENABLE_SHADOWS);
    AssignValueFromConfigFile(j, "max_point_lights", MAX_POINT_LIGHTS);
    AssignValueFromConfigFile(j, "max_spot_lights", MAX_SPOT_LIGHTS);
}
