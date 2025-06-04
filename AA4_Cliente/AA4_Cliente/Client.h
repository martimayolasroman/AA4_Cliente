#pragma once
#include <SFML/Network.hpp>
#include <cstdint>
#include "iostream"
#include <set>
#include <fstream>
#include <atomic>
#include <optional> // Para sf::IpAddress::resolve
#include <SFML/System/Time.hpp> // Para sf::Time

#define WIDTH 1280 // Considera unificar con Game.h (1024)
#define HEIGHT 720 // Considera unificar con Game.h (768)

// Estructura para el estado del oponente usado en la interpolación
struct OpponentInterpolationState {
    sf::Vector2f currentPosition = { -1.f, -1.f };
    sf::Time currentTimestamp;

    sf::Vector2f previousPosition = { -1.f, -1.f };
    sf::Time previousTimestamp;

    bool hasReceivedFirstUpdate = false;
    bool hasReceivedEnoughUpdatesForInterpolation = false;
};


class Client {
private:
    static Client* instanceClient;

    bool loginOk = false;
    bool RegisterOk = false;
    bool m_hasLoginResponse = false;
    bool m_hasRegisterResponse = false;
    bool m_isInMatchmakingQueue = false;
    bool m_matchFound = false;
    std::string m_gameServerIp;
    unsigned short m_gameServerUdpPort = 0;
    unsigned short m_myUdpPortForGame = 0;

    sf::UdpSocket gameUdpSocket;
    std::atomic<bool> m_isConnectedToGameServer{ false };

    std::string m_mapData; // Contenido del mapa recibido del servidor
    bool m_mapReceived;   // Se inicializará a false

    bool m_amIPlayerOne = false;
    void connectToGameServerUDP();
    void processGamePacket(sf::Packet& packet);

    // Estado del jugador propio (recibido del servidor)
    sf::Vector2f myPlayerPosition = { -1.f, -1.f };
    int myPlayerHealth = 0;
    int myPlayerLives = 0;

    // Estado del oponente (recibido del servidor, usado para interpolación)
    OpponentInterpolationState opponentInterpolationState;
    int opponentPlayerHealth = 0;
    int opponentPlayerLives = 0;


    Client();
    bool sendPacket(sf::Packet& packet); // Para TCP
    void processPacket(sf::Packet tcp_packet); // Para TCP

    sf::TcpSocket Clientsocket;
    bool connected; // Conexión TCP con el servidor de servicios
    const sf::IpAddress SERVER_IP = sf::IpAddress(127, 0, 0, 1); // O sf::IpAddress::LocalHost
    unsigned short myPort; // Puerto TCP local
    std::string clientNick;

public:
    static Client* getInstance();
    bool connectToServer(unsigned short port);
    void run(); // Lógica de red TCP

    bool loginAction(std::string nick, std::string pass);
    bool RegisterAction(std::string nick, std::string pass);
    std::string getNickname();
    bool requestMatchmakingFriendly();
    std::string mapFilePath = "Data/map.txt"; // Path donde se guarda/carga el mapa
    bool ReadWriteMapReceived(std::string& receivedMapContent); // Guarda el mapa recibido

    bool hasReceivedMap() const { return m_mapReceived; }
    bool hasLoginResponse() const { return m_hasLoginResponse; }
    bool hasRegisterResponse() const { return m_hasRegisterResponse; }
    void resetLoginResponse() { m_hasLoginResponse = false; }
    bool getLoginStatus() const { return loginOk; }
    bool hasMatchBeenFound() const { return m_matchFound; }
    bool getRegisterStatus() const { return RegisterOk; }
    void resetRegisterResponse() { m_hasRegisterResponse = false; }
    bool isInMatchmakingQueue_flag_getter() const { return m_isInMatchmakingQueue; }

    bool amIPlayerOne() const { return m_amIPlayerOne; }

    // Para el jugador propio
    sf::Vector2f getMyPlayerPosition() const { return myPlayerPosition; }
    int getMyPlayerHealth() const { return myPlayerHealth; }
    int getMyPlayerLives() const { return myPlayerLives; }

    // Para el oponente (usado por Game.cpp para interpolar)
    const OpponentInterpolationState& getOpponentInterpolationState() const { return opponentInterpolationState; }
    int getOpponentPlayerHealth() const { return opponentPlayerHealth; }
    int getOpponentPlayerLives() const { return opponentPlayerLives; }


    bool isConnectedToGameServer() const { return m_isConnectedToGameServer; }
    void receiveAndProcessGameData(); // Llama a gameUdpSocket.receive
    void sendPlayerInput(float moveDir, bool wantsToShoot);
};