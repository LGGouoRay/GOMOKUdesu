#pragma execution_character_set("utf-8")
#include "Network.h"
#include <iostream>

namespace {
constexpr const char* kRoomBroadcastMagic = "GOMOKU_ROOM_V1";
}

Network::Network() {
    m_socket.setBlocking(false);
    m_broadcastSocket.setBlocking(false);
    m_discoverySocket.setBlocking(false);
}

Network::~Network() {
    stopBroadcasting();
    disconnect();
}

bool Network::host(unsigned short port, const std::string& roomName, bool isPrivateRoom, const std::string& roomCode) {
    disconnect();
    m_listener.setBlocking(false);
    if (m_listener.listen(port) != sf::Socket::Status::Done) {
        std::cerr << "[Network] Failed to listen on port " << port << "\n";
        return false;
    }

    m_selector.clear();
    m_selector.add(m_listener);
    m_status = Status::Hosting;
    m_isHost = true;
    m_isAuthenticated = false;
    m_isPrivateRoom = isPrivateRoom;
    m_roomCode = roomCode;
    m_roomName = roomName;
    m_tcpPort = port;
    std::cout << "[Network] Hosting on port " << port << " with room name: " << roomName
              << (m_isPrivateRoom ? " [Private]" : " [Public]")
              << " code=" << m_roomCode << "\n";

    if (!m_isPrivateRoom) {
        startBroadcasting(roomName, port, 55002);
    }

    return true;
}

bool Network::connect(const std::string& ipAddress, unsigned short port, const std::string& roomCode) {
    disconnect();
    stopBroadcasting();

    std::string targetIP = ipAddress;
    const auto begin = targetIP.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return false;
    }
    const auto end = targetIP.find_last_not_of(" \t\r\n");
    targetIP = targetIP.substr(begin, end - begin + 1);

    std::string joinCode = roomCode;
    const auto codeBegin = joinCode.find_first_not_of(" \t\r\n");
    if (codeBegin == std::string::npos) {
        joinCode.clear();
    } else {
        const auto codeEnd = joinCode.find_last_not_of(" \t\r\n");
        joinCode = joinCode.substr(codeBegin, codeEnd - codeBegin + 1);
    }

    m_socket.setBlocking(true);
    auto addr = sf::IpAddress::resolve(targetIP);
    if (!addr) {
        m_socket.setBlocking(false);
        return false;
    }
    sf::Socket::Status status = m_socket.connect(addr.value(), port, sf::seconds(3.f));

    if (status != sf::Socket::Status::Done) {
        std::cerr << "[Network] Failed to connect to " << targetIP << ":" << port
                  << " (status=" << static_cast<int>(status) << ")\n";
        m_socket.setBlocking(false);
        return false;
    }

    m_socket.setBlocking(false);
    m_selector.clear();
    m_selector.add(m_socket);
    m_status = Status::Connected;
    m_isHost = false;
    m_isAuthenticated = false;

    sf::Packet joinPacket;
    joinPacket << static_cast<std::uint8_t>(PacketType::JoinRequest) << joinCode;
    if (m_socket.send(joinPacket) != sf::Socket::Status::Done) {
        std::cerr << "[Network] Failed to send join request.\n";
        disconnect();
        return false;
    }

    std::cout << "[Network] Connected to " << targetIP << ", waiting for join approval...\n";
    return true;
}

void Network::disconnect() {
    stopBroadcasting();
    m_listener.close();
    m_socket.disconnect();
    m_selector.clear();
    if (m_status == Status::Connected && m_onDisconnect) {
        m_onDisconnect();
    }
    m_isAuthenticated = false;
    m_isPrivateRoom = false;
    m_roomCode.clear();
    m_roomName = "Room";
    m_status = Status::Disconnected;
}

void Network::update() {
    // Update room discovery
    if (m_isDiscovering) {
        updateRoomDiscovery();
    }

    // Send broadcasts if hosting
    if (m_isBroadcasting) {
        sendBroadcastPacket();
    }

    if (m_status == Status::Disconnected) return;

    if (m_selector.wait(sf::milliseconds(10))) {
        if (m_status == Status::Hosting) {
            if (m_selector.isReady(m_listener)) {
                if (m_listener.accept(m_socket) == sf::Socket::Status::Done) {
                    m_status = Status::Connected;
                    m_listener.close(); 
                    m_selector.clear();
                    m_socket.setBlocking(false);
                    m_selector.add(m_socket);
                    m_isAuthenticated = false;

                    auto remoteIP = m_socket.getRemoteAddress();
                    std::cout << "[Network] Client connected: " 
                              << (remoteIP ? remoteIP->toString() : "Unknown") << "\n";
                }
            }
        } 
        else if (m_status == Status::Connected) {
            if (m_selector.isReady(m_socket)) {
                sf::Packet packet;
                sf::Socket::Status receiveStatus = m_socket.receive(packet);
                if (receiveStatus == sf::Socket::Status::Done) {
                    handlePacket(packet);
                } 
                else if (receiveStatus == sf::Socket::Status::Disconnected) {
                    std::cout << "[Network] Remote disconnected.\n";
                    if (m_isHost) {
                        m_socket.disconnect();
                        m_selector.clear();
                        m_isAuthenticated = false;

                        m_listener.setBlocking(false);
                        if (m_listener.listen(m_tcpPort) == sf::Socket::Status::Done) {
                            m_status = Status::Hosting;
                            m_selector.add(m_listener);

                            if (!m_isPrivateRoom && !m_isBroadcasting) {
                                startBroadcasting(m_roomName, m_tcpPort, m_broadcastPort);
                            }

                            std::cout << "[Network] Host resumed. Waiting for new client...\n";
                        } else {
                            disconnect();
                        }

                        if (m_onDisconnect) {
                            m_onDisconnect();
                        }
                    } else {
                        disconnect();
                    }
                }
            }
        }
    }
}

Network::Status Network::getStatus() const { return m_status; }
bool Network::isHost() const { return m_isHost; }
std::string Network::getLocalIP() const { 
    auto ip = sf::IpAddress::getLocalAddress();
    return ip ? ip->toString() : "Unknown"; 
}

void Network::startBroadcasting(const std::string& roomName, unsigned short port, unsigned short broadcastPort) {
    if (m_isBroadcasting) return;

    m_broadcastRoomName = roomName;
    m_tcpPort = port;
    m_broadcastPort = broadcastPort;
    m_isBroadcasting = true;
    m_lastBroadcastTime = std::chrono::steady_clock::now();

    m_broadcastSocket.unbind();
    m_broadcastSocket.bind(sf::Socket::AnyPort);
    m_broadcastSocket.setBlocking(false);

    std::cout << "[Network] Started broadcasting room: " << roomName << " on port " << broadcastPort << "\n";
}

void Network::stopBroadcasting() {
    if (!m_isBroadcasting) return;
    m_isBroadcasting = false;
    m_broadcastSocket.unbind();
    std::cout << "[Network] Stopped broadcasting.\n";
}

void Network::discoverRooms(unsigned short broadcastPort) {
    if (m_isDiscovering) return;

    m_broadcastPort = broadcastPort;
    m_isDiscovering = true;
    m_discoveredRooms.clear();
    m_lastDiscoveryUpdateTime = std::chrono::steady_clock::now();

    if (m_discoverySocket.bind(broadcastPort) != sf::Socket::Status::Done) {
        std::cerr << "[Network] Failed to bind discovery socket to port " << broadcastPort << "\n";
        m_isDiscovering = false;
        return;
    }

    m_discoverySocket.setBlocking(false);
    std::cout << "[Network] Started discovering rooms on port " << broadcastPort << "\n";
}

void Network::updateRoomDiscovery() {
    if (!m_isDiscovering) return;

    receiveBroadcastPackets();

    // Remove rooms not seen for more than 5 seconds
    auto now = std::chrono::steady_clock::now();
    auto it = m_discoveredRooms.begin();
    while (it != m_discoveredRooms.end()) {
        auto timeSinceLastSeen = std::chrono::duration_cast<std::chrono::seconds>(now - it->lastSeen).count();
        if (timeSinceLastSeen > 5) {
            it = m_discoveredRooms.erase(it);
        } else {
            ++it;
        }
    }
}

void Network::sendBroadcastPacket() {
    if (!m_isBroadcasting) return;

    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastBroadcast = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastBroadcastTime).count();

    if (timeSinceLastBroadcast < 1000) return;
    m_lastBroadcastTime = now;

    sf::Packet packet;
    packet << std::string(kRoomBroadcastMagic)
           << m_broadcastRoomName
           << m_tcpPort
           << m_roomCode
           << static_cast<std::uint8_t>(m_isPrivateRoom ? 1 : 0);

    m_broadcastSocket.send(packet, sf::IpAddress::Broadcast, m_broadcastPort);
    m_broadcastSocket.send(packet, sf::IpAddress::LocalHost, m_broadcastPort);
}

void Network::receiveBroadcastPackets() {
    if (!m_isDiscovering) return;

    while (true) {
        sf::Packet packet;
        std::optional<sf::IpAddress> sender;
        unsigned short senderPort = 0;
        sf::Socket::Status status = m_discoverySocket.receive(packet, sender, senderPort);
        if (status != sf::Socket::Status::Done) {
            break;
        }

        if (!sender) {
            continue;
        }

        std::string magic;
        std::string roomName;
        unsigned short tcpPort = 0;
        std::string roomCode;
        std::uint8_t privateFlag = 0;
        if (!(packet >> magic >> roomName >> tcpPort >> roomCode >> privateFlag)) {
            continue;
        }

        if (magic != kRoomBroadcastMagic) {
            continue;
        }

        bool isPrivate = (privateFlag != 0);
        if (isPrivate) {
            continue;
        }

        bool found = false;
        for (auto& room : m_discoveredRooms) {
            if (room.ip == sender.value() && room.port == tcpPort) {
                room.lastSeen = std::chrono::steady_clock::now();
                room.roomName = roomName;
                room.roomCode = roomCode;
                room.isPrivate = false;
                found = true;
                break;
            }
        }

        if (!found) {
            RoomInfo newRoom{
                sender.value(),
                tcpPort,
                roomName,
                roomCode,
                false,
                std::chrono::steady_clock::now()
            };
            m_discoveredRooms.push_back(newRoom);
            std::cout << "[Network] Discovered public room: " << roomName << " at "
                      << sender->toString() << ":" << tcpPort << "\n";
        }
    }
}

std::vector<Network::RoomInfo> Network::getAvailableRooms() const {
    return m_discoveredRooms;
}

void Network::setOnStonePlaced(std::function<void(int, int)> callback) { m_onStonePlaced = callback; }
void Network::setOnUndoRequest(std::function<void()> callback) { m_onUndoRequest = callback; }
void Network::setOnUndoResponse(std::function<void(bool)> callback) { m_onUndoResponse = callback; }
void Network::setOnRestartRequest(std::function<void()> callback) { m_onRestartRequest = callback; }
void Network::setOnRestartResponse(std::function<void(bool)> callback) { m_onRestartResponse = callback; }
void Network::setOnConnect(std::function<void()> callback) { m_onConnect = callback; }
void Network::setOnDisconnect(std::function<void()> callback) { m_onDisconnect = callback; }
void Network::setOnGameStart(std::function<void()> callback) { m_onGameStart = callback; }

void Network::sendStonePlaced(int row, int col) {
    if (m_status != Status::Connected || !m_isAuthenticated) return;
    sf::Packet packet;
    packet << static_cast<std::uint8_t>(PacketType::StonePlaced) << std::int32_t(row) << std::int32_t(col);
    m_socket.send(packet);
}

void Network::sendUndoRequest() {
    if (m_status != Status::Connected || !m_isAuthenticated) return;
    sf::Packet packet;
    packet << static_cast<std::uint8_t>(PacketType::UndoRequest);
    m_socket.send(packet);
}

void Network::sendUndoResponse(bool accepted) {
    if (m_status != Status::Connected || !m_isAuthenticated) return;
    sf::Packet packet;
    packet << static_cast<std::uint8_t>(accepted ? PacketType::UndoAccept : PacketType::UndoReject);
    m_socket.send(packet);
}

void Network::sendRestartRequest() {
    if (m_status != Status::Connected || !m_isAuthenticated) return;
    sf::Packet packet;
    packet << static_cast<std::uint8_t>(PacketType::RestartRequest);
    m_socket.send(packet);
}

void Network::sendRestartResponse(bool accepted) {
    if (m_status != Status::Connected || !m_isAuthenticated) return;
    sf::Packet packet;
    packet << static_cast<std::uint8_t>(accepted ? PacketType::RestartAccept : PacketType::RestartRequest);
    m_socket.send(packet);
}

void Network::sendGameStart() {
    if (m_status != Status::Connected || !m_isAuthenticated) return;
    sf::Packet packet;
    packet << static_cast<std::uint8_t>(PacketType::GameStart);
    m_socket.send(packet);
}

void Network::handlePacket(sf::Packet& packet) {
    std::uint8_t typeRaw;
    if (!(packet >> typeRaw)) return;

    PacketType type = static_cast<PacketType>(typeRaw);

    switch (type) {
        case PacketType::JoinRequest: {
            if (!m_isHost || m_status != Status::Connected) {
                break;
            }

            std::string code;
            if (!(packet >> code)) {
                break;
            }

            const bool accepted = m_roomCode.empty() ? true : (code == m_roomCode);

            sf::Packet response;
            response << static_cast<std::uint8_t>(accepted ? PacketType::JoinAccept : PacketType::JoinReject);
            m_socket.send(response);

            if (accepted) {
                if (!m_isAuthenticated) {
                    m_isAuthenticated = true;
                    if (m_onConnect) m_onConnect();
                }
            } else {
                std::cout << "[Network] Join rejected: invalid room code.\n";
                m_socket.disconnect();
                m_selector.clear();
                m_isAuthenticated = false;

                m_listener.setBlocking(false);
                if (m_listener.listen(m_tcpPort) == sf::Socket::Status::Done) {
                    m_status = Status::Hosting;
                    m_selector.add(m_listener);
                    std::cout << "[Network] Waiting for new client after reject.\n";
                } else {
                    disconnect();
                }
            }
            break;
        }
        case PacketType::JoinAccept:
            if (!m_isHost && !m_isAuthenticated) {
                m_isAuthenticated = true;
                std::cout << "[Network] Join accepted.\n";
                if (m_onConnect) m_onConnect();
            }
            break;
        case PacketType::JoinReject:
            if (!m_isHost) {
                std::cout << "[Network] Join rejected by host (code required or invalid).\n";
                disconnect();
            }
            break;
        case PacketType::StonePlaced: {
            if (!m_isAuthenticated) {
                break;
            }
            std::int32_t r, c;
            if (packet >> r >> c && m_onStonePlaced) {
                m_onStonePlaced(r, c);
            }
            break;
        }
        case PacketType::UndoRequest:
            if (!m_isAuthenticated) break;
            if (m_onUndoRequest) m_onUndoRequest();
            break;
        case PacketType::UndoAccept:
            if (!m_isAuthenticated) break;
            if (m_onUndoResponse) m_onUndoResponse(true);
            break;
        case PacketType::UndoReject:
            if (!m_isAuthenticated) break;
            if (m_onUndoResponse) m_onUndoResponse(false);
            break;
        case PacketType::RestartRequest:
            if (!m_isAuthenticated) break;
            if (m_onRestartRequest) m_onRestartRequest();
            break;
        case PacketType::RestartAccept:
            if (!m_isAuthenticated) break;
            if (m_onRestartResponse) m_onRestartResponse(true);
            break;
        case PacketType::GameStart:
            if (!m_isAuthenticated) break;
            if (m_onGameStart) m_onGameStart();
            break;
        default:
            break;
    }
}
