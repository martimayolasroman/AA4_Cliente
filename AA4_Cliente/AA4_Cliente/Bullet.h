#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>

// Nota: Las constantes como BULLET_WIDTH, PLAYER_WIDTH, etc.,
// serán accedidas desde Bullet.cpp, que incluirá Game.h

struct Bullet {
    sf::RectangleShape shape;
    sf::Vector2f velocity;

    Bullet(sf::Vector2f pos, bool facingRight); // Declaración del constructor
};

#endif // BULLET_H