#include "Game.h" 
#include <iostream>  

 
Player::Player() : health(PLAYER_HEALTH_MAX), lives(PLAYER_LIVES_MAX), onGround(false), shootTimer(0.0f), facingRight(true) {

    velocity = { 0, 0 };
    sprite->setPosition({ RESPAWN_X, RESPAWN_Y });
}

void Player::takeDamage() {
    health--;
    if (health <= 0) {
        lives--;
        if (lives > 0) {
            health = PLAYER_HEALTH_MAX;
            sprite->setPosition({ RESPAWN_X, RESPAWN_Y });
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

void Player::setTexture(const sf::Texture& texture)
{

    sprite.emplace(texture);  

    sf::FloatRect localBounds = sprite->getLocalBounds(); 


    // --- ESCALADO ---
    float desiredVisualWidth = PLAYER_WIDTH;    
    float desiredVisualHeight = PLAYER_HEIGHT;

    float scaleX = desiredVisualWidth / localBounds.size.x;
    float scaleY = desiredVisualHeight / localBounds.size.y;

    sprite->setScale({ scaleX, scaleY });

    sprite->setPosition({ RESPAWN_X, RESPAWN_Y });


}