#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System/Angle.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <optional>
#include <algorithm>

#include "Player.h"
#include "Bullet.h" // Asegúrate de que Bullet.h incluye sf::CircleShape

// --- Constantes de Color ---
const sf::Color COLOR_BACKGROUND(236, 236, 236);
const sf::Color COLOR_PLAYER_BLUE(sf::Color::Blue);
const sf::Color COLOR_BULLET_YELLOW(sf::Color::Red);
const sf::Color COLOR_PLATFORM_BROWN(139, 69, 19);
const sf::Color COLOR_DEFAULT_FLOOR_GRAY(100, 100, 100);
const sf::Color COLOR_TEXT_WHITE(sf::Color(255, 165, 0));
const sf::Color COLOR_GAMEOVER_RED(sf::Color::Red);
const sf::Color COLOR_OPPONENT_GREEN(sf::Color::Green);


// --- Constantes de Juego (Añadidas/Modificadas para balas) ---
const unsigned int WINDOW_WIDTH = 1024;
const unsigned int WINDOW_HEIGHT = 768;
const float TILE_SIZE = 32.0f;

const float PLAYER_SPEED = 250.0f;
const float JUMP_STRENGTH = 650.0f;
const float GRAVITY = 1200.0f;
const float PLAYER_WIDTH = TILE_SIZE * 0.9f;
const float PLAYER_HEIGHT = TILE_SIZE * 1.4f;
const int PLAYER_HEALTH_MAX = 5;
const int PLAYER_LIVES_MAX = 3;
const float RESPAWN_X = 100.0f;
const float RESPAWN_Y = 500.0f;

const float BULLET_SPEED = 500.0f;
const float BULLET_RADIUS = 5.0f;
const float SHOOT_COOLDOWN = 2.0f;


// --- Constantes para Interpolación ---
const float INTERPOLATION_DELAY_SECONDS = 0.1f;
const float FIXED_DELTA_TIME = 1.0f / 60.0f; // Vuelvo a 1/60, ya que el servidor usa 16ms (~1/62.5). 1/60 es más común.

// NUEVA ESTRUCTURA: Para interpolar balas del oponente
struct InterpolatedBullet {
    sf::CircleShape shape;
    sf::Vector2f currentPosition;
    sf::Time currentTimestamp;
    sf::Vector2f previousPosition;
    sf::Time previousTimestamp;
    bool hasReceivedFirstUpdate;
    bool isActive; // Si la bala se ha autodestruido o chocado en el servidor

    // Constructor para inicializar una bala interpolada
    InterpolatedBullet(sf::Vector2f pos, float radius) :
        currentPosition(pos), previousPosition(pos),
        hasReceivedFirstUpdate(false), isActive(true)
    {
        shape.setRadius(radius);
        shape.setFillColor(COLOR_BULLET_YELLOW); // O un color diferente para balas del oponente
        shape.setPosition(pos);
    }

    // Método para actualizar la posición para interpolación
    void updateInterpolation(sf::Vector2f newPos, sf::Time newTimestamp) {
        if (!hasReceivedFirstUpdate) {
            previousPosition = newPos;
            previousTimestamp = newTimestamp;
            currentPosition = newPos;
            currentTimestamp = newTimestamp;
            hasReceivedFirstUpdate = true;
        }
        else {
            if (newTimestamp > currentTimestamp) {
                previousPosition = currentPosition;
                previousTimestamp = currentTimestamp;
                currentPosition = newPos;
                currentTimestamp = newTimestamp;
            }
        }
    }
};


class Client;

class Game {
public:
    Game(sf::RenderWindow* window, Client* client_instance);
    ~Game();
    void run();

private:
    float m_currentMoveDirection;
    bool m_jumpRequestedThisFrame;
    bool m_shootRequestedThisFrame; // Para la solicitud de disparo de este frame
    float m_playerShootCooldown;    // Timer de cooldown del jugador local

    sf::RenderWindow* m_window;
    Client* m_client;

    sf::Texture m_playerTexture;
    Player m_player;
    Player m_opponentPlayer;
    bool m_gameHasStarted = false;

    std::vector<sf::RectangleShape> m_platforms;
    std::vector<Bullet> m_predictedMyBullets; // Balas predichas por mi cliente
    std::vector<InterpolatedBullet> m_interpolatedOpponentBullets; // Balas del oponente (del servidor)

    sf::Font m_font;
    bool m_fontLoaded = false;
    std::string m_gameFontPath = "Assets/Fonts/Straw Milky.otf";

    sf::Text* m_healthText;
    sf::Text* m_livesText;
    sf::Text* m_gameOverText;
    sf::Text* m_waitingText;

    bool m_gameOverState;
    sf::Clock m_gameLogicClock;
    sf::Clock m_interpolationRenderClock;

    float m_accumulatedTimeForPrediction;

    void centerTextOrigin(sf::Text& text);
    std::vector<sf::RectangleShape> loadMap(const std::string& filename);

    void applyPlayerMovement(Player& player, float moveDir, bool jumpRequested, float deltaTime);
    void reconcilePlayer();
    bool loadGameAssets();

    void updatePredictedBullets(float deltaTime);
    void updateInterpolatedOpponentBullets(float deltaTime);
};