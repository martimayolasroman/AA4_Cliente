#include "Client.h" // Tu clase Client que maneja la red
#include <SFML/Graphics.hpp> // Para sf::RenderWindow, sf::VideoMode, etc.
#include <iostream>    // Para std::cout, std::cerr

#include "Login.h"      // Tu clase para la pantalla de Login
#include "Game.h"       // Tu clase para la lógica del juego principal
#include "SearchMenu.h" // Tu clase para la pantalla de búsqueda de partida
#include "GameState.h"  // Tu enum GameState

// Estas defines ya están en Client.h (WIDTH, HEIGHT), podrías quitarlas de aquí
// para evitar redefiniciones si Client.h se incluye antes en alguna unidad de compilación.
// #define WIDTH 1280 // Si usas el de Client.h, este es 1280
// #define HEIGHT 720 // Si usas el de Client.h, este es 720
// Pero Game.h usa:
// const unsigned int WINDOW_WIDTH = 1024;
// const unsigned int WINDOW_HEIGHT = 768;
// ¡DEBES UNIFICAR ESTO! Usaré los de Game.h por ahora ya que son const unsigned int.

const unsigned int MAIN_WINDOW_WIDTH = 1024;  // Unificado con Game.h
const unsigned int MAIN_WINDOW_HEIGHT = 768; // Unificado con Game.h
const unsigned short SERVER_TCP_PORT = 55000; // Puerto del Servidor de Servicios


// La firma estándar de main en C++ es `int main()`
int main() {
    sf::RenderWindow* window = new sf::RenderWindow(sf::VideoMode({ MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT }), "| Cliente Shooter 2D |");
    if (!window) {
        std::cerr << "Error crítico: No se pudo crear la ventana de SFML." << std::endl;
        return -1; // Salir si la ventana no se puede crear
    }
    window->setFramerateLimit(60); // Limitar FPS para consistencia y no quemar CPU

    GameState currentState = GameState::LOGIN; // Estado inicial

    Client* clientInstance = Client::getInstance();
    if (!clientInstance) { // Buena práctica verificar si getInstance devolvió null (aunque no debería si es un singleton bien hecho)
        std::cerr << "Error crítico: No se pudo obtener la instancia del Cliente." << std::endl;
        delete window;
        return -1;
    }

    if (!clientInstance->connectToServer(SERVER_TCP_PORT)) {
        std::cerr << "No se pudo conectar al servidor de servicios. Saliendo." << std::endl;
        // Aquí podrías mostrar un mensaje en la ventana o un pop-up antes de cerrar.
        // Por ahora, simplemente cerramos.
        delete window;
        // No necesitas borrar clientInstance aquí si es un singleton manejado globalmente
        // y se limpiará al final del programa o con un método de shutdown explícito.
        return -1; // Salir si no hay conexión al servidor
    }

    // Crear instancias de los menús/pantallas del juego
    Login loginMenu(window);
    SearchGameMenu searchMenu(window);
    // Game shooterGame se instancia dentro del case GameState::GAME cuando sea necesario

    std::cout << "[Main] Iniciando bucle principal del cliente..." << std::endl;

    while (currentState != GameState::EXIT && window->isOpen()) {
        // Procesar la lógica de red TCP del cliente (recibir respuestas de login, matchmaking, etc.)
        // Esto se hace una vez por iteración del bucle principal.
        clientInstance->run();

        // La lógica de recepción de UDP (receiveAndProcessGameData) se llama DENTRO de Game::run()
        // cuando el estado es GAME.

        switch (currentState) {
        case GameState::LOGIN:
            currentState = loginMenu.Update();
            break;
        case GameState::SEARCH:
            // Antes de entrar a SEARCH, podrías querer resetear flags del cliente
            // como m_matchFound si vienes de una partida anterior.
            // clientInstance->resetMatchFlags(); // Método hipotético
            currentState = searchMenu.Update();
            break;
        case GameState::GAME: {
            // Solo entrar al estado GAME si realmente se encontró una partida y estamos conectados al servidor UDP
            if (clientInstance->hasMatchBeenFound() && clientInstance->isConnectedToGameServer()) {
                std::cout << "[Main] Entrando al estado GAME." << std::endl;
                Game shooterGame(window, clientInstance); // Crear instancia del juego
                shooterGame.run(); // Este es un bucle bloqueante hasta que Game::run() termina

                // Cuando shooterGame.run() termina, significa que el juego ha finalizado
                // o la ventana se cerró dentro de Game::run().
                if (window->isOpen()) {
                    // Si la ventana sigue abierta, el juego terminó lógicamente (ej. Game Over, partida finalizada)
                    // Volver al menú de búsqueda o a un menú post-partida.
                    std::cout << "[Main] Saliendo del estado GAME, volviendo a SEARCH." << std::endl;
                    currentState = GameState::SEARCH;
                    //clientInstance->resetMatchFlags(); // <- Necesitarás implementar este método en Client.h/cpp
                    // para resetear m_matchFound, m_isConnectedToGameServer (parcialmente), etc.
                    // para permitir buscar una nueva partida correctamente.
                }
                else {
                    // Si la ventana se cerró dentro de Game::run(), entonces salimos.
                    currentState = GameState::EXIT;
                }
            }
            else {
                // Si se intentó ir a GAME sin las condiciones, es un error de flujo.
                // Volver a un estado seguro.
                std::cerr << "[Main] Intento de entrar a GAME sin partida encontrada o sin conexión UDP. Volviendo a SEARCH." << std::endl;
                currentState = GameState::SEARCH;
            }
            break;
        }
        case GameState::EXIT:
            // El bucle while se encargará de salir.
            // Aquí se podría hacer alguna limpieza final si es necesario antes de que el programa termine.
            std::cout << "[Main] Estado EXIT alcanzado. Preparando para cerrar." << std::endl;
            break;
        default:
            std::cerr << "[Main] Estado desconocido en Main: " << static_cast<int>(currentState) << ". Saliendo." << std::endl;
            currentState = GameState::EXIT;
            break;
        }

        // Comprobación adicional: si la ventana se cerró en el Update() de un menú (LOGIN o SEARCH)
        if (!window->isOpen() && currentState != GameState::EXIT) {
            std::cout << "[Main] Ventana cerrada en un estado de menú. Saliendo." << std::endl;
            currentState = GameState::EXIT;
        }
    }

    std::cout << "[Main] Saliendo de la aplicación." << std::endl;

    // Limpieza de recursos
    // El cliente (singleton) usualmente no se borra aquí explícitamente a menos que tengas un método `Client::shutdown()`.
    // Dejar que el SO limpie la memoria del singleton al terminar el programa es común.
    // Si `Client::getInstance()` usa `new`, y no hay un `delete` correspondiente en algún lugar,
    // técnicamente es una fuga de memoria, pero para un programa que termina, a menudo se ignora.
    // Una mejor gestión sería un `std::unique_ptr` o un método de limpieza explícito.

    if (window) {
        delete window;
        window = nullptr;
    }

    return 0; // Indicar salida exitosa
}