#include "Login.h"

Login::Login(sf::RenderWindow* w)
{
	window = w;

	width = window->getSize().x;
	height = window->getSize().y;


	if (!font.openFromFile(fontsPath + fontName))
	{
		std::cerr << "Error al cargar la fuente" << std::endl;
	}

	title = new sf::Text(font, titleString, titleTextSize);


	//Set positions
	float centerX = width / 2.0f;

	titlePosition = sf::Vector2f(centerX - (title->getGlobalBounds().size.x / 2.0f), titleYPos);

	nameRectanglePosition = sf::Vector2f(centerX - (inputRectangleSize.x / 2.0f), nameRectangleYPos);
	passwordRectanglePosition = sf::Vector2f(nameRectanglePosition.x, nameRectanglePosition.y + inputRectanglesSeparation);

	// Login Button (izquierda del centro)
	loginButtonPosition = sf::Vector2f(centerX - (buttonSeparation / 2.0f) - (buttonSize.x / 2.0f), buttonYPos);
	loginButton = new Button(buttonSize, loginButtonPosition, loginButtonText, font, buttonColor, buttonTextColor);

	// Register Button (derecha del centro)
	registerButtonPosition = sf::Vector2f(centerX + (buttonSeparation / 2.0f) - (buttonSize.x / 2.0f), buttonYPos);
	registerButton = new Button(buttonSize, registerButtonPosition, registerButtonText, font, buttonColor, buttonTextColor);

	//Titulo
	title->setFillColor(buttonColor);
	title->setPosition(titlePosition);

	//input text-----------------
	nameText = new sf::Text(font, "", buttonTextSize);
	passwordText = new sf::Text(font, "", buttonTextSize);

	nameText->setFillColor(inputTextColor);
	passwordText->setFillColor(inputTextColor);

	nameText->setPosition(nameRectanglePosition);
	passwordText->setPosition(passwordRectanglePosition);


	nameRectangle = sf::RectangleShape(inputRectangleSize);
	nameRectangle.setPosition(nameRectanglePosition);
	nameRectangle.setFillColor(inputBackgroundColor);
	nameRectangle.setOutlineColor(inputRectangleNotFocussedColor);
	nameRectangle.setOutlineThickness(inputRectangleOutlineThickness);

	passwordRectangle = sf::RectangleShape(inputRectangleSize);
	passwordRectangle.setPosition(passwordRectanglePosition);
	passwordRectangle.setFillColor(inputBackgroundColor);
	passwordRectangle.setOutlineColor(inputRectangleNotFocussedColor);
	passwordRectangle.setOutlineThickness(inputRectangleOutlineThickness);
}

GameState Login::Update()
{


	while (window->isOpen()) {
		//	std::cout << "[DEBUG] Finestra no vàlida o tancada" << std::endl;
		//	return;
		//}
		// 
		// 
		//window->clear(backgroundColor);

		//window->display();



		while (const std::optional event = window->pollEvent()) {

			Client::getInstance()->run();
			Render(window);
			GameState state = EventHandler(*event);
			if (state != GameState::LOGIN) {
				return state;
			}

		}

	}

	/*return GameState::LOGIN;*/

}

void Login::Render(sf::RenderWindow* window)
{

	window->clear(backgroundColor);

	if (loginButton)
		loginButton->draw(*window);
	if (registerButton)
		registerButton->draw(*window);

	window->draw(nameRectangle);
	window->draw(passwordRectangle);
	window->draw(*nameText);
	window->draw(*passwordText);
	window->draw(*title);

	window->display();
}



void Login::setWindow(sf::RenderWindow* win)
{

	this->window = win;

}



GameState Login::EventHandler(const sf::Event& event)
{
	if (event.is<sf::Event::Closed>()) {

		window->close();
	}
	if (loginButton && loginButton->handleEvent(event, *window)) {
		
		if (Client::getInstance()->loginAction(nameText->getString().toAnsiString(), passwordText->getString().toAnsiString())) {
			
			return GameState::LOBBY;
		}
		else {
			std::cout << "[DEBUG] Login Fail" << std::endl;
		}
	}

	if (registerButton && registerButton->handleEvent(event, *window)) {
		
		if (Client::getInstance()->RegisterAction(nameText->getString().toAnsiString(), passwordText->getString().toAnsiString())) {
			return GameState::LOBBY;
		}
		else {
			std::cout << "[DEBUG] Register Fail" << std::endl;
		}
	}


	if (const sf::Event::MouseButtonPressed* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
		sf::Vector2i mousePos = mousePressed->position;

		if (nameRectangle.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
			focus = InputFieldFocussed::NAME;
			nameRectangle.setOutlineColor(inputRectangleFocussedColor);
			passwordRectangle.setOutlineColor(inputRectangleNotFocussedColor);

		}
		else if (passwordRectangle.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
			focus = InputFieldFocussed::PASSWORD;
			passwordRectangle.setOutlineColor(inputRectangleFocussedColor);
			nameRectangle.setOutlineColor(inputRectangleNotFocussedColor);
		}
		else {
			focus = InputFieldFocussed::COUNT;
			nameRectangle.setOutlineColor(inputRectangleNotFocussedColor);
			passwordRectangle.setOutlineColor(inputRectangleNotFocussedColor);
		}
	}

	switch (focus)
	{
	case InputFieldFocussed::NAME:
		if (const sf::Event::TextEntered* textEntered = event.getIf<sf::Event::TextEntered>()) {
			if (textEntered->unicode == 8) { // borrar
				if (!nameInput.empty())
					nameInput.pop_back();
			}
			else if (textEntered->unicode < 128 && nameInput.size() < maxCharacters && std::isalnum(textEntered->unicode)) {
				nameInput += static_cast<char>(textEntered->unicode);
			}
			nameText->setString(nameInput);
		}
		break;
	case InputFieldFocussed::PASSWORD:
		if (const sf::Event::TextEntered* textEntered = event.getIf<sf::Event::TextEntered>()) {
			if (textEntered->unicode == 8) {
				if (!passwordInput.empty())
					passwordInput.pop_back();
			}
			else if (textEntered->unicode < 128 && passwordInput.size() < maxCharacters && std::isalnum(textEntered->unicode)) {
				passwordInput += static_cast<char>(textEntered->unicode);
			}
			passwordText->setString(passwordInput);
		}
		break;
	case InputFieldFocussed::COUNT:
		break;
	default:
		break;
	}

	return GameState::LOGIN;
}
