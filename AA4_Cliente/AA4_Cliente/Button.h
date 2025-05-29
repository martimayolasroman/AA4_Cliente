#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class Button {

    void updateTextPosition();
    bool isMouseOver(const sf::RenderWindow& window) const;

    sf::RectangleShape shape;
    sf::Text buttonText;
    sf::Color defaultColor;
    sf::Color hoverColor;
    sf::Color pressedColor;
    bool isPressed = false;

public:
    Button(const sf::Vector2f& size, const sf::Vector2f& position, const std::string& text, const sf::Font& font, sf::Color buttonColor, sf::Color textColor);
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setText(const std::string& text);

    void setPosition(const sf::Vector2f& pos) {
        shape.setPosition(pos);
        updateTextPosition();
    }

    void setSize(const sf::Vector2f& size) {
        shape.setSize(size);
        updateTextPosition();
    }

};

