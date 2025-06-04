#pragma once
#include <SFML/Network.hpp>
#include <cstdint>
#include "iostream"
#include <set>
#include <fstream>
#include <atomic>
#include <optional>
#include <SFML/System/Time.hpp>
#include <deque> // Para m_pendingInputs
#include <SFML/Audio.hpp> // Para sonido

#define WIDTH 1280
#define HEIGHT 720

// Estructura para el estado del oponente usado en la interpolación
struct OpponentInterpolationState {
    sf::Vector2f currentPosition = { -1.f, -1.f };
    sf::Time currentTimestamp;
    sf::Vector2f previousPosition = { -1.f, -1.f };
    sf::Time previousTimestamp;
    bool hasReceivedFirstUpdate = false;
    bool hasReceivedEnoughUpdatesForInterpolation = false;
};

// Estructura para el historial de inputs enviados por el cliente
struct ClientInputRecord {
    float moveDirection;
    bool wantsToShoot;
    bool jumpRequested; // <--- NUEVO: Flag para la solicitud de salto
};

// <--- NUEVA ESTRUCTURA: Para pasar el estado de la bala del servidor al cliente para interpolación
struct ServerBulletState {
    sf::Vector2f position;
    sf::Vector2f velocity; // Podrías necesitar esto si quieres interpolar velocidad
    float radius;
    bool isActive; // Para saber si la bala existe
    int ownerPlayerId; // Opcional, si el cliente necesita saber quién disparó
    sf::Time timestamp; // <--- NUEVO: Timestamp de cuando el servidor generó este estado

    ServerBulletState(sf::Vector2f pos = { 0,0 }, sf::Vector2f vel = { 0,0 }, float r = 0, bool active = false, int owner = 0, sf::Time ts = sf::Time::Zero)
        : position(pos), velocity(vel), radius(r), isActive(active), ownerPlayerId(owner), timestamp(ts) {}
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

    std::string m_mapData;
    bool m_mapReceived;

    bool m_amIPlayerOne = false;
    void connectToGameServerUDP();
    void processGamePacket(sf::Packet& packet);

    // Estado del jugador propio (recibido del servidor)
    sf::Vector2f myPlayerPosition = { -1.f, -1.f }; // ¡OJO! Esta ya no se usa para la posición directa del jugador local
    int myPlayerHealth = 0;
    int myPlayerLives = 0;

    std::vector<ServerBulletState> m_opponentBulletStates;

    // Para la reconciliación y el estado autoritativo del jugador local
    sf::Vector2f m_lastServerConfirmedMyPlayerPosition;
    bool m_newServerStateReceived; // Flag para Game.cpp
    bool m_myPlayerOnGround;          // <--- NUEVO: Estado onGround del servidor
    sf::Vector2f m_myPlayerServerVelocity; // <--- NUEVO: Velocidad del servidor

    // Historial de inputs enviados
    std::deque<ClientInputRecord> m_pendingInputs;

    // Estado del oponente (recibido del servidor, usado para interpolación)
    OpponentInterpolationState opponentInterpolationState;
    int opponentPlayerHealth = 0;
    int opponentPlayerLives = 0;

    Client();
    bool sendPacket(sf::Packet& packet);
    void processPacket(sf::Packet tcp_packet);

 
    bool m_soundLoaded = false;
    void loadSounds();
    //sf::SoundBuffer m_tauntSoundBuffer;
   // std::optional<sf::Sound> m_tauntSound;

    sf::TcpSocket Clientsocket;
    bool connected;
    const sf::IpAddress SERVER_IP = sf::IpAddress(127, 0, 0, 1);
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
    bool ReadWriteMapReceived(std::string& receivedMapContent);

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

    // Para el jugador propio (salud y vidas)
    int getMyPlayerHealth() const { return myPlayerHealth; }
    int getMyPlayerLives() const { return myPlayerLives; }

    // Para la reconciliación y el estado autoritativo
    sf::Vector2f getLastServerConfirmedMyPlayerPosition() const { return m_lastServerConfirmedMyPlayerPosition; }
    bool hasNewServerState() const { return m_newServerStateReceived; }
    void consumeServerStateFlag() { m_newServerStateReceived = false; }
    bool getMyPlayerOnGround() const { return m_myPlayerOnGround; }         
    sf::Vector2f getMyPlayerServerVelocity() const { return m_myPlayerServerVelocity; } 

    // Para el oponente (usado por Game.cpp para interpolar)
    const OpponentInterpolationState& getOpponentInterpolationState() const { return opponentInterpolationState; }
    int getOpponentPlayerHealth() const { return opponentPlayerHealth; }
    int getOpponentPlayerLives() const { return opponentPlayerLives; }
    const std::vector<ServerBulletState>& getOpponentBulletStates() const { return m_opponentBulletStates; } // <--- NUEVO: Getter para balas del servidor

    bool isConnectedToGameServer() const { return m_isConnectedToGameServer; }
    void receiveAndProcessGameData();
    void sendPlayerInput(float moveDir, bool wantsToShoot, bool jumpRequestedThisTick); 
    void addSentInputToHistory(const ClientInputRecord& input);

   // void sendPlayerTaunt(); // enviar la notificación de burla
   // void playTauntSound();  //  función para reproducir el sonido localmente
};