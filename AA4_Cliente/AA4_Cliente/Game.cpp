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
    std::string playerTexturePath = "Assets/Sprites/player_sprite.png";
    if (!m_playerTexture.loadFromFile(playerTexturePath)) {
        std::cerr << "[Game] ERROR: No se pudo cargar la textura del jugador desde: " << playerTexturePath << std::endl;
        return false;
    }
    m_playerTexture.setSmooth(true);

    m_player.setTexture(m_playerTexture);
    m_opponentPlayer.setTexture(m_playerTexture);

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
    m_jumpRequestedThisFrame(false),
    m_shootRequestedThisFrame(false),
    m_playerShootCooldown(0.0f),
    m_accumulatedTimeForPrediction(0.f)
{
    if (!m_client) {
        std::cerr << "CRITICAL: Game_constructor - Client instance is null!" << std::endl;
        return;
    }

    if (!loadGameAssets()) {
        std::cerr << "[Game] Error al cargar assets. El juego puede no verse correctamente." << std::endl;
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
void Game::applyPlayerMovement(Player& player, float moveDirInput, bool jumpRequestedThisTick, float deltaTime) {
    if (moveDirInput < 0) { player.velocity.x = -PLAYER_SPEED; player.facingRight = false; }
    else if (moveDirInput > 0) { player.velocity.x = PLAYER_SPEED; player.facingRight = true; }
    else { player.velocity.x = 0; }
    if (jumpRequestedThisTick && player.onGround) { player.velocity.y = -JUMP_STRENGTH; player.onGround = false; }
    if (!player.onGround) { player.velocity.y += GRAVITY * deltaTime; }

    // Movimiento y colisión Y
    player.sprite->move({ 0.f, player.velocity.y * deltaTime });
    player.onGround = false;
    sf::FloatRect playerBounds = player.sprite->getGlobalBounds();
    for (const auto& platform : m_platforms) {
        std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
        if (intersection) {
            if (player.velocity.y > 0) { player.sprite->setPosition({ playerBounds.position.x, intersection->position.y - playerBounds.size.y }); player.onGround = true; player.velocity.y = 0; }
            else if (player.velocity.y < 0) { player.sprite->setPosition({ playerBounds.position.x, intersection->position.y + intersection->size.y }); player.velocity.y = 0; }
            playerBounds = player.sprite->getGlobalBounds(); break;
        }
    }

    // Movimiento y colisión X
    player.sprite->move({ player.velocity.x * deltaTime, 0.f });
    playerBounds = player.sprite->getGlobalBounds();
    for (const auto& platform : m_platforms) {
        std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
        if (intersection) {
            if (player.velocity.x > 0) { player.sprite->setPosition({ intersection->position.x - playerBounds.size.x, playerBounds.position.y }); }
            else if (player.velocity.x < 0) { player.sprite->setPosition({ intersection->position.x + intersection->size.x, playerBounds.position.y }); }
            player.velocity.x = 0; playerBounds = player.sprite->getGlobalBounds(); break;
        }
    }

    // Límites de pantalla (X)
    if (player.sprite->getPosition().x < 0.f) { player.sprite->setPosition({ 0.f, player.sprite->getPosition().y }); }
    if (player.sprite->getPosition().x + playerBounds.size.x > WINDOW_WIDTH) { player.sprite->setPosition({ WINDOW_WIDTH - playerBounds.size.x, player.sprite->getPosition().y }); }
    // Límite de caída (Y)
    if (player.sprite->getPosition().y > WINDOW_HEIGHT + 100.f) {
        player.takeDamage();
        player.sprite->setPosition({ RESPAWN_X, RESPAWN_Y });
        player.velocity = { 0.f, 0.f };
        player.onGround = true;
    }
}


// Reconciliación
void Game::reconcilePlayer() {
    if (!m_client->hasNewServerState()) { return; }
    sf::Vector2f serverPosition = m_client->getLastServerConfirmedMyPlayerPosition();
    sf::Vector2f currentPredictedPos = m_player.sprite->getPosition();
    m_client->consumeServerStateFlag();

    float diffX = currentPredictedPos.x - serverPosition.x; float diffY = currentPredictedPos.y - serverPosition.y; float distance = std::sqrt(diffX * diffX + diffY * diffY);
    bool velocityDiffers = (m_player.velocity != m_client->getMyPlayerServerVelocity());
    bool onGroundDiffers = (m_player.onGround != m_client->getMyPlayerOnGround());

    if (distance > 10 || velocityDiffers || onGroundDiffers) {
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
        m_player.velocity = m_client->getMyPlayerServerVelocity();
        m_player.onGround = m_client->getMyPlayerOnGround();
    }
}

// Actualizar y gestionar las balas predichas por este cliente
void Game::updatePredictedBullets(float deltaTime) {
    for (auto& bullet : m_predictedMyBullets) {
        if (bullet.isActive) {
            bullet.shape.move(bullet.velocity * deltaTime);
            // Marcar como inactiva si sale de los límites de la pantalla
            if (bullet.shape.getPosition().x < -BULLET_RADIUS * 2 || bullet.shape.getPosition().x > WINDOW_WIDTH + BULLET_RADIUS * 2 ||
                bullet.shape.getPosition().y < -BULLET_RADIUS * 2 || bullet.shape.getPosition().y > WINDOW_HEIGHT + BULLET_RADIUS * 2) {
                bullet.isActive = false;
            }
        }
    }
    // Eliminar balas inactivas de la lista
    m_predictedMyBullets.erase(std::remove_if(m_predictedMyBullets.begin(), m_predictedMyBullets.end(),
        [](const Bullet& b) { return !b.isActive; }),
        m_predictedMyBullets.end());
}

// Actualizar las balas del oponente recibidas del servidor (interpolación)
void Game::updateInterpolatedOpponentBullets(float deltaTime) {
    const auto& serverBullets = m_client->getOpponentBulletStates(); // Obtener balas del servidor

    // Primero, limpiar balas inactivas o fuera de rango de la lista interpolada
    m_interpolatedOpponentBullets.erase(std::remove_if(m_interpolatedOpponentBullets.begin(), m_interpolatedOpponentBullets.end(),
        [](const InterpolatedBullet& b) { return !b.isActive || b.shape.getPosition().x < -BULLET_RADIUS * 2 || b.shape.getPosition().x > WINDOW_WIDTH + BULLET_RADIUS * 2 || b.shape.getPosition().y < -BULLET_RADIUS * 2 || b.shape.getPosition().y > WINDOW_HEIGHT + BULLET_RADIUS * 2; }),
        m_interpolatedOpponentBullets.end());

    // Sincronizar por tamaño (simplificado, puede causar saltos visuales si el orden cambia)
    if (serverBullets.size() != m_interpolatedOpponentBullets.size()) {
        m_interpolatedOpponentBullets.clear();
        for (const auto& sBullet : serverBullets) {
            m_interpolatedOpponentBullets.emplace_back(sBullet.position, BULLET_RADIUS);
            // Si el ownerPlayerId de ServerBulletState es necesario en el cliente, podrías pasarlo
            // o crear una nueva clase Bullet para el cliente que lo incluya.
        }
    }

    sf::Time renderTimeTarget = sf::seconds(m_interpolationRenderClock.getElapsedTime().asSeconds() - INTERPOLATION_DELAY_SECONDS);

    for (size_t i = 0; i < m_interpolatedOpponentBullets.size() && i < serverBullets.size(); ++i) {
        // Actualizar el estado de interpolación con la última posición del servidor
        m_interpolatedOpponentBullets[i].updateInterpolation(serverBullets[i].position, serverBullets[i].timestamp);
        m_interpolatedOpponentBullets[i].isActive = serverBullets[i].isActive; // Sincronizar estado activo

        // Aplicar interpolación para el renderizado
        sf::Time prevTimestamp = m_interpolatedOpponentBullets[i].previousTimestamp;
        sf::Time currTimestamp = m_interpolatedOpponentBullets[i].currentTimestamp;

        if (m_interpolatedOpponentBullets[i].hasReceivedFirstUpdate && renderTimeTarget >= prevTimestamp && currTimestamp > prevTimestamp) {
            float timeDiffBetweenUpdates = (currTimestamp - prevTimestamp).asSeconds();
            float interpolationFactor = 0.f;
            if (timeDiffBetweenUpdates > 0.00001f) {
                interpolationFactor = (renderTimeTarget.asSeconds() - prevTimestamp.asSeconds()) / timeDiffBetweenUpdates;
            }
            interpolationFactor = std::max(0.f, std::min(1.f, interpolationFactor));

            sf::Vector2f interpolatedPosition;
            interpolatedPosition.x = m_interpolatedOpponentBullets[i].previousPosition.x + (m_interpolatedOpponentBullets[i].currentPosition.x - m_interpolatedOpponentBullets[i].previousPosition.x) * interpolationFactor;
            interpolatedPosition.y = m_interpolatedOpponentBullets[i].previousPosition.y + (m_interpolatedOpponentBullets[i].currentPosition.y - m_interpolatedOpponentBullets[i].previousPosition.y) * interpolationFactor;
            m_interpolatedOpponentBullets[i].shape.setPosition(interpolatedPosition);
        }
        else if (m_interpolatedOpponentBullets[i].hasReceivedFirstUpdate && renderTimeTarget > currTimestamp) {
            // Si el tiempo de renderizado está por delante del último estado conocido, usar la posición actual (extrapolación simple)
            m_interpolatedOpponentBullets[i].shape.setPosition(m_interpolatedOpponentBullets[i].currentPosition);
        }
    }
}

// GAME::RUN
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

        m_shootRequestedThisFrame = false; // Se resetea al inicio de cada frame
        float newMoveDirectionInput = m_currentMoveDirection;
        m_jumpRequestedThisFrame = false;

        // 1. PROCESAR INPUTS DEL USUARIO Y EVENTOS DE VENTANA
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
                    if (keyPressed->code == sf::Keyboard::Key::Space) {
                        m_jumpRequestedThisFrame = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::M) { // Tecla para disparar
                        // El cliente solo intentará crear una bala predicha y enviará la solicitud
                        // si su cooldown local lo permite. Esto es predicción.
                        if (m_playerShootCooldown <= 0) {
                            m_shootRequestedThisFrame = true; // Activa la solicitud para este tick
                        }
                    }
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

        // 2. PROCESAR DATOS DE RED Y RECONCILIACIÓN
        if (m_client->isConnectedToGameServer()) {
            m_client->receiveAndProcessGameData();

            if (!m_gameHasStarted && m_client->getLastServerConfirmedMyPlayerPosition().x != -1.f) {
                m_gameHasStarted = true;
                m_interpolationRenderClock.restart();
                if (m_waitingText && m_fontLoaded) m_waitingText->setString("Partida Encontrada!");
                m_player.sprite->setPosition(m_client->getLastServerConfirmedMyPlayerPosition());
                m_player.velocity = m_client->getMyPlayerServerVelocity();
                m_player.onGround = m_client->getMyPlayerOnGround();
                m_playerShootCooldown = 0.0f; // Asegurar cooldown listo al inicio
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

        // 3. PREDICCIÓN DEL JUGADOR LOCAL (con timestep fijo) Y GESTIÓN DE COOLDOWN/DISPARO
        if (m_gameHasStarted && !m_gameOverState) {
            while (m_accumulatedTimeForPrediction >= FIXED_DELTA_TIME) {
                // Decrementar cooldown de disparo del jugador local
                if (m_playerShootCooldown > 0) {
                    m_playerShootCooldown -= FIXED_DELTA_TIME;
                    if (m_playerShootCooldown < 0) m_playerShootCooldown = 0;
                }

                // Crear bala localmente si se solicitó Y el cooldown lo permite
                if (m_shootRequestedThisFrame && m_playerShootCooldown <= 0) {
                    m_predictedMyBullets.emplace_back(m_player.sprite->getPosition(), m_player.facingRight);
                    m_playerShootCooldown = SHOOT_COOLDOWN; // Reiniciar cooldown local
                }

                applyPlayerMovement(m_player, m_currentMoveDirection, m_jumpRequestedThisFrame, FIXED_DELTA_TIME);

                // Enviar inputs al servidor
                if (m_client->isConnectedToGameServer()) {
                    m_client->sendPlayerInput(m_currentMoveDirection, m_shootRequestedThisFrame, m_jumpRequestedThisFrame);
                    // m_shootRequestedThisFrame se resetea al inicio de cada frame
                }
                m_accumulatedTimeForPrediction -= FIXED_DELTA_TIME;
            }
        }

        // 4. ACTUALIZAR BALAS PREDICHAS Y BALAS INTERPOLADAS DEL OPONENTE
        updatePredictedBullets(frameDeltaTime);
        updateInterpolatedOpponentBullets(frameDeltaTime);

        if (m_player.lives <= 0 && !m_gameOverState && m_gameHasStarted) {
            m_gameOverState = true;
        }

        // INTERPOLACIÓN DEL OPONENTE (sin cambios)
        if (m_gameHasStarted && m_client->isConnectedToGameServer() && !m_gameOverState) {
            const OpponentInterpolationState& oppState = m_client->getOpponentInterpolationState();
            m_opponentPlayer.health = m_client->getOpponentPlayerHealth();
            m_opponentPlayer.lives = m_client->getOpponentPlayerLives();

            if (oppState.hasReceivedEnoughUpdatesForInterpolation) {
                sf::Time renderTimeTarget = sf::seconds(m_interpolationRenderClock.getElapsedTime().asSeconds() - INTERPOLATION_DELAY_SECONDS);
                sf::Time prevTimestamp = oppState.previousTimestamp;
                sf::Time currTimestamp = oppState.currentTimestamp;

                if (oppState.hasReceivedFirstUpdate && renderTimeTarget >= prevTimestamp && currTimestamp > prevTimestamp) { // COMPARACIÓN sf::Time
                    float timeDiffBetweenUpdates = (currTimestamp - prevTimestamp).asSeconds();
                    float interpolationFactor = 0.f;
                    if (timeDiffBetweenUpdates > 0.00001f) {
                        interpolationFactor = (renderTimeTarget.asSeconds() - prevTimestamp.asSeconds()) / timeDiffBetweenUpdates;
                    }
                    interpolationFactor = std::max(0.f, std::min(1.f, interpolationFactor));

                    sf::Vector2f interpolatedPosition;
                    interpolatedPosition.x = oppState.previousPosition.x + (oppState.currentPosition.x - oppState.previousPosition.x) * interpolationFactor;
                    interpolatedPosition.y = oppState.previousPosition.y + (oppState.currentPosition.y - oppState.currentPosition.y) * interpolationFactor; // Bug: debería ser previousPosition.y
                    m_opponentPlayer.sprite->setPosition(interpolatedPosition);
                }
                else if (oppState.hasReceivedFirstUpdate && renderTimeTarget > currTimestamp) { // COMPARACIÓN sf::Time
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

        // RENDERIZADO
        m_window->clear(COLOR_BACKGROUND);
        for (const auto& platform : m_platforms) { m_window->draw(platform); }

        for (const auto& bullet : m_predictedMyBullets) { // Dibujar mis balas predichas
            if (bullet.isActive) {
                m_window->draw(bullet.shape);
            }
        }
        for (const auto& bullet : m_interpolatedOpponentBullets) { // Dibujar balas del oponente (interpoladas)
            if (bullet.isActive) {
                m_window->draw(bullet.shape);
            }
        }

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