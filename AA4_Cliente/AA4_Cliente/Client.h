#pragma once
#include <SFML/Network.hpp>
#include <cstdint>
#include "iostream"
//#include "Game.h"
#include <set>
#include <fstream>


#define WIDTH 1280
#define HEIGHT 720
#define SERVER_PORT 55000

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

	std::string m_mapData;
	bool m_mapReceived = false;


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
	

		bool isGameReady() const;
		void setGameReady(bool ready);

		
		
		void receiveMoveFromPeer(sf::Packet& packet);
		bool MoveReceived = false;
		bool isMoveReceived();
		std::tuple<int, int, int> movement;
		std::tuple<int, int, int> getMovement();
		int getColor();
		std::string getNickname();
		
		
		void reconnectToServer();


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
		//MainMenu mainMenu;
		//Game parchis;
		
	//	sf::RenderWindow window;
		sf::Packet packet;








};

