// Must be first to avoid Windows header conflicts
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    // Undefine Windows macros that conflict with other libraries
    #ifdef ERROR
    #undef ERROR
    #endif
    #ifdef DELETE
    #undef DELETE
    #endif
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

#include "Network/Discovery/LANDiscovery.hpp"
#include "Network/Packet.hpp"
#include <Util/Logger.hpp>
#include <cstring>
#include <algorithm>

namespace Network {

LANDiscovery::LANDiscovery() = default;

LANDiscovery::~LANDiscovery() {
    CloseSocket();
}

void LANDiscovery::InitSocket() {
    if (m_Socket != INVALID_SOCKET_HANDLE) return;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("Failed to initialize Winsock for discovery");
        return;
    }
#endif

    m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_Socket == INVALID_SOCKET_HANDLE) {
        LOG_ERROR("Failed to create discovery socket");
        return;
    }

    // Enable broadcast
    int broadcastEnable = 1;
    setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST,
               reinterpret_cast<const char*>(&broadcastEnable), sizeof(broadcastEnable));

    // Set non-blocking
#ifdef _WIN32
    u_long nonBlocking = 1;
    ioctlsocket(m_Socket, FIONBIO, &nonBlocking);
#else
    int flags = fcntl(m_Socket, F_GETFL, 0);
    fcntl(m_Socket, F_SETFL, flags | O_NONBLOCK);
#endif

    // Allow address reuse
    int reuseAddr = 1;
    setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuseAddr), sizeof(reuseAddr));
}

void LANDiscovery::CloseSocket() {
    if (m_Socket != INVALID_SOCKET_HANDLE) {
#ifdef _WIN32
        closesocket(m_Socket);
        WSACleanup();
#else
        close(m_Socket);
#endif
        m_Socket = INVALID_SOCKET_HANDLE;
    }
}

bool LANDiscovery::StartResponding(const std::string& gameName, uint8_t maxPlayers, uint16_t gamePort) {
    StopResponding();
    StopDiscovery();

    InitSocket();
    if (m_Socket == INVALID_SOCKET_HANDLE) return false;

    // Bind to discovery port
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_Socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind discovery socket to port {}", DISCOVERY_PORT);
        CloseSocket();
        return false;
    }

    m_GameName = gameName;
    m_MaxPlayers = maxPlayers;
    m_GamePort = gamePort;
    m_CurrentPlayerCount = 1;  // Host counts as 1 player
    m_IsResponding = true;

    LOG_INFO("LAN Discovery responding on port {}", DISCOVERY_PORT);
    return true;
}

void LANDiscovery::StopResponding() {
    if (m_IsResponding) {
        CloseSocket();
        m_IsResponding = false;
        LOG_INFO("LAN Discovery stopped responding");
    }
}

void LANDiscovery::UpdateHost(uint8_t currentPlayerCount) {
    if (!m_IsResponding) return;

    m_CurrentPlayerCount = currentPlayerCount;

    // Check for incoming queries
    std::vector<uint8_t> buffer(256);
    std::string senderIp;

    while (ReceivePacket(buffer, senderIp)) {
        if (buffer.size() >= sizeof(DiscoveryQueryPacket)) {
            // Verify magic
            if (std::memcmp(buffer.data(), DISCOVERY_MAGIC, 4) == 0 &&
                buffer[4] == static_cast<uint8_t>(PacketType::BROADCAST_QUERY)) {

                // Send response
                auto response = PacketBuilder::DiscoveryResponse(
                    m_GameName.c_str(),
                    m_CurrentPlayerCount,
                    m_MaxPlayers,
                    m_GamePort
                );

                // Send back to sender
                sockaddr_in destAddr{};
                destAddr.sin_family = AF_INET;
                destAddr.sin_port = htons(DISCOVERY_PORT);
                inet_pton(AF_INET, senderIp.c_str(), &destAddr.sin_addr);

                sendto(m_Socket, reinterpret_cast<const char*>(response.data()),
                       static_cast<int>(response.size()), 0,
                       reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));

                LOG_DEBUG("Sent discovery response to {}", senderIp);
            }
        }
    }
}

void LANDiscovery::StartDiscovery() {
    StopDiscovery();
    StopResponding();

    InitSocket();
    if (m_Socket == INVALID_SOCKET_HANDLE) return;

    // Bind to any port for receiving responses
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_Socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        // Try binding to any port
        addr.sin_port = 0;
        if (bind(m_Socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            LOG_ERROR("Failed to bind discovery socket");
            CloseSocket();
            return;
        }
    }

    m_DiscoveredServers.clear();
    m_DiscoveryTimer = 0.0f;
    m_LocalTime = 0.0f;
    m_IsDiscovering = true;

    // Send initial broadcast
    SendBroadcast(PacketBuilder::DiscoveryQuery().data(), sizeof(DiscoveryQueryPacket));

    LOG_INFO("LAN Discovery started");
}

void LANDiscovery::StopDiscovery() {
    if (m_IsDiscovering) {
        CloseSocket();
        m_IsDiscovering = false;
        LOG_INFO("LAN Discovery stopped");
    }
}

void LANDiscovery::UpdateClient(float dt) {
    if (!m_IsDiscovering) return;

    m_LocalTime += dt;
    m_DiscoveryTimer += dt;

    // Send periodic broadcasts
    if (m_DiscoveryTimer >= m_BroadcastInterval) {
        SendBroadcast(PacketBuilder::DiscoveryQuery().data(), sizeof(DiscoveryQueryPacket));
        m_DiscoveryTimer = 0.0f;
    }

    // Receive responses
    std::vector<uint8_t> buffer(256);
    std::string senderIp;

    while (ReceivePacket(buffer, senderIp)) {
        auto response = PacketParser::ParseDiscoveryResponse(buffer);
        if (response) {
            // Check if server already in list
            auto it = std::find_if(m_DiscoveredServers.begin(), m_DiscoveredServers.end(),
                [&senderIp](const ServerInfo& s) { return s.ip == senderIp; });

            if (it != m_DiscoveredServers.end()) {
                // Update existing
                it->name = response->gameName;
                it->playerCount = response->playerCount;
                it->maxPlayers = response->maxPlayers;
                it->port = response->gamePort;
                it->lastSeen = m_LocalTime;
            } else {
                // Add new
                ServerInfo info;
                info.name = response->gameName;
                info.ip = senderIp;
                info.port = response->gamePort;
                info.playerCount = response->playerCount;
                info.maxPlayers = response->maxPlayers;
                info.lastSeen = m_LocalTime;

                m_DiscoveredServers.push_back(info);
                LOG_INFO("Discovered server: {} at {}:{}", info.name, info.ip, info.port);
            }
        }
    }

    // Cleanup stale servers
    CleanupStaleServers(5.0f);
}

void LANDiscovery::SendBroadcast(const void* data, size_t size) {
    if (m_Socket == INVALID_SOCKET_HANDLE) return;

    sockaddr_in broadcastAddr{};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(DISCOVERY_PORT);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(m_Socket, reinterpret_cast<const char*>(data), static_cast<int>(size), 0,
           reinterpret_cast<sockaddr*>(&broadcastAddr), sizeof(broadcastAddr));
}

bool LANDiscovery::ReceivePacket(std::vector<uint8_t>& buffer, std::string& senderIp) {
    if (m_Socket == INVALID_SOCKET_HANDLE) return false;

    sockaddr_in senderAddr{};
    socklen_t addrLen = sizeof(senderAddr);

    buffer.resize(256);
    int received = recvfrom(m_Socket, reinterpret_cast<char*>(buffer.data()),
                            static_cast<int>(buffer.size()), 0,
                            reinterpret_cast<sockaddr*>(&senderAddr), &addrLen);

    if (received <= 0) return false;

    buffer.resize(received);

    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &senderAddr.sin_addr, ipStr, sizeof(ipStr));
    senderIp = ipStr;

    return true;
}

void LANDiscovery::CleanupStaleServers(float timeout) {
    m_DiscoveredServers.erase(
        std::remove_if(m_DiscoveredServers.begin(), m_DiscoveredServers.end(),
            [this, timeout](const ServerInfo& s) {
                return (m_LocalTime - s.lastSeen) > timeout;
            }),
        m_DiscoveredServers.end()
    );
}

} // namespace Network
