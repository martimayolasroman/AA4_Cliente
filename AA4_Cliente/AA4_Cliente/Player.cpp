#include "Game.h" // Incluye Player.h transitivamente y las constantes necesarias
#include <iostream> // Para std::cout

// Definición del constructor de Player
// Las constantes (PLAYER_HEALTH_MAX, PLAYER_WIDTH, etc.) vienen de Game.h
Player::Player() : health(PLAYER_HEALTH_MAX), lives(PLAYER_LIVES_MAX), onGround(false), shootTimer(0.0f), facingRight(true) {
    shape.setSize({ PLAYER_WIDTH, PLAYER_HEIGHT });
    shape.setFillColor(COLOR_PLAYER_BLUE);
    shape.setPosition({ RESPAWN_X, RESPAWN_Y });
    velocity = { 0, 0 };
}

void Player::takeDamage() {
    health--;
    if (health <= 0) {
        lives--;
        if (lives > 0) {
            health = PLAYER_HEALTH_MAX;
            shape.setPosition({ RESPAWN_X, RESPAWN_Y });
            velocity = { 0, 0 };
            std::cout << "Player lost a life! Lives remaining: " << lives << std::endl;
        }
        else {
            std::cout << "GAME OVER!" << std::endl;
        }
    }
    else {
        std::cout << "Player took damage! Health: " << health << std::endl;
    }
}