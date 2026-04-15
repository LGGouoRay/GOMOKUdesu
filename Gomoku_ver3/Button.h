#pragma execution_character_set("utf-8")
#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>





class Button {
public:
    Button(const sf::Font& font, const std::string& label,
        sf::Vector2f position, sf::Vector2f size, unsigned int fontSize = 20);


    void draw(sf::RenderWindow& window) const;


    void update(sf::Vector2f mousePos);

    bool isClicked(sf::Vector2f mousePos) const;

    void setCallback(std::function<void()> callback);
    void triggerCallback();


    void setLabel(const std::string& label);


    void setColors(sf::Color normal, sf::Color hover, sf::Color pressed);
    void setTextColor(sf::Color color);


    void setToggleMode(bool enabled);
    bool isToggled() const;
    void setToggled(bool toggled);


    void setEnabled(bool enabled);
    bool isEnabled() const;


    void setPosition(sf::Vector2f pos);
    sf::Vector2f getPosition() const;
    sf::Vector2f getSize() const;

private:
    sf::RectangleShape m_shape;
    sf::Text m_text;
    std::function<void()> m_callback;

    sf::Color m_normalColor{ sf::Color(60, 60, 80) };
    sf::Color m_hoverColor{ sf::Color(80, 80, 110) };
    sf::Color m_pressedColor{ sf::Color(40, 40, 60) };
    sf::Color m_toggledColor{ sf::Color(50, 130, 90) };
    sf::Color m_disabledColor{ sf::Color(45, 45, 55) };
    sf::Color m_textColor{ sf::Color::White };

    bool m_isHovered = false;
    bool m_isToggleMode = false;
    bool m_isToggled = false;
    bool m_isEnabled = true;

    void centerText();
};

