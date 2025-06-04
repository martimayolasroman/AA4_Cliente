#include "Bullet.h" // Incluye Bullet.h transitivamente y las constantes necesarias
#include "Game.h" // Incluye Game.h transitivamente y las constantes necesarias

// Definición del constructor de Bullet
// Las constantes (BULLET_RADIUS, PLAYER_WIDTH, etc.) vienen de Game.h
Bullet::Bullet(sf::Vector2f pos, bool facingRight) {
    shape.setRadius(BULLET_RADIUS);
    shape.setFillColor(COLOR_BULLET_YELLOW);

    // Ajustar posición inicial para una esfera
    // Pos se asume que es la posición del sprite del jugador (esquina superior izquierda).
    // Queremos que la bala salga del centro vertical del jugador.
    float bulletStartXOffset = facingRight ? PLAYER_WIDTH : -BULLET_RADIUS * 2; // Salir desde el borde del jugador
    // Centrar verticalmente: (PLAYER_HEIGHT / 2.f) - BULLET_RADIUS
    shape.setPosition({ pos.x + bulletStartXOffset, pos.y + PLAYER_HEIGHT / 2.f - BULLET_RADIUS });

    velocity.x = facingRight ? BULLET_SPEED : -BULLET_SPEED;
    velocity.y = 0; // Las balas suelen ser horizontales
    isActive = true; // Inicializar como activa
}