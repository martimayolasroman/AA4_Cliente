#pragma once
#include <SFML/Network.hpp>
#include <cstdint>
#include "iostream"
//#include "Game.h"
#include <set>


#define WIDTH 1280
#define HEIGHT 720
#define SERVER_PORT 55000

class Client
{

private:

	static Client* instanceClient;
	// Evitamos copia y asignación
	//Client(const Client&) = delete;
//	Client& operator = (const Client&) = delete;

	bool loginOk = false;
	bool RegisterOk = false;




	Client();

	bool sendPacket(sf::Packet& packet);
	void startGame(sf::Packet& packet);

	void processPacket(sf::Packet packetType);

	sf::TcpSocket Clientsocket;
	bool connected;


	const sf::IpAddress SERVER_IP = sf::IpAddress(127, 0, 0, 1);

	sf::TcpListener listener;
	std::vector<sf::TcpSocket*> peerSockets;
	sf::SocketSelector selector;
	
	
	int currentMoveId = 0;
	std::set<int> processedMoveIds;
	

	unsigned short myPort; 
	bool gameReady = false;

	int ClientColor = 0;
	std::string clientNick;


public:

	struct PeerInfo {
		std::string nickname;
		std::string ip;
		unsigned short port;
	};

	std::vector<PeerInfo> peers;



		static Client* getInstance();
		
		bool connectToServer( unsigned short port);
		void run();

		bool loginAction(std::string nick, std::string pass);
		bool RegisterAction(std::string nick, std::string pass);
		void createRoom(std::string roomId);
		void joinRoom(std::string roomId);

		void startP2P();
		void checkP2PConnections();
		void processPeerPacket(sf::Packet& packet);

		bool isGameReady() const;
		void setGameReady(bool ready);

		void sendPacketToPeers(sf::Packet& packet);
		void sendMove(int color, int idCasilla, int numberMoves);
		void receiveMoveFromPeer(sf::Packet& packet);
		bool MoveReceived = false;
		bool isMoveReceived();
		std::tuple<int, int, int> movement;
		std::tuple<int, int, int> getMovement();
		int getColor();
		std::string getNickname();
		void sendGameOver(int playerColor);
		void processGameOver(sf::Packet& packet);
		void handlePeerDisconnect(sf::TcpSocket*  Clientsocket);
		void disonnectFromPeers();
		
		void reconnectToServer();



		//MainMenu mainMenu;
		//Game parchis;
		
	//	sf::RenderWindow window;
		sf::Packet packet;








};

