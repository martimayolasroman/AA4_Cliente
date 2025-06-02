#pragma once
#include <SFML/Network.hpp>
#include <cstdint>
#include "iostream"
//#include "Game.h"
#include <set>
#include <fstream>


#define WIDTH 1280
#define HEIGHT 720







class Client
{

private:

	static Client* instanceClient;
	

	bool loginOk = false;
	bool RegisterOk = false;
	bool m_hasLoginResponse = false;
	bool m_hasRegisterResponse = false;
	bool m_isInMatchmakingQueue = false;
	bool m_matchFound = false;
	std::string m_gameServerIp;
	unsigned short m_gameServerPort = 0;
	unsigned short m_gameServerUdpPort = 0;
	unsigned short m_myUdpPortForGame = 0; // Puerto UDP local que este cliente usará

	sf::UdpSocket gameUdpSocket;
	std::atomic<bool> m_isConnectedToGameServer{ false };

	std::string m_mapData;
	bool m_mapReceived = false;

	// --- Gameplay Loop (UDP) ---
	void connectToGameServerUDP(); // Intenta conectar (bind) y prepararse para UDP
	void runGameplayLoop();    // Bucle para enviar inputs y recibir estado del juego
	void sendPlayerInput(float moveDir, bool wantsToShoot);
	void processGamePacket(sf::Packet& packet); // Procesa S_GAME_STATE

	// Datos del estado del juego recibidos del servidor
	// Podrías tener structs para tu jugador y el oponente
	sf::Vector2f myPlayerPosition;
	int myPlayerHealth=3;
	int myPlayerLives=3;
	sf::Vector2f opponentPlayerPosition;
	int opponentPlayerHealth=3;
	int opponentPlayerLives=3;
	// ---------------------


	Client();

	bool sendPacket(sf::Packet& packet);

	void processPacket(sf::Packet packetType);

	sf::TcpSocket Clientsocket;
	bool connected;


	const sf::IpAddress SERVER_IP = sf::IpAddress(127, 0, 0, 1);

	sf::TcpListener listener;
	std::vector<sf::TcpSocket*> peerSockets;
	sf::SocketSelector selector;
	

	unsigned short myPort; 



	std::string clientNick;


public:

		static Client* getInstance();
		
		bool connectToServer( unsigned short port);
		void run();

		bool loginAction(std::string nick, std::string pass);
		bool RegisterAction(std::string nick, std::string pass);


		std::string getNickname();
		

		bool requestMatchmakingFriendly();
		std::string mapFilePath = "Data/map.txt";
		bool  ReadWriteMapReceived(std::string& receivedMapContent);

		bool hasReceivedMap() const { return m_mapReceived; }
		const std::string& getMapData() const { return m_mapData; }
		bool hasLoginResponse() const { return m_hasLoginResponse; }
		bool hasRegisterResponse() const { return m_hasRegisterResponse; }
		void resetLoginResponse() { m_hasLoginResponse = false; }
		bool getLoginStatus() const { return loginOk; }
		bool hasMatchBeenFound() const { return m_matchFound; }
		std::string getGameServerIp() const { return m_gameServerIp; }
		unsigned short getGameServerPort() const { return m_gameServerPort; }
		bool getRegisterStatus() const { return RegisterOk; }
		void resetRegisterResponse() { m_hasRegisterResponse = false; }

		
	//	sf::RenderWindow window;
		sf::Packet packet;








};

