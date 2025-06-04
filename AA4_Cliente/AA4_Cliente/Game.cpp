#include "Game.h"
#include "Client.h"
#include <SFML/Window/Event.hpp>
#include <iostream>
#include <algorithm>
#include <cmath> // Para std::sqrt

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

bool Game::loadGameAssets()
{
    std::string playerTexturePath = "Assets/Sprites/player_sprite.png"; // ¡CAMBIA ESTA RUTA!
    if (!m_playerTexture.loadFromFile(playerTexturePath)) {
        std::cerr << "[Game] ERROR: No se pudo cargar la textura del jugador desde: " << playerTexturePath << std::endl;
        return false;
    }
    m_playerTexture.setSmooth(true); // Opcional, para mejor apariencia al escalar

    // Si el oponente usa la misma textura:
    m_player.setTexture(m_playerTexture);
    m_opponentPlayer.setTexture(m_playerTexture); // Ambos usan la misma textura


    m_player.sprite->setColor(sf::Color::Yellow);
    m_opponentPlayer.sprite->setColor(sf::Color::Red); 

    return true;
}

void Game::centerTextOrigin(sf::Text& text) {
    if (!m_fontLoaded) return;
    sf::FloatRect text_bounds = text.getLocalBounds();
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
    m_currentMoveDirection(0.f),
    m_jumpRequestedThisFrame(false), // <--- NUEVO: Inicializar
    m_accumulatedTimeForPrediction(0.f)
{
    if (!m_client) {
        std::cerr << "CRITICAL: Game_constructor - Client instance is null!" << std::endl;
        return;
    }

    if (!loadGameAssets()) {
        std::cerr << "[Game] Error al cargar assets. El juego puede no verse correctamente." << std::endl;
        // Puedes decidir cerrar el juego o continuar con sprites vacíos/fallback.
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

    
}

Game::~Game() {
    delete m_healthText;
    delete m_livesText;
    delete m_gameOverText;
    delete m_waitingText;
}

// Lógica de movimiento/física que se usará para predicción y re-simulación
void Game::applyPlayerMovement(Player& player, float moveDirInput, bool jumpRequestedThisTick, float deltaTime) { // <--- MODIFICADO
    // Aplicar movimiento horizontal
    if (moveDirInput < 0) {
        player.velocity.x = -PLAYER_SPEED; player.facingRight = false;
    }
    else if (moveDirInput > 0) {
        player.velocity.x = PLAYER_SPEED; player.facingRight = true;
    }
    else {
        player.velocity.x = 0;
    }

    // Aplicar salto si se solicitó y el jugador está en el suelo (predicción cliente)
    if (jumpRequestedThisTick && player.onGround) { // <--- NUEVO: Procesar solicitud de salto
        player.velocity.y = -JUMP_STRENGTH;
        player.onGround = false;
    }

    // Aplicar gravedad
    if (!player.onGround) {
        player.velocity.y += GRAVITY * deltaTime;
    }

    // Mover primero en Y y luego en X para colisiones (es un orden común)
    player.sprite->move({ 0.f, player.velocity.y * deltaTime });
    player.onGround = false; // Asumir que ya no está en el suelo hasta que se detecte colisión
    sf::FloatRect playerBounds = player.sprite->getGlobalBounds();

    for (const auto& platform : m_platforms) {
        std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
        if (intersection) {
            if (player.velocity.y > 0) { // Moviéndose hacia abajo, chocó con el suelo
                player.sprite->setPosition({ playerBounds.position.x, intersection->position.y - playerBounds.size.y });
                player.onGround = true;
                player.velocity.y = 0; // Detener movimiento vertical
            }
            else if (player.velocity.y < 0) { // Moviéndose hacia arriba, chocó con el techo
                player.sprite->setPosition({ playerBounds.position.x, intersection->position.y + intersection->size.y });
                player.velocity.y = 0; // Detener movimiento vertical
            }
            playerBounds = player.sprite->getGlobalBounds(); // Actualizar bounds después del movimiento vertical
            break;
        }
    }

    // Mover en X
    player.sprite->move({ player.velocity.x * deltaTime, 0.f });
    playerBounds = player.sprite->getGlobalBounds(); // Actualizar bounds después del movimiento horizontal

    for (const auto& platform : m_platforms) {
        std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
        if (intersection) {
            if (player.velocity.x > 0) { // Moviéndose a la derecha, chocó con una pared
                player.sprite->setPosition({ intersection->position.x - playerBounds.size.x, playerBounds.position.y });
            }
            else if (player.velocity.x < 0) { // Moviéndose a la izquierda, chocó con una pared
                player.sprite->setPosition({ intersection->position.x + intersection->size.x, playerBounds.position.y });
            }
            player.velocity.x = 0; // Detener movimiento horizontal
            playerBounds = player.sprite->getGlobalBounds(); // Actualizar bounds
            break;
        }
    }

    // Límites de pantalla (ajuste si el jugador sale de los límites horizontales)
    if (player.sprite->getPosition().x < 0.f) { player.sprite->setPosition({ 0.f, player.sprite->getPosition().y }); }
    if (player.sprite->getPosition().x + playerBounds.size.x > WINDOW_WIDTH) { player.sprite->setPosition({ WINDOW_WIDTH - playerBounds.size.x, player.sprite->getPosition().y }); }
    // Considerar límites Y también para caídas mortales o techos si es necesario
}

// RECONCILIACIÓN SIMPLE: AJUSTAR POSICIÓN, VELOCIDAD Y ONGROUND
void Game::reconcilePlayer() {
    if (!m_client->hasNewServerState()) {
        return; // No hay nuevo estado del servidor para procesar
    }

    sf::Vector2f serverPosition = m_client->getLastServerConfirmedMyPlayerPosition();
    sf::Vector2f currentPredictedPos = m_player.sprite->getPosition();
    m_client->consumeServerStateFlag(); // Marcar el estado del servidor como procesado

    float diffX = currentPredictedPos.x - serverPosition.x;
    float diffY = currentPredictedPos.y - serverPosition.y;
    float distance = std::sqrt(diffX * diffX + diffY * diffY);

    const float RECONCILIATION_THRESHOLD = 1.0f; // Umbral de distancia en píxeles para reconciliar

    // También comprobamos si la velocidad o el estado onGround difieren
    bool velocityDiffers = (m_player.velocity != m_client->getMyPlayerServerVelocity());
    bool onGroundDiffers = (m_player.onGround != m_client->getMyPlayerOnGround());

    if (distance > RECONCILIATION_THRESHOLD || velocityDiffers || onGroundDiffers) { // <--- MODIFICADO: Reconciliar también por velocidad/onGround
        std::cout << "[Game RECONCILE - SIMPLE] DISCREPANCY! Snapping player to server state. Distance: " << distance
            << " VelocityDiff: " << (velocityDiffers ? "YES" : "NO")
            << " OnGroundDiff: " << (onGroundDiffers ? "YES" : "NO") << std::endl;

        std::cout << "  Server Pos: (" << serverPosition.x << "," << serverPosition.y
            << ") Vel: (" << m_client->getMyPlayerServerVelocity().x << "," << m_client->getMyPlayerServerVelocity().y
            << ") OnGround: " << (m_client->getMyPlayerOnGround() ? "True" : "False") << std::endl;
        std::cout << "  Predicted Pos: (" << currentPredictedPos.x << "," << currentPredictedPos.y
            << ") Vel: (" << m_player.velocity.x << "," << m_player.velocity.y
            << ") OnGround: " << (m_player.onGround ? "True" : "False") << std::endl;

        m_player.sprite->setPosition(serverPosition);
        m_player.velocity = m_client->getMyPlayerServerVelocity(); // <--- NUEVO: Ajustar velocidad
        m_player.onGround = m_client->getMyPlayerOnGround();     // <--- NUEVO: Ajustar onGround

        //std::cout << "  Player state AFTER snap: Pos(" << m_player.shape.getPosition().x << "," << m_player.shape.getPosition().y
        //    << ") Vel(" << m_player.velocity.x << "," << m_player.velocity.y
        //    << ") OnGround(" << m_player.onGround ? "True" : "False" << ")" << std::endl;
    }
}

// GAME::RUN (copiado de tu versión anterior, con llamadas a reconcilePlayer y applyPlayerMovement)
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

        bool shootPressedThisFrame = false; // Puedes activar esto con tu lógica de disparo
        float newMoveDirectionInput = m_currentMoveDirection;
        m_jumpRequestedThisFrame = false; // <--- NUEVO: Resetear cada frame para que sea un evento de un solo tick

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
                    if (keyPressed->code == sf::Keyboard::Key::W || keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::Space) {
                        m_jumpRequestedThisFrame = true; // <--- NUEVO: Activar flag de salto
                    }
                    // if (keyPressed->code == sf::Keyboard::Key::Enter) shootPressedThisFrame = true; // Ejemplo de disparo
                }
                if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
                    if (((keyReleased->code == sf::Keyboard::Key::A || keyReleased->code == sf::Keyboard::Key::Left) && newMoveDirectionInput < 0) ||
                        ((keyReleased->code == sf::Keyboard::Key::D || keyReleased->code == sf::Keyboard::Key::Right) && newMoveDirectionInput > 0)) {
                        newMoveDirectionInput = 0.f;
                    }
                    // No resetear m_jumpRequestedThisFrame en keyReleased, ya que es un evento de un solo disparo por pulsación
                }
            }
        }
        m_currentMoveDirection = newMoveDirectionInput;

        // 2. PROCESAR DATOS DE RED Y RECONCILIACIÓN
        if (m_client->isConnectedToGameServer()) {
            m_client->receiveAndProcessGameData();

            if (!m_gameHasStarted && m_client->getLastServerConfirmedMyPlayerPosition().x != -1.f) {
                m_gameHasStarted = true;
                m_interpolationRenderClock.restart();
                if (m_waitingText && m_fontLoaded) m_waitingText->setString("Partida Encontrada!");
                m_player.sprite->setPosition(m_client->getLastServerConfirmedMyPlayerPosition());
                // <--- NUEVO: Inicializar velocidad y onGround al inicio del juego con valores del servidor
                m_player.velocity = m_client->getMyPlayerServerVelocity();
                m_player.onGround = m_client->getMyPlayerOnGround();
            }
            if (m_gameHasStarted) {
                m_player.health = m_client->getMyPlayerHealth();
                m_player.lives = m_client->getMyPlayerLives();
            }

            if (m_gameHasStarted && !m_gameOverState) {
                reconcilePlayer(); // Reconcilia el estado del jugador local
            }
        }
        else if (m_client->hasMatchBeenFound() && m_waitingText && m_fontLoaded) {
            m_waitingText->setString("Conectando al servidor de juego...");
        }
        else if (m_waitingText && m_fontLoaded && !m_gameHasStarted) {
            m_waitingText->setString("Buscando partida...");
        }

        // 3. PREDICCIÓN DEL JUGADOR LOCAL (con timestep fijo)
        if (m_gameHasStarted && !m_gameOverState) {
            while (m_accumulatedTimeForPrediction >= FIXED_DELTA_TIME) {
                // Pasar la solicitud de salto a applyPlayerMovement para la predicción local
                applyPlayerMovement(m_player, m_currentMoveDirection, m_jumpRequestedThisFrame, FIXED_DELTA_TIME); // <--- MODIFICADO: Pasar jumpRequestedThisFrame

                if (m_client->isConnectedToGameServer()) {
                    // Enviar la solicitud de salto al servidor
                    m_client->sendPlayerInput(m_currentMoveDirection, shootPressedThisFrame, m_jumpRequestedThisFrame); // <--- MODIFICADO: Enviar jumpRequestedThisFrame
                }
                // Si m_jumpRequestedThisFrame fue true en este tick de predicción, se considera "consumido" para el siguiente tick.
                // shootPressedThisFrame = false; // Resetear si es un evento de un solo frame

                m_accumulatedTimeForPrediction -= FIXED_DELTA_TIME;
            }
        }

        if (m_player.lives <= 0 && !m_gameOverState && m_gameHasStarted) {
            m_gameOverState = true;
        }

        // INTERPOLACIÓN DEL OPONENTE (sin cambios, la lógica ya estaba)
        if (m_gameHasStarted && m_client->isConnectedToGameServer() && !m_gameOverState) {
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
                    m_opponentPlayer.sprite->setPosition(interpolatedPosition);
                }
                else if (renderTimeTarget > currTimestamp.asSeconds()) {
                    m_opponentPlayer.sprite->setPosition(oppState.currentPosition);
                }
                else {
                    m_opponentPlayer.sprite->setPosition(oppState.hasReceivedFirstUpdate ? oppState.currentPosition : oppState.previousPosition);
                }
            }
            else if (oppState.hasReceivedFirstUpdate) {
                m_opponentPlayer.sprite->setPosition(oppState.currentPosition);
            }
        }

        // RENDERIZADO (sin cambios)
        m_window->clear(COLOR_BACKGROUND);
        for (const auto& platform : m_platforms) { m_window->draw(platform); }

        if (m_gameHasStarted) {
            if (m_player.sprite) m_window->draw(*m_player.sprite);
            if (m_client->getOpponentInterpolationState().hasReceivedFirstUpdate) {
                if (m_opponentPlayer.sprite) m_window->draw(*m_opponentPlayer.sprite);
            }
            if (m_fontLoaded) {
                if (m_healthText) { m_healthText->setString("Health: " + std::to_string(m_player.health)); m_window->draw(*m_healthText); }
                if (m_livesText) { m_livesText->setString("Lives: " + std::to_string(m_player.lives)); m_window->draw(*m_livesText); }
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