#ifndef CS_APP_AUDIOMANAGER_HPP
#define CS_APP_AUDIOMANAGER_HPP

#include <SDL_mixer.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace App {

class AudioManager {
public:
    enum class SoundType : uint8_t {
        Footstep = 0,
        Gunshot,
        Death,
        Count,
    };

    void Initialize();
    void Reset();

    void UpdateListener(const glm::vec3& position,
                        const glm::vec3& forward,
                        const glm::vec3& right);

    void UpdateLoopEmitter(uint32_t emitterId,
                           SoundType sound,
                           const glm::vec3& position,
                           bool shouldPlay);

    void StopLoopEmitter(uint32_t emitterId);
    void StopAllLoopEmittersExcept(const std::vector<uint32_t>& activeEmitterIds);
    void PlayOneShot(SoundType sound, const glm::vec3& position);

private:
    struct SoundConfig {
        const char* fileName;
        int baseVolume;
        float maxDistance;
    };

    struct LoopEmitterState {
        int channel = -1;
        SoundType sound = SoundType::Footstep;
        glm::vec3 position{0.0f};
    };

    static constexpr size_t kSoundTypeCount = static_cast<size_t>(SoundType::Count);

    static size_t ToIndex(SoundType sound);

    const SoundConfig& GetConfig(SoundType sound) const;
    std::shared_ptr<Mix_Chunk> GetChunk(SoundType sound) const;
    void ApplyMixToChannel(int channel, SoundType sound, const glm::vec3& position) const;
    bool ComputeMix(SoundType sound,
                    const glm::vec3& position,
                    int& outVolume,
                    Uint8& outLeft,
                    Uint8& outRight) const;

    bool m_Initialized = false;
    glm::vec3 m_ListenerPosition{0.0f};
    glm::vec3 m_ListenerForward{0.0f, 0.0f, -1.0f};
    glm::vec3 m_ListenerRight{1.0f, 0.0f, 0.0f};
    std::array<std::shared_ptr<Mix_Chunk>, kSoundTypeCount> m_Chunks{};
    std::unordered_map<uint32_t, LoopEmitterState> m_LoopEmitters;
};

} // namespace App

#endif // CS_APP_AUDIOMANAGER_HPP