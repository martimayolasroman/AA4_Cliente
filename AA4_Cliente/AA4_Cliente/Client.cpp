#include "Client.h"



enum  PacketType
{
    HANDSHAKE = 0,
    LOGIN,
    REGISTER,
    CREATE_ROOM,
    JOIN_ROOM,
    DISCONNECT,
    LOGIN_OK,
    LOGIN_FAIL,
    REGISTER_OK,
    REGISTER_FAIL,
    ROOM_CREATED,
    ROOM_EXISTS,
    JOIN_OK,
    JOIN_FAIL,
    START_GAME,
    UNKNOWN
};


enum  PacketTypeP2P
{
    MOVE_PIECE,
    TURN_END,
    GAME_OVER,
  
    
};


Client* Client::instanceClient = nullptr;


sf::Packet& operator >> (sf::Packet& packet, PacketType& tipo) {

    int temp;
    packet >> temp;
    tipo = static_cast<PacketType>(temp);

    return packet;
}

sf::Packet& operator >> (sf::Packet& packet, PacketTypeP2P& tipo) {

    int temp;
    packet >> temp;
    tipo = static_cast<PacketTypeP2P>(temp);

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

	checkP2PConnections();

	//Ensenyar Menu de Login / Register

   // mainMenu.Update();  



}

bool Client::loginAction(std::string nick, std::string pass)
{
   
       packet.clear();

        packet << LOGIN << nick << pass;
        std::cout << "[CLIENT] Enviant LOGIN: " << nick << ", " << pass << std::endl;
        clientNick = nick;
        sendPacket(packet);

        Clientsocket.setBlocking(true);
        run();
        Clientsocket.setBlocking(false);
        return loginOk;
   
}

bool Client::RegisterAction(std::string nick, std::string pass)
{
    
    packet.clear();

        packet << REGISTER << nick << pass;
        std::cout << "[CLIENT] Enviant REGISTER: " << nick << ", " << pass << std::endl;
        sendPacket(packet);

        Clientsocket.setBlocking(true);
        run();
        Clientsocket.setBlocking(false);
        return RegisterOk;
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

    //if (packet.endOfPacket()) {
    //   // std::cout << "[CLIENT] Fi correcte del paquet." << std::endl;
    //}
    //else {
    //    std::cout << "[CLIENT] ATENCIÓ: El paquet encara té dades pendents." << std::endl;
    //}

    startP2P();

}



void Client::processPacket(sf::Packet packet)
{

    PacketType packetType;

    //sf::Packet originalPacket = packet;

     packet >> packetType;

    switch (packetType)
    {
    case LOGIN:
      //  LoginUser();
        break;
    case REGISTER:
       // RegisterUser();
        break;
    case CREATE_ROOM:

        break;
    case JOIN_ROOM:

        break;
    case LOGIN_OK:
        std::cout << "Usuari validat correctament " << std::endl;
        loginOk = true;

        //Menu de crear o unirse a sala

        break;
    case LOGIN_FAIL:
        std::cout << "Usuari o contrasenya incorrectes " << std::endl;
        break;
    case REGISTER_OK:
        std::cout << "Usuari registrat correctament " << std::endl;
        RegisterOk = true;
        //Menu de crear o unirse a sala

        break;
    case REGISTER_FAIL:
        std::cout << "Usuari ja existeix a la BBDD" << std::endl;
        break;
    case ROOM_CREATED:
        std::cout << "Sala creada correctament!" << std::endl;
        break;
    case ROOM_EXISTS:
        std::cout << "Ja existeix una sala amb aquest ID" << std::endl;
        break;
    case JOIN_OK:
        std::cout << "T'has unit correctament a la sala" << std::endl;
        break;
    case JOIN_FAIL:
        std::cout << "No s'ha pogut unir a la sala (no existeix o esta plena)" << std::endl;
        break;
    case START_GAME:
       
        startGame(packet);
        break;

    case DISCONNECT:
       
        break;
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

void Client::sendMove(int color, int idCasilla, int numberMoves) {

    packet.clear();

    packet << MOVE_PIECE << currentMoveId /*<< getNickname()*/ << color << idCasilla << numberMoves;
    std::cout << "[SendMove] Enviant moviment amb ID= " << currentMoveId
        << " color=" << color << " casella=" << idCasilla
        << " moviments=" << numberMoves << std::endl;

   
    sendPacketToPeers(packet);
    currentMoveId++;
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

void Client::sendGameOver(int playerColor)
{

    sf::Packet packet;
    packet << GAME_OVER << playerColor;
    sendPacketToPeers(packet);


}

void Client::processGameOver(sf::Packet& packet)
{

    int winnerColor;
    packet >> winnerColor;

    std::cout << "[CLIENT] Partida finalitzada! Ha guanyat el jugador amb color: " << winnerColor << std::endl;

    disonnectFromPeers();
   
     reconnectToServer();
        


}

void Client::handlePeerDisconnect(sf::TcpSocket* Clientsocket)
{
    selector.remove(*Clientsocket);

    auto it = std::find(peerSockets.begin(), peerSockets.end(), Clientsocket);

    if (it != peerSockets.end()) {
        std::cout << "[CLIENT] Eliminant peer desconnectat.  "  << std::endl;
        delete* it;
        peerSockets.erase(it);
    }





}

void Client::disonnectFromPeers()
{

    for (auto& peer : peerSockets) {
        peer->disconnect();
        delete peer;
    }
    peerSockets.clear();
    selector.clear();
    selector.add(listener);
    std::cout << "[CLIENT] Desconnectat de tots els peers." << std::endl;


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


void Client::sendPacketToPeers(sf::Packet& packet)
{


    for (auto& peers : peerSockets) {
        if (peers->send(packet) != sf::Socket::Status::Done) {
            std::cout << "[CLIENT] Error al enviar paquete a los peers "  << std::endl;
            handlePeerDisconnect(peers);
        };
    }



}

void Client::createRoom(std::string roomId)
{

    sf::Packet packet;

    packet << CREATE_ROOM << roomId;
    std::cout << "[CLIENT] Enviant CREATE_ROOM: " << roomId << std::endl;
    sendPacket(packet);


}

void Client::joinRoom(std::string roomId)
{

    sf::Packet packet;
    packet << JOIN_ROOM << roomId;
    std::cout << "[CLIENT] Enviant JOIN_ROOM: " << roomId << std::endl;
    sendPacket(packet);




}


// ------------------- IA -------------------------------
void Client::startP2P()
{

    Clientsocket.disconnect();

    if (listener.listen(myPort) != sf::Socket::Status::Done) {
        std::cerr << "[CLIENT] Error escoltant al port " << myPort << std::endl;
        return;
    }
    std::cout << "[CLIENT] Escoltant al port " << myPort << std::endl;

    selector.add(listener);

    for (const auto& peer : peers) {
        sf::TcpSocket* socket = new sf::TcpSocket();


        if (socket->connect(sf::IpAddress::resolve(peer.ip).value(), peer.port, sf::seconds(5.f)) == sf::Socket::Status::Done) {
            std::cout << "[CLIENT] Connectat a " << peer.nickname
                << " (" << peer.ip << ":" << peer.port << ")" << std::endl;
            peerSockets.push_back(socket);
            selector.add(*socket);
        }
        else {
            std::cerr << "[CLIENT] No s'ha pogut connectar a " << peer.nickname
                << " (" << peer.ip << ":" << peer.port << ")" << std::endl;
            delete socket;
        }
    }

    std::cout << "[CLIENT] Preparat per a connexions P2P." << std::endl;


    setGameReady(true);



}

void Client::checkP2PConnections()
{

    
    if (selector.wait(sf::milliseconds(1)))
    {
        // 1. Alguna connexió entrant?
        if (selector.isReady(listener))
        {
            sf::TcpSocket* newPeer = new sf::TcpSocket();
            if (listener.accept(*newPeer) == sf::Socket::Status::Done)
            {
                std::cout << "[CLIENT] Nova connexió P2P rebuda!" << std::endl;
                peerSockets.push_back(newPeer);
                selector.add(*newPeer);
            }
            else
            {
                delete newPeer;
            }
        }

        // 2. Missatges dels peers
        for (auto socket : peerSockets)
        {
            if (selector.isReady(*socket))
            {
                sf::Packet paquetRebut;
                if (socket->receive(paquetRebut) == sf::Socket::Status::Done)
                {
                    // Aquí tractarem el missatge rebut
                    std::cout << "[CLIENT] Missatge rebut d'un peer!" << std::endl;
                    processPeerPacket(paquetRebut);
                }
                else
                {
                    std::cerr << "[CLIENT] Error rebent paquet d'un peer!" << std::endl;
                    handlePeerDisconnect(socket);
                }
            }
        }
    }










}

// ------------------- IA -------------------------------

void Client::processPeerPacket(sf::Packet& packet)
{

    std::cout << "[CLIENT] Paquet P2P rebut " << std::endl;


    PacketTypeP2P packetTypep2P;


    packet >> packetTypep2P;

    switch (packetTypep2P)
    {
    case MOVE_PIECE:
        receiveMoveFromPeer(packet);
        break;
    case TURN_END:
        break;
    case GAME_OVER:
        processGameOver(packet);

        break;
    default:
        break;
    }


}




