#include "App/AudioManager.hpp"

#include "Util/Logger.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

std::shared_ptr<Mix_Chunk> LoadChunk(const std::string& filePath) {
    auto chunk = std::shared_ptr<Mix_Chunk>(Mix_LoadWAV(filePath.c_str()), Mix_FreeChunk);
    if (!chunk) {
        LOG_ERROR("Failed to load audio '{}': {}", filePath, Mix_GetError());
    }
    return chunk;
}

} // namespace

namespace App {

size_t AudioManager::ToIndex(SoundType sound) {
    return static_cast<size_t>(sound);
}

const AudioManager::SoundConfig& AudioManager::GetConfig(SoundType sound) const {
    static constexpr std::array<SoundConfig, kSoundTypeCount> kConfigs{{
        {"walking.mp3", 88, 18.0f},
        {"gunshot.mp3", 120, 60.0f},
        {"death.mp3", 112, 28.0f},
    }};

    return kConfigs[ToIndex(sound)];
}

std::shared_ptr<Mix_Chunk> AudioManager::GetChunk(SoundType sound) const {
    return m_Chunks[ToIndex(sound)];
}

void AudioManager::Initialize() {
    if (m_Initialized) {
        return;
    }

    for (size_t i = 0; i < kSoundTypeCount; ++i) {
        const SoundType sound = static_cast<SoundType>(i);
        const auto& config = GetConfig(sound);
        const std::string path = std::string(ASSETS_DIR) + "/sounds/" + config.fileName;
        m_Chunks[i] = LoadChunk(path);
    }

    m_Initialized = true;
}

void AudioManager::Reset() {
    for (const auto& [emitterId, state] : m_LoopEmitters) {
        (void)emitterId;
        if (state.channel >= 0) {
            Mix_HaltChannel(state.channel);
        }
    }
    m_LoopEmitters.clear();
}

void AudioManager::UpdateListener(const glm::vec3& position,
                                  const glm::vec3& forward,
                                  const glm::vec3& right) {
    m_ListenerPosition = position;

    glm::vec3 flatForward(forward.x, 0.0f, forward.z);
    if (glm::length2(flatForward) > 0.0001f) {
        m_ListenerForward = glm::normalize(flatForward);
    }

    glm::vec3 flatRight(right.x, 0.0f, right.z);
    if (glm::length2(flatRight) > 0.0001f) {
        m_ListenerRight = glm::normalize(flatRight);
    }
}

bool AudioManager::ComputeMix(SoundType sound,
                              const glm::vec3& position,
                              int& outVolume,
                              Uint8& outLeft,
                              Uint8& outRight) const {
    const auto& config = GetConfig(sound);
    const glm::vec3 offset = position - m_ListenerPosition;
    const float distance = glm::length(offset);
    if (distance >= config.maxDistance) {
        return false;
    }

    const float attenuation = 1.0f - (distance / config.maxDistance);
    outVolume = std::clamp(static_cast<int>(std::lround(config.baseVolume * attenuation)), 0, MIX_MAX_VOLUME);
    if (outVolume <= 0) {
        return false;
    }

    glm::vec3 horizontalOffset(offset.x, 0.0f, offset.z);
    float pan = 0.0f;
    if (glm::length2(horizontalOffset) > 0.0001f) {
        pan = std::clamp(glm::dot(glm::normalize(horizontalOffset), m_ListenerRight), -1.0f, 1.0f);
    }

    const float leftScale = std::clamp(1.0f - pan, 0.0f, 1.0f);
    const float rightScale = std::clamp(1.0f + pan, 0.0f, 1.0f);
    outLeft = static_cast<Uint8>(std::lround(255.0f * leftScale));
    outRight = static_cast<Uint8>(std::lround(255.0f * rightScale));
    return true;
}

void AudioManager::ApplyMixToChannel(int channel,
                                     SoundType sound,
                                     const glm::vec3& position) const {
    int volume = 0;
    Uint8 left = 255;
    Uint8 right = 255;
    if (!ComputeMix(sound, position, volume, left, right)) {
        Mix_HaltChannel(channel);
        return;
    }

    Mix_Volume(channel, volume);
    Mix_SetPanning(channel, left, right);
}

void AudioManager::UpdateLoopEmitter(uint32_t emitterId,
                                     SoundType sound,
                                     const glm::vec3& position,
                                     bool shouldPlay) {
    if (!m_Initialized) {
        Initialize();
    }

    auto chunk = GetChunk(sound);
    if (!chunk) {
        return;
    }

    if (!shouldPlay) {
        StopLoopEmitter(emitterId);
        return;
    }

    int volume = 0;
    Uint8 left = 255;
    Uint8 right = 255;
    if (!ComputeMix(sound, position, volume, left, right)) {
        StopLoopEmitter(emitterId);
        return;
    }

    auto& emitter = m_LoopEmitters[emitterId];
    emitter.sound = sound;
    emitter.position = position;

    if (emitter.channel < 0 || !Mix_Playing(emitter.channel)) {
        emitter.channel = Mix_PlayChannel(-1, chunk.get(), -1);
        if (emitter.channel < 0) {
            LOG_WARN("Failed to start looped sound '{}': {}", GetConfig(sound).fileName, Mix_GetError());
            m_LoopEmitters.erase(emitterId);
            return;
        }
    }

    Mix_Volume(emitter.channel, volume);
    Mix_SetPanning(emitter.channel, left, right);
}

void AudioManager::StopLoopEmitter(uint32_t emitterId) {
    auto it = m_LoopEmitters.find(emitterId);
    if (it == m_LoopEmitters.end()) {
        return;
    }

    if (it->second.channel >= 0) {
        Mix_HaltChannel(it->second.channel);
    }
    m_LoopEmitters.erase(it);
}

void AudioManager::StopAllLoopEmittersExcept(const std::vector<uint32_t>& activeEmitterIds) {
    for (auto it = m_LoopEmitters.begin(); it != m_LoopEmitters.end();) {
        if (std::find(activeEmitterIds.begin(), activeEmitterIds.end(), it->first) != activeEmitterIds.end()) {
            ++it;
            continue;
        }

        if (it->second.channel >= 0) {
            Mix_HaltChannel(it->second.channel);
        }
        it = m_LoopEmitters.erase(it);
    }
}

void AudioManager::PlayOneShot(SoundType sound, const glm::vec3& position) {
    if (!m_Initialized) {
        Initialize();
    }

    auto chunk = GetChunk(sound);
    if (!chunk) {
        return;
    }

    int volume = 0;
    Uint8 left = 255;
    Uint8 right = 255;
    if (!ComputeMix(sound, position, volume, left, right)) {
        return;
    }

    const int channel = Mix_PlayChannel(-1, chunk.get(), 0);
    if (channel < 0) {
        LOG_WARN("Failed to play sound '{}': {}", GetConfig(sound).fileName, Mix_GetError());
        return;
    }

    Mix_Volume(channel, volume);
    Mix_SetPanning(channel, left, right);
}

} // namespace App