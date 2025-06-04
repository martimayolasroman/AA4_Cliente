#pragma once

#include <SFML/Graphics.hpp>
#include "Button.h"
#include "GameState.h"
#include <string>
#include <optional>

class SearchGameMenu {
private:
    unsigned int width = 0, height = 0;
    sf::RenderWindow* window;
    Button* casualMatchmakingButton;
    sf::Text* titleText;
    sf::Text* m_statusDisplay;

    std::string titleString = "Buscar Partida";
    int titleTextSize = 50;
    float titleYPos = 150.f;

    std::string casualMatchmakingButtonText = "Buscar Partida Amistosa";
    float buttonYPos = 300.f;
    sf::Vector2f buttonSize = { 350.f, 60.f };

    sf::Color backgroundColor = sf::Color::White;
    sf::Color buttonColor = sf::Color(255, 165, 0);
    sf::Color buttonTextColor = sf::Color::White;
    sf::Color titleTextColor = sf::Color(255, 165, 0);

    sf::Font font; // Se carga en el constructor
    bool fontLoadedSuccessfully = false; // Flag para saber si la fuente cargó
    std::string fontsPath = "Assets/Fonts/";
    std::string fontName = "Straw Milky.otf";

    bool m_requestedMatchmaking = false;

    GameState EventHandler(const sf::Event& event);
    GameState onCasualMatchmakingPressed();

    void centerTextOrigin(sf::Text& text);

public:
    SearchGameMenu(sf::RenderWindow* w);
    ~SearchGameMenu();
    GameState Update();
    void Render(sf::RenderWindow* windowToRenderOn);
    void setWindow(sf::RenderWindow* win);
};