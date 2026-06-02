#ifndef CS_APP_VOICECHATMANAGER_HPP
#define CS_APP_VOICECHATMANAGER_HPP

#include <SDL.h>
#include <glm/glm.hpp>
#include <opus.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace App {

class VoiceChatManager {
public:
    using EncodedVoiceFrameCallback = std::function<void(uint32_t sequence, const std::vector<uint8_t>& encodedFrame)>;

    VoiceChatManager() = default;
    ~VoiceChatManager();

    void Initialize();
    void Reset();

    void SetLocalPlayerId(uint8_t playerId) { m_LocalPlayerId = playerId; }
    void SetPushToTalkEnabled(bool enabled);

    void UpdateListener(const glm::vec3& position,
                        const glm::vec3& forward,
                        const glm::vec3& right);

    void UpdateSpeakerPosition(uint8_t speakerId, const glm::vec3& position);
    void RemoveSpeaker(uint8_t speakerId);

    void Update(const EncodedVoiceFrameCallback& onEncodedFrame);
    void HandleIncomingFrame(uint8_t sourceId,
                             uint32_t sequence,
                             const std::vector<uint8_t>& encodedFrame);

private:
    struct SpeakerState {
        OpusDecoder* decoder = nullptr;
        std::deque<int16_t> pendingSamples;
        glm::vec3 position{0.0f};
        uint32_t lastSequence = 0;
        bool hasSequence = false;
    };

    static constexpr int kSampleRate = 48000;
    static constexpr int kCaptureChannels = 1;
    static constexpr int kPlaybackChannels = 2;
    static constexpr int kFrameDurationMs = 20;
    static constexpr int kFrameSamples = (kSampleRate * kFrameDurationMs) / 1000;
    static constexpr int kMaxEncodedBytes = 512;
    static constexpr float kMaxVoiceDistance = 35.0f;

    static void PlaybackCallback(void* userdata, Uint8* stream, int len);

    void MixPlayback(Uint8* stream, int len);
    SpeakerState& GetOrCreateSpeakerState(uint8_t speakerId);
    void ClearCaptureBacklog();
    void DestroySpeakerStates();

    bool m_Initialized = false;
    bool m_PushToTalkEnabled = false;
    uint8_t m_LocalPlayerId = 0xFF;
    uint32_t m_NextSequence = 0;

    SDL_AudioDeviceID m_CaptureDevice = 0;
    SDL_AudioDeviceID m_PlaybackDevice = 0;
    SDL_AudioSpec m_CaptureSpec{};
    SDL_AudioSpec m_PlaybackSpec{};

    OpusEncoder* m_Encoder = nullptr;

    glm::vec3 m_ListenerPosition{0.0f};
    glm::vec3 m_ListenerForward{0.0f, 0.0f, -1.0f};
    glm::vec3 m_ListenerRight{1.0f, 0.0f, 0.0f};

    std::deque<int16_t> m_CaptureSamples;
    std::unordered_map<uint8_t, SpeakerState> m_Speakers;
    std::mutex m_AudioMutex;
};

} // namespace App

#endif // CS_APP_VOICECHATMANAGER_HPP