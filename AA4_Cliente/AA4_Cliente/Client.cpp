#include "Client.h"
#include <SFML/System/Sleep.hpp> // Para sf::sleep si fuera necesario para depuración

// Definición local de PacketType
enum PacketType {
    C_REQUEST_LOGIN = 1,
    C_REQUEST_REGISTER = 2,
    C_REQUEST_MATCHMAKING_FRIENDLY = 3,
    C_MAP_RECEIVED_ACK = 4,
    C_PLAYER_INPUT = 5,
    S_MAP_DATA = 100,
    S_LOGIN_OK = 101,
    S_LOGIN_FAIL = 102,
    S_REGISTER_OK = 103,
    S_REGISTER_FAIL = 104,
    S_ADDED_TO_MATCHMAKING_QUEUE = 105,
    S_MATCH_FOUND = 106,
    S_GAME_STATE = 107,
    S_ERROR_GENERAL = 108,
    UNKNOWN = 255
};

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
m_mapReceived(true), // Asumimos que el mapa está localmente
loginOk(false), RegisterOk(false),
m_hasLoginResponse(false), m_hasRegisterResponse(false),
m_isInMatchmakingQueue(false), m_matchFound(false),
m_gameServerUdpPort(0), m_myUdpPortForGame(0),
m_isConnectedToGameServer(false), m_amIPlayerOne(false) {
    mapFilePath = "Data/map.txt";
    myPlayerPosition = { -1.f, -1.f };
    opponentPlayerPosition = { -1.f, -1.f };
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
    return true;
}

void Client::run() {
    if (!connected) return;

    sf::Packet incomingPacket;
    // std::cout << "[CLIENT DEBUG] Client::run() - Intentando recibir paquete TCP." << std::endl; // DEBUG
    sf::Socket::Status status = Clientsocket.receive(incomingPacket);
    // std::cout << "[CLIENT DEBUG] Client::run() - Estado de receive: " << static_cast<int>(status) << std::endl; // DEBUG


    if (status == sf::Socket::Status::Done) {
        // std::cout << "[CLIENT DEBUG] Client::run() - Paquete TCP recibido, llamando a processPacket." << std::endl; // DEBUG
        processPacket(incomingPacket);
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
    sf::Packet login_packet; // Usar un paquete local para esta acción
    login_packet << C_REQUEST_LOGIN << nick << pass;
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
    sf::Packet register_packet; // Usar un paquete local
    register_packet << C_REQUEST_REGISTER << nick << pass;
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
    std::cout << "[CLIENT DEBUG] Client::processPacket() - Procesando tipo: " << static_cast<int>(packetType) << std::endl; // DEBUG

    switch (packetType) {

    case S_MAP_DATA: {

        std::string receivedMapContent;

            if (tcp_packet >> receivedMapContent) {
            m_mapData = receivedMapContent;
            m_mapReceived = true;
            std::cout << "[CLIENT] Mapa recibido del servidor. Tamaño: " << m_mapData.length() << " bytes." << std::endl;
            ReadWriteMapReceived(m_mapData);
             std::cout << receivedMapContent << std::endl;
            }
         else {
            std::cout << receivedMapContent << std::endl;
            std::cerr << "[CLIENT] Error leyendo contenido del mapa del paquete S_MAP_DATA." << std::endl;
            }
        
        }
        break;

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
    pkt << C_REQUEST_MATCHMAKING_FRIENDLY;
    if (Clientsocket.send(pkt) == sf::Socket::Status::Done) {
        std::cout << "[CLIENT] Solicitud de matchmaking enviada." << std::endl;
        return true;
    }
    std::cerr << "[CLIENT] Error enviando solicitud de matchmaking." << std::endl;
    return false;
}

void Client::connectToGameServerUDP() {
    // Si ya estaba conectado a un game server anterior, desbindear (aunque raro en este flujo)
    if (m_isConnectedToGameServer) {
        gameUdpSocket.unbind();
        m_isConnectedToGameServer = false;
    }

    if (gameUdpSocket.bind(m_myUdpPortForGame) != sf::Socket::Status::Done) {
        std::cerr << "[Client-UDP] Error al enlazar socket UDP al puerto local: " << m_myUdpPortForGame << std::endl;
        m_isConnectedToGameServer = false;
        return;
    }
    gameUdpSocket.setBlocking(false);
    m_isConnectedToGameServer = true;
    std::cout << "[Client-UDP] Socket UDP enlazado al puerto local " << m_myUdpPortForGame
        << ". Listo para comunicarse con GameServer en "
        << m_gameServerIp << ":" << m_gameServerUdpPort << std::endl;
    myPlayerPosition = { -1.f, -1.f };
    opponentPlayerPosition = { -1.f, -1.f };
}

void Client::receiveAndProcessGameData() {
    if (!m_isConnectedToGameServer) return;

    sf::Packet gamePacket;
    std::optional<sf::IpAddress> senderIpOpt;
    unsigned short senderPort;

    // Bucle para procesar todos los paquetes UDP pendientes en este frame
    while (gameUdpSocket.receive(gamePacket, senderIpOpt, senderPort) == sf::Socket::Status::Done) {
        if (senderIpOpt.has_value()) {
            sf::IpAddress senderIp = senderIpOpt.value();
            // Verificar que la IP y el puerto coinciden con el GameServer esperado
            if (senderIp.toString() == m_gameServerIp && senderPort == m_gameServerUdpPort) {
                processGamePacket(gamePacket);
            }
            else {
                // Ignorar paquetes de fuentes inesperadas
                // std::cout << "[Client-UDP] Paquete UDP de fuente inesperada: "
                //           << senderIp.toString() << ":" << senderPort << std::endl;
            }
        }
        gamePacket.clear(); // Limpiar para la próxima recepción
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

        if (udp_packet >> p1x >> p1y >> p1h >> p1l >> p2x >> p2y >> p2h >> p2l) {
            if (m_amIPlayerOne) {
                //std::cout << "Posicion que recivo para el Jugador 111111: " << p2x << " , " << p2y << std::endl;

                myPlayerPosition = { p1x, p1y };
                myPlayerHealth = p1h;
                myPlayerLives = p1l;
                opponentPlayerPosition = { p2x, p2y };
                opponentPlayerHealth = p2h;
                opponentPlayerLives = p2l;
            }
            else {
                //std::cout << "Posicion que recivo para el Jugador 222222: " << p2x<< " , " << p2y<< std::endl;
                myPlayerPosition = { p2x, p2y };
                myPlayerHealth = p2h;
                myPlayerLives = p2l;
                opponentPlayerPosition = { p1x, p1y };
                opponentPlayerHealth = p1h;
                opponentPlayerLives = p1l;
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

    std::cout << "[CLIENT " << clientNick << " SENDING_INPUT] moveDir: " << moveDir << ", shoot: " << wantsToShoot << std::endl;


    sf::Packet inputPacket;
    inputPacket << static_cast<int>(C_PLAYER_INPUT) << moveDir << wantsToShoot;

    std::optional<sf::IpAddress> gameServerResolvedIpOpt = sf::IpAddress::resolve(m_gameServerIp);
    // Comprobación robusta de la IP resuelta
    if (!gameServerResolvedIpOpt ||
        gameServerResolvedIpOpt.value() == sf::IpAddress::Any) { // None es más estándar para fallo de resolve
        std::cerr << "[Client-UDP] No se pudo resolver la IP del GameServer de forma valida: " << m_gameServerIp << std::endl;
        return;
    }

    if (gameUdpSocket.send(inputPacket, gameServerResolvedIpOpt.value(), m_gameServerUdpPort) != sf::Socket::Status::Done) {
        std::cerr << "[Client-UDP] Error enviando paquete de input." << std::endl;
    }
}

bool Client::ReadWriteMapReceived(std::string& receivedMapContent)
{
    std::ofstream mapFile(mapFilePath); // Abre el archivo para escritura (sobrescribe si existe)


    if (mapFile.is_open()) {
        mapFile << receivedMapContent; // Escribe el string del mapa en el archivo
        mapFile.close();
        std::cout << "[Client] Mapa guardado correctamente en: " << mapFilePath << std::endl;
        return true;
    }
    else {
        std::cerr << "[Client] Error: No se pudo abrir el archivo para guardar el mapa: " << mapFilePath << std::endl;
        return false;
    }
   
}