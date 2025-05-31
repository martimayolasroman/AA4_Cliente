#pragma once

#include "Button.h" // Asumo que Button.h sigue siendo necesario y está disponible
#include <iostream>
#include "GameState.h" // Asumo que GameState.h sigue siendo necesario y está disponible
#include "Client.h"    // Asumo que Client.h sigue siendo necesario y está disponible

class SearchGameMenu
{
private:
    unsigned int width, height;
    sf::RenderWindow* window;
    Button* casualMatchmakingButton; // Cambiado de loginButton/registerButton

    int buttonTextSize = 24;

    std::string casualMatchmakingButtonText = "Casual Matchmaking"; // Nuevo texto del botón
    sf::Vector2f casualMatchmakingButtonPosition; // Nueva posición del botón

    sf::Color backgroundColor = sf::Color::White;
    sf::Color buttonColor = sf::Color(255, 165, 0); // Puedes ajustar si quieres
    sf::Color buttonTextColor = sf::Color::White;   // Puedes ajustar si quieres

    sf::Font font;
    sf::Text* title;
    std::string titleString = "SEARCH GAME"; // Nuevo título
    sf::Vector2f titlePosition;
    int titleYPos = 70;
    int titleTextSize = 100; // Puedes ajustar el tamaño del título si es necesario

    std::string fontsPath = "Assets/Fonts/"; // Mantengo la ruta de las fuentes
    std::string fontName = "Straw Milky.otf";  // Mantengo el nombre de la fuente

    int buttonYPos = 300; // Ajusta la posición Y del botón como necesites
    // int buttonSeparation = 300; // No necesario ya que solo hay un botón
    sf::Vector2f buttonSize = sf::Vector2f(300, 60); // Ajusta el tamaño del botón como necesites


    // Los campos de input de Login no son necesarios aquí
    // enum InputFieldFocussed { NAME, PASSWORD, COUNT }; // No necesario
    // ... (resto de variables de input eliminadas)
    // InputFieldFocussed focus; // No necesario


    // Función que se llamará al pulsar el botón
    void onCasualMatchmakingPressed();

    // GameState EventHandler(const sf::Event& event); // Cambiado el tipo de retorno si es necesario, o mantenido si GameState se usa de forma genérica
    GameState EventHandler(const sf::Event& event);


public:
    SearchGameMenu(sf::RenderWindow* w);
    ~SearchGameMenu(); // Añadido destructor para limpiar memoria
    GameState Update(); // El estado que devuelve podría ser SEARCH_GAME o similar
    void Render(sf::RenderWindow* window);

    void setWindow(sf::RenderWindow* win);
};
