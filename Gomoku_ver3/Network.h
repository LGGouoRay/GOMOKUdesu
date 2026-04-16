#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <functional>
#include <optional>
#include <cstdint>
#include <vector>
#include <chrono>
#include "Board.h"
#include "SaveLoad.h"

// ============================================================
//  Network.h Network Communication (LAN / P2P)
// ============================================================

enum class PacketType : std::uint8_t {
    JoinRequest,
    JoinAccept,
    JoinReject,
    StonePlaced,
    UndoRequest,
    UndoAccept,
    UndoReject,
    ChatMessage,
    RestartRequest,
    RestartAccept,
    GameStart,
    HostStatusUpdate,
    SyncGameState,
    UseSkill
};

class Network {
public:
    enum class Status { Disconnected, Hosting, Connecting, Connected };

    Network();
    ~Network();

    void update();
    Status getStatus() const;

    bool host(unsigned short port = 55001, const std::string& roomName = "Room", bool isPrivateRoom = false, const std::string& roomCode = "");

    bool connect(const std::string& ipAddress, unsigned short port = 55001, const std::string& roomCode = "");

    void disconnect();

    std::string getLocalIP() const;
    bool isHost() const;

    // UDP Broadcast for room discovery
    struct RoomInfo {
        sf::IpAddress ip;
        unsigned short port;
        std::string roomName;
        std::string roomCode;
        bool isPrivate = false;
        std::chrono::steady_clock::time_point lastSeen;
    };

    void startBroadcasting(const std::string& roomName, unsigned short port = 55001, unsigned short broadcastPort = 55002);
    void stopBroadcasting();
    void discoverRooms(unsigned short broadcastPort = 55002);
    void updateRoomDiscovery();
    std::vector<RoomInfo> getAvailableRooms() const;

    void setOnStonePlaced(std::function<void(int, int)> callback);
    void setOnUndoRequest(std::function<void()> callback);
    void setOnUndoResponse(std::function<void(bool accepted)> callback);
    void setOnRestartRequest(std::function<void()> callback);
    void setOnRestartResponse(std::function<void(bool accepted)> callback);
    void setOnConnect(std::function<void()> callback);
    void setOnDisconnect(std::function<void()> callback);
    void setOnGameStart(std::function<void(bool)> callback);
    void setOnHostStatusUpdate(std::function<void(bool)> callback);
    void setOnSyncGameState(std::function<void(const GameSaveData&)> callback);
    void setOnUseSkill(std::function<void(int, const std::vector<sf::Vector2i>&)> callback);
    void setOnChatMessage(std::function<void(int)> callback);

    void sendStonePlaced(int row, int col);
    void sendUndoRequest();
    void sendUndoResponse(bool accepted);
    void sendRestartRequest();
    void sendRestartResponse(bool accepted);
    void sendGameStart(bool isExperimental);
    void sendHostStatusUpdate(bool isSavingOrLoading);
    void sendSyncGameState(const GameSaveData& data);
    void sendUseSkill(int skillId, const std::vector<sf::Vector2i>& targets);
    void sendChatMessage(int emojiId);

private:
    Status m_status = Status::Disconnected;
    bool m_isHost = false;
    bool m_isAuthenticated = false;
    bool m_isPrivateRoom = false;
    std::string m_roomCode;
    std::string m_roomName = "Room";

    sf::TcpSocket m_socket;
    sf::TcpListener m_listener;
    sf::SocketSelector m_selector;

    std::function<void(int row, int col)> m_onStonePlaced;
    std::function<void()> m_onUndoRequest;
    std::function<void(bool)> m_onUndoResponse;
    std::function<void()> m_onRestartRequest;
    std::function<void(bool)> m_onRestartResponse;
    std::function<void()> m_onConnect;
    std::function<void()> m_onDisconnect;
    std::function<void(bool)> m_onGameStart;
    std::function<void(bool)> m_onHostStatusUpdate;
    std::function<void(const GameSaveData&)> m_onSyncGameState;
    std::function<void(int, const std::vector<sf::Vector2i>&)> m_onUseSkill;
    std::function<void(int)> m_onChatMessage;

    // UDP Broadcasting
    sf::UdpSocket m_broadcastSocket;
    bool m_isBroadcasting = false;
    std::string m_broadcastRoomName;
    unsigned short m_broadcastPort = 55002;
    unsigned short m_tcpPort = 55001;
    std::chrono::steady_clock::time_point m_lastBroadcastTime;

    // UDP Discovery
    sf::UdpSocket m_discoverySocket;
    bool m_isDiscovering = false;
    std::vector<RoomInfo> m_discoveredRooms;
    std::chrono::steady_clock::time_point m_lastDiscoveryUpdateTime;

    void handlePacket(sf::Packet& packet);
    void sendBroadcastPacket();
    void receiveBroadcastPackets();
};
