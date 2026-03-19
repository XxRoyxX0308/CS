#ifndef NETWORK_LAN_DISCOVERY_HPP
#define NETWORK_LAN_DISCOVERY_HPP

#include "Network/NetworkTypes.hpp"
#include <vector>
#include <string>
#include <cstdint>

// Platform-neutral socket handle type
// Actual value: SOCKET (uintptr_t) on Windows, int on Unix
// We use uintptr_t to avoid including platform headers
#ifdef _WIN32
    using SocketHandle = uintptr_t;
    constexpr SocketHandle INVALID_SOCKET_HANDLE = ~static_cast<SocketHandle>(0);
#else
    using SocketHandle = int;
    constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
#endif

namespace Network {

class LANDiscovery {
public:
    LANDiscovery();
    ~LANDiscovery();

    // Host mode: respond to discovery queries
    bool StartResponding(const std::string& gameName, uint8_t maxPlayers, uint16_t gamePort);
    void StopResponding();
    void UpdateHost(uint8_t currentPlayerCount);

    // Client mode: discover servers
    void StartDiscovery();
    void StopDiscovery();
    void UpdateClient(float dt);

    // Get discovered servers
    const std::vector<ServerInfo>& GetDiscoveredServers() const { return m_DiscoveredServers; }

    // Clear old servers (timeout)
    void CleanupStaleServers(float timeout = 5.0f);

    bool IsDiscovering() const { return m_IsDiscovering; }
    bool IsResponding() const { return m_IsResponding; }

private:
    void InitSocket();
    void CloseSocket();
    void SendBroadcast(const void* data, size_t size);
    bool ReceivePacket(std::vector<uint8_t>& buffer, std::string& senderIp);

    SocketHandle m_Socket = INVALID_SOCKET_HANDLE;
    bool m_IsResponding = false;
    bool m_IsDiscovering = false;

    // Host state
    std::string m_GameName;
    uint8_t m_MaxPlayers = MAX_PLAYERS;
    uint8_t m_CurrentPlayerCount = 0;
    uint16_t m_GamePort = DEFAULT_PORT;

    // Client state
    std::vector<ServerInfo> m_DiscoveredServers;
    float m_DiscoveryTimer = 0.0f;
    float m_BroadcastInterval = 1.0f;
    float m_LocalTime = 0.0f;
};

} // namespace Network

#endif // NETWORK_LAN_DISCOVERY_HPP
