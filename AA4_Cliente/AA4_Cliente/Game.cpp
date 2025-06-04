#include "Game.h"
#include "Client.h" 
#include <SFML/Window/Event.hpp>
#include <iostream>
#include <algorithm> // Para std::min/max

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

void Game::centerTextOrigin(sf::Text& text) {
    if (!m_fontLoaded) return;
    sf::FloatRect text_bounds = text.getLocalBounds();
    // Usando la sintaxis de tu último Game.cpp para setOrigin
    // Esto asume que tu sf::FloatRect tiene un miembro 'size' de tipo sf::Vector2f
    text.setOrigin({ text_bounds.size.x / 2.f, text_bounds.size.y / 2.f });
}

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
    m_gameOverState(false),
    m_currentMoveDirection(0.f) // Inicializar aquí
{
    if (!m_client) {
        std::cerr << "CRITICAL: Game_constructor - Client instance is null!" << std::endl;
        return;
    }

    m_platforms = loadMap(m_client->mapFilePath);

    if (m_font.openFromFile(m_gameFontPath)) {
        m_fontLoaded = true;
        std::cout << "[Game] Fuente '" << m_gameFontPath << "' cargada correctamente." << std::endl;

        m_waitingText = new sf::Text(m_font, "Buscando partida...", 30);
        if (m_waitingText) {
            m_waitingText->setFillColor(sf::Color::Black);
            centerTextOrigin(*m_waitingText);
            m_waitingText->setPosition({ static_cast<float>(WINDOW_WIDTH) / 2.f, static_cast<float>(WINDOW_HEIGHT) / 2.f });
        }

        m_healthText = new sf::Text(m_font, "Health: " + std::to_string(PLAYER_HEALTH_MAX), 24);
        if (m_healthText) {
            m_healthText->setFillColor(COLOR_TEXT_WHITE);
            m_healthText->setPosition({ 10.f, 10.f });
        }

        m_livesText = new sf::Text(m_font, "Lives: " + std::to_string(PLAYER_LIVES_MAX), 24);
        if (m_livesText) {
            m_livesText->setFillColor(COLOR_TEXT_WHITE);
            m_livesText->setPosition({ 10.f, 40.f });
        }

        m_gameOverText = new sf::Text(m_font, "GAME OVER", 72);
        if (m_gameOverText) {
            m_gameOverText->setFillColor(COLOR_GAMEOVER_RED);
            centerTextOrigin(*m_gameOverText);
            m_gameOverText->setPosition({ static_cast<float>(WINDOW_WIDTH) / 2.f, static_cast<float>(WINDOW_HEIGHT) / 2.f });
        }
    }
    else {
        std::cerr << "[Game] Error: No se pudo cargar la fuente: " << m_gameFontPath << ". Los textos no se mostrarán." << std::endl;
    }

    m_opponentPlayer.shape.setFillColor(COLOR_OPPONENT_GREEN);
    m_player.shape.setFillColor(COLOR_PLAYER_BLUE);
}

Game::~Game() {
    delete m_healthText;
    delete m_livesText;
    delete m_gameOverText;
    delete m_waitingText;
}

void Game::run() {
    if (!m_client || !m_window) {
        std::cerr << "[Game] Error: Client o Window no proporcionado a Game::run(). Saliendo." << std::endl;
        return;
    }

    m_interpolationRenderClock.restart(); // Reloj para el tiempo de renderizado de la interpolación

    while (m_window->isOpen()) {
        float deltaTime = m_gameLogicClock.restart().asSeconds(); // Para la lógica de predicción local
        bool shootToSend = false;

        std::optional<sf::Event> opt_event;
        while ((opt_event = m_window->pollEvent())) {
            sf::Event& event = *opt_event;
            if (event.is<sf::Event::Closed>()) {
                m_window->close();
                return;
            }

            if (m_gameHasStarted && !m_gameOverState) {
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->code == sf::Keyboard::Key::A || keyPressed->code == sf::Keyboard::Key::Left) {
                        m_player.velocity.x = -PLAYER_SPEED; m_player.facingRight = false; m_currentMoveDirection = -1.f;
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::D || keyPressed->code == sf::Keyboard::Key::Right) {
                        m_player.velocity.x = PLAYER_SPEED; m_player.facingRight = true; m_currentMoveDirection = 1.f;
                    }
                    else if ((keyPressed->code == sf::Keyboard::Key::W || keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::Space) && m_player.onGround) {
                        m_player.velocity.y = -JUMP_STRENGTH; m_player.onGround = false;
                    }
                }
                if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
                    if (((keyReleased->code == sf::Keyboard::Key::A || keyReleased->code == sf::Keyboard::Key::Left) && m_currentMoveDirection < 0) ||
                        ((keyReleased->code == sf::Keyboard::Key::D || keyReleased->code == sf::Keyboard::Key::Right) && m_currentMoveDirection > 0)) {
                        m_player.velocity.x = 0; m_currentMoveDirection = 0.f;
                    }
                }
            }
        }

        if (m_client->isConnectedToGameServer()) {
            m_client->receiveAndProcessGameData();

            sf::Vector2f server_my_pos = m_client->getMyPlayerPosition();
            if (server_my_pos.x != -1.f) {
                if (!m_gameHasStarted) {
                    m_gameHasStarted = true;
                    m_interpolationRenderClock.restart();
                    std::cout << "[Game] Primer estado del servidor recibido. Iniciando juego visualmente." << std::endl;
                    if (m_waitingText && m_fontLoaded) m_waitingText->setString("Partida Encontrada!");

                    m_player.shape.setPosition(server_my_pos);
                }
                m_player.health = m_client->getMyPlayerHealth();
                m_player.lives = m_client->getMyPlayerLives();
            }

            if (m_gameHasStarted && !m_gameOverState) {
                m_client->sendPlayerInput(m_currentMoveDirection, shootToSend);
            }
        }
        else if (m_client->hasMatchBeenFound() && m_waitingText && m_fontLoaded) {
            m_waitingText->setString("Conectando al servidor de juego...");
        }
        else if (m_waitingText && m_fontLoaded && !m_gameHasStarted) {
            m_waitingText->setString("Buscando partida...");
        }

        if (m_gameHasStarted && !m_gameOverState) {
            // Lógica de predicción del jugador local
            if (!m_player.onGround) {
                m_player.velocity.y += GRAVITY * deltaTime;
            }
            m_player.shape.move({ m_player.velocity.x * deltaTime, 0.f });
            sf::FloatRect playerBounds = m_player.shape.getGlobalBounds();

            for (const auto& platform : m_platforms) {
                std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
                if (intersection) {
                    if (m_player.velocity.x > 0) {
                        m_player.shape.setPosition({ intersection->position.x - playerBounds.size.x, playerBounds.position.y });
                    }
                    else if (m_player.velocity.x < 0) {
                        m_player.shape.setPosition({ intersection->position.x + intersection->size.x, playerBounds.position.y });
                    }
                    m_player.velocity.x = 0;
                    playerBounds = m_player.shape.getGlobalBounds();
                    break;
                }
            }

            m_player.shape.move({ 0.f, m_player.velocity.y * deltaTime });
            m_player.onGround = false;
            playerBounds = m_player.shape.getGlobalBounds();

            for (const auto& platform : m_platforms) {
                std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
                if (intersection) {
                    if (m_player.velocity.y > 0) {
                        m_player.shape.setPosition({ playerBounds.position.x, intersection->position.y - playerBounds.size.y });
                        m_player.onGround = true; m_player.velocity.y = 0;
                    }
                    else if (m_player.velocity.y < 0) {
                        m_player.shape.setPosition({ playerBounds.position.x, intersection->position.y + intersection->size.y });
                        m_player.velocity.y = 0;
                    }
                    playerBounds = m_player.shape.getGlobalBounds();
                    break;
                }
            }
            if (m_player.shape.getPosition().x < 0.f) { m_player.shape.setPosition({ 0.f, m_player.shape.getPosition().y }); m_player.velocity.x = 0; }
            if (m_player.shape.getPosition().x + playerBounds.size.x > WINDOW_WIDTH) { m_player.shape.setPosition({ WINDOW_WIDTH - playerBounds.size.x, m_player.shape.getPosition().y }); m_player.velocity.x = 0; }
            // Lógica de muerte por caída, etc.
            // ...
        }

        // Interpolación y actualización del Oponente
        if (m_gameHasStarted && m_client->isConnectedToGameServer()) {
            const OpponentInterpolationState& oppState = m_client->getOpponentInterpolationState();
            m_opponentPlayer.health = m_client->getOpponentPlayerHealth();
            m_opponentPlayer.lives = m_client->getOpponentPlayerLives();

            if (oppState.hasReceivedEnoughUpdatesForInterpolation) {
                // Usamos el tiempo del m_interpolationRenderClock que se reinicia cuando m_gameHasStarted es true.
                float renderTimeTarget = m_interpolationRenderClock.getElapsedTime().asSeconds() - INTERPOLATION_DELAY_SECONDS;

                sf::Time prevTimestamp = oppState.previousTimestamp;
                sf::Time currTimestamp = oppState.currentTimestamp;

                // Asegurarse que currTimestamp es posterior a prevTimestamp para evitar división por cero o negativa
                if (renderTimeTarget >= prevTimestamp.asSeconds() && currTimestamp > prevTimestamp) {
                    float timeDiffBetweenUpdates = (currTimestamp - prevTimestamp).asSeconds();
                    float interpolationFactor = 0.f;

                    // Solo interpola si hay una diferencia de tiempo válida
                    if (timeDiffBetweenUpdates > 0.00001f) {
                        interpolationFactor = (renderTimeTarget - prevTimestamp.asSeconds()) / timeDiffBetweenUpdates;
                    }

                    interpolationFactor = std::max(0.f, std::min(1.f, interpolationFactor)); // Clampear [0, 1]

                    sf::Vector2f interpolatedPosition;
                    interpolatedPosition.x = oppState.previousPosition.x + (oppState.currentPosition.x - oppState.previousPosition.x) * interpolationFactor;
                    interpolatedPosition.y = oppState.previousPosition.y + (oppState.currentPosition.y - oppState.previousPosition.y) * interpolationFactor;
                    m_opponentPlayer.shape.setPosition(interpolatedPosition);

                }
                else if (renderTimeTarget > currTimestamp.asSeconds()) {
                    // El tiempo de renderizado está más allá del último estado conocido, usar la posición más reciente (extrapolación simple)
                    m_opponentPlayer.shape.setPosition(oppState.currentPosition);
                }
                else {
                    // Demasiado pronto para este par de timestamps, o timestamps inválidos.
                    // Usar la posición anterior o la actual si es la única que tenemos.
                    m_opponentPlayer.shape.setPosition(oppState.hasReceivedFirstUpdate ? oppState.currentPosition : oppState.previousPosition);
                }
            }
            else if (oppState.hasReceivedFirstUpdate) {
                // No tenemos suficientes datos para interpolar, solo mostrar la última posición conocida.
                m_opponentPlayer.shape.setPosition(oppState.currentPosition);
            }
        }

        // Renderizado
        m_window->clear(COLOR_BACKGROUND);
        for (const auto& platform : m_platforms) {
            m_window->draw(platform);
        }

        if (m_gameHasStarted) {
            m_window->draw(m_player.shape);
            if (m_client->getOpponentInterpolationState().hasReceivedFirstUpdate) {
                m_window->draw(m_opponentPlayer.shape);
            }
            if (m_fontLoaded) {
                if (m_healthText) {
                    m_healthText->setString("Health: " + std::to_string(m_player.health));
                    m_window->draw(*m_healthText);
                }
                if (m_livesText) {
                    m_livesText->setString("Lives: " + std::to_string(m_player.lives));
                    m_window->draw(*m_livesText);
                }
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