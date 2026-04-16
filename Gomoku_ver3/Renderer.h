#pragma execution_character_set("utf-8")
#pragma once
#include <SFML/Graphics.hpp>
#include "Board.h"
#include <string>
#include <vector>
#include <memory>               




class Renderer {
public:
    Renderer();
    Renderer(const std::string& assetsPath);

    sf::Font& getFont();
    void loadAssets(const std::string& assetsPath);

    bool isFontLoaded() const;

    void drawBoardGrid(sf::RenderWindow& window, sf::Vector2f offset, float cellSize);

    void drawStones(sf::RenderWindow& window, const Board& board, sf::Vector2f offset, float cellSize);

    
    void drawHighlights(sf::RenderWindow& window, const Board& board, sf::Vector2f offset, float cellSize);

    
    void drawHoverStone(sf::RenderWindow& window, int r, int c, Cell color, sf::Vector2f offset, float cellSize);

    
    void updateAnimations(float dt);

    
    void addStoneAnimation(int r, int c);
    void addFireAnimation(int r, int c);
    void addSparkAnimation(int r, int c);
    void drawParticles(sf::RenderWindow& window, sf::Vector2f offset, float cellSize);

    void addRippleAnimation(int r, int c, sf::Color color);
    
    void addLightningStormAnimation(int r, int c);

    void drawNotification(sf::RenderWindow& window, const std::string& text, sf::Vector2f centerPos);

private:
    std::unique_ptr<sf::Font> m_font;
    bool m_fontLoaded = false;
    
    
    struct AnimState {
        int r, c;
        float scale;
    };
    std::vector<AnimState> m_animations;

    struct Particle {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life;
        float maxLife;
        sf::Color color;
        float size;
    };
    std::vector<Particle> m_particles;

    struct Ripple {
        sf::Vector2f pos; // grid coordinates (row, col)
        float currentRadius;
        float maxRadius;
        float life;
        float maxLife;
        sf::Color color;
    };
    std::vector<Ripple> m_ripples;

    struct LightningStormAnim {
        sf::Vector2f pos;
        float life;
        float maxLife;
    };
    std::vector<LightningStormAnim> m_lightningStorms;
};


