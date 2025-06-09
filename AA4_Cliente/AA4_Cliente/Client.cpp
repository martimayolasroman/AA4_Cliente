#include "Client.h"
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Clock.hpp>
#include <fstream>  

 // Y también con el Servidor Dedicado para los tipos relevantes
enum PacketType {
    C_REQUEST_LOGIN = 1, C_REQUEST_REGISTER = 2, C_REQUEST_MATCHMAKING_FRIENDLY = 3, C_MAP_RECEIVED_ACK = 4,
    C_PLAYER_INPUT = 5,
    S_MAP_DATA = 100, S_LOGIN_OK = 101, S_LOGIN_FAIL = 102, S_REGISTER_OK = 103, S_REGISTER_FAIL = 104,
    S_ADDED_TO_MATCHMAKING_QUEUE = 105, S_MATCH_FOUND = 106,
    S_GAME_STATE = 107, C_PLAYER_TAUNT = 108, S_OPPONENT_TAUNT = 109,
    S_ERROR_GENERAL = 110,
    UNKNOWN = 255
};

sf::Clock udpPacketReceiveClock;

// Sobrecarga del operador >> para extraer un PacketType de un sf::Packet.
inline sf::Packet& operator >> (sf::Packet& packet, PacketType& tipo) {
    int temp; packet >> temp; tipo = static_cast<PacketType>(temp); return packet;
}

// Sobrecarga del operador << para insertar un PacketType en un sf::Packet.
inline sf::Packet& operator << (sf::Packet& packet, PacketType tipo) {
    packet << static_cast<int>(tipo); return packet;
}

Client* Client::instanceClient = nullptr;

Client::Client() :
    connected(false), myPort(0),
    m_mapReceived(false),
    loginOk(false), RegisterOk(false),
    m_hasLoginResponse(false), m_hasRegisterResponse(false),
    m_isInMatchmakingQueue(false), m_matchFound(false),
    m_gameServerUdpPort(0), m_myUdpPortForGame(0),
    m_isConnectedToGameServer(false), m_amIPlayerOne(false),
    m_lastServerConfirmedMyPlayerPosition(-1.f, -1.f),
    m_newServerStateReceived(false),
    m_myPlayerOnGround(false),
    m_myPlayerServerVelocity(0.f, 0.f),
    m_soundLoaded(false)
{
    mapFilePath = "Data/map.txt";
    //loadSounds();  
}

// getInstance: Devuelve la única instancia del cliente (patrón Singleton).
Client* Client::getInstance() {
    if (instanceClient == nullptr) {
        instanceClient = new Client();
    }
    return instanceClient;
}

// connectToServer: Intenta conectar el cliente al servidor de servicios usando TCP.
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
    udpPacketReceiveClock.restart(); // Reinicia el reloj para medir el tiempo de los paquetes UDP.
    return true;
}

// run: Procesa los paquetes TCP entrantes del servidor de servicios.
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

// loginAction: Envía una solicitud de login al servidor con un nombre de usuario y contraseña.
bool Client::loginAction(std::string nick, std::string pass) {
    if (!connected) {
        std::cerr << "[CLIENT] No conectado al servidor para login." << std::endl;
        return false;
    }
    sf::Packet login_packet;
    login_packet << static_cast<int>(PacketType::C_REQUEST_LOGIN) << nick << pass;
    std::cout << "[CLIENT] Enviando LOGIN: " << nick << std::endl;
    clientNick = nick;
    m_hasLoginResponse = false; // Indica que se está esperando una respuesta de login.
    return sendPacket(login_packet);
}

// RegisterAction: Envía una solicitud de registro al servidor con un nombre de usuario y contraseña.
bool Client::RegisterAction(std::string nick, std::string pass) {
    if (!connected) {
        std::cerr << "[CLIENT] No conectado al servidor para register." << std::endl;
        return false;
    }
    sf::Packet register_packet;
    register_packet << static_cast<int>(PacketType::C_REQUEST_REGISTER) << nick << pass;
    std::cout << "[CLIENT] Enviando REGISTER: " << nick << std::endl;
    m_hasRegisterResponse = false; // Indica que se está esperando una respuesta de registro.
    return sendPacket(register_packet);
}

// sendPacket: Envía un paquete TCP al servidor de servicios.
bool Client::sendPacket(sf::Packet& packet_to_send) {
    if (!connected) return false;
    return Clientsocket.send(packet_to_send) == sf::Socket::Status::Done;
}

// processPacket: Procesa los diferentes tipos de paquetes TCP recibidos del servidor de servicios.
void Client::processPacket(sf::Packet tcp_packet) {
    PacketType packetType;
    if (!(tcp_packet >> packetType)) {
        std::cerr << "[CLIENT] Error al extraer PacketType del paquete TCP." << std::endl;
        return;
    }

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
    case S_LOGIN_OK:
        std::cout << "[CLIENT] Recibido S_LOGIN_OK." << std::endl;
        loginOk = true;
        m_hasLoginResponse = true;
        break;
    case S_LOGIN_FAIL:
        std::cout << "[CLIENT] Recibido S_LOGIN_FAIL." << std::endl;
        loginOk = false;
        m_hasLoginResponse = true;
        break;
    case S_REGISTER_OK:
        std::cout << "[CLIENT] Recibido S_REGISTER_OK." << std::endl;
        RegisterOk = true;
        m_hasRegisterResponse = true;
        break;
    case S_REGISTER_FAIL:
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

        // Deserializa la información del servidor de juego y del puerto UDP  
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

            opponentInterpolationState = OpponentInterpolationState(); // Inicializa el estado para la interpolación del oponente.
            connectToGameServerUDP(); // Intenta conectar al servidor de juego por UDP.
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

// getNickname: Devuelve el nickname del cliente.
std::string Client::getNickname() {
    return clientNick;
}

// requestMatchmakingFriendly: Envía una solicitud al servidor para entrar en la cola de matchmaking.
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

// connectToGameServerUDP: Intenta enlazar el socket UDP del cliente y conectar al servidor de juego.
void Client::connectToGameServerUDP() {
    //if (m_isConnectedToGameServer,m_gameServerIp) {
    //    gameUdpSocket.unbind(); // Desenlaza el socket si ya estaba conectado.
    //}

    if (gameUdpSocket.bind(m_myUdpPortForGame, sf::IpAddress::resolve(m_gameServerIp).value()) != sf::Socket::Status::Done) {
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

    udpPacketReceiveClock.restart(); // Reinicia el reloj para los paquetes de juego.
    opponentInterpolationState = OpponentInterpolationState(); // Restablece el estado de interpolación del oponente.
    m_opponentBulletStates.clear(); // Limpia el estado de las balas del oponente.
}

// receiveAndProcessGameData: Recibe y procesa los paquetes UDP entrantes del servidor de juego.
void Client::receiveAndProcessGameData() {
    if (!m_isConnectedToGameServer) return;

    sf::Packet gameUdpPacket;
    std::optional<sf::IpAddress> senderIpOpt;
    unsigned short senderPort;

    // Recibe paquetes hasta que no haya más pendientes o ocurra un error.
    while (gameUdpSocket.receive(gameUdpPacket, senderIpOpt, senderPort) == sf::Socket::Status::Done) {
        if (senderIpOpt.has_value()) {
            sf::IpAddress senderIp = senderIpOpt.value();
            // Verifica que el paquete provenga del servidor de juego esperado.
            if (senderIp.toString() == m_gameServerIp && senderPort == m_gameServerUdpPort) {
                processGamePacket(gameUdpPacket);
            }
        }
        gameUdpPacket.clear(); // Limpia el paquete para la siguiente recepción.
    }
}

// processGamePacket: Procesa los diferentes tipos de paquetes de juego UDP recibidos del servidor de juego.
void Client::processGamePacket(sf::Packet& udp_packet) {
    int rawPacketType;
    if (!(udp_packet >> rawPacketType)) {
        std::cerr << "[Client-UDP] Error leyendo tipo de paquete del GameServer." << std::endl;
        return;
    }

    if (rawPacketType == static_cast<int>(S_GAME_STATE)) {
        float p1x, p1y, p2x, p2y;
        int p1h, p1l, p2h, p2l;
        float p1vx, p1vy, p2vx, p2vy;
        bool p1og, p2og;

        sf::Time receptionTime = udpPacketReceiveClock.getElapsedTime();

        // Deserializa el estado de ambos jugadores (posición, salud, vidas, velocidad, en suelo).
        if (udp_packet >> p1x >> p1y >> p1h >> p1l >> p1vx >> p1vy >> p1og
            >> p2x >> p2y >> p2h >> p2l >> p2vx >> p2vy >> p2og) {
            sf::Vector2f serverMyPos;
            sf::Vector2f opponentPos;

            // Asigna los datos a la posición del jugador local y del oponente según si es el jugador 1 o 2.
            if (m_amIPlayerOne) {
                serverMyPos = { p1x, p1y };
                myPlayerHealth = p1h;
                myPlayerLives = p1l;
                m_myPlayerServerVelocity = { p1vx, p1vy };
                m_myPlayerOnGround = p1og;

                opponentPos = { p2x, p2y };
                opponentPlayerHealth = p2h;
                opponentPlayerLives = p2l;

            }
            else { // Soy jugador dos
                serverMyPos = { p2x, p2y };
                myPlayerHealth = p2h;
                myPlayerLives = p2l;
                m_myPlayerServerVelocity = { p2vx, p2vy };
                m_myPlayerOnGround = p2og;

                opponentPos = { p1x, p1y };
                opponentPlayerHealth = p1h;
                opponentPlayerLives = p1l;
            }

            m_lastServerConfirmedMyPlayerPosition = serverMyPos; // Almacena la última posición confirmada por el servidor del jugador local.
            m_newServerStateReceived = true; // Indica que se ha recibido un nuevo estado del servidor.

            // Actualiza el estado de interpolación del oponente.
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

            // Procesa el estado de las balas del servidor.
            int numBullets;
            if (udp_packet >> numBullets) {
                m_opponentBulletStates.clear(); // Limpia la lista actual de balas del oponente.
                for (int i = 0; i < numBullets; ++i) {
                    float bx, by, bvx, bvy, br;
                    bool bactive;
                    int bownerId;
                    if (udp_packet >> bx >> by >> bvx >> bvy >> br >> bactive >> bownerId) {
                        // Solo añadir a la lista si la bala pertenece al otro jugador.
                        if ((m_amIPlayerOne && bownerId == 2) || (!m_amIPlayerOne && bownerId == 1)) {
                            m_opponentBulletStates.emplace_back(
                                sf::Vector2f(bx, by), sf::Vector2f(bvx, bvy), br, bactive, bownerId, receptionTime
                            );
                        }
                    }
                    else {
                        std::cerr << "[Client-UDP] Error deserializando datos de una bala del paquete S_GAME_STATE." << std::endl;
                        break;
                    }
                }
            }


        }
        else {
            std::cerr << "[Client-UDP] Error deserializando S_GAME_STATE (faltan campos de velocidad/onGround?)." << std::endl;
        }
    }
    else {
        std::cerr << "[Client-UDP] Tipo de GamePacket desconocido (" << rawPacketType << ") desde GameServer." << std::endl;
    }
}

// addSentInputToHistory: Añade un registro de input del cliente a un historial limitado.
void Client::addSentInputToHistory(const ClientInputRecord& input) {
    m_pendingInputs.push_back(input);
    const size_t MAX_PENDING_INPUTS = 200;
    if (m_pendingInputs.size() > MAX_PENDING_INPUTS) {
        m_pendingInputs.pop_front(); // Elimina el input más antiguo si se pasa del límite.
    }
}

// sendPlayerInput: Envía las acciones del jugador (movimiento, disparo, salto) al servidor de juego vía UDP.
void Client::sendPlayerInput(float moveDir, bool wantsToShoot, bool jumpRequestedThisTick) {
    if (!m_isConnectedToGameServer) return;

    sf::Packet inputPacket;
    inputPacket << static_cast<int>(PacketType::C_PLAYER_INPUT) << moveDir << wantsToShoot << jumpRequestedThisTick;

    std::optional<sf::IpAddress> gameServerResolvedIpOpt = sf::IpAddress::resolve(m_gameServerIp);
    if (!gameServerResolvedIpOpt || gameServerResolvedIpOpt.value() == sf::IpAddress::Any) {
        std::cerr << "[Client-UDP] No se pudo resolver la IP del GameServer de forma valida o es Any: " << m_gameServerIp << std::endl;
        return;
    }

    // Envía el paquete y, si tiene éxito, añade el input al historial.
    if (gameUdpSocket.send(inputPacket, gameServerResolvedIpOpt.value(), m_gameServerUdpPort) == sf::Socket::Status::Done) {
        ClientInputRecord record;
        record.moveDirection = moveDir;
        record.wantsToShoot = wantsToShoot;
        record.jumpRequested = jumpRequestedThisTick;
        addSentInputToHistory(record);
    }
    else {
        std::cerr << "[Client-UDP] Error enviando paquete de input." << std::endl;
    }
}

// ReadWriteMapReceived: Guarda el contenido del mapa recibido en un archivo local.
bool Client::ReadWriteMapReceived(std::string& receivedMapContent) {
    std::ofstream mapFile(mapFilePath, std::ios::out | std::ios::trunc); // Abre el archivo para escritura, truncándolo si ya existe.

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