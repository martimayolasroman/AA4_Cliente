#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp> // Incluye sf::CircleShape y sf::RectangleShape
#include <SFML/System/Vector2.hpp>


struct Bullet {
    sf::CircleShape shape; // Usamos sf::CircleShape para la bala
    sf::Vector2f velocity;
    bool isActive; // Para marcar si la bala sigue viva o debe ser eliminada

    Bullet(sf::Vector2f pos, bool facingRight); // Declaración del constructor
};

#endif // BULLET_H