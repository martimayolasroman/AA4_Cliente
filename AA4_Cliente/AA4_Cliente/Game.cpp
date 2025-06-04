#include "Game.h"
#include "Client.h"
#include <SFML/Window/Event.hpp>
#include <iostream> // Para std::cout

// Implementación de loadMap (asumiendo que está igual que tu última versión)
std::vector<sf::RectangleShape> Game::loadMap(const std::string& filename) {
    std::vector<sf::RectangleShape> platforms_map;
    std::ifstream inputFile(filename);
    std::string line;
    float y_coord = 0;

    if (!inputFile.is_open()) {
        std::cerr << "[Game] Error: No se pudo abrir el archivo del mapa: " << filename << std::endl;
        sf::RectangleShape floor;
        floor.setSize({ static_cast<float>(WINDOW_WIDTH), TILE_SIZE });
        floor.setFillColor(COLOR_DEFAULT_FLOOR_GRAY);
        floor.setPosition({ 0.f, static_cast<float>(WINDOW_HEIGHT) - TILE_SIZE });
        platforms_map.push_back(floor);
        return platforms_map;
    }

    while (std::getline(inputFile, line)) {
        float x_coord = 0;
        for (char c : line) {
            if (c == 'P') {
                sf::RectangleShape platform;
                platform.setSize({ TILE_SIZE, TILE_SIZE });
                platform.setFillColor(COLOR_PLATFORM_BROWN);
                platform.setPosition({ x_coord, y_coord });
                platforms_map.push_back(platform);
            }
            x_coord += TILE_SIZE;
        }
        y_coord += TILE_SIZE;
    }
    inputFile.close();
    return platforms_map;
}

// Implementación de centerTextOrigin (asumiendo que está igual)
void Game::centerTextOrigin(sf::Text& text) {
    if (!m_fontLoaded) return;
    sf::FloatRect text_bounds = text.getLocalBounds();
    text.setOrigin({ text_bounds.size.x / 2.f, text_bounds.size.y / 2.f });
}

// Constructor (asumiendo que está igual)
Game::Game(sf::RenderWindow* window, Client* client_instance)
    : m_window(window),
    m_client(client_instance),
    m_player(),
    m_opponentPlayer(),
    m_gameHasStarted(false),
    m_fontLoaded(false),
    m_healthText(nullptr),
    m_livesText(nullptr),
    m_gameOverText(nullptr),
    m_waitingText(nullptr),
    m_gameOverState(false) {

    if (!m_client) {
        std::cerr << "CRITICAL: Game_constructor - Client instance is null!" << std::endl;
        return;
    }

    m_platforms = loadMap(m_client->mapFilePath);

    if (m_font.openFromFile(m_gameFontPath)) {
        m_fontLoaded = true;
        std::cout << "[Game] Fuente '" << m_gameFontPath << "' cargada correctamente." << std::endl;

        m_waitingText = new sf::Text(m_font, "Buscando partida...", 30);
        m_waitingText->setFillColor(sf::Color::Black);
        centerTextOrigin(*m_waitingText);
        m_waitingText->setPosition({ static_cast<float>(WINDOW_WIDTH) / 2.f, static_cast<float>(WINDOW_HEIGHT) / 2.f });

        m_healthText = new sf::Text(m_font, "Health: " + std::to_string(m_player.health), 24);
        m_healthText->setFillColor(COLOR_TEXT_WHITE);
        m_healthText->setPosition({ 10.f, 10.f });

        m_livesText = new sf::Text(m_font, "Lives: " + std::to_string(m_player.lives), 24);
        m_livesText->setFillColor(COLOR_TEXT_WHITE);
        m_livesText->setPosition({ 10.f, 40.f });

        m_gameOverText = new sf::Text(m_font, "GAME OVER", 72);
        m_gameOverText->setFillColor(COLOR_GAMEOVER_RED);
        centerTextOrigin(*m_gameOverText);
        m_gameOverText->setPosition({ static_cast<float>(WINDOW_WIDTH) / 2.f, static_cast<float>(WINDOW_HEIGHT) / 2.f });
    }
    else {
        std::cerr << "[Game] Error: No se pudo cargar la fuente: " << m_gameFontPath << ". Los textos no se mostrarán." << std::endl;
    }
    m_opponentPlayer.shape.setFillColor(COLOR_OPPONENT_GREEN);
}

Game::~Game() {
    delete m_healthText;
    delete m_livesText;
    delete m_gameOverText;
    delete m_waitingText;
}
// En Game.h (o donde definas la clase Game)
// class Game {
//     // ... otros miembros ...
// private:
//     float m_currentMoveDirection;
//     // ... otros miembros ...
// };

// En Game.cpp (constructor)
// Game::Game(sf::RenderWindow* window, Client* client_instance)
//     : m_window(window),
//       m_client(client_instance),
//       // ... otras inicializaciones ...
//       m_currentMoveDirection(0.f) { // Inicializar aquí
//     // ...
// }

// En Game.cpp
void Game::run() {
    if (!m_client) {
        std::cerr << "[Game] Error: Client no proporcionado a Game::run(). Saliendo." << std::endl;
        return;
    }

    while (m_window->isOpen()) {
        float deltaTime = m_clock.restart().asSeconds();
        // moveDirToSend se tomará de m_currentMoveDirection más adelante
        bool shootToSend = false; // Asumimos que el disparo también se manejaría por eventos si fuera necesario

        // 1. Procesar eventos de SFML
        std::optional<sf::Event> opt_event;
        while ((opt_event = m_window->pollEvent())) { // Usando tu forma de pollevent
            sf::Event& event = *opt_event;
            if (event.is<sf::Event::Closed>()) {
                m_window->close();
                // Considera notificar al cliente/servidor sobre la desconexión aquí si es necesario
                return;
            }

            if (m_gameHasStarted && !m_gameOverState) {
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->code == sf::Keyboard::Key::A || keyPressed->code == sf::Keyboard::Key::Left) {
                        m_player.velocity.x = -PLAYER_SPEED; // Para predicción local
                        m_player.facingRight = false;
                        m_currentMoveDirection = -1.f;     // Actualiza la dirección de input
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::D || keyPressed->code == sf::Keyboard::Key::Right) {
                        m_player.velocity.x = PLAYER_SPEED; // Para predicción local
                        m_player.facingRight = true;
                        m_currentMoveDirection = 1.f;      // Actualiza la dirección de input
                    }
                    else if ((keyPressed->code == sf::Keyboard::Key::W || keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::Space) && m_player.onGround) {
                        m_player.velocity.y = -JUMP_STRENGTH; // Para predicción local
                        m_player.onGround = false;
                        // Podrías tener un m_wantsToJump = true; si el salto se envía como un input discreto
                    }
                    // Aquí podrías manejar el input de disparo también, actualizando shootToSend
                    // else if (keyPressed->code == sf::Keyboard::Key::F) { // Ejemplo para disparar
                    //    shootToSend = true; // Se enviará este frame, pero se reseteará. Mejor manejarlo como un pulso.
                    // }
                }
                if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
                    // Solo cambia m_currentMoveDirection si la tecla soltada corresponde a la dirección actual de movimiento
                    if (((keyReleased->code == sf::Keyboard::Key::A || keyReleased->code == sf::Keyboard::Key::Left) && m_currentMoveDirection < 0) ||
                        ((keyReleased->code == sf::Keyboard::Key::D || keyReleased->code == sf::Keyboard::Key::Right) && m_currentMoveDirection > 0)) {
                        m_player.velocity.x = 0; // Para predicción local
                        m_currentMoveDirection = 0.f; // Resetea la dirección de input
                    }
                }
            }
        }

        // 2. Lógica de Red
        if (m_client->isConnectedToGameServer()) {
            m_client->receiveAndProcessGameData();

            sf::Vector2f server_my_pos = m_client->getMyPlayerPosition();
            sf::Vector2f server_opp_pos = m_client->getOpponentPlayerPosition();

            // DEBUGGING OUTPUT (puedes mantenerlo o quitarlo)
            // if (server_my_pos.x != -1.f || server_opp_pos.x != -1.f) {
            //     std::cout << "ServerMyPos=" << server_my_pos.x << ", ServerOppPos=" << server_opp_pos.x << std::endl;
            // }

            if (!m_gameHasStarted && server_my_pos.x != -1.f) {
                m_gameHasStarted = true;
                std::cout << "[Game] Primer estado del servidor recibido. Iniciando juego visualmente." << std::endl;
                if (m_waitingText && m_fontLoaded) m_waitingText->setString("Partida Encontrada!");

                m_player.shape.setPosition(server_my_pos); // Sincroniza posición inicial con el servidor
                m_player.health = m_client->getMyPlayerHealth();
                m_player.lives = m_client->getMyPlayerLives();
            }

            if (m_gameHasStarted) {
                if (server_opp_pos.x != -1.f) { // Solo actualiza si la posición del oponente es válida
                    m_opponentPlayer.shape.setPosition(server_opp_pos);
                }
                // La posición de m_player es manejada por la predicción local.
                // Podrías añadir lógica de reconciliación aquí si server_my_pos difiere mucho de la predicción.
                // Por ejemplo: if (server_my_pos.x != -1.f) m_player.shape.setPosition(server_my_pos);
                // Pero esto puede causar "saltos" si no se hace con suavizado (lerp) o si la latencia es alta.
            }
        }
        else if (m_client->hasMatchBeenFound() && m_waitingText && m_fontLoaded) {
            m_waitingText->setString("Conectando al servidor de juego...");
        }
        else if (m_waitingText && m_fontLoaded && !m_gameHasStarted) {
            m_waitingText->setString("Buscando partida...");
        }

        // 3. Actualizar Lógica de Juego LOCAL (Predicción)
        if (m_gameHasStarted && !m_gameOverState) {
            // Usar la dirección de input determinada por los eventos
            float moveDirToSend = m_currentMoveDirection;

            // Aplicar gravedad (predicción local)
            if (!m_player.onGround) {
                m_player.velocity.y += GRAVITY * deltaTime;
            }

            // Mover horizontalmente (predicción local usando la velocidad actualizada por eventos)
            m_player.shape.move({ m_player.velocity.x * deltaTime, 0.f });
            sf::FloatRect playerBounds = m_player.shape.getGlobalBounds();

            // Colisiones horizontales (predicción local)
            for (const auto& platform : m_platforms) {
                std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
                if (intersection) {
                    if (m_player.velocity.x > 0) { // Movimiento a la derecha
                        m_player.shape.setPosition({ intersection->position.x - playerBounds.size.x, playerBounds.position.y });
                    }
                    else if (m_player.velocity.x < 0) { // Movimiento a la izquierda
                        m_player.shape.setPosition({ intersection->position.x + intersection->size.x, playerBounds.position.y });
                    }
                    m_player.velocity.x = 0; // Detener movimiento horizontal al colisionar
                    playerBounds = m_player.shape.getGlobalBounds(); // Actualizar bounds para la siguiente comprobación o para el movimiento vertical
                    break;
                }
            }

            // Mover verticalmente (predicción local)
            m_player.shape.move({ 0.f, m_player.velocity.y * deltaTime });
            m_player.onGround = false; // Asumir que no está en el suelo hasta que se compruebe colisión
            playerBounds = m_player.shape.getGlobalBounds();

            // Colisiones verticales (predicción local)
            for (const auto& platform : m_platforms) {
                std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
                if (intersection) {
                    if (m_player.velocity.y > 0) { // Cayendo
                        m_player.shape.setPosition({ playerBounds.position.x, intersection->position.y - playerBounds.size.y });
                        m_player.onGround = true;
                        m_player.velocity.y = 0;
                    }
                    else if (m_player.velocity.y < 0) { // Saltando hacia arriba
                        m_player.shape.setPosition({ playerBounds.position.x, intersection->position.y + intersection->size.y });
                        m_player.velocity.y = 0; // Detener movimiento hacia arriba al golpear techo
                    }
                    playerBounds = m_player.shape.getGlobalBounds();
                    break;
                }
            }

            // Límites de la ventana (predicción local)
            if (m_player.shape.getPosition().x < 0.f) {
                m_player.shape.setPosition({ 0.f, m_player.shape.getPosition().y });
                m_player.velocity.x = 0;
            }
            if (m_player.shape.getPosition().x + m_player.shape.getSize().x > WINDOW_WIDTH) {
                m_player.shape.setPosition({ WINDOW_WIDTH - m_player.shape.getSize().x, m_player.shape.getPosition().y });
                m_player.velocity.x = 0;
            }
            if (m_player.shape.getPosition().y > WINDOW_HEIGHT + TILE_SIZE * 2) { // Si cae muy abajo
                // Lógica de "muerte" o respawn aquí
                // Por ejemplo: m_player.shape.setPosition(spawnPoint); m_player.lives--;
            }

            // DEBUGGING OUTPUT (puedes mantenerlo o quitarlo)
            // std::cout << "PredictedMyPos=" << m_player.shape.getPosition().x << std::endl;

            // Enviar input al servidor
            m_client->sendPlayerInput(moveDirToSend, shootToSend);
            // shootToSend debería resetearse después de enviar si es un evento de un solo frame.
            // Por ejemplo: if (shootToSend) shootToSend = false;
            // O manejarlo de forma que solo se active cuando se presiona la tecla de disparo.

            // Actualizar UI
            if (m_fontLoaded) {
                if (m_healthText) m_healthText->setString("Health: " + std::to_string(m_player.health));
                if (m_livesText) m_livesText->setString("Lives: " + std::to_string(m_player.lives));
            }
        }

        // 4. Renderizado
        m_window->clear(COLOR_BACKGROUND);
        for (const auto& platform : m_platforms) {
            m_window->draw(platform);
        }

        if (m_gameHasStarted) {
            // DEBUGGING OUTPUT (puedes mantenerlo o quitarlo)
            // std::cout << "Rendering MyPos=" << m_player.shape.getPosition().x 
            //           << ", OppPos=" << m_opponentPlayer.shape.getPosition().x << std::endl;
            m_window->draw(m_player.shape);
            m_window->draw(m_opponentPlayer.shape);
            if (m_fontLoaded) {
                if (m_healthText) m_window->draw(*m_healthText);
                if (m_livesText) m_window->draw(*m_livesText);
            }
        }
        else if (m_waitingText && m_fontLoaded) {
            m_window->draw(*m_waitingText);
        }

        if (m_gameOverState && m_gameOverText && m_fontLoaded) {
            m_window->draw(*m_gameOverText);
        }
        m_window->display();
    }
}