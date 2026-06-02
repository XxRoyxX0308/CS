// ============================================================================
//  AppMatch.cpp — Application match-state and post-match flow
// ============================================================================

#include "App/App.hpp"

#include <algorithm>
#include <cstring>

namespace App {

namespace {
constexpr uint8_t TEAM_CT = 0;
constexpr uint8_t TEAM_T = 1;
constexpr uint8_t INVALID_TEAM = 0xFF;
constexpr uint8_t MATCH_KILL_TARGET = 20;
constexpr float MATCH_RESULT_DURATION = 3.0f;
constexpr float MATCH_SUMMARY_DURATION = 10.0f;

Network::MatchStatePhase ToNetworkMatchPhase(MatchPhase phase) {
    switch (phase) {
    case MatchPhase::RESULT:
        return Network::MatchStatePhase::RESULT;
    case MatchPhase::SUMMARY:
        return Network::MatchStatePhase::SUMMARY;
    case MatchPhase::LIVE:
    default:
        return Network::MatchStatePhase::LIVE;
    }
}

MatchPhase FromNetworkMatchPhase(Network::MatchStatePhase phase) {
    switch (phase) {
    case Network::MatchStatePhase::RESULT:
        return MatchPhase::RESULT;
    case Network::MatchStatePhase::SUMMARY:
        return MatchPhase::SUMMARY;
    case Network::MatchStatePhase::LIVE:
    default:
        return MatchPhase::LIVE;
    }
}
} // namespace

void Application::ResetMatchState() {
    m_MatchState = {};
}

void Application::InitializeMatchState() {
    ResetMatchState();

    const auto lobbyPlayers = m_Network.GetLobbyPlayers();
    for (const auto& playerInfo : lobbyPlayers) {
        m_MatchState.participants.push_back(MatchParticipantStats{
            playerInfo.playerId,
            playerInfo.name,
            playerInfo.teamId,
            false,
            0,
            0,
        });

        if (playerInfo.playerId != m_Network.GetLocalPlayerId()) {
            auto it = m_GameManager.GetRemotePlayers().find(playerInfo.playerId);
            if (it != m_GameManager.GetRemotePlayers().end()) {
                it->second.SetTeamId(playerInfo.teamId);
            }
        }
    }

    const auto& bots = m_GameManager.GetBotPlayers();
    for (size_t i = 0; i < bots.size(); ++i) {
        m_MatchState.participants.push_back(MatchParticipantStats{
            MakeBotParticipantId(i),
            bots[i].GetName(),
            bots[i].GetTeamId(),
            true,
            0,
            0,
        });
    }
}

void Application::StartMatchResult(uint8_t winningTeam) {
    if (m_MatchState.phase != MatchPhase::LIVE) {
        return;
    }

    m_AudioManager.Reset();
    m_MatchState.phase = MatchPhase::RESULT;
    m_MatchState.winningTeam = winningTeam;
    m_MatchState.phaseTimeRemaining = MATCH_RESULT_DURATION;
    m_StateManager.SetState(GameState::MATCH_RESULT);
}

void Application::UpdatePostMatch(float dt) {
    if (!m_Network.IsHost()) {
        return;
    }

    m_MatchState.phaseTimeRemaining = std::max(0.0f, m_MatchState.phaseTimeRemaining - dt);

    if (m_MatchState.phase == MatchPhase::RESULT && m_MatchState.phaseTimeRemaining <= 0.0f) {
        m_MatchState.phase = MatchPhase::SUMMARY;
        m_MatchState.phaseTimeRemaining = MATCH_SUMMARY_DURATION;
        m_StateManager.SetState(GameState::MATCH_SUMMARY);
        return;
    }

    if (m_MatchState.phase == MatchPhase::SUMMARY && m_MatchState.phaseTimeRemaining <= 0.0f) {
        ReturnToLobby(true);
    }
}

void Application::ReturnToLobby(bool broadcastToClients) {
    if (broadcastToClients && m_Network.IsHost()) {
        m_Network.BroadcastReturnToLobby();
    }

    m_AudioManager.Reset();
    m_UIManager.SetBuyMenuVisible(false);
    m_UIManager.SetDebugPanelVisible(false);
    if (m_InputManager.IsCursorLocked()) {
        m_InputManager.UnlockCursor();
    }

    m_GameManager.Cleanup();
    ResetMatchState();
    m_StateManager.SetState(GameState::LOBBY);
}

void Application::SyncClientAuthoritativeState() {
    if (!m_Network.IsClient()) {
        return;
    }

    auto state = m_Network.GetLocalPlayerState();
    if (!state) {
        return;
    }

    auto& player = m_GameManager.GetPlayer();
    player.SetMoney(state->money);
}

void Application::SyncClientMatchState() {
    if (!m_Network.IsClient()) {
        return;
    }

    auto view = m_Network.GetLatestMatchState();
    if (!view) {
        return;
    }

    m_MatchState.ctKills = view->ctKills;
    m_MatchState.tKills = view->tKills;
    m_MatchState.phase = FromNetworkMatchPhase(view->phase);
    m_MatchState.winningTeam = view->winningTeam;
    m_MatchState.phaseTimeRemaining = view->phaseTimeRemaining;
    m_MatchState.participants.clear();

    for (uint8_t i = 0; i < view->participantCount && i < Network::MAX_MATCH_PARTICIPANTS; ++i) {
        const auto& participant = view->participants[i];
        m_MatchState.participants.push_back(MatchParticipantStats{
            participant.participantId,
            participant.name,
            participant.teamId,
            participant.IsBot(),
            participant.kills,
            participant.deaths,
        });
    }

    switch (m_MatchState.phase) {
    case MatchPhase::RESULT:
        if (!m_StateManager.IsInState(GameState::MATCH_RESULT)) {
            m_StateManager.SetState(GameState::MATCH_RESULT);
        }
        break;
    case MatchPhase::SUMMARY:
        if (!m_StateManager.IsInState(GameState::MATCH_SUMMARY)) {
            m_StateManager.SetState(GameState::MATCH_SUMMARY);
        }
        break;
    case MatchPhase::LIVE:
    default:
        break;
    }
}

uint8_t Application::GetLocalTeamId() const {
    const auto* participant = FindMatchParticipant(m_Network.GetLocalPlayerId());
    if (participant) {
        return participant->teamId;
    }

    return m_GameManager.GetCharacterTypeId();
}

bool Application::IsLocalWinner() const {
    return m_MatchState.winningTeam != INVALID_TEAM &&
           GetLocalTeamId() == m_MatchState.winningTeam;
}

MatchParticipantStats* Application::FindMatchParticipant(uint8_t participantId) {
    auto it = std::find_if(
        m_MatchState.participants.begin(),
        m_MatchState.participants.end(),
        [participantId](const MatchParticipantStats& participant) {
            return participant.participantId == participantId;
        }
    );

    return (it != m_MatchState.participants.end()) ? &(*it) : nullptr;
}

const MatchParticipantStats* Application::FindMatchParticipant(uint8_t participantId) const {
    auto it = std::find_if(
        m_MatchState.participants.begin(),
        m_MatchState.participants.end(),
        [participantId](const MatchParticipantStats& participant) {
            return participant.participantId == participantId;
        }
    );

    return (it != m_MatchState.participants.end()) ? &(*it) : nullptr;
}

void Application::RecordKill(uint8_t killerId, uint8_t victimId) {
    auto* killer = FindMatchParticipant(killerId);
    auto* victim = FindMatchParticipant(victimId);

    if (victim) {
        ++victim->deaths;
    }

    if (!killer || !victim || killerId == victimId) {
        return;
    }

    if (killer->teamId == victim->teamId) {
        return;
    }

    ++killer->kills;

    if (killer->teamId == TEAM_CT) {
        ++m_MatchState.ctKills;
    } else if (killer->teamId == TEAM_T) {
        ++m_MatchState.tKills;
    }

    if (!killer->isBot && m_Network.IsHost()) {
        if (killerId == m_Network.GetLocalPlayerId()) {
            m_GameManager.GetPlayer().AddMoney(500);
        } else {
            auto it = m_GameManager.GetRemotePlayers().find(killerId);
            if (it != m_GameManager.GetRemotePlayers().end()) {
                it->second.AddMoney(500);
            }
        }
    }

    if (m_MatchState.ctKills >= MATCH_KILL_TARGET) {
        StartMatchResult(TEAM_CT);
    } else if (m_MatchState.tKills >= MATCH_KILL_TARGET) {
        StartMatchResult(TEAM_T);
    }
}

Network::MatchStateView Application::BuildNetworkMatchStateView() const {
    Network::MatchStateView view{};
    view.ctKills = m_MatchState.ctKills;
    view.tKills = m_MatchState.tKills;
    view.phase = ToNetworkMatchPhase(m_MatchState.phase);
    view.winningTeam = m_MatchState.winningTeam;
    view.phaseTimeRemaining = m_MatchState.phaseTimeRemaining;
    view.participantCount = static_cast<uint8_t>(std::min(
        m_MatchState.participants.size(),
        static_cast<size_t>(Network::MAX_MATCH_PARTICIPANTS)
    ));

    for (uint8_t i = 0; i < view.participantCount; ++i) {
        const auto& participant = m_MatchState.participants[i];
        auto& out = view.participants[i];
        out.participantId = participant.participantId;
        out.teamId = participant.teamId;
        out.kills = static_cast<uint8_t>(std::clamp(participant.kills, 0, 255));
        out.deaths = static_cast<uint8_t>(std::clamp(participant.deaths, 0, 255));
        out.flags = participant.isBot ? Network::MATCH_FLAG_IS_BOT : 0;
        std::strncpy(out.name, participant.name.c_str(), sizeof(out.name) - 1);
        out.name[sizeof(out.name) - 1] = '\0';
    }

    return view;
}

std::vector<UIManager::MatchSummaryRow> Application::BuildMatchSummaryRows() const {
    std::vector<UIManager::MatchSummaryRow> rows;
    rows.reserve(m_MatchState.participants.size());

    for (const auto& participant : m_MatchState.participants) {
        rows.push_back(UIManager::MatchSummaryRow{
            participant.name,
            participant.teamId,
            !participant.isBot && participant.participantId == m_Network.GetLocalPlayerId(),
            participant.isBot,
            participant.kills,
            participant.deaths,
        });
    }

    std::stable_sort(rows.begin(), rows.end(), [](const UIManager::MatchSummaryRow& lhs,
                                                  const UIManager::MatchSummaryRow& rhs) {
        if (lhs.teamId != rhs.teamId) {
            return lhs.teamId < rhs.teamId;
        }
        if (lhs.kills != rhs.kills) {
            return lhs.kills > rhs.kills;
        }
        return lhs.name < rhs.name;
    });

    return rows;
}

uint8_t Application::MakeBotParticipantId(size_t botIndex) {
    return static_cast<uint8_t>(Network::MAX_PLAYERS + botIndex);
}

} // namespace App