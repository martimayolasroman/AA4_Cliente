#include "Client.h"
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Clock.hpp> // Para obtener el tiempo actual

// Definición local de PacketType (DEBE COINCIDIR CON EL SERVIDOR DE SERVICIOS)
// Y también con el Servidor Dedicado para los tipos relevantes
enum PacketType {
    // Cliente -> Servidor de Servicios (TCP)
    C_REQUEST_LOGIN = 1,                // Server.cpp (Servicios) tiene LOGIN = 1
    C_REQUEST_REGISTER = 2,             // Server.cpp (Servicios) tiene REGISTER = 2
    C_REQUEST_MATCHMAKING_FRIENDLY = 3, // Server.cpp (Servicios) NECESITA AÑADIR ESTO con valor 3
    C_MAP_RECEIVED_ACK = 4,             // No implementado aún completamente

    // Cliente <-> Servidor Dedicado (UDP)
    C_PLAYER_INPUT = 5,                 // GameRoom.h tiene GR_C_PLAYER_INPUT = 5 (OK)

    // Servidor de Servicios -> Cliente (TCP)
    S_MAP_DATA = 100,                   // Server.cpp (Servicios) NECESITA AÑADIR ESTO con valor 100
    S_LOGIN_OK = 101,                   // Server.cpp (Servicios) tiene LOGIN_OK = 6 ¡¡¡NO COINCIDE!!!
    S_LOGIN_FAIL = 102,                 // Server.cpp (Servicios) tiene LOGIN_FAIL = 7 ¡¡¡NO COINCIDE!!!
    S_REGISTER_OK = 103,                // Server.cpp (Servicios) tiene REGISTER_OK = 8 ¡¡¡NO COINCIDE!!!
    S_REGISTER_FAIL = 104,              // Server.cpp (Servicios) tiene REGISTER_FAIL = 9 ¡¡¡NO COINCIDE!!!
    S_ADDED_TO_MATCHMAKING_QUEUE = 105, // Server.cpp (Servicios) NECESITA AÑADIR ESTO con valor 105
    S_MATCH_FOUND = 106,                // Server.cpp (Servicios) NECESITA AÑADIR ESTO con valor 106

    // Servidor Dedicado -> Cliente (UDP)
    S_GAME_STATE = 107,                 // GameRoom.h tiene GR_S_GAME_STATE = 107 (OK)

    // Servidor de Servicios -> Cliente (TCP)
    S_ERROR_GENERAL = 108,              // Server.cpp (Servicios) podría usar esto con valor 108
    UNKNOWN = 255
};

// Reloj global para timestamps de paquetes UDP (si no vienen del servidor)
sf::Clock udpPacketReceiveClock;


inline sf::Packet& operator >> (sf::Packet& packet, PacketType& tipo) {
    int temp;
    packet >> temp;
    tipo = static_cast<PacketType>(temp);
    return packet;
}

inline sf::Packet& operator << (sf::Packet& packet, PacketType tipo) {
    packet << static_cast<int>(tipo);
    return packet;
}

Client* Client::instanceClient = nullptr;

Client::Client() :connected(false), myPort(0),
m_mapReceived(false),
loginOk(false), RegisterOk(false),
m_hasLoginResponse(false), m_hasRegisterResponse(false),
m_isInMatchmakingQueue(false), m_matchFound(false),
m_gameServerUdpPort(0), m_myUdpPortForGame(0),
m_isConnectedToGameServer(false), m_amIPlayerOne(false),
m_lastServerConfirmedMyPlayerPosition(-1.f, -1.f),
m_newServerStateReceived(false) {
    mapFilePath = "Data/map.txt";
    myPlayerPosition = { -1.f, -1.f };
}

Client* Client::getInstance() {
    if (instanceClient == nullptr) {
        instanceClient = new Client();
    }
    return instanceClient;
}

bool Client::connectToServer(unsigned short port) {
    if (Clientsocket.connect(SERVER_IP, port) != sf::Socket::Status::Done) {
        std::cerr << "Error al conectar al servidor de servicios" << std::endl;
        connected = false;
        return false;
    }
    Clientsocket.setBlocking(false);
    connected = true;
    myPort = Clientsocket.getLocalPort();
    std::cout << "Conectado al servidor de servicios correctamente en el puerto local " << myPort << std::endl;
    udpPacketReceiveClock.restart();
    return true;
}

void Client::run() {
    if (!connected) return;

    sf::Packet incomingTcpPacket;
    sf::Socket::Status status = Clientsocket.receive(incomingTcpPacket);

    if (status == sf::Socket::Status::Done) {
        processPacket(incomingTcpPacket);
    }
    else if (status == sf::Socket::Status::Disconnected) {
        std::cout << "[Client] Desconectado del servidor de servicios." << std::endl;
        connected = false;
        m_isInMatchmakingQueue = false;
        m_matchFound = false;
        m_isConnectedToGameServer = false;
    }
    else if (status == sf::Socket::Status::Error) {
        std::cerr << "[Client] Error de socket TCP al recibir del servidor de servicios." << std::endl;
    }
}

bool Client::loginAction(std::string nick, std::string pass) {
    if (!connected) {
        std::cerr << "[CLIENT] No conectado al servidor para login." << std::endl;
        return false;
    }
    sf::Packet login_packet;
    login_packet << static_cast<int>(PacketType::C_REQUEST_LOGIN) << nick << pass;
    std::cout << "[CLIENT] Enviando LOGIN: " << nick << std::endl;
    clientNick = nick;
    m_hasLoginResponse = false;
    return sendPacket(login_packet);
}

bool Client::RegisterAction(std::string nick, std::string pass) {
    if (!connected) {
        std::cerr << "[CLIENT] No conectado al servidor para register." << std::endl;
        return false;
    }
    sf::Packet register_packet;
    register_packet << static_cast<int>(PacketType::C_REQUEST_REGISTER) << nick << pass;
    std::cout << "[CLIENT] Enviando REGISTER: " << nick << std::endl;
    m_hasRegisterResponse = false;
    return sendPacket(register_packet);
}

bool Client::sendPacket(sf::Packet& packet_to_send) {
    if (!connected) return false;
    return Clientsocket.send(packet_to_send) == sf::Socket::Status::Done;
}

void Client::processPacket(sf::Packet tcp_packet) {
    PacketType packetType;
    if (!(tcp_packet >> packetType)) {
        std::cerr << "[CLIENT] Error al extraer PacketType del paquete TCP." << std::endl;
        return;
    }
    // std::cout << "[CLIENT DEBUG] Client::processPacket() - Procesando tipo TCP: " << static_cast<int>(packetType) << std::endl;

    switch (packetType) {
    case S_MAP_DATA: {
        std::string receivedMapContent;
        if (tcp_packet >> receivedMapContent) {
            m_mapData = receivedMapContent;
            if (ReadWriteMapReceived(m_mapData)) {
                m_mapReceived = true;
                std::cout << "[CLIENT] Mapa recibido y guardado. Tamaño: " << m_mapData.length() << " bytes." << std::endl;
            }
            else {
                m_mapReceived = false;
                std::cerr << "[CLIENT] Mapa recibido pero NO PUDO SER GUARDADO." << std::endl;
            }
        }
        else {
            std::cerr << "[CLIENT] Error leyendo contenido del mapa del paquete S_MAP_DATA." << std::endl;
        }
        break;
    }
                   // ¡¡¡ATENCIÓN!!! CAMBIA ESTOS VALORES EN EL ENUM LOCAL O EN EL SERVIDOR PARA QUE COINCIDAN
    case S_LOGIN_OK: // Client espera 101, Server(Servicios) envía 6.
        std::cout << "[CLIENT] Recibido S_LOGIN_OK." << std::endl;
        loginOk = true;
        m_hasLoginResponse = true;
        break;
    case S_LOGIN_FAIL: // Client espera 102, Server(Servicios) envía 7.
        std::cout << "[CLIENT] Recibido S_LOGIN_FAIL." << std::endl;
        loginOk = false;
        m_hasLoginResponse = true;
        break;
    case S_REGISTER_OK: // Client espera 103, Server(Servicios) envía 8.
        std::cout << "[CLIENT] Recibido S_REGISTER_OK." << std::endl;
        RegisterOk = true;
        m_hasRegisterResponse = true;
        break;
    case S_REGISTER_FAIL: // Client espera 104, Server(Servicios) envía 9.
        std::cout << "[CLIENT] Recibido S_REGISTER_FAIL." << std::endl;
        RegisterOk = false;
        m_hasRegisterResponse = true;
        break;
    case S_ADDED_TO_MATCHMAKING_QUEUE:
        std::cout << "[CLIENT] Recibido S_ADDED_TO_MATCHMAKING_QUEUE." << std::endl;
        m_isInMatchmakingQueue = true;
        break;
    case S_MATCH_FOUND: {
        std::cout << "[CLIENT] Recibido S_MATCH_FOUND." << std::endl;
        std::string gameServerIpStr;
        unsigned short gameServerUdpPortVal;
        unsigned short myUdpPortForGameVal;
        bool isPlayerOneVal;

        if (tcp_packet >> gameServerIpStr >> gameServerUdpPortVal >> myUdpPortForGameVal >> isPlayerOneVal) {
            m_gameServerIp = gameServerIpStr;
            m_gameServerUdpPort = gameServerUdpPortVal;
            m_myUdpPortForGame = myUdpPortForGameVal;
            m_amIPlayerOne = isPlayerOneVal;
            m_matchFound = true;
            m_isInMatchmakingQueue = false;

            std::cout << "[CLIENT] Partida encontrada! GameServer en "
                << m_gameServerIp << ":" << m_gameServerUdpPort
                << ". Usare mi puerto UDP local: " << m_myUdpPortForGame
                << ". Soy Jugador Uno: " << (m_amIPlayerOne ? "Si" : "No") << std::endl;

            opponentInterpolationState = OpponentInterpolationState();
            myPlayerPosition = { -1.f, -1.f };

            connectToGameServerUDP();
        }
        else {
            std::cerr << "[CLIENT] Error leyendo datos de S_MATCH_FOUND." << std::endl;
            m_matchFound = false;
        }
        break;
    }
    case S_ERROR_GENERAL: {
        std::cout << "[CLIENT] Recibido S_ERROR_GENERAL." << std::endl;
        std::string errorMessage;
        if (tcp_packet >> errorMessage) {
            std::cerr << "[CLIENT] Error general del servidor: " << errorMessage << std::endl;
        }
        else {
            std::cerr << "[CLIENT] Error general del servidor (sin mensaje)." << std::endl;
        }
        m_isInMatchmakingQueue = false;
        m_matchFound = false;
        break;
    }
    default:
        std::cerr << "[CLIENT] Tipo de paquete TCP desconocido o no manejado: " << static_cast<int>(packetType) << std::endl;
        break;
    }
}

std::string Client::getNickname() {
    return clientNick;
}

bool Client::requestMatchmakingFriendly() {
    if (!connected) {
        std::cerr << "[CLIENT] No conectado al servidor para matchmaking." << std::endl;
        return false;
    }
    sf::Packet pkt;
    pkt << static_cast<int>(PacketType::C_REQUEST_MATCHMAKING_FRIENDLY);
    if (Clientsocket.send(pkt) == sf::Socket::Status::Done) {
        std::cout << "[CLIENT] Solicitud de matchmaking enviada." << std::endl;
        return true;
    }
    std::cerr << "[CLIENT] Error enviando solicitud de matchmaking." << std::endl;
    return false;
}

void Client::connectToGameServerUDP() {
    if (m_isConnectedToGameServer) {
        gameUdpSocket.unbind();
    }

    if (gameUdpSocket.bind(m_myUdpPortForGame) != sf::Socket::Status::Done) {
        std::cerr << "[Client-UDP] Error al enlazar socket UDP al puerto local: " << m_myUdpPortForGame << std::endl;
        m_isConnectedToGameServer = false;
        m_matchFound = false;
        return;
    }
    gameUdpSocket.setBlocking(false);
    m_isConnectedToGameServer = true;
    std::cout << "[Client-UDP] Socket UDP enlazado al puerto local " << m_myUdpPortForGame
        << ". Listo para comunicarse con GameServer en "
        << m_gameServerIp << ":" << m_gameServerUdpPort << std::endl;

    udpPacketReceiveClock.restart();
    opponentInterpolationState = OpponentInterpolationState();
}

void Client::receiveAndProcessGameData() {
    if (!m_isConnectedToGameServer) return;

    sf::Packet gameUdpPacket;
    std::optional<sf::IpAddress> senderIpOpt;
    unsigned short senderPort;

    while (gameUdpSocket.receive(gameUdpPacket, senderIpOpt, senderPort) == sf::Socket::Status::Done) {
        if (senderIpOpt.has_value()) {
            sf::IpAddress senderIp = senderIpOpt.value();
            if (senderIp.toString() == m_gameServerIp && senderPort == m_gameServerUdpPort) {
                processGamePacket(gameUdpPacket);
            }
        }
        gameUdpPacket.clear();
    }
}

void Client::processGamePacket(sf::Packet& udp_packet) {
    int rawPacketType;
    if (!(udp_packet >> rawPacketType)) {
        std::cerr << "[Client-UDP] Error leyendo tipo de paquete del GameServer." << std::endl;
        return;
    }

    if (rawPacketType == static_cast<int>(S_GAME_STATE)) {
        float p1x, p1y, p2x, p2y;
        int p1h, p1l, p2h, p2l;

        sf::Time receptionTime = udpPacketReceiveClock.getElapsedTime();

        if (udp_packet >> p1x >> p1y >> p1h >> p1l >> p2x >> p2y >> p2h >> p2l) {
            sf::Vector2f serverMyPos;
            sf::Vector2f opponentPos;

            if (m_amIPlayerOne) {
                serverMyPos = { p1x, p1y }; myPlayerHealth = p1h; myPlayerLives = p1l;
                opponentPos = { p2x, p2y }; opponentPlayerHealth = p2h; opponentPlayerLives = p2l;
            }
            else {
                serverMyPos = { p2x, p2y }; myPlayerHealth = p2h; myPlayerLives = p2l;
                opponentPos = { p1x, p1y }; opponentPlayerHealth = p1h; opponentPlayerLives = p1l;
            }

            // Log para verificar
            // std::cout << "[Client::processGamePacket] Received server state. My server pos: ("
            //           << serverMyPos.x << "," << serverMyPos.y << ")" << std::endl;

            m_lastServerConfirmedMyPlayerPosition = serverMyPos;
            m_newServerStateReceived = true; // Indicar a Game.cpp que hay un nuevo estado

            // Actualizar el estado del oponente para la interpolación
            if (!opponentInterpolationState.hasReceivedFirstUpdate) {
                opponentInterpolationState.previousPosition = opponentPos;
                opponentInterpolationState.previousTimestamp = receptionTime;
                opponentInterpolationState.currentPosition = opponentPos;
                opponentInterpolationState.currentTimestamp = receptionTime;
                opponentInterpolationState.hasReceivedFirstUpdate = true;
            }
            else {
                if (receptionTime > opponentInterpolationState.currentTimestamp) {
                    opponentInterpolationState.previousPosition = opponentInterpolationState.currentPosition;
                    opponentInterpolationState.previousTimestamp = opponentInterpolationState.currentTimestamp;
                    opponentInterpolationState.currentPosition = opponentPos;
                    opponentInterpolationState.currentTimestamp = receptionTime;
                    opponentInterpolationState.hasReceivedEnoughUpdatesForInterpolation = true;
                }
            }
        }
        else {
            std::cerr << "[Client-UDP] Error deserializando S_GAME_STATE." << std::endl;
        }
    }
    else {
        std::cerr << "[Client-UDP] Tipo de GamePacket desconocido (" << rawPacketType << ") desde GameServer." << std::endl;
    }
}

void Client::sendPlayerInput(float moveDir, bool wantsToShoot) {
    if (!m_isConnectedToGameServer) return;

    sf::Packet inputPacket;
    inputPacket << static_cast<int>(PacketType::C_PLAYER_INPUT) << moveDir << wantsToShoot;

    std::optional<sf::IpAddress> gameServerResolvedIpOpt = sf::IpAddress::resolve(m_gameServerIp);
    if (!gameServerResolvedIpOpt || gameServerResolvedIpOpt.value() == sf::IpAddress::Any) {
        std::cerr << "[Client-UDP] No se pudo resolver la IP del GameServer de forma valida o es Any: " << m_gameServerIp << std::endl;
        return;
    }

    if (gameUdpSocket.send(inputPacket, gameServerResolvedIpOpt.value(), m_gameServerUdpPort) == sf::Socket::Status::Done) {
        // Añadir al historial (incluso si no hay re-simulación, es bueno tenerlo por si acaso o para depurar)
        ClientInputRecord record;
        record.moveDirection = moveDir;
        record.wantsToShoot = wantsToShoot;
        addSentInputToHistory(record);
    }
    else {
        std::cerr << "[Client-UDP] Error enviando paquete de input." << std::endl;
    }
}

void Client::addSentInputToHistory(const ClientInputRecord& input) {
    m_pendingInputs.push_back(input);
    const size_t MAX_PENDING_INPUTS = 200; // Límite para evitar crecimiento indefinido
    if (m_pendingInputs.size() > MAX_PENDING_INPUTS) {
        m_pendingInputs.pop_front();
    }
}

/*
void Client::clearOldPendingInputs(unsigned int upToSequenceNumber) {
    while (!m_pendingInputs.empty() && m_pendingInputs.front().sequenceNumber <= upToSequenceNumber) {
        m_pendingInputs.pop_front();
    }
    m_lastAckedInputSequenceByServer = upToSequenceNumber;
}
*/


bool Client::ReadWriteMapReceived(std::string& receivedMapContent) {
    std::ofstream mapFile(mapFilePath, std::ios::out | std::ios::trunc);

    if (mapFile.is_open()) {
        mapFile << receivedMapContent;
        mapFile.close();
        return true;
    }
    else {
        std::cerr << "[Client] Error: No se pudo abrir el archivo para guardar el mapa: " << mapFilePath << std::endl;
        return false;
    }
}