#include "Lobby.h"

GameState Lobby::EventHandler(const sf::Event& event)
{
	if (event.is<sf::Event::Closed>()) {

		window->close();
	}
	if (enterLobbyButton && enterLobbyButton->handleEvent(event, *window)) {
		std::cout << "Lobby id to join: " << idLobbyText->getString().toAnsiString() << std::endl;

		Client::getInstance()->requestMatchmakingFriendly();
		/*Client::getInstance()->joinRoom(idLobbyText->getString().toAnsiString());*/

 	}

	if (createLobbyButton && createLobbyButton->handleEvent(event, *window)) {
		std::cout << "Lobby id to create: " << idLobbyText->getString().toAnsiString() << std::endl;
		/*Client::getInstance()->createRoom(idLobbyText->getString().toAnsiString());*/
		Client::getInstance()->requestMatchmakingFriendly();

	}


	if (const sf::Event::MouseButtonPressed* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
		sf::Vector2i mousePos = mousePressed->position;

		if (lobbyIDRectangle.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
			textFocus = true;
			lobbyIDRectangle.setOutlineColor(inputRectangleFocussedColor);
		}
		else {
			textFocus = false;
			lobbyIDRectangle.setOutlineColor(inputRectangleNotFocussedColor);
		}
	}

	if (textFocus)
	{
		if (const sf::Event::TextEntered* textEntered = event.getIf<sf::Event::TextEntered>()) {
			if (textEntered->unicode == 8) {
				if (!input.empty())
					input.pop_back();
			}
			else if (textEntered->unicode < 128 && input.size() < maxCharacters && std::isalnum(textEntered->unicode)) {
				input += static_cast<char>(textEntered->unicode);
			}
			idLobbyText->setString(input);
		}
	}


	return GameState::LOBBY;
}

Lobby::Lobby(sf::RenderWindow* w)
{
	window = w;
	width = window->getSize().x;
	height = window->getSize().y;

	if (!font.openFromFile(fontsPath + fontName))
	{
		std::cerr << "Error al cargar la fuente" << std::endl;
	}

	title = new sf::Text(font, titleString, titleTextSize);
	float centerX = width / 2.0f;

	titlePosition = sf::Vector2f(centerX - (title->getGlobalBounds().size.x / 2.0f), titleYPos);
	lobbyIDRectanglePosition = sf::Vector2f(centerX - (inputRectangleSize.x / 2.0f), lobbyIDRectangleYPos);

	lobbyFieldText = new sf::Text(font, lobbyFieldTextString, lobbyFieldTextTextSize);
	lobbyFieldTextPosition = sf::Vector2f(centerX - (lobbyFieldText->getGlobalBounds().size.x / 2.0f), titleYPos + lobbyTextSeparation);



	enterLobbyButtonPosition = sf::Vector2f(centerX - (buttonSeparation / 2.0f) - (buttonSize.x / 2.0f), buttonYPos);
	createLobbyButtonPosition = sf::Vector2f(centerX + (buttonSeparation / 2.0f) - (buttonSize.x / 2.0f), buttonYPos);
	enterLobbyButton = new Button(buttonSize, enterLobbyButtonPosition, enterLobbyButtonText, font, buttonColor, buttonTextColor);
	createLobbyButton = new Button(buttonSize, createLobbyButtonPosition, createLobbyButtonText, font, buttonColor, buttonTextColor);


	title->setFillColor(buttonColor);
	title->setPosition(titlePosition);

	lobbyFieldText->setFillColor(buttonColor);
	lobbyFieldText->setPosition(lobbyFieldTextPosition);

	idLobbyText = new sf::Text(font, "", inputTextSize);
	idLobbyText->setFillColor(inputTextColor); 
	idLobbyText->setPosition(lobbyIDRectanglePosition);
	
	lobbyIDRectangle = sf::RectangleShape(inputRectangleSize);
	lobbyIDRectangle.setPosition(lobbyIDRectanglePosition);
	lobbyIDRectangle.setFillColor(inputBackgroundColor);
	lobbyIDRectangle.setOutlineColor(inputRectangleNotFocussedColor);
	lobbyIDRectangle.setOutlineThickness(inputRectangleOutlineThickness);
}

GameState Lobby::Update()
{
	while (window->isOpen()) {

		Client::getInstance()->run();

		if (Client::getInstance()->isGameReady()) {
			std::cout << "[LOBBY] Connexió P2P completada! Anant al GAME." << std::endl << std::endl << std::endl << std::endl;
			return GameState::GAME;
		}


		while (const std::optional event = window->pollEvent()) {

			
			Render();
			GameState state = EventHandler(*event);
			if (state != GameState::LOGIN) {
				return state;
			}


			

		}
		Render();
		 
		return GameState::LOBBY;

	}
}

void Lobby::Render()
{
	window->clear(backgroundColor);

	if (enterLobbyButton)
		enterLobbyButton->draw(*window);
	if (createLobbyButton)
		createLobbyButton->draw(*window);

	window->draw(lobbyIDRectangle); //poner todo con ifs en todos los renders
	window->draw(*idLobbyText);
	window->draw(*title);
	window->draw(*lobbyFieldText);
	

	window->display();
}
