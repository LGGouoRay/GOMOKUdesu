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

    sf::Font& getFont();
    void loadAssets(const std::string& assetsPath);

    bool isFontLoaded() const;

    void drawBoardGrid(sf::RenderWindow& window, sf::Vector2f offset, float cellSize);

    void drawStones(sf::RenderWindow& window, const Board& board, sf::Vector2f offset, float cellSize);

    
    void drawHighlights(sf::RenderWindow& window, const Board& board, sf::Vector2f offset, float cellSize);

    
    void drawHoverStone(sf::RenderWindow& window, int r, int c, Cell color, sf::Vector2f offset, float cellSize);

    
    void updateAnimations(float dt);

    
    void addStoneAnimation(int r, int c);

    
    void drawNotification(sf::RenderWindow& window, const std::string& text, sf::Vector2f centerPos);

private:
    std::unique_ptr<sf::Font> m_font;
    bool m_fontLoaded = false;
    
    
    struct AnimState {
        int r, c;
        float scale;
    };
    std::vector<AnimState> m_animations;
};

