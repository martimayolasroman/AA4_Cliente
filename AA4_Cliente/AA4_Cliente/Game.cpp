#include "Game.h"
#include "Client.h"
#include <SFML/Window/Event.hpp>
#include <iostream>
#include <algorithm>
#include <cmath> 
#include <fstream>  

// loadMap: Carga el mapa desde un archivo de texto y crea las plataformas.
std::vector<sf::RectangleShape> Game::loadMap(const std::string& filename) {
    std::vector<sf::RectangleShape> platforms_map;
    std::ifstream inputFile(filename);
    std::string line;
    float y_coord = 0;

    if (!inputFile.is_open()) {
        std::cerr << "[Game] Error: No se pudo abrir el archivo del mapa: " << filename << std::endl;
        // Si no se puede abrir el mapa, al menos ponemos un suelo para que no se caiga al vacío.
        sf::RectangleShape floor;
        floor.setSize({ static_cast<float>(WINDOW_WIDTH), TILE_SIZE });
        floor.setFillColor(COLOR_DEFAULT_FLOOR_GRAY);
        floor.setPosition({ 0.f, static_cast<float>(WINDOW_HEIGHT) - TILE_SIZE });
        platforms_map.push_back(floor);
        return platforms_map;
    }

    // Lee el archivo línea por línea para construir el mapa.
    while (std::getline(inputFile, line)) {
        float x_coord = 0;
        for (char c : line) {
            // 'P' significa una plataforma.
            if (c == 'P') {
                sf::RectangleShape platform;
                platform.setSize({ TILE_SIZE, TILE_SIZE });
                platform.setFillColor(COLOR_PLATFORM_BROWN);
                platform.setPosition({ x_coord, y_coord });
                platforms_map.push_back(platform);
            }
            x_coord += TILE_SIZE; // Avanza en X para el siguiente "tile".
        }
        y_coord += TILE_SIZE; // Baja a la siguiente fila.
    }
    inputFile.close();
    return platforms_map;
}

// loadGameAssets: Carga las texturas y configura los sprites de los jugadores.
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

    // Damos colores diferentes para distinguirlos fácilmente.
    m_player.sprite->setColor(sf::Color::Yellow);
    m_opponentPlayer.sprite->setColor(sf::Color::Red);

    return true;
}

// centerTextOrigin: Centra el origen de un texto para que al posicionarlo, se centre de verdad.
void Game::centerTextOrigin(sf::Text& text) {
    if (!m_fontLoaded) return; // Si la fuente no está cargada, no hacemos nada.
    sf::FloatRect text_bounds = text.getLocalBounds();
    text.setOrigin({ text_bounds.size.x / 2.f, text_bounds.size.y / 2.f });
}

// Constructor  
Game::Game(sf::RenderWindow* window, Client* client_instance)
    : m_window(window),
    m_client(client_instance),
    m_player(), // Inicializa el jugador local.
    m_opponentPlayer(), // Inicializa el jugador oponente.
    m_gameHasStarted(false),
    m_fontLoaded(false),
    m_healthText(nullptr), // Punteros a null hasta que se creen los objetos.
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

    // Cargar el mapa.
    m_platforms = loadMap(m_client->mapFilePath);

    // Cargar la fuente para los textos. Si no carga, los textos no se verán.
    if (m_font.openFromFile(m_gameFontPath)) {
        m_fontLoaded = true;
        std::cout << "[Game] Fuente '" << m_gameFontPath << "' cargada correctamente." << std::endl;

        // Crear los textos de interfaz de usuario.
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

// Destructor de Game: Libera la memoria de los textos creados dinámicamente.
Game::~Game() {
    delete m_healthText;
    delete m_livesText;
    delete m_gameOverText;
    delete m_waitingText;
}

// applyPlayerMovement: Aplica la física y el movimiento a un jugador.
void Game::applyPlayerMovement(Player& player, float moveDirInput, bool jumpRequestedThisTick, float deltaTime) {

    if (moveDirInput < 0) { player.velocity.x = -PLAYER_SPEED; player.facingRight = false; }
    else if (moveDirInput > 0) { player.velocity.x = PLAYER_SPEED; player.facingRight = true; }
    else { player.velocity.x = 0; }

    // Aplica el salto si se pidió y el jugador está en el suelo.
    if (jumpRequestedThisTick && player.onGround) { player.velocity.y = -JUMP_STRENGTH; player.onGround = false; }

    // Aplica la gravedad si el jugador no está en el suelo.
    if (!player.onGround) { player.velocity.y += GRAVITY * deltaTime; }

    // Movimiento vertical y detección de colisiones con plataformas.
    player.sprite->move({ 0.f, player.velocity.y * deltaTime });
    player.onGround = false;  
    sf::FloatRect playerBounds = player.sprite->getGlobalBounds();
    for (const auto& platform : m_platforms) {
        std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
        if (intersection) {
            // Si choca por abajo, lo pone encima de la plataforma y resetea la velocidad Y.
            if (player.velocity.y > 0) { player.sprite->setPosition({ playerBounds.position.x, intersection->position.y - playerBounds.size.y }); player.onGround = true; player.velocity.y = 0; }
            // Si choca por arriba, lo empuja hacia abajo y resetea la velocidad Y.
            else if (player.velocity.y < 0) { player.sprite->setPosition({ playerBounds.position.x, intersection->position.y + intersection->size.y }); player.velocity.y = 0; }
            playerBounds = player.sprite->getGlobalBounds();  
            break;  
        }
    }

    // Movimiento horizontal y detección de colisiones con plataformas.
    player.sprite->move({ player.velocity.x * deltaTime, 0.f });
    playerBounds = player.sprite->getGlobalBounds();
    for (const auto& platform : m_platforms) {
        std::optional<sf::FloatRect> intersection = playerBounds.findIntersection(platform.getGlobalBounds());
        if (intersection) {
            // Si choca por la derecha, lo mueve a la izquierda de la plataforma.
            if (player.velocity.x > 0) { player.sprite->setPosition({ intersection->position.x - playerBounds.size.x, playerBounds.position.y }); }
            // Si choca por la izquierda, lo mueve a la derecha de la plataforma.
            else if (player.velocity.x < 0) { player.sprite->setPosition({ intersection->position.x + intersection->size.x, playerBounds.position.y }); }
            player.velocity.x = 0; // Detiene el movimiento horizontal.
            playerBounds = player.sprite->getGlobalBounds();
            break;
        }
    }

    // Límites de pantalla (horizontal). Evita que el jugador se salga de la ventana.
    if (player.sprite->getPosition().x < 0.f) { player.sprite->setPosition({ 0.f, player.sprite->getPosition().y }); }
    if (player.sprite->getPosition().x + playerBounds.size.x > WINDOW_WIDTH) { player.sprite->setPosition({ WINDOW_WIDTH - playerBounds.size.x, player.sprite->getPosition().y }); }

    // Límite de caída (vertical). Si se cae muy abajo, pierde vida y reaparece.
    if (player.sprite->getPosition().y > WINDOW_HEIGHT + 100.f) {
        player.takeDamage();
        player.sprite->setPosition({ RESPAWN_X, RESPAWN_Y }); // Posición de reaparición.
        player.velocity = { 0.f, 0.f }; // Resetea la velocidad.
        player.onGround = true;  
    }
}


// reconcilePlayer: Compara la posición predicha del jugador local con la posición confirmada por el servidor.
// Si hay una diferencia significativa, ajusta la posición del jugador local para que coincida con la del servidor.
void Game::reconcilePlayer() {
    // Solo reconcilia si el cliente ha recibido un nuevo estado del servidor.
    if (!m_client->hasNewServerState()) { return; }

    sf::Vector2f serverPosition = m_client->getLastServerConfirmedMyPlayerPosition(); // La verdad del servidor.
    sf::Vector2f currentPredictedPos = m_player.sprite->getPosition(); // Lo que nuestro cliente predijo.
    m_client->consumeServerStateFlag(); // Marcamos que ya hemos procesado este estado del servidor.

    // Calcula la distancia entre la posición predicha y la del servidor.
    float diffX = currentPredictedPos.x - serverPosition.x;
    float diffY = currentPredictedPos.y - serverPosition.y;
    float distance = std::sqrt(diffX * diffX + diffY * diffY);

    // Comprueba si la velocidad o el estado "en suelo" también difieren.
    bool velocityDiffers = (m_player.velocity != m_client->getMyPlayerServerVelocity());
    bool onGroundDiffers = (m_player.onGround != m_client->getMyPlayerOnGround());

    // Si hay una discrepancia grande (distancia > 5 píxeles, o diferencia en velocidad/suelo), "teletransporta" al jugador a la posición del servidor.
    if (distance > 5 || velocityDiffers || onGroundDiffers) {
        m_player.sprite->setPosition(serverPosition);
        m_player.velocity = m_client->getMyPlayerServerVelocity();
        m_player.onGround = m_client->getMyPlayerOnGround();
    }
}

// updatePredictedBullets: Mueve y gestiona las balas que el propio cliente ha disparado (predicción).
void Game::updatePredictedBullets(float deltaTime) {
    for (auto& bullet : m_predictedMyBullets) {
        if (bullet.isActive) {
            bullet.shape.move(bullet.velocity * deltaTime); // Mueve la bala.
            // Si la bala se sale de la pantalla, la marca como inactiva.
            if (bullet.shape.getPosition().x < -BULLET_RADIUS * 2 || bullet.shape.getPosition().x > WINDOW_WIDTH + BULLET_RADIUS * 2 ||
                bullet.shape.getPosition().y < -BULLET_RADIUS * 2 || bullet.shape.getPosition().y > WINDOW_HEIGHT + BULLET_RADIUS * 2) {
                bullet.isActive = false;
            }
        }
    }
    // Elimina todas las balas inactivas de la lista para no dibujar cosas que ya no existen.
    m_predictedMyBullets.erase(std::remove_if(m_predictedMyBullets.begin(), m_predictedMyBullets.end(),
        [](const Bullet& b) { return !b.isActive; }),
        m_predictedMyBullets.end());
}

void Game::updateInterpolatedOpponentBullets(float deltaTime) {
    const auto& serverBullets = m_client->getOpponentBulletStates(); // Cogemos la lista de balas del oponente que nos ha mandado el servidor.

    // limpiamos las balas que se han salido de la pantalla.
    m_interpolatedOpponentBullets.erase(std::remove_if(m_interpolatedOpponentBullets.begin(), m_interpolatedOpponentBullets.end(),
        [](const InterpolatedBullet& b) {
            return !b.isActive ||
                b.shape.getPosition().x < -BULLET_RADIUS * 2 ||
                b.shape.getPosition().x > WINDOW_WIDTH + BULLET_RADIUS * 2 ||
                b.shape.getPosition().y < -BULLET_RADIUS * 2 ||
                b.shape.getPosition().y > WINDOW_HEIGHT + BULLET_RADIUS * 2;
        }),
        m_interpolatedOpponentBullets.end());

    // Reiniciamos la lista de balas del oponente con las que recibimos del servidor.
    m_interpolatedOpponentBullets.clear();
    for (const auto& sBullet : serverBullets) {
        m_interpolatedOpponentBullets.emplace_back(sBullet.position, BULLET_RADIUS);
        m_interpolatedOpponentBullets.back().isActive = sBullet.isActive;

        m_interpolatedOpponentBullets.back().currentTimestamp = sBullet.timestamp;
        m_interpolatedOpponentBullets.back().previousTimestamp = sBullet.timestamp;
        m_interpolatedOpponentBullets.back().hasReceivedFirstUpdate = true;
    }
}

void Game::run() {
    if (!m_client || !m_window) {
        std::cerr << "[Game] Error: Client o Window no proporcionado a Game::run(). Saliendo." << std::endl;
        return;
    }


    m_client->m_gameOver = false; // Resetear al inicio de la instancia de juego
    m_client->m_gameOverMessage = "";


    m_interpolationRenderClock.restart(); // Reinicia el reloj para la interpolación visual.
    m_gameLogicClock.restart(); // Reinicia el reloj para la lógica del juego.
    m_accumulatedTimeForPrediction = 0.f; // Resetea el tiempo acumulado para la predicción.

    while (m_window->isOpen()) {
        float frameDeltaTime = m_gameLogicClock.restart().asSeconds(); // Tiempo que ha pasado desde el último frame.
        m_accumulatedTimeForPrediction += frameDeltaTime; // Acumula el tiempo para los ticks de física.

        m_shootRequestedThisFrame = false; // Reiniciamos la solicitud de disparo al inicio de cada frame.
        float newMoveDirectionInput = m_currentMoveDirection; // Guarda la dirección de movimiento actual.
        m_jumpRequestedThisFrame = false; // Reiniciamos la solicitud de salto.

        // 1. PROCESAR INPUTS  Y EVENTOS 
        std::optional<sf::Event> opt_event;
        while ((opt_event = m_window->pollEvent())) {
            sf::Event& event = *opt_event;
            if (event.is<sf::Event::Closed>()) {
                m_window->close();
            }

            if (m_gameHasStarted && !m_gameOverState) {
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    // Mover izquierda o derecha.
                    if (keyPressed->code == sf::Keyboard::Key::A || keyPressed->code == sf::Keyboard::Key::Left) {
                        newMoveDirectionInput = -1.f;
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::D || keyPressed->code == sf::Keyboard::Key::Right) {
                        newMoveDirectionInput = 1.f;
                    }
                    // Saltar.
                    if (keyPressed->code == sf::Keyboard::Key::Space) {
                        m_jumpRequestedThisFrame = true;
                    }
                    // Disparar.
                    if (keyPressed->code == sf::Keyboard::Key::M) {
                        // El cliente predice el disparo si el cooldown lo permite.
                        if (m_playerShootCooldown <= 0) {
                            m_shootRequestedThisFrame = true; // Activa la solicitud para este "tick" de juego.
                        }
                    }
                    if (keyPressed->code == sf::Keyboard::Key::T) {
                        std::cout << "[Game - DEBUG] Tecla T presionada. Teleportando localmente a ("
                            << m_teleportTestPosition.x << ", " << m_teleportTestPosition.y << ")" << std::endl;

                        // Mueve el sprite del jugador local directamente
                        // Si tienes una clase Player local para predicción, actualiza su posición
                        m_player.sprite->setPosition(m_teleportTestPosition);
                        // Si m_player es tu representación local para predicción:
                        // m_player.position = m_teleportTestPosition;
                        // m_player.velocity = {0.f, 0.f}; // Detenerlo si se teleporta

                        m_clientSideTeleported = true;
                        // NO enviamos este cambio al servidor como un input normal.
                    }



                }
                if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
                    // Si se suelta la tecla de movimiento y esa era la que estábamos usando, paramos el movimiento.
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
            m_client->receiveAndProcessGameData(); // Recibe y procesa los datos de juego del servidor 

            // Si la partida no ha empezado pero ya tenemos una posición confirmada del servidor, la iniciamos.
            if (!m_gameHasStarted && m_client->getLastServerConfirmedMyPlayerPosition().x != -1.f) {
                m_gameHasStarted = true;
                m_interpolationRenderClock.restart();
                if (m_waitingText && m_fontLoaded) m_waitingText->setString("Partida Encontrada!");
                // Sincroniza la posición, velocidad  
                m_player.sprite->setPosition(m_client->getLastServerConfirmedMyPlayerPosition());
                m_player.velocity = m_client->getMyPlayerServerVelocity();
                m_player.onGround = m_client->getMyPlayerOnGround();
                m_playerShootCooldown = 0.0f; // Asegura que se pueda disparar al inicio.
            }
            if (m_gameHasStarted) {
                m_player.health = m_client->getMyPlayerHealth();
                m_player.lives = m_client->getMyPlayerLives();
            }

            if (m_client->m_gameOver) { // Comprobar si la partida ha terminado
                m_gameOverState = true; // Activa el mensaje de GAME OVER en la UI de Game

                // Reconcilia el estado del jugador local con lo que dice el servidor.
                if (m_gameHasStarted && !m_gameOverState) {
                    reconcilePlayer();
                }
            }
            else if (m_client->hasMatchBeenFound() && m_waitingText && m_fontLoaded) {
                m_waitingText->setString("Conectando al servidor de juego...");
            }


            // 3. PREDICCIÓN DEL JUGADOR LOCAL 
            if (m_gameHasStarted && !m_gameOverState) {
                while (m_accumulatedTimeForPrediction >= FIXED_DELTA_TIME) {
                    // Restamos cooldown del dispro
                    if (m_playerShootCooldown > 0) {
                        m_playerShootCooldown -= FIXED_DELTA_TIME;
                        if (m_playerShootCooldown < 0) m_playerShootCooldown = 0;
                    }

                    // Si el jugador pidió disparar y el cooldown lo permite, crea una bala  
                    if (m_shootRequestedThisFrame && m_playerShootCooldown <= 0) {
                        m_predictedMyBullets.emplace_back(m_player.sprite->getPosition(), m_player.facingRight);
                        m_playerShootCooldown = SHOOT_COOLDOWN;
                    }

                    // Aplica el movimiento al jugador local directamtne sin servidor
                    applyPlayerMovement(m_player, m_currentMoveDirection, m_jumpRequestedThisFrame, FIXED_DELTA_TIME);

                    // Envía los inputs del jugador al servidor.
                    if (m_client->isConnectedToGameServer()) {
                        m_client->sendPlayerInput(m_currentMoveDirection, m_shootRequestedThisFrame, m_jumpRequestedThisFrame);
                    }
                    m_accumulatedTimeForPrediction -= FIXED_DELTA_TIME;
                }
            }

            // 4. ACTUALIZAR BALAS 
            updatePredictedBullets(frameDeltaTime); // Actualiza las balas que nosotros hemos disparado.
            updateInterpolatedOpponentBullets(frameDeltaTime); // Actualiza las balas que ha disparado el oponente (recibidas del servidor).

            if (m_player.lives <= 0 && !m_gameOverState && m_gameHasStarted) {
                m_gameOverState = true;
            }

            // INTERPOLACIÓN DEL OPONENTE ayuda ia
            if (m_gameHasStarted && m_client->isConnectedToGameServer() && !m_gameOverState) {
                const OpponentInterpolationState& oppState = m_client->getOpponentInterpolationState();
                m_opponentPlayer.health = m_client->getOpponentPlayerHealth();
                m_opponentPlayer.lives = m_client->getOpponentPlayerLives();

                if (oppState.hasReceivedEnoughUpdatesForInterpolation) {
                    sf::Time renderTimeTarget = sf::seconds(m_interpolationRenderClock.getElapsedTime().asSeconds() - INTERPOLATION_DELAY_SECONDS);
                    sf::Time prevTimestamp = oppState.previousTimestamp;
                    sf::Time currTimestamp = oppState.currentTimestamp;

                    // Si tenemos suficientes datos y el tiempo de renderizado está entre el estado anterior y el actual, interpolamos.
                    if (oppState.hasReceivedFirstUpdate && renderTimeTarget >= prevTimestamp && currTimestamp > prevTimestamp) {
                        float timeDiffBetweenUpdates = (currTimestamp - prevTimestamp).asSeconds();
                        float interpolationFactor = 0.f;
                        if (timeDiffBetweenUpdates > 0.00001f) {
                            interpolationFactor = (renderTimeTarget.asSeconds() - prevTimestamp.asSeconds()) / timeDiffBetweenUpdates;
                        }
                        interpolationFactor = std::max(0.f, std::min(1.f, interpolationFactor));

                        sf::Vector2f interpolatedPosition;
                        interpolatedPosition.x = oppState.previousPosition.x + (oppState.currentPosition.x - oppState.previousPosition.x) * interpolationFactor;
                        interpolatedPosition.y = oppState.previousPosition.y + (oppState.currentPosition.y - oppState.previousPosition.y) * interpolationFactor;
                        m_opponentPlayer.sprite->setPosition(interpolatedPosition);
                    }
                    // Si el tiempo de renderizado ya ha pasado el último estado conocido, solo usamos la posición actual.
                    else if (oppState.hasReceivedFirstUpdate && renderTimeTarget > currTimestamp) {
                        m_opponentPlayer.sprite->setPosition(oppState.currentPosition);
                    }
                    // Si no hay suficientes actualizaciones para interpolar, usa la posición más reciente.
                    else {
                        m_opponentPlayer.sprite->setPosition(oppState.hasReceivedFirstUpdate ? oppState.currentPosition : oppState.previousPosition);
                    }
                }
                // Si solo hemos recibido la primera actualización,   coloca al oponente en esa posición.
                else if (oppState.hasReceivedFirstUpdate) {
                    m_opponentPlayer.sprite->setPosition(oppState.currentPosition);
                }
            }

            // RENDERIZADO: Dibujar todo en la pantalla.
            m_window->clear(COLOR_BACKGROUND);
            for (const auto& platform : m_platforms) { m_window->draw(platform); } // Dibuja todas las plataformas.

            for (const auto& bullet : m_predictedMyBullets) {
                if (bullet.isActive) {
                    m_window->draw(bullet.shape);
                }
            }
            for (const auto& bullet : m_interpolatedOpponentBullets) {
                if (bullet.isActive) {
                    m_window->draw(bullet.shape);
                }
            }

            if (m_gameHasStarted) {
                if (m_player.sprite) m_window->draw(*m_player.sprite);
                if (m_client->getOpponentInterpolationState().hasReceivedFirstUpdate) {
                    if (m_opponentPlayer.sprite) m_window->draw(*m_opponentPlayer.sprite); // Dibuja al oponente.
                }
                // Dibuja la salud y vidas  
                if (m_fontLoaded) {
                    if (m_healthText) { m_healthText->setString("Health: " + std::to_string(m_player.health)); m_window->draw(*m_healthText); }
                    if (m_livesText) { m_livesText->setString("Lives: " + std::to_string(m_player.lives)); m_window->draw(*m_livesText); }
                }





            }
            // Si la partida no ha empezado, muestra el texto de espera.
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
}