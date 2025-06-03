#pragma once

#include "Button.h"
#include <iostream>
#include "GameState.h"
#include "Client.h"

class Login
{
private:
    unsigned int width, height;
    sf::RenderWindow* window;
    Button* loginButton;
    Button* registerButton;

    int buttonTextSize = 24;

    std::string loginButtonText = "Login";
    sf::Vector2f loginButtonPosition;

    std::string registerButtonText = "Register";
    sf::Vector2f registerButtonPosition;

    sf::Color backgroundColor = sf::Color::White;
    sf::Color buttonColor = sf::Color(255, 165, 0);
    sf::Color buttonTextColor = sf::Color::White;

    sf::Font font;
    sf::Text* title;
    std::string titleString = "The DUCK Shooter";
    sf::Vector2f titlePosition;
    int titleYPos = 70;
    int titleTextSize = 70;

    std::string fontsPath = "Assets/Fonts/";
    std::string fontName = "Straw Milky.otf";

    int buttonYPos = 530;
    int buttonSeparation = 300;
    sf::Vector2f buttonSize = sf::Vector2f(200, 60);


    //CUADROS DE INPUT
    sf::Color inputBackgroundColor = sf::Color(240, 240, 240);
    sf::Color inputRectangleFocussedColor = buttonColor;
    sf::Color inputRectangleNotFocussedColor = sf::Color(100, 100, 100);
    sf::Color inputTextColor = sf::Color(70, 70, 70);
    sf::Vector2f inputRectangleSize = sf::Vector2f(400, 40);
    int inputRectangleOutlineThickness = 4;

    enum InputFieldFocussed { NAME, PASSWORD, COUNT };

    const int maxCharacters = 15;
    sf::RectangleShape nameRectangle;
    std::string nameInput;

    sf::RectangleShape passwordRectangle;
    std::string passwordInput;

    sf::Text* nameText;
    sf::Text* passwordText;

    sf::Vector2f nameRectanglePosition;
    int nameRectangleYPos = 250;
    sf::Vector2f passwordRectanglePosition;
    int inputRectanglesSeparation = 150;


    InputFieldFocussed focus;





    GameState EventHandler(const sf::Event& event);

public:
    Login(sf::RenderWindow* w);
    GameState Update();
    void Render(sf::RenderWindow* window);





    void setWindow(sf::RenderWindow* win);
};

