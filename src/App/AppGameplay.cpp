// ============================================================================
//  AppGameplay.cpp — Application gameplay loop and combat handlers
// ============================================================================

#include "App/App.hpp"

#include "Util/Time.hpp"

namespace App {

void Application::Update() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    auto& camera = m_GameManager.GetCamera();
    auto& player = m_GameManager.GetPlayer();

    if (m_InputManager.IsExitRequested()) {
        m_Network.Disconnect();
        m_StateManager.SetState(GameState::GAME_END);
        return;
    }

    if (m_InputManager.IsTabPressed()) {
        m_InputManager.ToggleCursor();
        m_UIManager.SetDebugPanelVisible(!m_InputManager.IsCursorLocked());
    }

    if (m_InputManager.IsToggleModelPressed()) {
        player.ToggleModelVisibility();
    }

    if (m_InputManager.IsBuyMenuPressed()) {
        m_UIManager.ToggleBuyMenu();
        if (m_UIManager.IsBuyMenuVisible()) {
            m_InputManager.UnlockCursor();
        } else {
            m_InputManager.LockCursor();
        }
    }

    if (m_InputManager.IsSwitchCharacterPressed()) {
        HandleCharacterSwitch();
    }

    if (m_Network.IsHost()) {
        const auto matchView = BuildNetworkMatchStateView();
        m_NetworkController.UpdateHost(
            dt, m_Network, player, camera,
            m_GameManager.GetRemotePlayers(),
            matchView
        );
    } else if (m_Network.IsClient()) {
        auto inputState = m_InputManager.SampleInput(
            camera, player.GetPosition(),
            player.IsWalking(), player.IsOnGround(), player.IsCrouching()
        );
        m_NetworkController.UpdateClient(
            dt, m_Network, inputState,
            m_GameManager.GetRemotePlayers()
        );
        SyncClientAuthoritativeState();
        SyncClientMatchState();
        if (!m_StateManager.IsInState(GameState::GAME_UPDATE)) {
            return;
        }
    }

    m_GameManager.UpdatePlayer(dt);
    m_GameManager.UpdateBots(dt);
    UpdateAudio();

    HandleBotGunfire();
    if (!m_StateManager.IsInState(GameState::GAME_UPDATE)) {
        return;
    }

    HandleBulletHit();
    if (!m_StateManager.IsInState(GameState::GAME_UPDATE)) {
        return;
    }

    m_CombatManager.CheckLocalRespawn(player, camera, m_GameManager.GetCollisionMesh());

    if (m_Network.IsHost()) {
        m_CombatManager.CheckRemoteRespawns(
            m_GameManager.GetRemotePlayers(),
            m_GameManager.GetCollisionMesh()
        );
        m_CombatManager.CheckBotRespawns(
            m_GameManager.GetBotPlayers(),
            m_GameManager.GetCollisionMesh()
        );
    }

    m_GameManager.UpdateEffects(dt);
    m_InputManager.ProcessMouseLook(camera);

    m_GameManager.Render();
    m_GameManager.DrawEffects();
    m_UIManager.RenderHUD(player);
    m_UIManager.RenderTeamScore(m_MatchState.ctKills, m_MatchState.tKills);
    m_UIManager.RenderCrosshair(player);
    m_UIManager.RenderBuyMenu(player.GetMoney());
    m_UIManager.RenderDebugPanel(
        camera, player,
        m_GameManager.GetCollisionMesh(),
        m_GameManager.GetRenderStats(),
        m_Network,
        m_GameManager.GetRemotePlayers().size(),
        m_GameManager.GetBulletHoleCount(),
        dt
    );
}

void Application::MatchResult() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    m_VoiceChatManager.SetPushToTalkEnabled(false);

    if (m_Network.IsHost()) {
        UpdatePostMatch(dt);
        if (!m_StateManager.IsInState(GameState::MATCH_RESULT)) {
            return;
        }
    }

    UpdatePassiveNetwork(dt);

    if (m_Network.IsClient()) {
        SyncClientAuthoritativeState();
        SyncClientMatchState();
        if (!m_StateManager.IsInState(GameState::MATCH_RESULT)) {
            return;
        }
    }

    m_GameManager.Render();
    m_GameManager.DrawEffects();
    m_UIManager.RenderMatchResult(IsLocalWinner());
}

void Application::MatchSummary() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    m_VoiceChatManager.SetPushToTalkEnabled(false);

    if (m_Network.IsHost()) {
        UpdatePostMatch(dt);
        if (!m_StateManager.IsInState(GameState::MATCH_SUMMARY)) {
            return;
        }
    }

    UpdatePassiveNetwork(dt);

    if (m_Network.IsClient()) {
        SyncClientAuthoritativeState();
        SyncClientMatchState();
        if (!m_StateManager.IsInState(GameState::MATCH_SUMMARY)) {
            return;
        }
    }

    m_GameManager.Render();
    m_GameManager.DrawEffects();
    m_UIManager.RenderMatchSummary(BuildMatchSummaryRows(), m_MatchState.phaseTimeRemaining);
}

void Application::HandleBulletHit() {
    auto& player = m_GameManager.GetPlayer();
    auto* gun = player.GetWeapon();
    if (!gun) return;

    const uint32_t shotSequence = gun->GetShotSequence();
    if (shotSequence == 0 || shotSequence == m_LastProcessedLocalShotSequence) {
        return;
    }
    m_LastProcessedLocalShotSequence = shotSequence;

    auto& camera = m_GameManager.GetCamera();
    const glm::vec3 shotOrigin = camera.GetPosition();
    m_AudioManager.PlayOneShot(AudioManager::SoundType::Gunshot, shotOrigin);
    if (m_Network.IsHost()) {
        m_Network.BroadcastGunshot(m_Network.GetLocalPlayerId(), shotOrigin);
    } else if (m_Network.IsClient()) {
        m_Network.SendGunshot(shotOrigin);
    }

    const float damagePerProjectile = gun->GetDamagePerProjectile();

    for (const auto& projectile : gun->GetProjectileResults()) {
        const auto& mapHit = projectile.mapHit;
        const float maxDist = mapHit.hit ? mapHit.distance : gun->GetBulletRange();

        auto playerHit = m_CombatManager.CheckPlayerHit(
            camera.GetPosition(),
            projectile.fireDir,
            maxDist,
            m_GameManager.GetRemotePlayers()
        );

        auto botHit = m_CombatManager.CheckBotHit(
            camera.GetPosition(),
            projectile.fireDir,
            maxDist,
            m_GameManager.GetBotPlayers()
        );

        if (playerHit.hit && (!botHit.hit || playerHit.distance <= botHit.distance)) {
            if (m_CombatManager.HandleDamage(
                    playerHit.playerId, damagePerProjectile, playerHit.point,
                    m_Network, m_GameManager.GetRemotePlayers())) {
                PlayDeathSoundForParticipant(playerHit.playerId);
                RecordKill(m_Network.GetLocalPlayerId(), playerHit.playerId);
            }
            continue;
        }

        if (botHit.hit) {
            if (m_CombatManager.HandleBotDamage(
                    botHit.playerId, damagePerProjectile,
                    m_GameManager.GetBotPlayers())) {
                const uint8_t victimId = MakeBotParticipantId(botHit.playerId);
                PlayDeathSoundForParticipant(victimId);
                if (m_Network.IsHost()) {
                    RecordKill(m_Network.GetLocalPlayerId(), victimId);
                } else if (m_Network.IsClient()) {
                    m_Network.SendMatchKill(m_Network.GetLocalPlayerId(), victimId);
                }
            }
            continue;
        }

        if (mapHit.hit) {
            m_GameManager.SpawnBulletHole(mapHit.point, mapHit.normal);

            if (m_Network.IsHost()) {
                m_Network.BroadcastBulletEffect(mapHit.point, mapHit.normal);
            } else if (m_Network.IsClient()) {
                m_Network.SendBulletEffect(mapHit.point, mapHit.normal);
            }
        }
    }
}

void Application::HandleBotGunfire() {
    auto& player = m_GameManager.GetPlayer();

    for (auto& bot : m_GameManager.GetBotPlayers()) {
        if (!bot.ConsumeShotThisFrame()) {
            continue;
        }

        const glm::vec3 shotOrigin = bot.GetEyePosition();
        m_AudioManager.PlayOneShot(AudioManager::SoundType::Gunshot, shotOrigin);
        if (m_Network.IsHost()) {
            m_Network.BroadcastGunshot(MakeBotParticipantId(bot.GetBotId()), shotOrigin);
        }

        const auto* gun = bot.GetGameplayWeapon();
        if (!gun) {
            continue;
        }

        const float damagePerProjectile = gun->GetDamagePerProjectile();

        for (const auto& projectile : gun->GetProjectileResults()) {
            const auto& mapHit = projectile.mapHit;
            const float maxDist = mapHit.hit ? mapHit.distance : gun->GetBulletRange();

            auto playerHit = m_CombatManager.CheckLocalPlayerHit(
                bot.GetEyePosition(),
                projectile.fireDir,
                maxDist,
                player
            );

            if (playerHit.hit && playerHit.distance <= maxDist) {
                if (m_CombatManager.HandleLocalPlayerDamage(player, damagePerProjectile)) {
                    m_AudioManager.PlayOneShot(AudioManager::SoundType::Death, player.GetPosition());
                    const uint8_t killerId = MakeBotParticipantId(bot.GetBotId());
                    const uint8_t victimId = m_Network.GetLocalPlayerId();
                    if (m_Network.IsHost()) {
                        RecordKill(killerId, victimId);
                    } else if (m_Network.IsClient()) {
                        m_Network.SendMatchKill(killerId, victimId);
                    }
                }
                continue;
            }

            if (mapHit.hit) {
                m_GameManager.SpawnBulletHole(mapHit.point, mapHit.normal);

                if (m_Network.IsHost()) {
                    m_Network.BroadcastBulletEffect(mapHit.point, mapHit.normal);
                }
            }
        }
    }
}

void Application::UpdateAudio() {
    auto& camera = m_GameManager.GetCamera();
    m_AudioManager.UpdateListener(camera.GetPosition(), camera.GetFront(), camera.GetRight());
    m_VoiceChatManager.SetLocalPlayerId(m_Network.GetLocalPlayerId());
    m_VoiceChatManager.UpdateListener(camera.GetPosition(), camera.GetFront(), camera.GetRight());

    std::vector<uint32_t> activeLoopEmitters;
    const auto& remotePlayers = m_GameManager.GetRemotePlayers();
    const auto& bots = m_GameManager.GetBotPlayers();
    activeLoopEmitters.reserve(remotePlayers.size() + bots.size());

    for (const auto& [playerId, remote] : remotePlayers) {
        const uint32_t emitterId = MakeRemoteFootstepEmitterId(playerId);
        activeLoopEmitters.push_back(emitterId);
        m_AudioManager.UpdateLoopEmitter(
            emitterId,
            AudioManager::SoundType::Footstep,
            remote.GetPosition(),
            remote.IsAlive() && remote.IsWalking() && !remote.IsCrouching());

            m_VoiceChatManager.UpdateSpeakerPosition(playerId, remote.GetPosition());
    }

    for (const auto& bot : bots) {
        const uint32_t emitterId = MakeBotFootstepEmitterId(bot.GetBotId());
        activeLoopEmitters.push_back(emitterId);
        m_AudioManager.UpdateLoopEmitter(
            emitterId,
            AudioManager::SoundType::Footstep,
            bot.GetPosition(),
            bot.IsAlive() && bot.IsWalking() && !bot.IsCrouching());
    }

    m_AudioManager.StopAllLoopEmittersExcept(activeLoopEmitters);

    m_VoiceChatManager.SetPushToTalkEnabled(m_Network.IsOnline() && m_InputManager.IsVoiceChatPressed());
    m_VoiceChatManager.Update([this](uint32_t sequence, const std::vector<uint8_t>& encodedFrame) {
        if (encodedFrame.empty()) {
            return;
        }

        if (m_Network.IsHost()) {
            m_Network.BroadcastVoiceFrame(
                m_Network.GetLocalPlayerId(),
                sequence,
                encodedFrame.data(),
                static_cast<uint16_t>(encodedFrame.size())
            );
        } else if (m_Network.IsClient()) {
            m_Network.SendVoiceFrame(
                sequence,
                encodedFrame.data(),
                static_cast<uint16_t>(encodedFrame.size())
            );
        }
    });
}

void Application::HandleVoiceFrame(uint8_t sourceId,
                                   uint32_t sequence,
                                   const std::vector<uint8_t>& encodedFrame) {
    m_VoiceChatManager.HandleIncomingFrame(sourceId, sequence, encodedFrame);
}

void Application::PlayDeathSoundForParticipant(uint8_t participantId) {
    auto position = GetParticipantPosition(participantId);
    if (!position) {
        return;
    }

    m_AudioManager.PlayOneShot(AudioManager::SoundType::Death, *position);
}

std::optional<glm::vec3> Application::GetParticipantPosition(uint8_t participantId) const {
    if (participantId >= Network::MAX_PLAYERS) {
        const size_t botIndex = static_cast<size_t>(participantId - Network::MAX_PLAYERS);
        const auto& bots = m_GameManager.GetBotPlayers();
        if (botIndex < bots.size()) {
            return bots[botIndex].GetPosition();
        }
        return std::nullopt;
    }

    if (participantId == m_Network.GetLocalPlayerId()) {
        return m_GameManager.GetPlayer().GetPosition();
    }

    const auto& remotePlayers = m_GameManager.GetRemotePlayers();
    auto it = remotePlayers.find(participantId);
    if (it != remotePlayers.end()) {
        return it->second.GetPosition();
    }

    return std::nullopt;
}

uint32_t Application::MakeRemoteFootstepEmitterId(uint8_t playerId) {
    return 0x10000u | static_cast<uint32_t>(playerId);
}

uint32_t Application::MakeBotFootstepEmitterId(uint8_t botId) {
    return 0x20000u | static_cast<uint32_t>(botId);
}

void Application::UpdatePassiveNetwork(float dt) {
    auto& player = m_GameManager.GetPlayer();
    auto& camera = m_GameManager.GetCamera();
    const auto matchView = BuildNetworkMatchStateView();
    m_NetworkController.SyncPaused(
        dt,
        m_Network,
        player,
        camera,
        m_GameManager.GetRemotePlayers(),
        matchView
    );
}

} // namespace App