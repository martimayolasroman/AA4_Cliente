#include "Bullet.h" // Incluye Bullet.h transitivamente y las constantes necesarias
#include "Game.h" // Incluye Bullet.h transitivamente y las constantes necesarias
// Definición del constructor de Bullet
// Las constantes (BULLET_WIDTH, PLAYER_WIDTH, etc.) vienen de Game.h
Bullet::Bullet(sf::Vector2f pos, bool facingRight) {
    shape.setSize({ BULLET_WIDTH, BULLET_HEIGHT });
    shape.setFillColor(COLOR_BULLET_YELLOW);

    float bulletStartXOffset = facingRight ? PLAYER_WIDTH : -BULLET_WIDTH;
    shape.setPosition({ pos.x + bulletStartXOffset, pos.y + PLAYER_HEIGHT / 2.f - BULLET_HEIGHT / 2.f });

    velocity.x = facingRight ? BULLET_SPEED : -BULLET_SPEED;
    velocity.y = 0;
}