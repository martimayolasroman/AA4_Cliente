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

    m_interpolationRenderClock.restart();
    m_gameLogicClock.restart();
    m_accumulatedTimeForPrediction = 0.f;

    while (m_window->isOpen()) {
        float frameDeltaTime = m_gameLogicClock.restart().asSeconds();
        m_accumulatedTimeForPrediction += frameDeltaTime;

        bool shootPressedThisFrame = false;
        float newMoveDirectionInput = m_currentMoveDirection;

        std::optional<sf::Event> opt_event;
        while ((opt_event = m_window->pollEvent())) {
            sf::Event& event = *opt_event;
            if (event.is<sf::Event::Closed>()) {
                m_window->close();
            }

            if (m_gameHasStarted && !m_gameOverState) {
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->code == sf::Keyboard::Key::A || keyPressed->code == sf::Keyboard::Key::Left) {
                        newMoveDirectionInput = -1.f;
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::D || keyPressed->code == sf::Keyboard::Key::Right) {
                        newMoveDirectionInput = 1.f;
                    }
                    if ((keyPressed->code == sf::Keyboard::Key::W || keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::Space) && m_player.onGround) {
                        m_player.velocity.y = -JUMP_STRENGTH;
                        m_player.onGround = false;
                    }
                    // if (keyPressed->code == sf::Keyboard::Key::Enter) shootPressedThisFrame = true; // Ejemplo
                }
                if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
                    if (((keyReleased->code == sf::Keyboard::Key::A || keyReleased->code == sf::Keyboard::Key::Left) && newMoveDirectionInput < 0) ||
                        ((keyReleased->code == sf::Keyboard::Key::D || keyReleased->code == sf::Keyboard::Key::Right) && newMoveDirectionInput > 0)) {
                        newMoveDirectionInput = 0.f;
                    }
                }
            }
        }
        m_currentMoveDirection = newMoveDirectionInput;

        if (m_client->isConnectedToGameServer()) {
            m_client->receiveAndProcessGameData();

            if (!m_gameHasStarted && m_client->getLastServerConfirmedMyPlayerPosition().x != -1.f) {
                m_gameHasStarted = true;
                m_interpolationRenderClock.restart();
                if (m_waitingText && m_fontLoaded) m_waitingText->setString("Partida Encontrada!");
                m_player.shape.setPosition(m_client->getLastServerConfirmedMyPlayerPosition());
            }
            if (m_gameHasStarted) {
                m_player.health = m_client->getMyPlayerHealth();
                m_player.lives = m_client->getMyPlayerLives();
            }

            if (m_gameHasStarted && !m_gameOverState) {
                // Añadir log antes de llamar a reconcilePlayer
                // if (m_client->hasNewServerState()) {
                //     std::cout << "[Game::run] Client has new server state. Calling reconcilePlayer()." << std::endl;
                // }
                reconcilePlayer();
            }
        }
        else if (m_client->hasMatchBeenFound() && m_waitingText && m_fontLoaded) {
            m_waitingText->setString("Conectando al servidor de juego...");
        }
        else if (m_waitingText && m_fontLoaded && !m_gameHasStarted) {
            m_waitingText->setString("Buscando partida...");
        }

        if (m_gameHasStarted && !m_gameOverState) {
            while (m_accumulatedTimeForPrediction >= FIXED_DELTA_TIME) {
                applyPlayerMovement(m_player, m_currentMoveDirection, FIXED_DELTA_TIME);

                if (m_client->isConnectedToGameServer()) {
                    m_client->sendPlayerInput(m_currentMoveDirection, shootPressedThisFrame);
                }
                // shootPressedThisFrame = false; // Resetear si es un evento de un solo frame
                m_accumulatedTimeForPrediction -= FIXED_DELTA_TIME;
            }
        }

        if (m_player.lives <= 0 && !m_gameOverState && m_gameHasStarted) { // Solo si el juego ha empezado
            m_gameOverState = true;
            // ...
        }

        // INTERPOLACIÓN DEL OPONENTE (sin cambios)
        if (m_gameHasStarted && m_client->isConnectedToGameServer() && !m_gameOverState) {
            // ... (tu código de interpolación de oponente)
            const OpponentInterpolationState& oppState = m_client->getOpponentInterpolationState();
            m_opponentPlayer.health = m_client->getOpponentPlayerHealth();
            m_opponentPlayer.lives = m_client->getOpponentPlayerLives();

            if (oppState.hasReceivedEnoughUpdatesForInterpolation) {
                float renderTimeTarget = m_interpolationRenderClock.getElapsedTime().asSeconds() - INTERPOLATION_DELAY_SECONDS;
                sf::Time prevTimestamp = oppState.previousTimestamp;
                sf::Time currTimestamp = oppState.currentTimestamp;

                if (renderTimeTarget >= prevTimestamp.asSeconds() && currTimestamp > prevTimestamp) {
                    float timeDiffBetweenUpdates = (currTimestamp - prevTimestamp).asSeconds();
                    float interpolationFactor = 0.f;
                    if (timeDiffBetweenUpdates > 0.00001f) {
                        interpolationFactor = (renderTimeTarget - prevTimestamp.asSeconds()) / timeDiffBetweenUpdates;
                    }
                    interpolationFactor = std::max(0.f, std::min(1.f, interpolationFactor));

                    sf::Vector2f interpolatedPosition;
                    interpolatedPosition.x = oppState.previousPosition.x + (oppState.currentPosition.x - oppState.previousPosition.x) * interpolationFactor;
                    interpolatedPosition.y = oppState.previousPosition.y + (oppState.currentPosition.y - oppState.previousPosition.y) * interpolationFactor;
                    m_opponentPlayer.shape.setPosition(interpolatedPosition);
                }
                else if (renderTimeTarget > currTimestamp.asSeconds()) {
                    m_opponentPlayer.shape.setPosition(oppState.currentPosition);
                }
                else {
                    m_opponentPlayer.shape.setPosition(oppState.hasReceivedFirstUpdate ? oppState.currentPosition : oppState.previousPosition);
                }
            }
            else if (oppState.hasReceivedFirstUpdate) {
                m_opponentPlayer.shape.setPosition(oppState.currentPosition);
            }
        }


        // RENDERIZADO (sin cambios)
        m_window->clear(COLOR_BACKGROUND);
        for (const auto& platform : m_platforms) { m_window->draw(platform); }
        // for (const auto& bullet : m_bullets) { m_window->draw(bullet.shape); }

        if (m_gameHasStarted) {
            m_window->draw(m_player.shape);
            if (m_client->getOpponentInterpolationState().hasReceivedFirstUpdate) {
                m_window->draw(m_opponentPlayer.shape);
            }
            if (m_fontLoaded) {
                if (m_healthText) { /* ... */ m_healthText->setString("Health: " + std::to_string(m_player.health)); m_window->draw(*m_healthText); }
                if (m_livesText) { /* ... */ m_livesText->setString("Lives: " + std::to_string(m_player.lives)); m_window->draw(*m_livesText); }
            }
        }
        else if (m_waitingText && m_fontLoaded) {
            m_window->draw(*m_waitingText);
        }
        if (m_gameOverState && m_gameOverText && m_fontLoaded) {
            m_window->draw(*m_gameOverText);
        }
        m_window->display();

        if (!m_window->isOpen()) {
            break;
        }
    }
}

void Game::applyPlayerMovement(Player& player, float moveDirInput, float deltaTime) {
    if (moveDirInput < 0) {
        player.velocity.x = -PLAYER_SPEED; player.facingRight = false;
    }
    else if (moveDirInput > 0) {
        player.velocity.x = PLAYER_SPEED; player.facingRight = true;
    }
    else {
        player.velocity.x = 0;
    }

    if (!player.onGround) {
        player.velocity.y += GRAVITY * deltaTime;
    }
    player.shape.move({ player.velocity.x * deltaTime, 0.f });
    sf::FloatRect playerBounds = player.shape.getGlobalBounds();

    for (const auto& platform : m_platforms) {
        std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
        if (intersection) {
            if (player.velocity.x > 0) {
                player.shape.setPosition({ intersection->position.x - playerBounds.size.x, playerBounds.position.y });
            }
            else if (player.velocity.x < 0) {
                player.shape.setPosition({ intersection->position.x + intersection->size.x, playerBounds.position.y });
            }
            player.velocity.x = 0;
            playerBounds = player.shape.getGlobalBounds();
            break;
        }
    }

    player.shape.move({ 0.f, player.velocity.y * deltaTime });
    player.onGround = false;
    playerBounds = player.shape.getGlobalBounds();

    for (const auto& platform : m_platforms) {
        std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
        if (intersection) {
            if (player.velocity.y > 0) {
                player.shape.setPosition({ playerBounds.position.x, intersection->position.y - playerBounds.size.y });
                player.onGround = true; player.velocity.y = 0;
            }
            else if (player.velocity.y < 0) {
                player.shape.setPosition({ playerBounds.position.x, intersection->position.y + intersection->size.y });
                player.velocity.y = 0;
            }
            playerBounds = player.shape.getGlobalBounds();
            break;
        }
    }
    if (player.shape.getPosition().x < 0.f) { player.shape.setPosition({ 0.f, player.shape.getPosition().y }); }
    if (player.shape.getPosition().x + playerBounds.size.x > WINDOW_WIDTH) { player.shape.setPosition({ WINDOW_WIDTH - playerBounds.size.x, player.shape.getPosition().y }); }
}

// RECONCILIACIÓN SIMPLE: AJUSTAR POSICIÓN SIN RE-SIMULACIÓN DE INPUTS
void Game::reconcilePlayer() {
    if (!m_client->hasNewServerState()) {
        return; // No hay nuevo estado del servidor para procesar
    }

    sf::Vector2f serverPosition = m_client->getLastServerConfirmedMyPlayerPosition();
    sf::Vector2f currentPredictedPos = m_player.shape.getPosition();
    m_client->consumeServerStateFlag(); // Marcar el estado del servidor como procesado

    float diffX = currentPredictedPos.x - serverPosition.x;
    float diffY = currentPredictedPos.y - serverPosition.y;
    float distance = std::sqrt(diffX * diffX + diffY * diffY);

    const float RECONCILIATION_THRESHOLD = 1.0f; // Umbral de distancia para reconciliar

    if (distance > RECONCILIATION_THRESHOLD) {
        std::cout << "[Game RECONCILE - SIMPLE] DISCREPANCY! Snapping player to server position. Distance: " << distance << std::endl;
        std::cout << "  Server position to set: (" << serverPosition.x << "," << serverPosition.y << ")" << std::endl;
        std::cout << "  Player position BEFORE snap: (" << m_player.shape.getPosition().x << "," << m_player.shape.getPosition().y << ")" << std::endl;

        m_player.shape.setPosition(serverPosition);

        // Opcional: Resetear velocidad si la discrepancia es muy grande y no quieres que la predicción continúe con una velocidad "incorrecta"
        // m_player.velocity = {0.f, 0.f}; // O la velocidad que el servidor pudiera enviar.

        std::cout << "  Player position AFTER snap: (" << m_player.shape.getPosition().x << "," << m_player.shape.getPosition().y << ")" << std::endl;
    }
    // No hay re-simulación de inputs pendientes aquí. La predicción normal en Game::run tomará el relevo.
}
