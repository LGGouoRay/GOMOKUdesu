#pragma execution_character_set("utf-8")
#include "Renderer.h"
#include <iostream>
#include <filesystem>
#include <cmath>






Renderer::Renderer() {
}

Renderer::Renderer(const std::string& assetsPath) {
    loadAssets(assetsPath);
}

void Renderer::loadAssets(const std::string& assetsPath) {
    std::string fontPath = assetsPath + "/fonts/font.ttf";
    if (std::filesystem::exists(fontPath)) {
        m_font = std::make_unique<sf::Font>();
        if (m_font->openFromFile(fontPath)) {
            m_fontLoaded = true;
            std::cout << "[Renderer] Loaded font: " << fontPath << "\n";
        }
    } else {
        std::cout << "[Renderer] Font not found: " << fontPath << ". Using fallback system if available.\n";
    }
}

sf::Font& Renderer::getFont() {
    if (!m_fontLoaded) {
        
        static sf::Font emptyFont;
        return emptyFont;
    }
    return *m_font;
}

bool Renderer::isFontLoaded() const {
    return m_fontLoaded;
}

void Renderer::drawBoardGrid(sf::RenderWindow& window, sf::Vector2f offset, float cellSize) {
    
    sf::RectangleShape background;
    float boardPixelSize = (BOARD_SIZE - 1) * cellSize + 2 * cellSize;
    background.setSize({boardPixelSize, boardPixelSize});
    background.setPosition({offset.x - cellSize, offset.y - cellSize});
    background.setFillColor(sf::Color(210, 166, 111)); 
    background.setOutlineColor(sf::Color(101, 67, 33));
    window.draw(background);

    
    for (int i = 0; i < BOARD_SIZE; ++i) {
        
        sf::Vertex hLine[] = {
            sf::Vertex({offset.x, offset.y + i * cellSize}, sf::Color::Black),
            sf::Vertex({offset.x + (BOARD_SIZE - 1) * cellSize, offset.y + i * cellSize}, sf::Color::Black)
        };
        window.draw(hLine, 2, sf::PrimitiveType::Lines);

        
        sf::Vertex vLine[] = {
            sf::Vertex({offset.x + i * cellSize, offset.y}, sf::Color::Black),
            sf::Vertex({offset.x + i * cellSize, offset.y + (BOARD_SIZE - 1) * cellSize}, sf::Color::Black)
        };
        window.draw(vLine, 2, sf::PrimitiveType::Lines);
    }

    
    auto drawStar = [&](int r, int c) {
        sf::CircleShape star(4.f);
        star.setFillColor(sf::Color::Black);
        star.setOrigin({4.f, 4.f});
        star.setPosition({offset.x + c * cellSize, offset.y + r * cellSize});
        window.draw(star);
    };
    drawStar(3, 3);
    drawStar(3, 11);
    drawStar(7, 7);
    drawStar(11, 3);
    drawStar(11, 11);
}

void Renderer::drawStones(sf::RenderWindow& window, const Board& board, sf::Vector2f offset, float cellSize) {
    float radius = cellSize * 0.45f;
    sf::CircleShape stone(radius);
    stone.setOrigin({radius, radius});

    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            Cell cell = board.getCell(r, c);
            if (cell == Cell::EMPTY) continue;

            stone.setPosition({offset.x + c * cellSize, offset.y + r * cellSize});

            
            float scale = 1.0f;
            for (const auto& anim : m_animations) {
                if (anim.r == r && anim.c == c) {
                    scale = anim.scale;
                    break;
                }
            }
            stone.setScale({scale, scale});

            if (cell == Cell::BLACK) {
                stone.setFillColor(sf::Color(30, 30, 30));
                stone.setOutlineThickness(1.f);
                stone.setOutlineColor(sf::Color(10, 10, 10));
            } else if (cell == Cell::WHITE) {
                stone.setFillColor(sf::Color(240, 240, 240));
                stone.setOutlineThickness(1.f);
                stone.setOutlineColor(sf::Color(180, 180, 180));
            }
            window.draw(stone);
        }
    }
}

void Renderer::drawHighlights(sf::RenderWindow& window, const Board& board, sf::Vector2f offset, float cellSize) {
    auto& history = board.getMoveHistory();
    if (!history.empty()) {
        const Move& last = history.back();
        sf::CircleShape marker(4.f);
        marker.setFillColor(sf::Color::Red);
        marker.setOrigin({4.f, 4.f});
        marker.setPosition({offset.x + last.col * cellSize, offset.y + last.row * cellSize});
        window.draw(marker);
    }

    
    int winRow[5], winCol[5];
    if (board.getWinLine(winRow, winCol)) {
        for (int i = 0; i < 5; ++i) {
            sf::CircleShape glow(cellSize * 0.5f);
            glow.setOrigin({glow.getRadius(), glow.getRadius()});
            glow.setPosition({offset.x + winCol[i] * cellSize, offset.y + winRow[i] * cellSize});
            glow.setFillColor(sf::Color(255, 215, 0, 150)); 
            window.draw(glow);
        }
    }
}

void Renderer::drawHoverStone(sf::RenderWindow& window, int r, int c, Cell color, sf::Vector2f offset, float cellSize) {
    if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) return;

    float radius = cellSize * 0.45f;
    sf::CircleShape stone(radius);
    stone.setOrigin({radius, radius});
    stone.setPosition({offset.x + c * cellSize, offset.y + r * cellSize});

    if (color == Cell::BLACK) {
        stone.setFillColor(sf::Color(30, 30, 30, 128));
    } else if (color == Cell::WHITE) {
        stone.setFillColor(sf::Color(240, 240, 240, 128));
    }
    window.draw(stone);
}

void Renderer::updateAnimations(float dt) {
    for (auto it = m_animations.begin(); it != m_animations.end();) {
        it->scale += dt * 5.0f; 
        if (it->scale >= 1.0f) {
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        it->pos += it->vel * dt;
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_particles.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_ripples.begin(); it != m_ripples.end(); ) {
        it->currentRadius += (it->maxRadius / it->maxLife) * dt;
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_ripples.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_lightningStorms.begin(); it != m_lightningStorms.end(); ) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_lightningStorms.erase(it);
        } else {
            ++it;
        }
    }
}

void Renderer::addStoneAnimation(int r, int c) {
    m_animations.push_back({r, c, 0.5f}); 
}

void Renderer::addFireAnimation(int r, int c) {
    for (int i = 0; i < 40; ++i) {
        float angle = (rand() % 360) * 3.14159f / 180.f;
        float speed = 0.2f + (rand() % 15) / 10.f; // Cells per second
        float life = 0.5f + (rand() % 10) / 20.f; // 0.5 to 1.0 seconds
        float scale = 0.8f + (rand() % 5) / 10.f;
        sf::Color color = (rand() % 2 == 0) ? sf::Color(255, 100, 0) : sf::Color(255, 50, 0); // Orange/Red
        m_particles.push_back({{static_cast<float>(c), static_cast<float>(r)}, {std::cos(angle) * speed, std::sin(angle) * speed}, life, life, color, scale});
    }
}

void Renderer::addSparkAnimation(int r, int c) {
    for (int i = 0; i < 30; ++i) {
        float angle = (rand() % 360) * 3.14159f / 180.f;
        float speed = 0.3f + (rand() % 15) / 10.f;
        float life = 0.3f + (rand() % 10) / 20.f;
        float scale = 0.6f + (rand() % 5) / 10.f;
        sf::Color color = sf::Color(100, 200, 255); // Magic blue
        m_particles.push_back({{static_cast<float>(c), static_cast<float>(r)}, {std::cos(angle) * speed, std::sin(angle) * speed}, life, life, color, scale});
    }
}

void Renderer::drawParticles(sf::RenderWindow& window, sf::Vector2f offset, float cellSize) {
    for (const auto& r : m_ripples) {
        sf::CircleShape shape(r.currentRadius * cellSize);
        shape.setOrigin({shape.getRadius(), shape.getRadius()});
        shape.setPosition({offset.x + r.pos.x * cellSize, offset.y + r.pos.y * cellSize});
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineThickness(3.f * (r.life / r.maxLife));
        shape.setOutlineColor(sf::Color(r.color.r, r.color.g, r.color.b, static_cast<uint8_t>(255 * (r.life / r.maxLife))));
        window.draw(shape);
    }

    for (const auto& p : m_particles) {
        sf::CircleShape shape(cellSize * 0.2f * p.size * (p.life / p.maxLife));
        shape.setOrigin({shape.getRadius(), shape.getRadius()});
        shape.setPosition({offset.x + p.pos.x * cellSize, offset.y + p.pos.y * cellSize});
        shape.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b, static_cast<uint8_t>(255 * (p.life / p.maxLife))));
        window.draw(shape);
    }

    for (const auto& anim : m_lightningStorms) {
        float blink = (std::sin(anim.life * 60.0f) + 1.0f) * 0.5f; 
        float scale = 1.0f + blink * 1.5f; 
        
        sf::CircleShape spark(cellSize * 0.5f * scale, 5); 
        spark.setOrigin({spark.getRadius(), spark.getRadius()});
        spark.setPosition({offset.x + anim.pos.x * cellSize, offset.y + anim.pos.y * cellSize});
        
        uint8_t alpha = static_cast<uint8_t>(255 * (anim.life / anim.maxLife));
        if (static_cast<int>(anim.life * 30) % 2 == 0) {
            spark.setFillColor(sf::Color(255, 255, 0, alpha)); 
        } else {
            spark.setFillColor(sf::Color(255, 255, 255, alpha)); 
        }
        window.draw(spark);
        
        sf::CircleShape innerSpark(cellSize * 0.2f * scale, 4); 
        innerSpark.setOrigin({innerSpark.getRadius(), innerSpark.getRadius()});
        innerSpark.setPosition({offset.x + anim.pos.x * cellSize, offset.y + anim.pos.y * cellSize});
        innerSpark.setFillColor(sf::Color(255, 255, 255, alpha));
        window.draw(innerSpark);
    }
}

void Renderer::addRippleAnimation(int r, int c, sf::Color color) {
    m_ripples.push_back({{static_cast<float>(c), static_cast<float>(r)}, 0.0f, 3.0f, 0.8f, 0.8f, color});
}

void Renderer::addLightningStormAnimation(int r, int c) {
    m_lightningStorms.push_back({{static_cast<float>(c), static_cast<float>(r)}, 1.0f, 1.0f});
}

void Renderer::drawNotification(sf::RenderWindow& window, const std::string& textStr, sf::Vector2f centerPos) {
    if (!m_fontLoaded) return;
    
    sf::Text text(m_font.operator*(), textStr, 40);
    text.setFillColor(sf::Color::Red);
    text.setOutlineColor(sf::Color::White);
    text.setOutlineThickness(2.f);
    
    auto bounds = text.getLocalBounds();
    text.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    text.setPosition(centerPos);
    
    window.draw(text);
}

