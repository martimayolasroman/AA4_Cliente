#include "Bullet.h" 
#include "Game.h"  

 
Bullet::Bullet(sf::Vector2f pos, bool facingRight) {
    shape.setRadius(BULLET_RADIUS);
    shape.setFillColor(COLOR_BULLET_YELLOW);
 
    float bulletStartXOffset = facingRight ? PLAYER_WIDTH : -BULLET_RADIUS * 2; 
     
    shape.setPosition({ pos.x + bulletStartXOffset, pos.y + PLAYER_HEIGHT / 2.f - BULLET_RADIUS });

    velocity.x = facingRight ? BULLET_SPEED : -BULLET_SPEED;
    velocity.y = 0;  
    isActive = true;  
}