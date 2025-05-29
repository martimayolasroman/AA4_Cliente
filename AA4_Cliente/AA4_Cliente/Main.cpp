#pragma once


#include "Client.h"
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include "Login.h"
#include "Lobby.h"

#define SERVER_PORT 55000

#define HEIGHT 720
#define WIDTH 1280










void main() {

	

	
	sf::RenderWindow* window = new sf::RenderWindow(sf::VideoMode({ WIDTH, HEIGHT }), "| Cliente Parchis |");;
	
	GameState currentState = GameState::LOGIN;

	Login loginMenu(window);
	Lobby lobbyMenu(window);
	/*Game *parchis;*/

	
	
	Client::getInstance()->connectToServer(55000);

	while (currentState != GameState::EXIT)
	{
		

		switch (currentState) 
		{
		case GameState::LOGIN:
			currentState = loginMenu.Update();
			break;
		case GameState::LOBBY:
			currentState = lobbyMenu.Update();
			break;
		case GameState::GAME:
			/*parchis = new Game(window);*/
		/*	currentState = parchis->Update();*/
			break;
		case GameState::EXIT:
			break;
		default:
			std::cout << "[DEBUG] default state Main: "<< (int)currentState << std::endl;
			break;
		}
	}

}


