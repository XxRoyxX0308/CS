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

    const auto& mapHit = gun->GetLastHit();
    static glm::vec3 lastHitPoint(0.0f);

    if (mapHit.hit && mapHit.point != lastHitPoint) {
        lastHitPoint = mapHit.point;

        auto& camera = m_GameManager.GetCamera();
        auto playerHit = m_CombatManager.CheckPlayerHit(
            camera.GetPosition(),
            gun->GetLastFireDir(),
            mapHit.distance,
            m_GameManager.GetRemotePlayers()
        );

        if (playerHit.hit && playerHit.distance < mapHit.distance) {
            auto botHit = m_CombatManager.CheckBotHit(
                camera.GetPosition(),
                gun->GetLastFireDir(),
                mapHit.distance,
                m_GameManager.GetBotPlayers()
            );

            if (botHit.hit && botHit.distance < playerHit.distance) {
                if (m_CombatManager.HandleBotDamage(
                        botHit.playerId, gun->GetDamage(),
                        m_GameManager.GetBotPlayers())) {
                    const uint8_t victimId = MakeBotParticipantId(botHit.playerId);
                    if (m_Network.IsHost()) {
                        RecordKill(m_Network.GetLocalPlayerId(), victimId);
                    } else if (m_Network.IsClient()) {
                        m_Network.SendMatchKill(m_Network.GetLocalPlayerId(), victimId);
                    }
                }
            } else {
                if (m_CombatManager.HandleDamage(
                        playerHit.playerId, gun->GetDamage(), playerHit.point,
                        m_Network, m_GameManager.GetRemotePlayers())) {
                    RecordKill(m_Network.GetLocalPlayerId(), playerHit.playerId);
                }
            }
        } else {
            auto botHit = m_CombatManager.CheckBotHit(
                camera.GetPosition(),
                gun->GetLastFireDir(),
                mapHit.distance,
                m_GameManager.GetBotPlayers()
            );

            if (botHit.hit && botHit.distance < mapHit.distance) {
                if (m_CombatManager.HandleBotDamage(
                        botHit.playerId, gun->GetDamage(),
                        m_GameManager.GetBotPlayers())) {
                    const uint8_t victimId = MakeBotParticipantId(botHit.playerId);
                    if (m_Network.IsHost()) {
                        RecordKill(m_Network.GetLocalPlayerId(), victimId);
                    } else if (m_Network.IsClient()) {
                        m_Network.SendMatchKill(m_Network.GetLocalPlayerId(), victimId);
                    }
                }
            } else {
                m_GameManager.SpawnBulletHole(mapHit.point, mapHit.normal);

                if (m_Network.IsHost()) {
                    m_Network.BroadcastBulletEffect(mapHit.point, mapHit.normal);
                } else if (m_Network.IsClient()) {
                    m_Network.SendBulletEffect(mapHit.point, mapHit.normal);
                }
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

        const auto* gun = bot.GetGameplayWeapon();
        if (!gun) {
            continue;
        }

        const auto& mapHit = gun->GetLastHit();
        float maxDist = mapHit.hit ? mapHit.distance : gun->GetBulletRange();

        auto playerHit = m_CombatManager.CheckLocalPlayerHit(
            bot.GetEyePosition(),
            gun->GetLastFireDir(),
            maxDist,
            player
        );

        if (playerHit.hit && playerHit.distance <= maxDist) {
            if (m_CombatManager.HandleLocalPlayerDamage(player, gun->GetDamage())) {
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