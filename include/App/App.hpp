#ifndef CS_APP_APP_HPP
#define CS_APP_APP_HPP

// Prevent Windows min/max macros from interfering with std library and GLM
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#include "App/StateManager.hpp"
#include "App/InputManager.hpp"
#include "App/UIManager.hpp"
#include "App/CombatManager.hpp"
#include "App/NetworkController.hpp"
#include "App/AudioManager.hpp"
#include "App/VoiceChatManager.hpp"
#include "App/GameManager.hpp"
#include "Network/NetworkManager.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace App {

enum class MatchPhase : uint8_t {
    LIVE = 0,
    RESULT = 1,
    SUMMARY = 2,
};

struct MatchParticipantStats {
    uint8_t participantId = 0xFF;
    std::string name;
    uint8_t teamId = 0;
    bool isBot = false;
    int kills = 0;
    int deaths = 0;
};

struct MatchState {
    uint8_t ctKills = 0;
    uint8_t tKills = 0;
    MatchPhase phase = MatchPhase::LIVE;
    uint8_t winningTeam = 0xFF;
    float phaseTimeRemaining = 0.0f;
    std::vector<MatchParticipantStats> participants;
};

/**
 * @brief Main application coordinator.
 *
 * Acts as a Mediator between all managers, delegating work to
 * specialized components for a clean separation of concerns.
 */
class Application {
public:
    /** @brief State enum exposed for main loop compatibility. */
    using State = GameState;

    Application();

    /** @brief Get current game state. */
    State GetCurrentState() const { return m_StateManager.GetState(); }

    /** @brief Main Menu state handler. */
    void MainMenu();

    /** @brief Lobby state handler. */
    void Lobby();

    /** @brief Game Start state handler. */
    void Start();

    /** @brief Game Update state handler. */
    void Update();

    /** @brief Post-match result state handler. */
    void MatchResult();

    /** @brief Post-match summary state handler. */
    void MatchSummary();

    /** @brief Game End state handler. */
    void End();

private:
    // ── Managers ──
    StateManager m_StateManager;
    InputManager m_InputManager;
    UIManager m_UIManager;
    CombatManager m_CombatManager;
    NetworkController m_NetworkController;
    AudioManager m_AudioManager;
    VoiceChatManager m_VoiceChatManager;
    GameManager m_GameManager;

    // ── Network ──
    Network::NetworkManager m_Network;

    // ── Initialization Flag ──
    bool m_CallbacksInitialized = false;

    // ── Bot configuration ──
    int m_CTBotCount = 0;
    int m_TBotCount = 0;
    MatchState m_MatchState;
    uint32_t m_LastProcessedLocalShotSequence = 0;

    // ── Helper Methods ──
    void SetupUICallbacks();
    void SetupNetworkCallbacks();
    void HandleCharacterSwitch();
    void HandleBulletHit();
    void HandleBotGunfire();
    void UpdateAudio();
    void HandleVoiceFrame(uint8_t sourceId, uint32_t sequence, const std::vector<uint8_t>& encodedFrame);
    void SendCharacterConfig();
    void ResetMatchState();
    void InitializeMatchState();
    void StartMatchResult(uint8_t winningTeam);
    void UpdatePostMatch(float dt);
    void ReturnToLobby(bool broadcastToClients = false);
    void SyncClientAuthoritativeState();
    void SyncClientMatchState();
    void UpdatePassiveNetwork(float dt);
    uint8_t GetLocalTeamId() const;
    bool IsLocalWinner() const;
    MatchParticipantStats* FindMatchParticipant(uint8_t participantId);
    const MatchParticipantStats* FindMatchParticipant(uint8_t participantId) const;
    void RecordKill(uint8_t killerId, uint8_t victimId);
    void PlayDeathSoundForParticipant(uint8_t participantId);
    std::optional<glm::vec3> GetParticipantPosition(uint8_t participantId) const;
    Network::MatchStateView BuildNetworkMatchStateView() const;
    std::vector<UIManager::MatchSummaryRow> BuildMatchSummaryRows() const;
    static uint8_t MakeBotParticipantId(size_t botIndex);
    static uint32_t MakeRemoteFootstepEmitterId(uint8_t playerId);
    static uint32_t MakeBotFootstepEmitterId(uint8_t botId);
};

} // namespace App

#endif // CS_APP_APP_HPP
