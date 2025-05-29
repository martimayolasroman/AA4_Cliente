#pragma once
#include "Button.h"
#include "Client.h"
#include "GameState.h"


class Lobby
{
private:
	unsigned int width, height;
	sf::RenderWindow* window;
	sf::Color backgroundColor = sf::Color::White;


	Button* enterLobbyButton;
	std::string enterLobbyButtonText = "JOIN";
	sf::Vector2f enterLobbyButtonPosition;

	sf::Color buttonColor = sf::Color(255, 165, 0);
	sf::Color buttonTextColor = sf::Color::White;
	int buttonTextSize = 24;
	int buttonYPos = 530;
	sf::Vector2f buttonSize = sf::Vector2f(150, 60);


	Button* createLobbyButton;
	std::string createLobbyButtonText = "CREATE";
	sf::Vector2f createLobbyButtonPosition;

	int buttonSeparation = 250;

	sf::Font font;
	sf::Text* title;
	std::string titleString = "Lobby selector";
	sf::Vector2f titlePosition;
	int titleYPos = 70;
	int titleTextSize = 70;

	sf::Text* lobbyFieldText;
	std::string lobbyFieldTextString = "Enter Lobby ID to JOIN or CREATE a Lobby";
	sf::Vector2f lobbyFieldTextPosition;
	int lobbyFieldTextYPos;
	int lobbyFieldTextTextSize = 22;
	int lobbyTextSeparation = 100;

	std::string fontsPath = "Assets/Fonts/";
	std::string fontName = "Straw Milky.otf";



	sf::Color inputBackgroundColor = sf::Color(240, 240, 240);
	sf::Color inputRectangleFocussedColor = buttonColor;
	sf::Color inputRectangleNotFocussedColor = sf::Color(100, 100, 100);
	sf::Color inputTextColor = sf::Color(70, 70, 70);
	sf::Vector2f inputRectangleSize = sf::Vector2f(120, 40);
	int inputRectangleOutlineThickness = 4;

	bool textFocus = false;

	const int maxCharacters = 4;

	sf::RectangleShape lobbyIDRectangle;
	std::string input;
	sf::Text* idLobbyText;
	sf::Vector2f lobbyIDRectanglePosition;
	int inputTextSize = 30;
	int lobbyIDRectangleYPos = 350;

	GameState EventHandler(const sf::Event& event);
public:
	Lobby(sf::RenderWindow* w);
	GameState Update();
	void Render();


};

