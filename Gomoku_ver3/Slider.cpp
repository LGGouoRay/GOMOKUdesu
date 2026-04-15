#include "Slider.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

Slider::Slider(const sf::Font& font, const std::string& labelText, sf::Vector2f position, sf::Vector2f size, float initialValue)
    : m_label(font), m_baseText(labelText), m_value(initialValue), m_isDragging(false), m_isHovered(false), m_pos(position), m_size(size)
{
    // Initialize track
    m_track.setPosition(position);
    m_track.setSize(size);
    m_track.setFillColor(sf::Color(60, 60, 80));
    m_track.setOutlineThickness(2.f);
    m_track.setOutlineColor(sf::Color(100, 100, 150));

    // Initialize fill
    m_fill.setPosition(position);
    m_fill.setFillColor(sf::Color(50, 130, 90));

    // Initialize handle
    m_handle.setSize(sf::Vector2f(10.f, size.y + 10.f));
    m_handle.setOrigin({5.f, 5.f});
    m_handle.setFillColor(sf::Color::White);

    // Initialize label
    m_label.setCharacterSize(20);
    m_label.setFillColor(sf::Color::White);

    updateVisuals();
}

void Slider::draw(sf::RenderWindow& window) const {
    window.draw(m_track);
    window.draw(m_fill);
    window.draw(m_handle);
    window.draw(m_label);
}

bool Slider::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (press->button == sf::Mouse::Button::Left) {
            sf::FloatRect trackBounds(m_pos, m_size);
            sf::FloatRect handleBounds = m_handle.getGlobalBounds();
            // Allow click on handle or track
            if (trackBounds.contains(mousePos) || handleBounds.contains(mousePos)) {
                m_isDragging = true;
                update(mousePos); // Update immediately on click
                return true;
            }
        }
    }
    else if (auto* release = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (release->button == sf::Mouse::Button::Left && m_isDragging) {
            m_isDragging = false;
            return true;
        }
    }
    return false;
}

void Slider::update(sf::Vector2f mousePos) {
    sf::FloatRect trackBounds(m_pos, m_size);
    sf::FloatRect handleBounds = m_handle.getGlobalBounds();
    m_isHovered = trackBounds.contains(mousePos) || handleBounds.contains(mousePos);

    if (m_isHovered) {
        m_track.setOutlineColor(sf::Color(150, 150, 200));
    } else {
        m_track.setOutlineColor(sf::Color(100, 100, 150));
    }

    if (m_isDragging) {
        float relX = mousePos.x - m_pos.x;
        relX = std::clamp(relX, 0.f, m_size.x);
        m_value = relX / m_size.x;
        updateVisuals();

        if (m_callback) {
            m_callback(m_value);
        }
    }
}

float Slider::getValue() const {
    return m_value;
}

void Slider::setValue(float val) {
    m_value = std::clamp(val, 0.f, 1.0f);
    updateVisuals();
}

void Slider::setCallback(std::function<void(float)> callback) {
    m_callback = callback;
}

void Slider::updateVisuals() {
    float filledWidth = m_value * m_size.x;
    m_fill.setSize(sf::Vector2f(filledWidth, m_size.y));
    m_handle.setPosition(sf::Vector2f(m_pos.x + filledWidth, m_pos.y));

    std::ostringstream oss;
    oss << m_baseText << ": " << static_cast<int>(m_value * 100) << "%";
    m_label.setString(oss.str());

    // Center text vertically, right offset horizontally
    sf::FloatRect tBounds = m_label.getLocalBounds();
    m_label.setPosition(sf::Vector2f(m_pos.x + m_size.x + 20.f, m_pos.y + (m_size.y - tBounds.size.y) / 2.f - tBounds.position.y));
}
