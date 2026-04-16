#pragma execution_character_set("utf-8")
#include "Button.h"





Button::Button(const sf::Font& font, const std::string& label,
    sf::Vector2f position, sf::Vector2f size, unsigned int fontSize)
    : m_text(font, sf::String::fromUtf8(label.begin(), label.end()), fontSize)
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

    if (!m_particles.empty()) {
        sf::CircleShape part(2.f);
        for (const auto& p : m_particles) {
            part.setRadius(p.size > 0.1f ? p.size : 2.f);
            part.setPosition(p.pos - sf::Vector2f(part.getRadius(), part.getRadius()));
            sf::Color c = p.color;
            if (m_glowEffect == GlowEffect::Storm) {
                // Storm particles flicker
                c.a = static_cast<std::uint8_t>((128 + rand() % 127) * (p.life / p.maxLife));
            } else {
                c.a = static_cast<std::uint8_t>(255.f * (p.life / p.maxLife));
            }
            part.setFillColor(c);
            window.draw(part);
        }
    }

    if (!m_lightnings.empty()) {
        for (const auto& l : m_lightnings) {
            sf::VertexArray va = l.lines;
            sf::Color c = m_glowColor;
            c.a = static_cast<std::uint8_t>(255.f * (l.life / l.maxLife));
            for (std::size_t i = 0; i < va.getVertexCount(); ++i) {
                va[i].color = c;
            }
            window.draw(va);
        }
    }
}

void Button::update(sf::Vector2f mousePos, float dt) {
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

    if (m_isGlowing && dt > 0.f) {
        m_particleTimer -= dt;
        m_effectTimer -= dt;

        if (m_glowEffect == GlowEffect::Particles && m_particleTimer <= 0.f) {
            float perimeter = 2 * (m_shape.getSize().x + m_shape.getSize().y);
            float dist = static_cast<float>(rand()) / RAND_MAX * perimeter;
            sf::Vector2f pos = m_shape.getPosition();
            if (dist < m_shape.getSize().x) {
                pos.x += dist;
            } else if (dist < m_shape.getSize().x + m_shape.getSize().y) {
                pos.x += m_shape.getSize().x;
                pos.y += (dist - m_shape.getSize().x);
            } else if (dist < 2 * m_shape.getSize().x + m_shape.getSize().y) {
                pos.x += m_shape.getSize().x - (dist - m_shape.getSize().x - m_shape.getSize().y);
                pos.y += m_shape.getSize().y;
            } else {
                pos.y += m_shape.getSize().y - (dist - 2 * m_shape.getSize().x - m_shape.getSize().y);
            }
            float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.f;
            float speed = 10.f + static_cast<float>(rand() % 30);
            m_particles.push_back({ pos, sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed), 0.6f, 0.6f, m_glowColor, 0.f, 2.f });
            m_particleTimer = 0.05f; // spawn rate
        }
        else if (m_glowEffect == GlowEffect::Lightning && m_effectTimer <= 0.f) {
            float perimeter = 2 * (m_shape.getSize().x + m_shape.getSize().y);
            float dist = static_cast<float>(rand()) / RAND_MAX * perimeter;
            sf::Vector2f pos = m_shape.getPosition();
            if (dist < m_shape.getSize().x) pos.x += dist;
            else if (dist < m_shape.getSize().x + m_shape.getSize().y) { pos.x += m_shape.getSize().x; pos.y += (dist - m_shape.getSize().x); }
            else if (dist < 2 * m_shape.getSize().x + m_shape.getSize().y) { pos.x += m_shape.getSize().x - (dist - m_shape.getSize().x - m_shape.getSize().y); pos.y += m_shape.getSize().y; }
            else { pos.y += m_shape.getSize().y - (dist - 2 * m_shape.getSize().x - m_shape.getSize().y); }

            sf::VertexArray va(sf::PrimitiveType::LineStrip);
            sf::Vector2f cur = pos;
            va.append(sf::Vertex({cur, m_glowColor}));
            for(int i = 0; i < 4; ++i) {
                float dx = (static_cast<float>(rand() % 30) - 15.f);
                float dy = (static_cast<float>(rand() % 30) - 15.f);
                cur += sf::Vector2f(dx, dy);
                va.append(sf::Vertex({cur, m_glowColor}));
            }
            m_lightnings.push_back({va, 0.3f, 0.3f});
            m_effectTimer = 0.08f;
        }
        else if (m_glowEffect == GlowEffect::Storm) {
            if (m_particleTimer <= 0.f) {
                sf::Vector2f pos = m_shape.getPosition() + sf::Vector2f(static_cast<float>(rand() % static_cast<int>(m_shape.getSize().x)), static_cast<float>(rand() % static_cast<int>(m_shape.getSize().y)));
                m_particles.push_back({ pos, sf::Vector2f(0.f, -40.f - static_cast<float>(rand() % 40)), 0.8f, 0.8f, m_glowColor, 0.f, 1.5f + static_cast<float>(rand() % 3) });
                m_particleTimer = 0.02f;
            }
            if (m_effectTimer <= 0.f) {
                sf::Vector2f p1 = m_shape.getPosition() + sf::Vector2f(static_cast<float>(rand() % static_cast<int>(m_shape.getSize().x)), static_cast<float>(rand() % static_cast<int>(m_shape.getSize().y)));
                sf::Vector2f p2 = m_shape.getPosition() + sf::Vector2f(static_cast<float>(rand() % static_cast<int>(m_shape.getSize().x)), static_cast<float>(rand() % static_cast<int>(m_shape.getSize().y)));
                sf::VertexArray va(sf::PrimitiveType::LineStrip);
                va.append(sf::Vertex({p1, sf::Color::White}));
                va.append(sf::Vertex({(p1 + p2) / 2.f + sf::Vector2f(static_cast<float>(rand() % 20 - 10), static_cast<float>(rand() % 20 - 10)), m_glowColor}));
                va.append(sf::Vertex({p2, sf::Color::White}));
                m_lightnings.push_back({va, 0.15f, 0.15f});
                m_effectTimer = 0.1f + static_cast<float>(rand() % 10) * 0.01f;
            }
        }
    }

    if (dt > 0.f) {
        for (auto it = m_particles.begin(); it != m_particles.end(); ) {
            it->life -= dt;
            if (it->life <= 0) {
                it = m_particles.erase(it);
            } else {
                it->pos += it->vel * dt;
                ++it;
            }
        }
        for (auto it = m_lightnings.begin(); it != m_lightnings.end(); ) {
            it->life -= dt;
            if (it->life <= 0) {
                it = m_lightnings.erase(it);
            } else {
                ++it;
            }
        }
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

void Button::setToggledColor(sf::Color toggled) {
    m_toggledColor = toggled;
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

void Button::setGlowingEffect(bool enabled, sf::Color color, GlowEffect effect) {
    m_isGlowing = enabled;
    m_glowEffect = effect;
    if (enabled) {
        m_glowColor = color;
    } else {
        m_particles.clear();
        m_lightnings.clear();
    }
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

