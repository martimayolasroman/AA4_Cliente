#pragma once
#include <SFML/Network.hpp>
#include <cstdint>
#include "iostream"
#include <set>
#include <fstream>
#include <atomic>
#include <optional> // Para sf::IpAddress::resolve

#define WIDTH 1280
#define HEIGHT 720

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

    std::string m_mapData;
    bool m_mapReceived;

    bool m_amIPlayerOne = false;
    void connectToGameServerUDP();
    void processGamePacket(sf::Packet& packet);

    sf::Vector2f myPlayerPosition = { -1.f, -1.f };
    int myPlayerHealth = 0;
    int myPlayerLives = 0;
    sf::Vector2f opponentPlayerPosition = { -1.f, -1.f };
    int opponentPlayerHealth = 0;
    int opponentPlayerLives = 0;

    Client();
    bool sendPacket(sf::Packet& packet);
    void processPacket(sf::Packet packetType);

    sf::TcpSocket Clientsocket;
    bool connected;
    const sf::IpAddress SERVER_IP = sf::IpAddress(127, 0, 0, 1); // O sf::IpAddress::LocalHost
    unsigned short myPort;
    std::string clientNick;

public:
    static Client* getInstance();
    bool connectToServer(unsigned short port);
    void run();

    bool loginAction(std::string nick, std::string pass);
    bool RegisterAction(std::string nick, std::string pass);
    std::string getNickname();
    bool requestMatchmakingFriendly();
    std::string mapFilePath = "Data/map.txt";

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
    sf::Vector2f getMyPlayerPosition() const { return myPlayerPosition; }
    int getMyPlayerHealth() const { return myPlayerHealth; }
    int getMyPlayerLives() const { return myPlayerLives; }
    sf::Vector2f getOpponentPlayerPosition() const { return opponentPlayerPosition; }
    int getOpponentPlayerHealth() const { return opponentPlayerHealth; }
    int getOpponentPlayerLives() const { return opponentPlayerLives; }
    bool isConnectedToGameServer() const { return m_isConnectedToGameServer; }
    void receiveAndProcessGameData();
    void sendPlayerInput(float moveDir, bool wantsToShoot);

    sf::Packet packet; // Para TCP, aunque sería mejor no tenerlo como miembro si solo se usa temporalmente
};