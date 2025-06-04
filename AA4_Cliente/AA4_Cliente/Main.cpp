#pragma once // No es necesario en .cpp pero no hace daño

#include "Client.h"
#include <SFML/Graphics.hpp>

#include "Login.h"
#include "Game.h"
#include "SearchMenu.h"
#include "GameState.h"

// Estas defines ya están en Client.h, podrías quitarlas de aquí para evitar redefiniciones
// si Client.h se incluye antes que este archivo en alguna unidad de compilación.
// #define WIDTH 1280
// #define HEIGHT 720
const unsigned short SERVER_TCP_PORT = 55000;


void main() { // En C++ estándar, main debe devolver int: int main()
    sf::RenderWindow* window = new sf::RenderWindow(sf::VideoMode({ WIDTH, HEIGHT }), "| Cliente Shooter |");
    window->setFramerateLimit(60);

    GameState currentState = GameState::LOGIN;

    Client* clientInstance = Client::getInstance();
    if (!clientInstance->connectToServer(SERVER_TCP_PORT)) {
        std::cerr << "No se pudo conectar al servidor de servicios. Saliendo." << std::endl;
        if (window) delete window;
        // delete clientInstance; // Singleton, manejo de vida más complejo
        return;
    }

    Login loginMenu(window);
    SearchGameMenu searchMenu(window);
    // Game shooterGame se instancia dentro del case

    while (currentState != GameState::EXIT && window->isOpen()) { // Añadir window->isOpen()
        //std::cout << "[Main DEBUG] Llamando a clientInstance->run()";
        clientInstance->run(); // Procesar paquetes TCP

        // Procesar eventos de la ventana aquí si no se hace en cada estado
        // std::optional<sf::Event> event;
        // while((event = window->pollEvent())) {
        //    if(event->is<sf::Event::Closed>()) {
        //        window->close();
        //        currentState = GameState::EXIT;
        //    }
        //    // Pasar evento a los menús si es necesario
        // }
        // Si ya lo haces en los Update() de los menús, está bien.

        switch (currentState) {
        case GameState::LOGIN:
            currentState = loginMenu.Update();
            break;
        case GameState::SEARCH:
            currentState = searchMenu.Update();
            break;
        case GameState::GAME: {
            if (clientInstance->hasMatchBeenFound() && clientInstance->isConnectedToGameServer()) { // Asegurarse que está conectado a UDP
                std::cout << "[Main] Entrando al estado GAME." << std::endl;
                Game shooterGame(window, clientInstance);
                shooterGame.run();
                // Cuando run() termina (ventana cerrada o juego terminado lógicamente)
                // Si la ventana se cerró dentro de Game::run(), window->isOpen() será false.
                if (window->isOpen()) { // Si el juego terminó pero la ventana no se cerró
                    currentState = GameState::SEARCH; // Volver a buscar partida, o EXIT
                    std::cout << "[Main] Saliendo del estado GAME, volviendo a SEARCH." << std::endl;
                }
                else {
                    currentState = GameState::EXIT;
                }
            }
            else {
                std::cout << "[Main] Intento de entrar a GAME sin partida encontrada o sin conexion UDP. Volviendo a SEARCH." << std::endl;
                currentState = GameState::SEARCH;
            }
            break;
        }
        case GameState::EXIT:
            // El bucle while se encargará de salir
            break;
        default:
            std::cout << "[DEBUG] Estado desconocido en Main: " << static_cast<int>(currentState) << ". Saliendo." << std::endl;
            currentState = GameState::EXIT;
            break;
        }
        if (!window->isOpen() && currentState != GameState::EXIT) { // Si la ventana se cerró en un Update() de estado
            currentState = GameState::EXIT;
        }
    }

    std::cout << "[Main] Saliendo de la aplicacion." << std::endl;
    if (window) delete window;
    // delete Client::getInstance(); // El singleton puede necesitar un método de limpieza explícito
                                 // o simplemente dejar que el SO limpie al terminar el programa.
}