#include "Client.h"
#include <SFML/Graphics.hpp>
#include <iostream>

#include "Login.h"
#include "Game.h"
#include "SearchMenu.h"
#include "GameState.h"

const unsigned int MAIN_WINDOW_WIDTH = 1024;
const unsigned int MAIN_WINDOW_HEIGHT = 768;
const unsigned short SERVER_TCP_PORT = 55000;

int main() {
    sf::RenderWindow* window = new sf::RenderWindow(sf::VideoMode({ MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT }), "| Cliente Shooter 2D |");
    if (!window) {
        std::cerr << "Error crítico: No se pudo crear la ventana de SFML." << std::endl;
        return -1;
    }
    window->setFramerateLimit(60);

    GameState currentState = GameState::LOGIN;

    Client* clientInstance = Client::getInstance();
    if (!clientInstance) {
        std::cerr << "Error crítico: No se pudo obtener la instancia del Cliente." << std::endl;
        delete window;
        return -1;
    }

    if (!clientInstance->connectToServer(SERVER_TCP_PORT)) {
        std::cerr << "No se pudo conectar al servidor de servicios. Saliendo." << std::endl;
        delete window;
        return -1;
    }

    SearchGameMenu searchMenu(window);

    std::cout << "[Main] Iniciando bucle principal del cliente..." << std::endl;

    while (currentState != GameState::EXIT && window->isOpen()) {
        clientInstance->run();

        switch (currentState) {
        case GameState::LOGIN: {
            Login loginMenu(window);
            currentState = loginMenu.Update();
        }
            break;
        case GameState::SEARCH: {
            currentState = searchMenu.Update();
        }
            break;
        case GameState::GAME: {
            if (clientInstance->hasMatchBeenFound() && clientInstance->isConnectedToGameServer()) {
                std::cout << "[Main] Entrando al estado GAME." << std::endl;
                Game shooterGame(window, clientInstance);
                currentState = shooterGame.run();
                std::cout << "[Main] NUEVO CURRENT STATE.: " << (int)currentState << std::endl;
            break;
        }
        case GameState::EXIT:
            std::cout << "[Main] Estado EXIT alcanzado. Preparando para cerrar." << std::endl;
            break;
        default:
            std::cerr << "[Main] Estado desconocido en Main: " << static_cast<int>(currentState) << ". Saliendo." << std::endl;
            currentState = GameState::EXIT;
            break;
        }

        if (!window->isOpen() && currentState != GameState::EXIT) {
            std::cout << "[Main] Ventana cerrada en un estado de menú. Saliendo." << std::endl;
            currentState = GameState::EXIT;
        }
    }

    std::cout << std::endl << std::endl << "[Main] Saliendo de la aplicación." << std::endl;

    if (window) {
        delete window;
        window = nullptr;
    }

    return 0;
}