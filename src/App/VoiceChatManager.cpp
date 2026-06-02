#include "App/VoiceChatManager.hpp"

#include "Util/Logger.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace App {

namespace {

int16_t ClampSample(int value) {
    return static_cast<int16_t>(std::clamp(value, -32768, 32767));
}

} // namespace

VoiceChatManager::~VoiceChatManager() {
    Reset();
}

void VoiceChatManager::Initialize() {
    if (m_Initialized) {
        return;
    }

    SDL_AudioSpec desiredCapture{};
    desiredCapture.freq = kSampleRate;
    desiredCapture.format = AUDIO_S16SYS;
    desiredCapture.channels = kCaptureChannels;
    desiredCapture.samples = static_cast<Uint16>(kFrameSamples);
    desiredCapture.callback = nullptr;

    m_CaptureDevice = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &desiredCapture, &m_CaptureSpec, 0);
    if (m_CaptureDevice == 0) {
        LOG_ERROR("Failed to open voice capture device: {}", SDL_GetError());
        Reset();
        return;
    }

    SDL_AudioSpec desiredPlayback{};
    desiredPlayback.freq = kSampleRate;
    desiredPlayback.format = AUDIO_S16SYS;
    desiredPlayback.channels = kPlaybackChannels;
    desiredPlayback.samples = static_cast<Uint16>(kFrameSamples);
    desiredPlayback.callback = &VoiceChatManager::PlaybackCallback;
    desiredPlayback.userdata = this;

    m_PlaybackDevice = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &desiredPlayback, &m_PlaybackSpec, 0);
    if (m_PlaybackDevice == 0) {
        LOG_ERROR("Failed to open voice playback device: {}", SDL_GetError());
        Reset();
        return;
    }

    int opusError = OPUS_OK;
    m_Encoder = opus_encoder_create(kSampleRate, kCaptureChannels, OPUS_APPLICATION_VOIP, &opusError);
    if (opusError != OPUS_OK || m_Encoder == nullptr) {
        LOG_ERROR("Failed to create Opus encoder: {}", opus_strerror(opusError));
        Reset();
        return;
    }

    opus_encoder_ctl(m_Encoder, OPUS_SET_BITRATE(24000));
    opus_encoder_ctl(m_Encoder, OPUS_SET_COMPLEXITY(5));
    opus_encoder_ctl(m_Encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(m_Encoder, OPUS_SET_INBAND_FEC(1));
    opus_encoder_ctl(m_Encoder, OPUS_SET_PACKET_LOSS_PERC(5));

    SDL_PauseAudioDevice(m_PlaybackDevice, 0);
    SDL_PauseAudioDevice(m_CaptureDevice, 0);

    m_Initialized = true;
}

void VoiceChatManager::Reset() {
    {
        std::lock_guard<std::mutex> lock(m_AudioMutex);
        DestroySpeakerStates();
        m_CaptureSamples.clear();
    }

    if (m_Encoder) {
        opus_encoder_destroy(m_Encoder);
        m_Encoder = nullptr;
    }

    if (m_CaptureDevice != 0) {
        SDL_ClearQueuedAudio(m_CaptureDevice);
        SDL_CloseAudioDevice(m_CaptureDevice);
        m_CaptureDevice = 0;
    }

    if (m_PlaybackDevice != 0) {
        SDL_CloseAudioDevice(m_PlaybackDevice);
        m_PlaybackDevice = 0;
    }

    m_PushToTalkEnabled = false;
    m_NextSequence = 0;
    m_Initialized = false;
}

void VoiceChatManager::SetPushToTalkEnabled(bool enabled) {
    if (m_PushToTalkEnabled == enabled) {
        return;
    }

    m_PushToTalkEnabled = enabled;
    if (!m_PushToTalkEnabled) {
        ClearCaptureBacklog();
    }
}

void VoiceChatManager::UpdateListener(const glm::vec3& position,
                                      const glm::vec3& forward,
                                      const glm::vec3& right) {
    std::lock_guard<std::mutex> lock(m_AudioMutex);
    m_ListenerPosition = position;

    glm::vec3 flatForward(forward.x, 0.0f, forward.z);
    if (glm::length(flatForward) > 0.0001f) {
        m_ListenerForward = glm::normalize(flatForward);
    }

    glm::vec3 flatRight(right.x, 0.0f, right.z);
    if (glm::length(flatRight) > 0.0001f) {
        m_ListenerRight = glm::normalize(flatRight);
    }
}

void VoiceChatManager::UpdateSpeakerPosition(uint8_t speakerId, const glm::vec3& position) {
    if (speakerId == m_LocalPlayerId) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_AudioMutex);
    auto& speaker = GetOrCreateSpeakerState(speakerId);
    speaker.position = position;
}

void VoiceChatManager::RemoveSpeaker(uint8_t speakerId) {
    std::lock_guard<std::mutex> lock(m_AudioMutex);
    auto it = m_Speakers.find(speakerId);
    if (it == m_Speakers.end()) {
        return;
    }

    if (it->second.decoder) {
        opus_decoder_destroy(it->second.decoder);
    }
    m_Speakers.erase(it);
}

void VoiceChatManager::Update(const EncodedVoiceFrameCallback& onEncodedFrame) {
    if (!m_Initialized || m_CaptureDevice == 0 || m_Encoder == nullptr) {
        return;
    }

    if (!m_PushToTalkEnabled) {
        ClearCaptureBacklog();
        return;
    }

    const Uint32 queuedBytes = SDL_GetQueuedAudioSize(m_CaptureDevice);
    if (queuedBytes == 0) {
        return;
    }

    std::vector<int16_t> capturedSamples(queuedBytes / sizeof(int16_t));
    const Uint32 bytesRead = SDL_DequeueAudio(m_CaptureDevice, capturedSamples.data(), queuedBytes);
    capturedSamples.resize(bytesRead / sizeof(int16_t));

    {
        std::lock_guard<std::mutex> lock(m_AudioMutex);
        for (int16_t sample : capturedSamples) {
            m_CaptureSamples.push_back(sample);
        }
    }

    while (true) {
        std::array<opus_int16, kFrameSamples> frameSamples{};
        {
            std::lock_guard<std::mutex> lock(m_AudioMutex);
            if (m_CaptureSamples.size() < static_cast<size_t>(kFrameSamples)) {
                break;
            }

            for (int i = 0; i < kFrameSamples; ++i) {
                frameSamples[static_cast<size_t>(i)] = m_CaptureSamples.front();
                m_CaptureSamples.pop_front();
            }
        }

        int maxAmplitude = 0;
        for (opus_int16 sample : frameSamples) {
            maxAmplitude = std::max(maxAmplitude, std::abs(static_cast<int>(sample)));
        }
        if (maxAmplitude < 250) {
            continue;
        }

        std::array<unsigned char, kMaxEncodedBytes> encodedBytes{};
        const int encodedSize = opus_encode(
            m_Encoder,
            frameSamples.data(),
            kFrameSamples,
            encodedBytes.data(),
            static_cast<opus_int32>(encodedBytes.size())
        );

        if (encodedSize <= 0) {
            LOG_WARN("Failed to encode voice frame: {}", opus_strerror(encodedSize));
            continue;
        }

        if (onEncodedFrame) {
            onEncodedFrame(++m_NextSequence, std::vector<uint8_t>(encodedBytes.begin(), encodedBytes.begin() + encodedSize));
        }
    }
}

void VoiceChatManager::HandleIncomingFrame(uint8_t sourceId,
                                           uint32_t sequence,
                                           const std::vector<uint8_t>& encodedFrame) {
    if (!m_Initialized || sourceId == m_LocalPlayerId || encodedFrame.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_AudioMutex);
    auto& speaker = GetOrCreateSpeakerState(sourceId);
    if (speaker.decoder == nullptr) {
        return;
    }

    if (speaker.hasSequence && sequence <= speaker.lastSequence) {
        return;
    }

    std::array<opus_int16, kFrameSamples> decodedSamples{};
    const int decodedCount = opus_decode(
        speaker.decoder,
        reinterpret_cast<const unsigned char*>(encodedFrame.data()),
        static_cast<opus_int32>(encodedFrame.size()),
        decodedSamples.data(),
        kFrameSamples,
        0
    );

    if (decodedCount < 0) {
        LOG_WARN("Failed to decode voice frame from {}: {}", sourceId, opus_strerror(decodedCount));
        return;
    }

    speaker.lastSequence = sequence;
    speaker.hasSequence = true;

    for (int i = 0; i < decodedCount; ++i) {
        speaker.pendingSamples.push_back(decodedSamples[static_cast<size_t>(i)]);
    }

    const size_t maxBufferedSamples = static_cast<size_t>(kFrameSamples * 10);
    while (speaker.pendingSamples.size() > maxBufferedSamples) {
        speaker.pendingSamples.pop_front();
    }
}

void VoiceChatManager::PlaybackCallback(void* userdata, Uint8* stream, int len) {
    auto* manager = static_cast<VoiceChatManager*>(userdata);
    if (!manager) {
        std::memset(stream, 0, static_cast<size_t>(len));
        return;
    }

    manager->MixPlayback(stream, len);
}

void VoiceChatManager::MixPlayback(Uint8* stream, int len) {
    std::memset(stream, 0, static_cast<size_t>(len));

    const int samplePairs = len / static_cast<int>(sizeof(int16_t) * kPlaybackChannels);
    auto* output = reinterpret_cast<int16_t*>(stream);

    std::lock_guard<std::mutex> lock(m_AudioMutex);

    for (auto& [speakerId, speaker] : m_Speakers) {
        (void)speakerId;
        if (speaker.pendingSamples.empty()) {
            continue;
        }

        const glm::vec3 delta = speaker.position - m_ListenerPosition;
        const float distance = glm::length(delta);
        const float attenuation = std::clamp(1.0f - (distance / kMaxVoiceDistance), 0.0f, 1.0f);
        if (attenuation <= 0.0f) {
            size_t skipSamples = std::min(static_cast<size_t>(samplePairs), speaker.pendingSamples.size());
            for (size_t i = 0; i < skipSamples; ++i) {
                speaker.pendingSamples.pop_front();
            }
            continue;
        }

        float pan = 0.0f;
        glm::vec3 horizontalDelta(delta.x, 0.0f, delta.z);
        if (glm::length(horizontalDelta) > 0.0001f) {
            pan = std::clamp(glm::dot(glm::normalize(horizontalDelta), m_ListenerRight), -1.0f, 1.0f);
        }

        const float leftGain = attenuation * std::clamp(1.0f - pan, 0.0f, 1.0f);
        const float rightGain = attenuation * std::clamp(1.0f + pan, 0.0f, 1.0f);

        for (int frameIndex = 0; frameIndex < samplePairs; ++frameIndex) {
            if (speaker.pendingSamples.empty()) {
                break;
            }

            const int monoSample = speaker.pendingSamples.front();
            speaker.pendingSamples.pop_front();

            const int outIndex = frameIndex * kPlaybackChannels;
            output[outIndex] = ClampSample(static_cast<int>(output[outIndex]) + static_cast<int>(std::lround(static_cast<float>(monoSample) * leftGain)));
            output[outIndex + 1] = ClampSample(static_cast<int>(output[outIndex + 1]) + static_cast<int>(std::lround(static_cast<float>(monoSample) * rightGain)));
        }
    }
}

VoiceChatManager::SpeakerState& VoiceChatManager::GetOrCreateSpeakerState(uint8_t speakerId) {
    auto it = m_Speakers.find(speakerId);
    if (it != m_Speakers.end()) {
        return it->second;
    }

    int opusError = OPUS_OK;
    SpeakerState state;
    state.decoder = opus_decoder_create(kSampleRate, kCaptureChannels, &opusError);
    if (opusError != OPUS_OK || state.decoder == nullptr) {
        LOG_ERROR("Failed to create Opus decoder for player {}: {}", speakerId, opus_strerror(opusError));
        return m_Speakers.emplace(speakerId, SpeakerState{}).first->second;
    }

    return m_Speakers.emplace(speakerId, std::move(state)).first->second;
}

void VoiceChatManager::ClearCaptureBacklog() {
    std::lock_guard<std::mutex> lock(m_AudioMutex);
    m_CaptureSamples.clear();
    if (m_CaptureDevice != 0) {
        SDL_ClearQueuedAudio(m_CaptureDevice);
    }
}

void VoiceChatManager::DestroySpeakerStates() {
    for (auto& [speakerId, speaker] : m_Speakers) {
        (void)speakerId;
        if (speaker.decoder) {
            opus_decoder_destroy(speaker.decoder);
        }
    }
    m_Speakers.clear();
}

} // namespace App