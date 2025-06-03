#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>

// Nota: Las constantes como PLAYER_HEALTH_MAX, PLAYER_WIDTH, etc.,
// serán accedidas desde Player.cpp, que incluirá Game.h

struct Player {
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    int health;
    int lives;
    bool onGround;
    float shootTimer;
    bool facingRight; // For bullet direction

    Player(); // Declaración del constructor
    void takeDamage();
};

#endif // PLAYER_H