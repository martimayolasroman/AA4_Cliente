#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp>  
#include <SFML/System/Vector2.hpp>


struct Bullet {
    sf::CircleShape shape;  
    sf::Vector2f velocity;
    bool isActive;  

    Bullet(sf::Vector2f pos, bool facingRight);  
};

#endif  