#include "Client.h"



enum  PacketType
{
    // Cliente a Servidor
    C_REQUEST_LOGIN = 1,
    C_REQUEST_REGISTER = 2,
    C_REQUEST_MATCHMAKING_FRIENDLY = 3,
    C_MAP_RECEIVED_ACK = 4,
    C_PLAYER_INPUT = 5,

    // Servidor a Cliente
    S_MAP_DATA = 100,
    S_LOGIN_OK = 101,
    S_LOGIN_FAIL = 102,
    S_REGISTER_OK = 103,
    S_REGISTER_FAIL = 104,
    S_ADDED_TO_MATCHMAKING_QUEUE = 105,
    S_MATCH_FOUND = 106,
    S_GAME_STATE = 107,
    S_ERROR_GENERAL = 108, // Para errores genéricos


    UNKNOWN = 255

  
   
};


enum  PacketTypeP2P
{
    MOVE_PIECE,
    TURN_END,
    GAME_OVER,
  
    
};


Client* Client::instanceClient = nullptr;


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


void Client::connectToGameServerUDP()
{

    // Clientsocket.disconnect();

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

    // Inicializar posiciones (o esperar al primer S_GAME_STATE)
    myPlayerPosition = sf::Vector2f(100, 100); // Posición por defecto o de spawn
    opponentPlayerPosition = sf::Vector2f(200, 100); // Posición por defecto


}

void Client::runGameplayLoop()
{
    if (!m_isConnectedToGameServer) {
        return;
    }

    // 1. Enviar inputs (esto se llamaría desde la lógica de juego cuando el jugador actúe)
    // Ejemplo: sendPlayerInput(currentMoveDirection);
    // Lo pongo aquí solo para ilustrar, pero el envío real se haría en respuesta al input del jugador.

    // 2. Recibir estado del juego
    sf::Packet gamePacket;
    std::optional<sf::IpAddress> senderIp;
    unsigned short senderPort;

    // gameUdpSocket es no bloqueante
    if (gameUdpSocket.receive(gamePacket, senderIp, senderPort) == sf::Socket::Status::Done) {
        // Verificar que el paquete viene del GameServer esperado
        if (senderIp.value().toString() == m_gameServerIp && senderPort == m_gameServerUdpPort) {
            processGamePacket(gamePacket);
        }
        else {
            std::cout << "[Client-UDP] Paquete UDP recibido de una fuente inesperada: "
                << senderIp.value() << ":" << senderPort << std::endl;
        }
    }




}

void Client::sendPlayerInput(float moveDir, bool wantsToShoot)
{

    if (!m_isConnectedToGameServer) return;

    sf::Packet inputPacket;
    inputPacket <<C_PLAYER_INPUT << moveDir << wantsToShoot;

    // Enviar al GameServer
    if (gameUdpSocket.send(inputPacket, sf::IpAddress::resolve(m_gameServerIp).value(), m_gameServerUdpPort) != sf::Socket::Status::Done) {
        std::cerr << "[Client-UDP] Error enviando paquete de input." << std::endl;
    }
}

void Client::processGamePacket(sf::Packet& packet)
{
    PacketType type;
    if (!(packet >> type)) {
        std::cerr << "[Client-UDP] Error leyendo GamePacketType del GameServer." << std::endl;
        return;
    }

    if (type == S_GAME_STATE) {
        // Asumimos que el servidor envía:
        // packet << S_GAME_STATE
        //        << p1_pos.x << p1_pos.y << p1_health << p1_lives
        //        << p2_pos.x << p2_pos.y << p2_health << p2_lives;
        // Y el cliente necesita saber cuál es él (podría ser por orden o con un ID)
        // Por ahora, asumimos que el servidor siempre envía "tu estado" primero, luego "el del oponente"
        // O, el servidor podría identificar los estados con un ID de jugador.
        // Para este ejemplo, vamos a deserializar directamente a myPlayer y opponentPlayer.
        // NECESITAS UNA FORMA DE SABER CUÁL ES CUÁL.
        // Por ejemplo, si el servidor sabe tu IP:Puerto, podría enviar tu estado específico.
        // O si te asignó un ID (0 o 1) al inicio de la partida.
        // Por ahora, una suposición simple:
        float p1x, p1y, p2x, p2y;
        int p1h, p1l, p2h, p2l;

        if (packet >> p1x >> p1y >> p1h >> p1l >> p2x >> p2y >> p2h >> p2l) {
            // ¿Cómo sé si soy P1 o P2 según el servidor?
            // Por ahora, actualicemos ambos y la clase Game decidirá cómo dibujar.
            // Idealmente, el servidor te dice "tú eres el jugador con estos datos..."
            // O tu cliente siempre renderiza "myPlayer" en el centro y el oponente relativo a eso.

            // Suposición muy básica: si mi puerto UDP local coincide con el que
            // el servidor de servicios dijo que era el "player1UdpPort" en la notificación
            // original al servidor dedicado, entonces soy P1 para el servidor dedicado.
            // Esto es un poco enrevesado. Una ID de jugador sería mejor.

            // Simplificación: El Game.cpp tomará myPlayerPosition y opponentPlayerPosition
            // y los renderizará. El cliente debe mantenerlos actualizados.
            // Aquí, el servidor envía los datos de los dos jugadores. El cliente
            // tiene que saber cuál es "yo" y cuál es "el otro".
            // Para el ejemplo, llenaremos myPlayer y opponentPlayer.
            // La clase Game los usará.
            myPlayerPosition = { p1x, p1y }; // OJO: Necesitas saber cuál es cuál.
            myPlayerHealth = p1h;
            myPlayerLives = p1l;
            opponentPlayerPosition = { p2x, p2y };
            opponentPlayerHealth = p2h;
            opponentPlayerLives = p2l;

            //std::cout << "[Client-UDP] Estado recibido: P1(" << p1x << "," << p1y << ") P2(" << p2x << "," << p2y << ")" << std::endl;

        }
        else {
            std::cerr << "[Client-UDP] Error deserializando S_GAME_STATE." << std::endl;
        }
    }
    else {
        std::cerr << "[Client-UDP] Tipo de GamePacket desconocido: " << static_cast<int>(type) << std::endl;
    }


}

Client::Client():connected(false),myPort(0),m_mapReceived(false),loginOk(false),RegisterOk(false)
{
    mapFilePath = "Data/map.txt"; // Asegurar consistencia
   
}



Client* Client::getInstance()
{
    if (instanceClient == nullptr) {
        instanceClient = new Client();
    }
    return instanceClient;
}


bool Client::connectToServer(unsigned short port)
{
   

    if (Clientsocket.connect(SERVER_IP, port) != sf::Socket::Status::Done) {
        std::cerr << "error al conectar al servidor" << std::endl;
        connected = false;
        return false;
    }
    else {
       
        Clientsocket.setBlocking(false);
        connected = true;
        myPort = Clientsocket.getLocalPort();
        std::cout << "conectat al servidor correctament " << std::endl;
        return true;
        
    }

    

}

void Client::run()
{
    if (!connected) return; // No hacer nada si no estamos conectados al servidor de servicios

    sf::Packet incomingPacket;
    // Clientsocket (TCP) debe ser no bloqueante para que esto no congele la UI
    if (Clientsocket.receive(incomingPacket) == sf::Socket::Status::Done) {
        processPacket(incomingPacket); // Procesa paquetes del servidor de servicios
    }
    else if (Clientsocket.receive(incomingPacket) == sf::Socket::Status::Disconnected) {
        std::cout << "[Client] Desconectado del servidor de servicios." << std::endl;
        connected = false;
        // Aquí podrías limpiar estados relacionados con el servidor de servicios
        m_isInMatchmakingQueue = false;
    }

}

bool Client::loginAction(std::string nick, std::string pass)
{
   
       packet.clear();

        packet << C_REQUEST_LOGIN << nick << pass;
        std::cout << "[CLIENT] Enviant LOGIN: " << nick << ", " << pass << std::endl;
        clientNick = nick;
      
        m_hasLoginResponse = false; // Resetear para esperar nueva respuesta
        return sendPacket(packet); // Devuelve si el envío fue exitoso

   
}

bool Client::RegisterAction(std::string nick, std::string pass)
{
    
    packet.clear();

        packet << C_REQUEST_REGISTER << nick << pass;
        std::cout << "[CLIENT] Enviant REGISTER: " << nick << ", " << pass << std::endl;
        m_hasRegisterResponse = false; // Resetear para esperar nueva respuesta
        return sendPacket(packet);

}

bool Client::sendPacket(sf::Packet& packet)
{
    return Clientsocket.send(packet) == sf::Socket::Status::Done;
}


void Client::processPacket(sf::Packet packet)
{

    PacketType packetType;
     packet >> packetType;

     switch (packetType)
     {
     case S_MAP_DATA: {

            std::string receivedMapContent;

            if (packet >> receivedMapContent) {
                m_mapData = receivedMapContent;
                m_mapReceived = true;
                std::cout << "[CLIENT] Mapa recibido del servidor. Tamaño: " << m_mapData.length() << " bytes." << std::endl;
                ReadWriteMapReceived(receivedMapContent);
               // std::cout << receivedMapContent << std::endl;
            }
            else {
                std::cerr << "[CLIENT] Error leyendo contenido del mapa del paquete S_MAP_DATA." << std::endl;
            }
        }
        break;

    case S_LOGIN_OK:
        std::cout << "Usuari validat correctament " << std::endl;
        loginOk = true;
        m_hasLoginResponse = true;
        break;
    case S_LOGIN_FAIL:
        std::cout << "Usuari o contrasenya incorrectes " << std::endl;
         m_hasLoginResponse = true;
        break;
    case S_REGISTER_OK:
        std::cout << "Usuari registrat correctament " << std::endl;
        RegisterOk = true;
        m_hasRegisterResponse = true; // Nueva flag
        //Menu de crear o unirse a sala

        break;
    case S_REGISTER_FAIL:
        std::cout << "Usuari ja existeix a la BBDD" << std::endl;
        m_hasRegisterResponse = true; // Nueva flag
        break;
    case S_ADDED_TO_MATCHMAKING_QUEUE:
        std::cout << "[CLIENT] Añadido a la cola de matchmaking." << std::endl;
        // Actualizar estado de la UI a "En cola..."
        m_isInMatchmakingQueue = true;
        break;
    case S_MATCH_FOUND:
        {
        std::cout << "[CLIENT] Partida encontrada! Conectar a GameServer en " << std::endl;
        std::string gameServerIpStr;
        unsigned short gameServerUdpPortVal;
        unsigned short myUdpPortForGameVal; // Puerto que este cliente debe usar
        // El servidor de servicios debe enviar estos tres datos:
            // packet << S_MATCH_FOUND << gameServerIp.toString() << gameServerUdpPort << client1UdpPortToUse;
            // packet << S_MATCH_FOUND << gameServerIp.toString() << gameServerUdpPort << client2UdpPortToUse;
        if (packet >> gameServerIpStr>> gameServerUdpPortVal >> myUdpPortForGameVal) {
            std::cout << "[CLIENT] Partida encontrada! Conectar a GameServer "<< std::endl;
     

            m_gameServerIp = gameServerIpStr;
            m_gameServerUdpPort = gameServerUdpPortVal;
            m_myUdpPortForGame = myUdpPortForGameVal;
            m_matchFound = true;
            m_isInMatchmakingQueue = false; // Ya no estamos en cola
            // Aquí el cliente debería cambiar de estado para desconectarse de este servidor
            // y conectarse al GameServer.

            std::cout << "[CLIENT] Partida encontrada! GameServer en "
                << m_gameServerIp << ":" << m_gameServerUdpPort
                << ". Usaré mi puerto UDP local: " << m_myUdpPortForGame << std::endl;

            
            connectToGameServerUDP();
        }
        else {
            std::cerr << "[CLIENT] Error leyendo datos de S_MATCH_FOUND." << std::endl;
        }
    
        break;
    
        }
       
   
    }


}



std::string Client::getNickname()
{
    return clientNick;
}



bool Client::requestMatchmakingFriendly()
{
    if (!connected) return false;
    sf::Packet pkt;
    pkt << C_REQUEST_MATCHMAKING_FRIENDLY;
    if (Clientsocket.send(pkt) == sf::Socket::Status::Done) {
        std::cout << "[CLIENT] Solicitud de matchmaking enviada." << std::endl;
        return true;
    }
    std::cerr << "[CLIENT] Error enviando matchmaking." << std::endl;
    return false;
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










