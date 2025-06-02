#include "Client.h"



enum  PacketType
{
    // Cliente a Servidor
    C_REQUEST_LOGIN = 1,
    C_REQUEST_REGISTER = 2,
    C_REQUEST_MATCHMAKING_FRIENDLY = 3,
    C_MAP_RECEIVED_ACK = 4,

    // Servidor a Cliente
    S_MAP_DATA = 100,
    S_LOGIN_OK = 101,
    S_LOGIN_FAIL = 102,
    S_REGISTER_OK = 103,
    S_REGISTER_FAIL = 104,
    S_ADDED_TO_MATCHMAKING_QUEUE = 105,
    S_MATCH_FOUND = 106,
    S_ERROR_GENERAL = 107, // Para errores genéricos


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


Client::Client() 
{
    
    connected = false;
    
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


   // std::cout << std::endl << "//////////////HAGO CLIENT RUN!" << std::endl;

	sf::Packet incomingPacked;
	if (Clientsocket.receive(incomingPacked) == sf::Socket::Status::Done) {
		//std::cout << "Paquet rebut!" << std::endl;
		processPacket(incomingPacked);
	}

	

	//Ensenyar Menu de Login / Register

   // mainMenu.Update();  



}

bool Client::loginAction(std::string nick, std::string pass)
{
   
       packet.clear();

        packet << C_REQUEST_LOGIN << nick << pass;
        std::cout << "[CLIENT] Enviant LOGIN: " << nick << ", " << pass << std::endl;
        clientNick = nick;
      
        m_hasLoginResponse = false; // Resetear para esperar nueva respuesta
        return sendPacket(packet); // Devuelve si el envío fue exitoso

       /* Clientsocket.setBlocking(true);
        run();
        Clientsocket.setBlocking(false);
        return loginOk;*/
   
}

bool Client::RegisterAction(std::string nick, std::string pass)
{
    
    packet.clear();

        packet << C_REQUEST_REGISTER << nick << pass;
        std::cout << "[CLIENT] Enviant REGISTER: " << nick << ", " << pass << std::endl;
        m_hasRegisterResponse = false; // Resetear para esperar nueva respuesta
        return sendPacket(packet);

       /* Clientsocket.setBlocking(true);
        run();
        Clientsocket.setBlocking(false);
        return RegisterOk;*/
}

bool Client::sendPacket(sf::Packet& packet)
{
    return Clientsocket.send(packet) == sf::Socket::Status::Done;
}

void Client::startGame(sf::Packet& packet)
{

    std::cout << "[CLIENT] START_GAME rebut!: "  << std::endl;
    peers.clear();

    std::int32_t numPeers;
    packet >> numPeers;

    if (!(packet >> numPeers)) {
        std::cerr << "[CLIENT] Error llegint numPeers!" << std::endl;
        return;
    }

   // std::cout << "[CLIENT] Nombre de companys rebuts: " << numPeers << std::endl;

    int color;
    if (!(packet >> color)) {
        std::cerr << "[CLIENT] Error llegint color!" << std::endl;
        return;
    }
    ClientColor = color;
    std::cerr << "[CLIENT] Color: " << color << std::endl;


    for (int i = 0; i < numPeers; i++) {
        std::string nick, ipStr;
        unsigned short port;
        
       

      //  packet >> nick >> ipStr >> port;


        if (!(packet >> nick)) {
            std::cerr << "[CLIENT] Error llegint nickname!" << std::endl;
            return;
        }
        if (!(packet >> ipStr)) {
            std::cerr << "[CLIENT] Error llegint IP!" << std::endl;
            return;
        }
        if (!(packet >> port)) {
            std::cerr << "[CLIENT] Error llegint port!" << std::endl;
            return;
        }
        

        

        PeerInfo peer;
        peer.nickname = nick;
        peer.ip = ipStr;
        peer.port = port;

        peers.push_back(peer);

        std::cout << " - Company " << i + 1 << ": " << nick
            << " (" << ipStr << ":" << port << ")" << std::endl;
    }

 

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
        //std::string gameServerIp;
        //unsigned short gameServerPort;
        //if (packet >> gameServerIp >> gameServerPort) {
        //    std::cout << "[CLIENT] Partida encontrada! Conectar a GameServer en " << gameServerIp << ":" << gameServerPort << std::endl;
        //    m_gameServerIp = gameServerIp;
        //    m_gameServerPort = gameServerPort;
        //    m_matchFound = true; // Nueva flag
        //    // Aquí el cliente debería cambiar de estado para desconectarse de este servidor
        //    // y conectarse al GameServer.
        //}
        //else {
        //    std::cerr << "[CLIENT] Error leyendo datos de S_MATCH_FOUND." << std::endl;
        //}
    
        break;
    
        }
       
   
    }


}

bool Client::isGameReady() const
{
    return gameReady;
}

void Client::setGameReady(bool ready)
{
    processedMoveIds.clear();
    currentMoveId = 0;
    gameReady = ready;
}



void Client::receiveMoveFromPeer(sf::Packet& packet)
{

    int moveId, color, idcasilla, numerMoves;
  
    packet >>moveId  >> color >> idcasilla >> numerMoves;


    if (processedMoveIds.count(moveId) > 0) {
       // std::cout << "[CLIENT] Moviment duplicat ignorant (ID=" << moveId << ")" << std::endl;
        return;
    }

    processedMoveIds.insert(moveId);

    std::cout << "--- Move rebut de un peer (ID=" << moveId << "): "
        << "color=" << color
        << ", idCasella=" << idcasilla
        << ", numberMoves=" << numerMoves << std::endl;


    currentMoveId++;
    movement = std::make_tuple(color, idcasilla, numerMoves);
    MoveReceived = true;
  
}

bool Client::isMoveReceived()
{
    if (MoveReceived){
        MoveReceived = false;
        return true;
    }
    return MoveReceived;
}

std::tuple<int, int, int> Client::getMovement()
{
   // MoveReceived = false;
   
    return movement;
}

int Client::getColor()
{
    return ClientColor;
}

std::string Client::getNickname()
{
    return clientNick;
}





void Client::reconnectToServer()
{
    if (Clientsocket.connect(SERVER_IP, SERVER_PORT) == sf::Socket::Status::Done)
    {
        Clientsocket.setBlocking(false);
        connected = true;
        std::cout << "[CLIENT] Reconnectat al servidor bootstrap correctament." << std::endl;
    }
    else
    {
        std::cerr << "[CLIENT] Error reconnectant al servidor!" << std::endl;
        connected = false;
    }

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










