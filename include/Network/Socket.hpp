#ifndef NETWORK_SOCKET_HPP
#define NETWORK_SOCKET_HPP

// Prevent Windows macros from interfering with standard library and other libs
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#include <enet/enet.h>

// Undefine Windows macros that conflict with other libraries
#ifdef _WIN32
    #ifdef ERROR
    #undef ERROR
    #endif
    #ifdef DELETE
    #undef DELETE
    #endif
    #ifdef near
    #undef near
    #endif
    #ifdef far
    #undef far
    #endif
    #ifdef IN
    #undef IN
    #endif
    #ifdef OUT
    #undef OUT
    #endif
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Network {

// ENet channel definitions
constexpr uint8_t CHANNEL_RELIABLE = 0;    // Connection, game state sync
constexpr uint8_t CHANNEL_UNRELIABLE = 1;  // Position updates
constexpr uint8_t CHANNEL_COUNT = 2;

// Forward declarations
class Socket;

// ── Event Types ─────────────────────────────────────────────────────────────

enum class SocketEventType {
    None,
    Connect,
    Disconnect,
    Receive
};

struct SocketEvent {
    SocketEventType type = SocketEventType::None;
    uint32_t peerId = 0;
    std::vector<uint8_t> data;
    uint8_t channel = 0;
};

// ── Socket Class (ENet Wrapper) ─────────────────────────────────────────────

class Socket {
public:
    Socket();
    ~Socket();

    // Prevent copying
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Initialize ENet (call once at startup)
    static bool InitializeENet();
    static void ShutdownENet();

    // Server mode
    bool CreateServer(uint16_t port, size_t maxClients);

    // Client mode
    bool CreateClient();
    bool Connect(const std::string& address, uint16_t port);
    void Disconnect();

    // Common operations
    void Close();
    bool IsOpen() const { return m_Host != nullptr; }
    bool IsConnected() const { return m_ServerPeer != nullptr && m_ServerPeer->state == ENET_PEER_STATE_CONNECTED; }

    // Polling
    bool PollEvent(SocketEvent& event, uint32_t timeoutMs = 0);

    // Sending
    void SendToAll(const void* data, size_t size, uint8_t channel, bool reliable);
    void SendToPeer(uint32_t peerId, const void* data, size_t size, uint8_t channel, bool reliable);
    void SendToServer(const void* data, size_t size, uint8_t channel, bool reliable);

    // Peer management
    void DisconnectPeer(uint32_t peerId);
    std::string GetPeerAddress(uint32_t peerId) const;
    size_t GetConnectedPeerCount() const;

private:
    ENetHost* m_Host = nullptr;
    ENetPeer* m_ServerPeer = nullptr;  // Only used in client mode
    std::vector<ENetPeer*> m_Peers;    // All connected peers (server mode)

    static bool s_ENetInitialized;
};

} // namespace Network

#endif // NETWORK_SOCKET_HPP
