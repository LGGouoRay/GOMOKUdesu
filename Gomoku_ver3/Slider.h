#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Slider {
public:
    Slider(const sf::Font& font, const std::string& labelText, sf::Vector2f position, sf::Vector2f size, float initialValue = 0.5f);

    void draw(sf::RenderWindow& window) const;

    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    void update(sf::Vector2f mousePos);

    float getValue() const;
    void setValue(float val);

    void setCallback(std::function<void(float)> callback);

private:
    sf::RectangleShape m_track;
    sf::RectangleShape m_fill;
    sf::RectangleShape m_handle;
    sf::Text m_label;

    std::string m_baseText;
    std::function<void(float)> m_callback;

    float m_value; // 0.0 to 1.0
    bool m_isDragging;
    bool m_isHovered;
    sf::Vector2f m_pos;
    sf::Vector2f m_size;

    void updateVisuals();
};
