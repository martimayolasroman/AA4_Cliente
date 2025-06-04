#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
 

struct Player {
    std::optional<sf::Sprite> sprite;  
    sf::Vector2f velocity;
    int health;
    int lives;
    bool onGround;
    float shootTimer;
    bool facingRight; 

    Player();  
    void takeDamage();
    void setTexture(const sf::Texture& texture);  

};

#endif  