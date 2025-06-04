#include "Button.h"
#include <algorithm> 

Button::Button(const sf::Vector2f& size, const sf::Vector2f& position, const std::string& text, const sf::Font& font, sf::Color buttonColor, sf::Color textColor)
    : buttonText(font, text, 20)
{
    defaultColor = buttonColor;
    shape.setSize(size);
    shape.setPosition(position);
    shape.setFillColor(defaultColor);

    int incremento = 40;
    int r = std::min(defaultColor.r + incremento, 255);
    int g = std::min(defaultColor.g + incremento, 255);
    int b = std::min(defaultColor.b + incremento, 255);
    sf::Color colorBrillante(r, g, b);

    incremento = -40;
    r = std::max(defaultColor.r + incremento, 0);
    g = std::max(defaultColor.g + incremento, 0);
    b = std::max(defaultColor.b + incremento, 0);

    sf::Color colorOscruo(r, g, b);
    hoverColor = colorBrillante;
    pressedColor = colorOscruo;
    buttonText.setFillColor(textColor);
    updateTextPosition();
}

 // Detecta si el botón es presionado o soltado, y si el cursor está sobre él.
bool Button::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (const sf::Event::MouseButtonPressed* mousePress = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePress->button == sf::Mouse::Button::Left && isMouseOver(window)) {
            isPressed = true;
            return false;
        }
    }

    if (const sf::Event::MouseButtonReleased* mouseRelease = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseRelease->button == sf::Mouse::Button::Left) {
            if (isPressed && isMouseOver(window)) {
                isPressed = false;
                return true;
            }
            isPressed = false;
        }
    }
    return false;
}

 
void Button::draw(sf::RenderWindow& window) {
    if (isPressed) {
        shape.setFillColor(pressedColor);
    }
    else if (isMouseOver(window)) {
        shape.setFillColor(hoverColor);
    }
    else {
        shape.setFillColor(defaultColor);
    }

    window.draw(shape);
    window.draw(buttonText);
}

 
bool Button::isMouseOver(const sf::RenderWindow& window) const {
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    return shape.getGlobalBounds().contains(sf::Vector2f(mousePos.x, mousePos.y));
}

 // Se asegura de que el texto esté centrado dentro de la forma del botón, ajustando su origen y posición.
void Button::updateTextPosition() {
    sf::Vector2f textCenter = buttonText.getGlobalBounds().getCenter();
    buttonText.setOrigin(textCenter);

    sf::Vector2f offset = shape.getSize() / 2.f;
    sf::Vector2f pos = shape.getPosition() + offset;

    buttonText.setPosition(pos);
}

// setText: Esta función permite cambiar el texto que se muestra en el botón.
void Button::setText(const std::string& text) {
    buttonText.setString(text);
    updateTextPosition();
}