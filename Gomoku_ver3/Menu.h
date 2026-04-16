#pragma execution_character_set("utf-8")
#pragma once
#include <SFML/Graphics.hpp>
#include "Button.h"
#include "Slider.h"
#include "Network.h"
#include <vector>
#include <memory>
#include <string>

enum class MenuState { Main, LocalPlay, Multiplayer, Settings, RoomList, ManualConnect };
enum class MenuAction { None, StartLocalPvP, StartLocalPvAI, StartExperimental, HostLAN, ConnectLAN, JoinRoom, Quit };

struct MenuResult {
    MenuAction action = MenuAction::None;
    std::string ipAddress = "";
    unsigned short port = 55001;
    std::string roomCode = "";
    bool hostPrivate = false;
    int aiDifficultyLevel = 0; // 0=Easy, 1=Normal, 2=Hard
    bool experimentalMode = false;
};

class Menu {
public:
    Menu(sf::RenderWindow& window, sf::Font& font);


    bool handleEvent(const sf::Event& event, sf::RenderWindow& window);


    MenuResult update(sf::RenderWindow& window);


    void draw(sf::RenderWindow& window);

    void reset();

    void initRoomListButtons(sf::RenderWindow& window, const std::vector<Network::RoomInfo>& rooms);
    void initManualConnectUI();
    float getRoomListScrollOffset() const { return m_roomListScrollY; }
    bool isRoomListState() const { return m_state == MenuState::RoomList; }

private:
    MenuState m_state = MenuState::Main;
    sf::Font& m_font;
    MenuResult m_pendingResult;

    std::vector<std::unique_ptr<Button>> m_mainButtons;
    std::vector<std::unique_ptr<Button>> m_localButtons;
    std::vector<std::unique_ptr<Button>> m_multiButtons;
    std::vector<std::unique_ptr<Button>> m_settingsButtons;
    std::vector<std::unique_ptr<Button>> m_roomListButtons;
    std::vector<std::unique_ptr<Slider>> m_settingsSliders;

    Button* m_backButton;
    Button* m_aiToggleBtn;
    Button* m_aiDiffBtn;
    int m_aiDiffState = 0; // 0=Easy, 1=Normal, 2=Hard

    // Manual Connect UI
    std::string m_inputIPAddress;
    std::string m_inputPort;
    std::string m_inputRoomCode;
    int m_manualInputField = 0;
    float m_cursorBlinkTimer = 0.0f;
    const float m_cursorBlinkInterval = 0.5f;

    // Room List Scrolling
    float m_roomListScrollY = 0.0f;
    float m_roomListMaxScroll = 0.0f;

    void initMainButtons(sf::RenderWindow& window);
    void initLocalButtons(sf::RenderWindow& window);
    void initMultiButtons(sf::RenderWindow& window);
    void initSettingsButtons(sf::RenderWindow& window);
};

