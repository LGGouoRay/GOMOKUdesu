#pragma execution_character_set("utf-8")
#include "Menu.h"
#include "Settings.h"
#include <iostream>
#include <random>

namespace {
std::string generateRoomCode() {
    static std::mt19937 rng{std::random_device{}()};
    static constexpr char hex[] = "0123456789abcdef";
    std::string code;
    code.reserve(6);
    for (int i = 0; i < 4; ++i) {
        code.push_back(hex[rng() % 16]);
    }
    code.push_back('.');
    code.push_back(hex[rng() % 16]);
    return code;
}
}





Menu::Menu(sf::RenderWindow& window, sf::Font& font) : m_font(font) {
    initMainButtons(window);
    initLocalButtons(window);
    initMultiButtons(window);
    initSettingsButtons(window);
}

void Menu::initMainButtons(sf::RenderWindow& window) {
    float cx = window.getSize().x / 2.f;
    float startY = 300.f;
    float gap = 80.f;
    sf::Vector2f size(300.f, 50.f);

    auto btnPlay = std::make_unique<Button>(m_font, "Local Play", sf::Vector2f(cx - size.x/2, startY), size);
    btnPlay->setCallback([this]() { 
        m_state = MenuState::LocalPlay;
    });

    auto btnMulti = std::make_unique<Button>(m_font, "Multiplayer (LAN)", sf::Vector2f(cx - size.x/2, startY + gap), size);
    btnMulti->setCallback([this]() { m_state = MenuState::Multiplayer; });

    auto btnSettings = std::make_unique<Button>(m_font, "Settings", sf::Vector2f(cx - size.x/2, startY + gap * 2), size);
    btnSettings->setCallback([this]() { m_state = MenuState::Settings; });

    auto btnQuit = std::make_unique<Button>(m_font, "Quit", sf::Vector2f(cx - size.x/2, startY + gap * 3), size);
    btnQuit->setCallback([this]() { m_pendingResult.action = MenuAction::Quit; });

    m_mainButtons.push_back(std::move(btnPlay));
    m_mainButtons.push_back(std::move(btnMulti));
    m_mainButtons.push_back(std::move(btnSettings));
    m_mainButtons.push_back(std::move(btnQuit));
}

void Menu::initLocalButtons(sf::RenderWindow& window) {
    float cx = window.getSize().x / 2.f;
    float startY = 320.f;
    float gap = 80.f;
    sf::Vector2f size(300.f, 50.f);

    auto btnAI = std::make_unique<Button>(m_font, "AI Opponent: OFF", sf::Vector2f(cx - size.x/2, startY), size);
    btnAI->setToggleMode(true);
    btnAI->setCallback([this]() {
        m_aiToggleBtn->setLabel(m_aiToggleBtn->isToggled() ? "AI Opponent: ON" : "AI Opponent: OFF");
    });
    m_aiToggleBtn = btnAI.get();

    auto btnStart = std::make_unique<Button>(m_font, "Start Game", sf::Vector2f(cx - size.x/2, startY + gap), size);
    btnStart->setCallback([this]() { 
        m_pendingResult.action = m_aiToggleBtn->isToggled() ? MenuAction::StartLocalPvAI : MenuAction::StartLocalPvP; 
    });

    auto btnBack = std::make_unique<Button>(m_font, "Back", sf::Vector2f(cx - size.x/2, startY + gap * 2), size);
    btnBack->setCallback([this]() { m_state = MenuState::Main; });

    m_localButtons.push_back(std::move(btnAI));
    m_localButtons.push_back(std::move(btnStart));
    m_localButtons.push_back(std::move(btnBack));
}

void Menu::initMultiButtons(sf::RenderWindow& window) {
    float cx = window.getSize().x / 2.f;
    float startY = 320.f;
    float gap = 70.f;
    sf::Vector2f size(300.f, 50.f);

    auto btnHostPublic = std::make_unique<Button>(m_font, "Host Public Room", sf::Vector2f(cx - size.x/2, startY), size);
    btnHostPublic->setCallback([this]() {
        m_pendingResult.action = MenuAction::HostLAN;
        m_pendingResult.hostPrivate = false;
        m_pendingResult.roomCode = generateRoomCode();
    });

    auto btnHostPrivate = std::make_unique<Button>(m_font, "Host Private Room", sf::Vector2f(cx - size.x/2, startY + gap), size);
    btnHostPrivate->setCallback([this]() {
        m_pendingResult.action = MenuAction::HostLAN;
        m_pendingResult.hostPrivate = true;
        m_pendingResult.roomCode = generateRoomCode();
    });

    auto btnFindRoom = std::make_unique<Button>(m_font, "Find Public Rooms", sf::Vector2f(cx - size.x/2, startY + gap * 2), size);
    btnFindRoom->setCallback([this]() { m_state = MenuState::RoomList; });

    auto btnConnect = std::make_unique<Button>(m_font, "Connect Manually", sf::Vector2f(cx - size.x/2, startY + gap * 3), size);
    btnConnect->setCallback([this]() { 
        m_state = MenuState::ManualConnect;
        initManualConnectUI();
    });

    auto btnBack = std::make_unique<Button>(m_font, "Back", sf::Vector2f(cx - size.x/2, startY + gap * 4), size);
    btnBack->setCallback([this]() { m_state = MenuState::Main; });

    m_multiButtons.push_back(std::move(btnHostPublic));
    m_multiButtons.push_back(std::move(btnHostPrivate));
    m_multiButtons.push_back(std::move(btnFindRoom));
    m_multiButtons.push_back(std::move(btnConnect));
    m_multiButtons.push_back(std::move(btnBack));
}

void Menu::initSettingsButtons(sf::RenderWindow& window) {
    float cx = window.getSize().x / 2.f;
    float startY = 320.f; // 稍微往上移一點以容納更多按鈕
    float gap = 70.f;     // 縮小一點間距
    sf::Vector2f size(300.f, 50.f);

    auto btnTimer = std::make_unique<Button>(m_font, "Timer: OFF", sf::Vector2f(cx - size.x/2, startY), size);
    btnTimer->setToggleMode(true);
    btnTimer->setCallback([btnTimerPtr = btnTimer.get()]() {
        getGameSettings().timerEnabled = btnTimerPtr->isToggled();
        btnTimerPtr->setLabel(btnTimerPtr->isToggled() ? "Timer: ON (30s)" : "Timer: OFF");
    });

    auto btnUndo = std::make_unique<Button>(m_font, "Undo: ON", sf::Vector2f(cx - size.x/2, startY + gap), size);
    btnUndo->setToggleMode(true);
    btnUndo->setToggled(true);
    btnUndo->setCallback([btnUndoPtr = btnUndo.get()]() {
        getGameSettings().undoEnabled = !btnUndoPtr->isToggled(); 
        
        getGameSettings().undoEnabled = btnUndoPtr->isToggled();
        btnUndoPtr->setLabel(btnUndoPtr->isToggled() ? "Undo: ON" : "Undo: OFF");
    });
    btnUndo->setToggled(getGameSettings().undoEnabled);

    // 客製化鼠標按鈕
    auto btnCursor = std::make_unique<Button>(m_font, "Custom Cursor: OFF", sf::Vector2f(cx - size.x/2, startY + gap * 2), size);
    btnCursor->setToggleMode(true);
    btnCursor->setCallback([btnCursorPtr = btnCursor.get()]() {
        getGameSettings().customCursorEnabled = btnCursorPtr->isToggled();
        btnCursorPtr->setLabel(btnCursorPtr->isToggled() ? "Custom Cursor: ON" : "Custom Cursor: OFF");
    });
    btnCursor->setToggled(getGameSettings().customCursorEnabled);
    btnCursor->setLabel(getGameSettings().customCursorEnabled ? "Custom Cursor: ON" : "Custom Cursor: OFF");

    // Volume Slider
    auto volumeSlider = std::make_unique<Slider>(m_font, "Volume", sf::Vector2f(cx - size.x/2, startY + gap * 3), size, getGameSettings().soundVolume / 100.f);
    volumeSlider->setCallback([](float v) {
        getGameSettings().soundVolume = v * 100.f;
    });
    m_settingsSliders.push_back(std::move(volumeSlider));

    auto btnBack = std::make_unique<Button>(m_font, "Back", sf::Vector2f(cx - size.x/2, startY + gap * 4), size);
    btnBack->setCallback([this]() { m_state = MenuState::Main; });

    m_settingsButtons.push_back(std::move(btnTimer));
    m_settingsButtons.push_back(std::move(btnUndo));
    m_settingsButtons.push_back(std::move(btnCursor));
    m_settingsButtons.push_back(std::move(btnBack));
}

bool Menu::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mousePos;
    if (auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
        mousePos = sf::Vector2f(press->position.x, press->position.y);
    } else if (auto* release = event.getIf<sf::Event::MouseButtonReleased>()) {
        mousePos = sf::Vector2f(release->position.x, release->position.y);
    } else if (auto* move = event.getIf<sf::Event::MouseMoved>()) {
        mousePos = sf::Vector2f(move->position.x, move->position.y);
    }

    // Handle manual connect text input
    if (m_state == MenuState::ManualConnect) {
        if (auto* textEvent = event.getIf<sf::Event::TextEntered>()) {
            if (textEvent->unicode < 128) {
                char c = static_cast<char>(textEvent->unicode);
                if (c == '\b') {
                    if (m_manualInputField == 0 && !m_inputIPAddress.empty()) {
                        m_inputIPAddress.pop_back();
                    } else if (m_manualInputField == 1 && !m_inputPort.empty()) {
                        m_inputPort.pop_back();
                    } else if (m_manualInputField == 2 && !m_inputRoomCode.empty()) {
                        m_inputRoomCode.pop_back();
                    }
                } else if (c == '\t') {
                    m_manualInputField = (m_manualInputField + 1) % 3;
                } else if (c >= 32 && c != 127) {
                    if (m_manualInputField == 0 && m_inputIPAddress.length() < 15) {
                        m_inputIPAddress += c;
                    } else if (m_manualInputField == 1 && m_inputPort.length() < 5) {
                        m_inputPort += c;
                    } else if (m_manualInputField == 2 && m_inputRoomCode.length() < 16) {
                        m_inputRoomCode += c;
                    }
                }
                return true;
            }
        }
    }

    // Handle room list scrolling
    if (m_state == MenuState::RoomList) {
        if (auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (scroll->wheel == sf::Mouse::Wheel::Vertical) {
                m_roomListScrollY -= scroll->delta * 30.0f;
                if (m_roomListScrollY < 0.0f) m_roomListScrollY = 0.0f;
                if (m_roomListScrollY > m_roomListMaxScroll) m_roomListScrollY = m_roomListMaxScroll;
                return true;
            }
        }
    }

    if (m_state == MenuState::Settings) {
        for (auto& slider : m_settingsSliders) {
            if (slider->handleEvent(event, mousePos)) {
                return true;
            }
        }
    }

    if (auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (press->button == sf::Mouse::Button::Left) {
            sf::Vector2f mPos(press->position.x, press->position.y);

            // Handle manual connect button clicks
            if (m_state == MenuState::ManualConnect) {
                for (auto& btn : m_roomListButtons) {
                    if (btn->isClicked(mPos)) {
                        btn->triggerCallback();
                        return true;
                    }
                }
            }
            // Handle room list button clicks with scroll offset
            else if (m_state == MenuState::RoomList) {
                mPos.y += m_roomListScrollY;
                for (auto& btn : m_roomListButtons) {
                    if (btn->isClicked(mPos)) {
                        btn->triggerCallback();
                        return true;
                    }
                }
                mPos.y -= m_roomListScrollY;
            }

            auto& activeBtns = (m_state == MenuState::Main) ? m_mainButtons : 
                               (m_state == MenuState::LocalPlay) ? m_localButtons :
                               (m_state == MenuState::Multiplayer) ? m_multiButtons :
                               (m_state == MenuState::RoomList || m_state == MenuState::ManualConnect) ? m_roomListButtons :
                               m_settingsButtons;

            for (auto& btn : activeBtns) {
                if (btn->isClicked(mPos)) {
                    btn->triggerCallback();
                    return true;
                }
            }
        }
    }
    return false;
}

MenuResult Menu::update(sf::RenderWindow& window) {
    sf::Vector2f mousePos(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);

    auto& activeBtns = (m_state == MenuState::Main) ? m_mainButtons : 
                       (m_state == MenuState::LocalPlay) ? m_localButtons :
                       (m_state == MenuState::Multiplayer) ? m_multiButtons :
                       (m_state == MenuState::RoomList || m_state == MenuState::ManualConnect) ? m_roomListButtons :
                       m_settingsButtons;

    for (auto& btn : activeBtns) {
        btn->update(mousePos);
    }

    // Update cursor blinking for manual connect
    if (m_state == MenuState::ManualConnect) {
        m_cursorBlinkTimer += 0.016f; // Roughly 60 FPS
        if (m_cursorBlinkTimer > m_cursorBlinkInterval * 2) {
            m_cursorBlinkTimer = 0.0f;
        }
    }

    if (m_state == MenuState::Settings) {
        for (auto& slider : m_settingsSliders) {
            slider->update(mousePos);
        }
    }

    MenuResult res = m_pendingResult;
    m_pendingResult.action = MenuAction::None;
    return res;
}

void Menu::draw(sf::RenderWindow& window) {

    sf::Text title(m_font, "GOMOKU", 100);
    title.setFillColor(sf::Color(220, 220, 255));
    auto tBounds = title.getLocalBounds();
    title.setOrigin({tBounds.position.x + tBounds.size.x / 2.f, 0});
    title.setPosition({window.getSize().x / 2.f, 100.f});
    window.draw(title);

    auto& activeBtns = (m_state == MenuState::Main) ? m_mainButtons : 
                       (m_state == MenuState::LocalPlay) ? m_localButtons :
                       (m_state == MenuState::Multiplayer) ? m_multiButtons :
                       (m_state == MenuState::RoomList || m_state == MenuState::ManualConnect) ? m_roomListButtons :
                       m_settingsButtons;

    if (m_state == MenuState::RoomList) {
        // Draw title
        sf::Text instructionText(m_font, "Available Rooms", 50);
        instructionText.setFillColor(sf::Color(200, 200, 255));
        auto tBounds = instructionText.getLocalBounds();
        instructionText.setOrigin({tBounds.position.x + tBounds.size.x / 2.f, 0});
        instructionText.setPosition({window.getSize().x / 2.f, 220.f});
        window.draw(instructionText);

        // Draw room list with scroll effect
        // Store original positions
        std::vector<sf::Vector2f> originalPositions;
        for (auto& btn : activeBtns) {
            originalPositions.push_back(btn->getPosition());
        }

        // Adjust positions for scroll
        for (size_t i = 0; i < activeBtns.size(); ++i) {
            sf::Vector2f pos = originalPositions[i];
            pos.y -= m_roomListScrollY;
            activeBtns[i]->setPosition(pos);
        }

        // Draw all buttons
        for (auto& btn : activeBtns) {
            btn->draw(window);
        }

        // Restore original positions
        for (size_t i = 0; i < activeBtns.size(); ++i) {
            activeBtns[i]->setPosition(originalPositions[i]);
        }

        if (m_roomListButtons.size() <= 1) { // Only back button
            sf::Text noRoomsText(m_font, "No rooms found. Searching...", 30);
            noRoomsText.setFillColor(sf::Color(150, 150, 150));
            noRoomsText.setPosition({300.f, 390.f});
            window.draw(noRoomsText);
        }

        // Draw scroll hint if needed
        if (m_roomListMaxScroll > 0.f) {
            sf::Text scrollHint(m_font, "Scroll with mouse wheel", 18);
            scrollHint.setFillColor(sf::Color(100, 150, 200));
            scrollHint.setPosition({300.f, 750.f});
            window.draw(scrollHint);
        }
    } else if (m_state == MenuState::ManualConnect) {
        // Draw manual connect UI with better layout
        float cx = window.getSize().x / 2.f;
        float startY = 260.f;
        float labelY = startY;
        float inputBoxY = labelY + 45.f;
        float gap = 100.f;

        // Title
        sf::Text titleText(m_font, "Connect to Server", 40);
        titleText.setFillColor(sf::Color(200, 200, 255));
        auto tBounds = titleText.getLocalBounds();
        titleText.setOrigin({tBounds.position.x + tBounds.size.x / 2.f, 0});
        titleText.setPosition({cx, 180.f});
        window.draw(titleText);

        // IP Address section
        sf::Text ipLabel(m_font, "IP Address:", 28);
        ipLabel.setFillColor(sf::Color(180, 180, 255));
        ipLabel.setPosition({cx - 350.f, labelY});
        window.draw(ipLabel);

        sf::RectangleShape ipBox(sf::Vector2f(500.f, 50.f));
        ipBox.setPosition({cx - 250.f, inputBoxY});
        ipBox.setFillColor(m_manualInputField == 0 ? sf::Color(60, 80, 140) : sf::Color(40, 40, 80));
        ipBox.setOutlineColor(m_manualInputField == 0 ? sf::Color::Cyan : sf::Color(100, 100, 200));
        ipBox.setOutlineThickness(2.f);
        window.draw(ipBox);

        sf::Text ipText(m_font, m_inputIPAddress + (m_manualInputField == 0 && m_cursorBlinkTimer < m_cursorBlinkInterval ? "|" : ""), 24);
        ipText.setFillColor(sf::Color::White);
        ipText.setPosition({cx - 240.f, inputBoxY + 12.f});
        window.draw(ipText);

        // Port section
        sf::Text portLabel(m_font, "Port:", 28);
        portLabel.setFillColor(sf::Color(180, 180, 255));
        portLabel.setPosition({cx - 350.f, labelY + gap});
        window.draw(portLabel);

        sf::RectangleShape portBox(sf::Vector2f(500.f, 50.f));
        portBox.setPosition({cx - 250.f, inputBoxY + gap});
        portBox.setFillColor(m_manualInputField == 1 ? sf::Color(60, 80, 140) : sf::Color(40, 40, 80));
        portBox.setOutlineColor(m_manualInputField == 1 ? sf::Color::Cyan : sf::Color(100, 100, 200));
        portBox.setOutlineThickness(2.f);
        window.draw(portBox);

        sf::Text portText(m_font, m_inputPort + (m_manualInputField == 1 && m_cursorBlinkTimer < m_cursorBlinkInterval ? "|" : ""), 24);
        portText.setFillColor(sf::Color::White);
        portText.setPosition({cx - 240.f, inputBoxY + gap + 12.f});
        window.draw(portText);

        sf::Text codeLabel(m_font, "Room Code:", 28);
        codeLabel.setFillColor(sf::Color(180, 180, 255));
        codeLabel.setPosition({cx - 350.f, labelY + gap * 2});
        window.draw(codeLabel);

        sf::RectangleShape codeBox(sf::Vector2f(500.f, 50.f));
        codeBox.setPosition({cx - 250.f, inputBoxY + gap * 2});
        codeBox.setFillColor(m_manualInputField == 2 ? sf::Color(60, 80, 140) : sf::Color(40, 40, 80));
        codeBox.setOutlineColor(m_manualInputField == 2 ? sf::Color::Cyan : sf::Color(100, 100, 200));
        codeBox.setOutlineThickness(2.f);
        window.draw(codeBox);

        sf::Text codeText(m_font, m_inputRoomCode + (m_manualInputField == 2 && m_cursorBlinkTimer < m_cursorBlinkInterval ? "|" : ""), 24);
        codeText.setFillColor(sf::Color::White);
        codeText.setPosition({cx - 240.f, inputBoxY + gap * 2 + 12.f});
        window.draw(codeText);

        // Help text
        sf::Text helpText(m_font, "Press TAB to switch fields, BACKSPACE to delete", 16);
        helpText.setFillColor(sf::Color(120, 120, 150));
        helpText.setPosition({cx - 250.f, inputBoxY + gap * 2 + 35.f});
        window.draw(helpText);

        // Draw buttons
        for (auto& btn : activeBtns) {
            btn->draw(window);
        }

    } else {
        for (auto& btn : activeBtns) {
            btn->draw(window);
        }
    }

    if (m_state == MenuState::Settings) {
        for (auto& slider : m_settingsSliders) {
            slider->draw(window);
        }
    }
}

void Menu::reset() {
    m_state = MenuState::Main;
    m_pendingResult.action = MenuAction::None;
}

void Menu::initRoomListButtons(sf::RenderWindow& window, const std::vector<Network::RoomInfo>& rooms) {
    m_roomListButtons.clear();

    float startX = 300.f;
    float startY = 300.f;
    float btnWidth = 600.f;
    float btnHeight = 60.f;
    float gap = 75.f;

    // Create buttons for each discovered room
    for (size_t i = 0; i < rooms.size(); ++i) {
        std::string btnLabel = std::string(rooms[i].isPrivate ? "[Private] " : "[Public] ") +
                               rooms[i].roomName +
                               " [Code: " + rooms[i].roomCode + "] (" +
                               rooms[i].ip.toString() + ":" + std::to_string(rooms[i].port) + ")";
        auto btn = std::make_unique<Button>(m_font, btnLabel, sf::Vector2f(startX, startY + i * gap), sf::Vector2f(btnWidth, btnHeight));

        const auto& room = rooms[i];
        btn->setCallback([this, room]() {
            m_pendingResult.action = MenuAction::JoinRoom;
            m_pendingResult.ipAddress = room.ip.toString();
            m_pendingResult.port = room.port;
            m_pendingResult.roomCode = room.roomCode;
        });

        m_roomListButtons.push_back(std::move(btn));
    }

    // Back button - positioned below the room list
    auto btnBack = std::make_unique<Button>(m_font, "Back", sf::Vector2f(startX, startY + static_cast<float>(rooms.size()) * gap + 30.f), sf::Vector2f(btnWidth, btnHeight));
    btnBack->setCallback([this]() { m_state = MenuState::Multiplayer; });
    m_roomListButtons.push_back(std::move(btnBack));

    // Calculate max scroll for room list
    float totalButtonsHeight = (rooms.size() + 1) * gap;
    float viewportHeight = 600.f; // Available height for room list
    m_roomListMaxScroll = (totalButtonsHeight > viewportHeight) ? (totalButtonsHeight - viewportHeight) : 0.0f;
    if (m_roomListScrollY > m_roomListMaxScroll) {
        m_roomListScrollY = m_roomListMaxScroll;
    }
    if (m_roomListScrollY < 0.0f) {
        m_roomListScrollY = 0.0f;
    }
}

void Menu::initManualConnectUI() {
    m_inputIPAddress = "127.0.0.1";
    m_inputPort = "55001";
    m_inputRoomCode.clear();
    m_manualInputField = 0;
    m_roomListButtons.clear();

    float cx = 600.f; // Window center x (assuming 1200 width)
    float buttonStartY = 620.f;
    float buttonGap = 70.f;
    sf::Vector2f size(300.f, 50.f);

    auto btnConnect = std::make_unique<Button>(m_font, "Connect", sf::Vector2f(cx - size.x/2, buttonStartY), size);
    btnConnect->setCallback([this]() {
        if (!m_inputIPAddress.empty()) {
            m_pendingResult.action = MenuAction::ConnectLAN;
            m_pendingResult.ipAddress = m_inputIPAddress;
            m_pendingResult.roomCode = m_inputRoomCode;
            try {
                m_pendingResult.port = static_cast<unsigned short>(std::stoi(m_inputPort));
            } catch (...) {
                m_pendingResult.port = 55001;
            }
        }
    });

    auto btnBack = std::make_unique<Button>(m_font, "Back", sf::Vector2f(cx - size.x/2, buttonStartY + buttonGap), size);
    btnBack->setCallback([this]() { m_state = MenuState::Multiplayer; });

    m_roomListButtons.push_back(std::move(btnConnect));
    m_roomListButtons.push_back(std::move(btnBack));
}

