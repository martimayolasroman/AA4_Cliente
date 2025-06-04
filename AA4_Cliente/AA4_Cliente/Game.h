#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp> // No se usa aún, pero estaba
#include <SFML/System/Angle.hpp> // No se usa aún, pero estaba
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <optional>

#include "Player.h" // Asumo que Player.h y Bullet.h existen y son correctos
#include "Bullet.h"

// --- Constantes de Color ---
const sf::Color COLOR_BACKGROUND(236, 236, 236);
const sf::Color COLOR_PLAYER_BLUE(sf::Color::Blue);
const sf::Color COLOR_BULLET_YELLOW(sf::Color::Red); // Ajusta si el nombre es confuso
const sf::Color COLOR_PLATFORM_BROWN(139, 69, 19);
const sf::Color COLOR_DEFAULT_FLOOR_GRAY(100, 100, 100);
const sf::Color COLOR_TEXT_WHITE(sf::Color::White);
const sf::Color COLOR_GAMEOVER_RED(sf::Color::Red);
const sf::Color COLOR_OPPONENT_GREEN(sf::Color::Green); // Para el oponente

// --- Constantes de Juego ---
const unsigned int WINDOW_WIDTH = 1024; // Debe coincidir con tu #define WIDTH si lo usas globalmente
const unsigned int WINDOW_HEIGHT = 768; // Debe coincidir con tu #define HEIGHT
const float TILE_SIZE = 32.0f;

// Player
 const float PLAYER_SPEED = 250.0f; // Estas están en Player.h o deberían
 const float JUMP_STRENGTH = 550.0f;
 const float GRAVITY = 1200.0f;
 const float PLAYER_WIDTH = TILE_SIZE * 0.9f;
 const float PLAYER_HEIGHT = TILE_SIZE * 1.4f;
 const int PLAYER_HEALTH_MAX = 5;
 const int PLAYER_LIVES_MAX = 3;
 const float RESPAWN_X = 100.0f;
 const float RESPAWN_Y = 100.0f;

 //Bullet
 const float BULLET_SPEED = 500.0f;
 const float BULLET_WIDTH = 10.0f;
 const float BULLET_HEIGHT = 5.0f;
 const float SHOOT_COOLDOWN = 0.5f;

class Client; // Forward declaration

class Game {
public:
    Game(sf::RenderWindow* window, Client* client_instance);
    ~Game();
    void run();

private:
    float m_currentMoveDirection;

    sf::RenderWindow* m_window;
    Client* m_client;

    Player m_player;
    Player m_opponentPlayer;
    bool m_gameHasStarted = false;

    std::vector<sf::RectangleShape> m_platforms;
    std::vector<Bullet> m_bullets; // Balas locales del jugador

    sf::Font m_font;
    bool m_fontLoaded = false; // Para saber si la fuente cargó
    std::string m_gameFontPath = "Assets/Fonts/Straw Milky.otf"; // O la fuente que uses

    sf::Text* m_healthText;
    sf::Text* m_livesText;
    sf::Text* m_gameOverText;
    sf::Text* m_waitingText;

    bool m_gameOverState;
    sf::Clock m_clock;

    // Funciones auxiliares
    void centerTextOrigin(sf::Text& text);
    std::vector<sf::RectangleShape> loadMap(const std::string& filename); // Declaración movida aquí
};