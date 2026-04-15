#pragma execution_character_set("utf-8")
#include "Button.h"





Button::Button(const sf::Font& font, const std::string& label,
    sf::Vector2f position, sf::Vector2f size, unsigned int fontSize)
    : m_text(font, label, fontSize)
{
    m_shape.setSize(size);
    m_shape.setPosition(position);
    m_shape.setFillColor(m_normalColor);
    m_shape.setOutlineThickness(2.f);
    m_shape.setOutlineColor(sf::Color(100, 100, 140));

    m_text.setFillColor(m_textColor);
    centerText();
}

void Button::draw(sf::RenderWindow& window) const {
    window.draw(m_shape);
    window.draw(m_text);
}

void Button::update(sf::Vector2f mousePos) {
    if (!m_isEnabled) {
        m_shape.setFillColor(m_disabledColor);
        return;
    }

    m_isHovered = m_shape.getGlobalBounds().contains(mousePos);

    if (m_isToggleMode && m_isToggled) {
        m_shape.setFillColor(m_isHovered ?
            sf::Color(m_toggledColor.r + 20, m_toggledColor.g + 20, m_toggledColor.b + 20) :
            m_toggledColor);
    }
    else {
        m_shape.setFillColor(m_isHovered ? m_hoverColor : m_normalColor);
    }


    if (m_isHovered) {
        m_shape.setOutlineColor(sf::Color(160, 160, 220));
        m_shape.setOutlineThickness(2.5f);
    }
    else {
        m_shape.setOutlineColor(sf::Color(100, 100, 140));
        m_shape.setOutlineThickness(2.f);
    }
}

bool Button::isClicked(sf::Vector2f mousePos) const {
    if (!m_isEnabled) return false;
    return m_shape.getGlobalBounds().contains(mousePos);
}

void Button::setCallback(std::function<void()> callback) {
    m_callback = std::move(callback);
}

void Button::triggerCallback() {
    if (m_isToggleMode) {
        m_isToggled = !m_isToggled;
    }
    if (m_callback) m_callback();
}

void Button::setLabel(const std::string& label) {
    m_text.setString(sf::String::fromUtf8(label.begin(), label.end()));
    centerText();
}

void Button::setColors(sf::Color normal, sf::Color hover, sf::Color pressed) {
    m_normalColor = normal;
    m_hoverColor = hover;
    m_pressedColor = pressed;
}

void Button::setTextColor(sf::Color color) {
    m_textColor = color;
    m_text.setFillColor(color);
}

void Button::setToggleMode(bool enabled) {
    m_isToggleMode = enabled;
}

bool Button::isToggled() const {
    return m_isToggled;
}

void Button::setToggled(bool toggled) {
    m_isToggled = toggled;
}

void Button::setEnabled(bool enabled) {
    m_isEnabled = enabled;
}

bool Button::isEnabled() const {
    return m_isEnabled;
}

void Button::setPosition(sf::Vector2f pos) {
    m_shape.setPosition(pos);
    centerText();
}

sf::Vector2f Button::getPosition() const {
    return m_shape.getPosition();
}

sf::Vector2f Button::getSize() const {
    return m_shape.getSize();
}

void Button::centerText() {
    auto bounds = m_text.getLocalBounds();
    m_text.setOrigin({ bounds.position.x + bounds.size.x / 2.f,
                      bounds.position.y + bounds.size.y / 2.f });
    auto shapePos = m_shape.getPosition();
    auto shapeSize = m_shape.getSize();
    m_text.setPosition({ shapePos.x + shapeSize.x / 2.f,
                        shapePos.y + shapeSize.y / 2.f });
}

