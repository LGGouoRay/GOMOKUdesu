#include "Game.h"
#include "Settings.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <functional>
#include <chrono>

namespace {
constexpr std::size_t kGameBtnUndo = 0;
constexpr std::size_t kGameBtnMainMenu = 1;
constexpr std::size_t kGameBtnToggleAI = 2;
constexpr std::size_t kGameBtnSave = 3;
constexpr std::size_t kGameBtnLoad = 4;

std::size_t computeRoomListHash(const std::vector<Network::RoomInfo>& rooms) {
    std::size_t seed = rooms.size();
    std::hash<std::string> stringHasher;

    for (const auto& room : rooms) {
        seed ^= stringHasher(room.ip.toString()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= static_cast<std::size_t>(room.port) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= stringHasher(room.roomName) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= stringHasher(room.roomCode) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= static_cast<std::size_t>(room.isPrivate ? 1 : 0) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
}
}

// ============================================================
//  Game.cpp — 遊戲主邏輯實作
// ============================================================

Game::Game()
    : m_window(sf::VideoMode({ 1200, 900 }), "Gomoku_ver3", sf::Style::Default),
    m_menu(m_window, m_renderer.getFont()),
    m_ai(Cell::WHITE)
{
    m_window.setFramerateLimit(60);
    // 載入資源 (此處路徑為預設)
    m_renderer.loadAssets("assets");
    m_soundMgr.loadAll("assets");

    m_menu.~Menu(); 
    new (&m_menu) Menu(m_window, m_renderer.getFont());

    // 網路事件設定
    m_network.setOnStonePlaced([this](int r, int c) {
        performMove(r, c);
        });

    m_network.setOnGameStart([this]() {
        if (m_playMode == PlayMode::NetworkHost || m_playMode == PlayMode::NetworkClient) {
            resetGame();
            startTransition(GameState::Playing);
        }
    });

    m_network.setOnUndoRequest([this]() {
        m_pendingUndoRequest = true;
    });

    m_network.setOnUndoResponse([this](bool accepted) {
        if (accepted) {
            applyUndoMove();
            m_soundMgr.play(SoundEffect::Click);
        } else {
            m_soundMgr.play(SoundEffect::Error);
        }
    });

    m_network.setOnRestartRequest([this]() {
        resetGame();
    });

    m_network.setOnDisconnect([this]() {
        m_pendingUndoRequest = false;
    });

    // 載入自訂鼠標紋理
    for (int i = 0; i < 6; ++i) {
        sf::Texture texture;
        std::string path = "assets/cursor/frame_" + std::to_string(i) + ".png";
        if (texture.loadFromFile(path)) {
            m_cursorTextures.push_back(texture);
        }
    }

    // 初始化鼠標Sprite
    if (!m_cursorTextures.empty()) {
        m_cursorSprite = std::make_unique<sf::Sprite>(m_cursorTextures[0]);
    }

    initGameUI();
    initReplayUI();
    initLobbyUI();
    initSaveLoadUI();

    // 根據設定隱藏或顯示預設鼠標
    m_cursorVisibilityApplied = !getGameSettings().customCursorEnabled;
    m_window.setMouseCursorVisible(m_cursorVisibilityApplied);

    m_soundMgr.playMenuMusic();
}

void Game::initReplayUI() {
    float uiX = 900.f;
    float startY = 300.f;
    float gap = 80.f;
    sf::Vector2f size(200.f, 50.f);

    auto btnPrev = std::make_unique<Button>(m_renderer.getFont(), "Prev Move", sf::Vector2f(uiX, startY), size);
    btnPrev->setCallback([this]() { m_replay.prevMove(); m_soundMgr.play(SoundEffect::Click); });

    auto btnNext = std::make_unique<Button>(m_renderer.getFont(), "Next Move", sf::Vector2f(uiX, startY + gap), size);
    btnNext->setCallback([this]() { m_replay.nextMove(); m_soundMgr.play(SoundEffect::Click); });

    auto btnAuto = std::make_unique<Button>(m_renderer.getFont(), "Auto Play", sf::Vector2f(uiX, startY + 2 * gap), size);
    btnAuto->setCallback([this]() {
        m_replay.setAutoPlay(!m_replay.isAutoPlaying(), 1.0f);
        m_soundMgr.play(SoundEffect::Click);
    });

    auto btnExit = std::make_unique<Button>(m_renderer.getFont(), "Exit Replay", sf::Vector2f(uiX, startY + 3 * gap), size);
    btnExit->setCallback([this]() {
        m_replay.setAutoPlay(false, 1.0f);
        startTransition(GameState::Menu);
        m_soundMgr.play(SoundEffect::Click);
    });

    m_replayButtons.push_back(std::move(btnPrev));
    m_replayButtons.push_back(std::move(btnNext));
    m_replayButtons.push_back(std::move(btnAuto));
    m_replayButtons.push_back(std::move(btnExit));
}

void Game::startTransition(GameState targetState) {
    if (m_fadeState != 0) return;
    m_nextState = targetState;
    m_fadeState = 1; // Fade Out
    m_fadeAlpha = 0.0f;

    if (targetState == GameState::Menu) {
        m_soundMgr.playMenuMusic();
    } else if (targetState == GameState::Playing || targetState == GameState::ReplayMode) {
        m_soundMgr.playGameMusic();
    }
}

void Game::updateTransition(float dt) {
    if (m_fadeState == 1) { // fading out
        m_fadeAlpha += 600.0f * dt; 
        if (m_fadeAlpha >= 255.0f) {
            m_fadeAlpha = 255.0f;
            m_state = m_nextState;
            m_fadeState = 2; // Fade In
        }
    } else if (m_fadeState == 2) { // fading in
        m_fadeAlpha -= 600.0f * dt;
        if (m_fadeAlpha <= 0.0f) {
            m_fadeAlpha = 0.0f;
            m_fadeState = 0; // Done
        }
    }
}

void Game::drawTransition() {
    if (m_fadeState != 0) {
        sf::RectangleShape rect(sf::Vector2f(m_window.getSize()));
        rect.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(m_fadeAlpha)));
        m_window.draw(rect);
    }
}

void Game::toggleAI() {
    m_playMode = (m_playMode == PlayMode::LocalPvAI) ? PlayMode::LocalPvP : PlayMode::LocalPvAI;
}

void Game::saveCurrentGame() {
    if (!canUseSaveLoad()) {
        m_soundMgr.play(SoundEffect::Error);
        return;
    }
    m_isSaving = true;
    initSaveLoadUI();
    startTransition(GameState::SaveLoad);
}

void Game::loadSavedGame() {
    if (!canUseSaveLoad()) {
        m_soundMgr.play(SoundEffect::Error);
        return;
    }
    m_isSaving = false;
    initSaveLoadUI();
    startTransition(GameState::SaveLoad);
}

bool Game::isNetworkMode() const {
    return m_playMode == PlayMode::NetworkHost || m_playMode == PlayMode::NetworkClient;
}

bool Game::canUseSaveLoad() const {
    if (!isNetworkMode()) {
        return true;
    }
    return m_playMode == PlayMode::NetworkHost;
}

bool Game::isGameButtonVisible(std::size_t index) const {
    (void)index;
    return true;
}

void Game::updateGameButtonStates() {
    if (m_gameButtons.size() > kGameBtnToggleAI) {
        m_gameButtons[kGameBtnToggleAI]->setEnabled(!isNetworkMode());
    }
    if (m_gameButtons.size() > kGameBtnSave) {
        m_gameButtons[kGameBtnSave]->setEnabled(canUseSaveLoad());
    }
    if (m_gameButtons.size() > kGameBtnLoad) {
        m_gameButtons[kGameBtnLoad]->setEnabled(canUseSaveLoad());
    }
}

bool Game::applyUndoMove() {
    if (!m_board.undoMove()) {
        return false;
    }

    m_currentTurn = (m_board.getMoveCount() % 2 == 0) ? Cell::BLACK : Cell::WHITE;
    m_timer.reset();

    if (m_state == GameState::GameOver) {
        m_state = GameState::Playing;
        m_winner = Cell::EMPTY;
    }

    return true;
}

void Game::enterReplayMode() {
    m_replay.loadHistory(m_board.getMoveHistory());
    startTransition(GameState::ReplayMode);
}

void Game::initLobbyUI() {
    m_lobbyButtons.clear();
    float cx = m_window.getSize().x / 2.f;
    float startY = 500.f;
    float gap = 80.f;
    sf::Vector2f size(200.f, 50.f);

    auto btnStart = std::make_unique<Button>(m_renderer.getFont(), "Start Game", sf::Vector2f(cx - size.x/2, startY), size);
    btnStart->setCallback([this]() {
        if (m_playMode == PlayMode::NetworkHost && m_network.getStatus() == Network::Status::Connected) {
            m_network.sendGameStart();
            resetGame();
            m_state = GameState::Playing;
            m_soundMgr.play(SoundEffect::Click);
        }
    });

    auto btnLeave = std::make_unique<Button>(m_renderer.getFont(), "Leave", sf::Vector2f(cx - size.x/2, startY + gap), size);
    btnLeave->setCallback([this]() {
        m_network.disconnect();
        m_menu.reset();
        startTransition(GameState::Menu);
        m_soundMgr.play(SoundEffect::Click);
    });

    m_lobbyButtons.push_back(std::move(btnStart));
    m_lobbyButtons.push_back(std::move(btnLeave));
}

void Game::initRoomListUI() {
    auto rooms = m_network.getAvailableRooms();
    m_menu.initRoomListButtons(m_window, rooms);
}

void Game::beginNetworkConnect(const std::string& ip, unsigned short port, const std::string& roomCode) {
    if (m_connectInProgress) {
        return;
    }

    m_connectErrorMessage.clear();
    m_connectErrorTimer = 0.f;
    m_connectElapsed = 0.f;
    m_connectInProgress = true;
    m_selectedRoomIP = ip;
    m_selectedRoomPort = port;

    m_state = GameState::Connecting;
    m_connectFuture = std::async(std::launch::async, [this, ip, port, roomCode]() {
        return m_network.connect(ip, port, roomCode);
    });
}

void Game::updateConnecting(float dt) {
    if (!m_connectInProgress || !m_connectFuture.valid()) {
        m_state = GameState::Menu;
        return;
    }

    m_connectElapsed += dt;

    if (m_connectFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        return;
    }

    const bool connected = m_connectFuture.get();
    m_connectInProgress = false;

    if (connected) {
        m_playMode = PlayMode::NetworkClient;
        m_myNetworkColor = Cell::WHITE;
        m_clientConnected = true;
        startTransition(GameState::Lobby);
        return;
    }

    m_state = GameState::Menu;
    m_connectErrorMessage = L"加入失敗，請確認是否正確地按下確認按鈕";
    m_connectErrorTimer = 3.0f;
}

void Game::initGameUI() {
    float uiX = 900.f;
    float startY = 300.f;
    float gap = 80.f;
    sf::Vector2f size(200.f, 50.f);

    auto btnUndo = std::make_unique<Button>(m_renderer.getFont(), "Undo", sf::Vector2f(uiX, startY), size);
    btnUndo->setCallback([this]() {
        if (!getGameSettings().undoEnabled) return;
        if (isNetworkMode()) {
            m_network.sendUndoRequest();
            m_soundMgr.play(SoundEffect::Click);
            return;
        }
        if (applyUndoMove()) {
            if (m_playMode == PlayMode::LocalPvAI) {
                applyUndoMove(); // 退兩步
            }
            m_soundMgr.play(SoundEffect::Click);
        }
        });

    auto btnMenu = std::make_unique<Button>(m_renderer.getFont(), "Main Menu", sf::Vector2f(uiX, startY + gap), size);
    btnMenu->setCallback([this]() {
        startTransition(GameState::Menu);
        m_network.disconnect();
        m_menu.reset();
        m_soundMgr.play(SoundEffect::Click);
        });

    auto btnToggleAI = std::make_unique<Button>(m_renderer.getFont(), "Toggle AI", sf::Vector2f(uiX, startY + 2 * gap), size);
    btnToggleAI->setCallback([this]() { toggleAI(); m_soundMgr.play(SoundEffect::Click); });

    auto btnSave = std::make_unique<Button>(m_renderer.getFont(), "Save Game", sf::Vector2f(uiX, startY + 3 * gap), size);
    btnSave->setCallback([this]() { saveCurrentGame(); m_soundMgr.play(SoundEffect::Click); });

    auto btnLoad = std::make_unique<Button>(m_renderer.getFont(), "Load Game", sf::Vector2f(uiX, startY + 4 * gap), size);
    btnLoad->setCallback([this]() { loadSavedGame(); m_soundMgr.play(SoundEffect::Click); });

    auto btnReplay = std::make_unique<Button>(m_renderer.getFont(), "View Replay", sf::Vector2f(uiX, startY + 5 * gap), size);
    btnReplay->setCallback([this]() { enterReplayMode(); m_soundMgr.play(SoundEffect::Click); });

    m_btnPlayAgain = std::make_unique<Button>(m_renderer.getFont(), "Play Again", sf::Vector2f(600.f - 100.f, 450.f), sf::Vector2f(200.f, 60.f));
    m_btnPlayAgain->setCallback([this]() {
        if (isNetworkMode()) {
            m_network.sendRestartRequest();
        }
        resetGame();
        m_soundMgr.play(SoundEffect::Click);
    });

    m_btnUndoAccept = std::make_unique<Button>(m_renderer.getFont(), "Accept Undo", sf::Vector2f(600.f - 220.f, 370.f), sf::Vector2f(200.f, 60.f));
    m_btnUndoAccept->setCallback([this]() {
        const bool accepted = applyUndoMove();
        m_network.sendUndoResponse(accepted);
        m_pendingUndoRequest = false;
        m_soundMgr.play(accepted ? SoundEffect::Click : SoundEffect::Error);
    });

    m_btnUndoReject = std::make_unique<Button>(m_renderer.getFont(), "Reject Undo", sf::Vector2f(600.f + 20.f, 370.f), sf::Vector2f(200.f, 60.f));
    m_btnUndoReject->setCallback([this]() {
        m_network.sendUndoResponse(false);
        m_pendingUndoRequest = false;
        m_soundMgr.play(SoundEffect::Error);
    });

    m_gameButtons.push_back(std::move(btnUndo));
    m_gameButtons.push_back(std::move(btnMenu));
    m_gameButtons.push_back(std::move(btnToggleAI));
    m_gameButtons.push_back(std::move(btnSave));
    m_gameButtons.push_back(std::move(btnLoad));
    m_gameButtons.push_back(std::move(btnReplay));
}

void Game::run() {
    m_clock.restart();
    while (m_window.isOpen()) {
        float dt = m_clock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}

void Game::processEvents() {
    while (auto event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
        }
        
        if (m_state == GameState::SaveLoad) {
            if (auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (scroll->wheel == sf::Mouse::Wheel::Vertical) {
                    m_saveLoadScrollY -= scroll->delta * 30.0f; // Scroll speed
                    if (m_saveLoadScrollY < 0.0f) m_saveLoadScrollY = 0.0f;
                    
                    // Optional: Calc max scroll height
                    float maxScroll = static_cast<float>(m_saveLoadButtons.size()) * 80.0f + 100.f - m_window.getSize().y + 100.f;
                    if (maxScroll < 0) maxScroll = 0.0f;
                    if (m_saveLoadScrollY > maxScroll) m_saveLoadScrollY = maxScroll;
                }
            }
        }

        if (m_state == GameState::Menu) {
            if (m_menu.handleEvent(*event, m_window)) {
                m_soundMgr.play(SoundEffect::Click);
            }
        }
        else if (m_state == GameState::SaveLoad) {
            if (auto* press = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (press->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mousePos(press->position.x, press->position.y + m_saveLoadScrollY);
                    for (auto& btn : m_saveLoadButtons) {
                        if (btn->isClicked(mousePos)) {
                            btn->triggerCallback();
                            goto EventHandled;
                        }
                    }
                }
            }
        }
        else if (m_state == GameState::Playing || m_state == GameState::GameOver || m_state == GameState::ReplayMode || m_state == GameState::Lobby) {
            // UI 按鈕
            if (auto* press = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (press->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mousePos(press->position.x, press->position.y);

                    if ((m_state == GameState::Playing || m_state == GameState::GameOver) &&
                        m_pendingUndoRequest &&
                        (m_playMode == PlayMode::NetworkHost || m_playMode == PlayMode::NetworkClient)) {
                        if (m_btnUndoAccept && m_btnUndoAccept->isClicked(mousePos)) {
                            m_btnUndoAccept->triggerCallback();
                            goto EventHandled;
                        }
                        if (m_btnUndoReject && m_btnUndoReject->isClicked(mousePos)) {
                            m_btnUndoReject->triggerCallback();
                            goto EventHandled;
                        }
                    }

                    if (m_state == GameState::Lobby) {
                        for (auto& btn : m_lobbyButtons) {
                            if (btn->isClicked(mousePos)) {
                                btn->triggerCallback();
                                goto EventHandled;
                            }
                        }
                    } else if (m_state == GameState::ReplayMode) {
                        for (auto& btn : m_replayButtons) {
                            if (btn->isClicked(mousePos)) {
                                btn->triggerCallback();
                                goto EventHandled;
                            }
                        }
                    } else {
                        if (m_state == GameState::GameOver && m_btnPlayAgain) {
                            if (m_btnPlayAgain->isClicked(mousePos)) {
                                m_btnPlayAgain->triggerCallback();
                                goto EventHandled;
                            }
                        }
                        for (std::size_t i = 0; i < m_gameButtons.size(); ++i) {
                            if (!isGameButtonVisible(i)) {
                                continue;
                            }
                            if (m_gameButtons[i]->isClicked(mousePos)) {
                                m_gameButtons[i]->triggerCallback();
                                goto EventHandled;
                            }
                        }
                    }

                    // 處理棋盤點擊
                    if (m_state == GameState::Playing) {
                        float boardEndX = m_boardOffset.x + (BOARD_SIZE - 1) * m_cellSize;
                        float boardEndY = m_boardOffset.y + (BOARD_SIZE - 1) * m_cellSize;

                        if (mousePos.x >= m_boardOffset.x - m_cellSize / 2 && mousePos.x <= boardEndX + m_cellSize / 2 &&
                            mousePos.y >= m_boardOffset.y - m_cellSize / 2 && mousePos.y <= boardEndY + m_cellSize / 2)
                        {
                            int c = std::round((mousePos.x - m_boardOffset.x) / m_cellSize);
                            int r = std::round((mousePos.y - m_boardOffset.y) / m_cellSize);
                            handleBoardClick(r, c);
                        }
                    }
                }
            }
        EventHandled:;
        }
    }
}

void Game::handleBoardClick(int row, int col) {
    if (m_state != GameState::Playing) return;

    // AI 模式中，若是 AI 回合則忽略點擊
    if (m_playMode == PlayMode::LocalPvAI && m_currentTurn != Cell::BLACK) return;

    // 網路模式中，若不是我的回合則忽略點擊
    if ((m_playMode == PlayMode::NetworkHost || m_playMode == PlayMode::NetworkClient) && m_currentTurn != m_myNetworkColor) return;

    if (m_board.getCell(row, col) != Cell::EMPTY) {
        m_soundMgr.play(SoundEffect::Error);
        return;
    }

    performMove(row, col);

    // 如果是網路對打，傳送封包
    if (m_playMode == PlayMode::NetworkHost || m_playMode == PlayMode::NetworkClient) {
        m_network.sendStonePlaced(row, col);
    }
}

void Game::performMove(int row, int col) {
    if (m_board.placeStone(row, col, m_currentTurn)) {
        m_renderer.addStoneAnimation(row, col);
        m_soundMgr.play(SoundEffect::PlaceStone);

        switchTurn();
    }
}

void Game::switchTurn() {
    m_winner = m_board.checkWin();
    if (m_winner != Cell::EMPTY) {
        m_state = GameState::GameOver;
        m_soundMgr.play(SoundEffect::Win); // 簡化：不分輸贏都播 Win
        return;
    }

    if (m_board.isFull()) {
        m_state = GameState::GameOver;
        m_winner = Cell::EMPTY; // 平手
        return;
    }

    m_currentTurn = (m_currentTurn == Cell::BLACK) ? Cell::WHITE : Cell::BLACK;
    m_timer.reset();
}

void Game::resetGame() {
    m_board.reset();
    m_currentTurn = Cell::BLACK;
    m_winner = Cell::EMPTY;
    m_pendingUndoRequest = false;
    m_state = GameState::Playing;
    m_timer.setEnabled(getGameSettings().timerEnabled);
    m_timer.setTimeLimit(getGameSettings().timerLimit);
    m_timer.reset();
    m_soundMgr.play(SoundEffect::Start);
}

void Game::initSaveLoadUI() {
    m_saveLoadButtons.clear();
    float startX = 100.f;
    float startY = 100.f;
    float btnWidth = 800.f;
    float btnHeight = 60.f;
    float gap = 80.f;

    SaveLoad::ensureSaveDirectory("saves");

    std::vector<std::filesystem::path> saveFiles;
    for (const auto& entry : std::filesystem::directory_iterator("saves")) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".sav") {
            saveFiles.push_back(entry.path());
        }
    }
    std::sort(saveFiles.begin(), saveFiles.end());

    auto addSaveLoadButton = [this, startX, startY, gap, btnWidth, btnHeight](const std::string& display, const std::string& filename, int rowIndex) {
        auto btn = std::make_unique<Button>(m_renderer.getFont(), display, sf::Vector2f(startX, startY + rowIndex * gap), sf::Vector2f(btnWidth, btnHeight));
        btn->setCallback([this, filename]() {
            m_soundMgr.play(SoundEffect::Click);
            if (m_isSaving) {
                GameSaveData data;
                data.currentTurn = static_cast<int>(m_currentTurn);
                data.timerEnabled = getGameSettings().timerEnabled;
                data.timerLimit = getGameSettings().timerLimit;
                data.undoEnabled = getGameSettings().undoEnabled;
                data.aiEnabled = (m_playMode == PlayMode::LocalPvAI);
                data.moveHistory = m_board.getMoveHistory();
                for (int r = 0; r < BOARD_SIZE; ++r) {
                    for (int c = 0; c < BOARD_SIZE; ++c) {
                        data.grid[r][c] = static_cast<int>(m_board.getCell(r, c));
                    }
                }
                SaveLoad::saveGame(filename, data);
                startTransition(GameState::Playing);
            } else {
                GameSaveData data;
                if (SaveLoad::loadGame(filename, data)) {
                    m_currentTurn = static_cast<Cell>(data.currentTurn);
                    m_playMode = data.aiEnabled ? PlayMode::LocalPvAI : PlayMode::LocalPvP;
                    m_board.reset();
                    for (const auto& m : data.moveHistory) {
                        m_board.placeStone(m.row, m.col, m.color);
                    }
                    startTransition(GameState::Playing);
                }
            }
        });
        m_saveLoadButtons.push_back(std::move(btn));
    };

    int rowIndex = 0;
    if (m_isSaving) {
        int nextIndex = 0;
        for (const auto& path : saveFiles) {
            const std::string stem = path.stem().string();
            const std::string prefix = "savegame_";
            if (stem.rfind(prefix, 0) == 0) {
                std::string numPart = stem.substr(prefix.size());
                bool allDigits = !numPart.empty() && std::all_of(numPart.begin(), numPart.end(), [](char ch) {
                    return ch >= '0' && ch <= '9';
                });
                if (allDigits) {
                    nextIndex = std::max(nextIndex, std::stoi(numPart) + 1);
                }
            }
        }

        std::string newFilename = "saves/savegame_" + std::to_string(nextIndex) + ".sav";
        std::string newDisplay = "Create New Save - savegame_" + std::to_string(nextIndex) + ".sav";
        addSaveLoadButton(newDisplay, newFilename, rowIndex++);
    }

    for (const auto& path : saveFiles) {
        std::string filename = path.generic_string();
        std::string display = path.filename().string();

        GameSaveData data;
        if (SaveLoad::loadGame(filename, data)) {
            display += " - Moves: " + std::to_string(data.moveHistory.size());
            if (!data.saveTime.empty()) {
                display += "  |  " + data.saveTime;
            }
        }

        addSaveLoadButton(display, filename, rowIndex++);
    }

    if (!m_isSaving && saveFiles.empty()) {
        auto noFileBtn = std::make_unique<Button>(m_renderer.getFont(), "No save files found", sf::Vector2f(startX, startY), sf::Vector2f(btnWidth, btnHeight));
        noFileBtn->setCallback([]() {});
        m_saveLoadButtons.push_back(std::move(noFileBtn));
        rowIndex = 1;
    }

    m_maxSaveSlots = rowIndex;

    auto backBtn = std::make_unique<Button>(m_renderer.getFont(), "Back", sf::Vector2f(startX, startY + rowIndex * gap), sf::Vector2f(btnWidth, btnHeight));
    backBtn->setCallback([this]() {
        m_soundMgr.play(SoundEffect::Click);
        startTransition(m_isSaving ? GameState::Playing : GameState::Menu);
    });
    m_saveLoadButtons.push_back(std::move(backBtn));
}
void Game::update(float dt) {
    updateTransition(dt);

    if (m_connectErrorTimer > 0.f) {
        m_connectErrorTimer -= dt;
        if (m_connectErrorTimer <= 0.f) {
            m_connectErrorTimer = 0.f;
            m_connectErrorMessage.clear();
        }
    }

    // 更新自訂鼠標
    if (getGameSettings().customCursorEnabled && !m_cursorTextures.empty()) {
        m_cursorAnimTime += dt;
        if (m_cursorAnimTime >= m_cursorAnimMaxTime) {
            m_cursorAnimTime = 0.0f;
            m_cursorFrame = (m_cursorFrame + 1) % m_cursorTextures.size();
            if (m_cursorSprite) {
                m_cursorSprite->setTexture(m_cursorTextures[m_cursorFrame]);
            }
        }

        // 更新鼠標位置
        if (m_cursorSprite) {
            auto mousePos = sf::Mouse::getPosition(m_window);
            if (mousePos != m_lastMousePos) {
                m_lastMousePos = mousePos;
                m_cursorSprite->setPosition(sf::Vector2f(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)));
            }
        }
    }

    // 根據設定更新鼠標可見性（僅在變更時呼叫）
    const bool shouldShowSystemCursor = !getGameSettings().customCursorEnabled;
    if (shouldShowSystemCursor != m_cursorVisibilityApplied) {
        m_cursorVisibilityApplied = shouldShowSystemCursor;
        m_window.setMouseCursorVisible(m_cursorVisibilityApplied);
    }

    if (m_soundMgr.getVolume() != getGameSettings().soundVolume) {
        m_soundMgr.setVolume(getGameSettings().soundVolume);
    }

    if (m_fadeState != 0 && m_nextState != GameState::Menu) {
        // block logic updates during heavy transition if needed, but simple enough to just let it run
    }

    m_renderer.updateAnimations(dt);

    if (m_state == GameState::Menu) {
        if (m_menu.isRoomListState()) {
            static bool roomDiscoveryStarted = false;
            if (!roomDiscoveryStarted) {
                m_network.discoverRooms(55002);
                roomDiscoveryStarted = true;
            }

            m_network.updateRoomDiscovery();

            m_roomListRefreshTimer += dt;
            const auto rooms = m_network.getAvailableRooms();
            const std::size_t currentHash = computeRoomListHash(rooms);
            const bool shouldRefresh = (m_roomListRefreshTimer >= 0.25f) || (currentHash != m_lastRoomListHash);

            if (shouldRefresh) {
                initRoomListUI();
                m_lastRoomListHash = currentHash;
                m_roomListRefreshTimer = 0.f;
            }
        } else {
            m_roomListRefreshTimer = 0.f;
        }
        MenuResult res = m_menu.update(m_window);
        switch (res.action) {
        case MenuAction::StartLocalPvP:
            m_playMode = PlayMode::LocalPvP;
            resetGame();
            startTransition(GameState::Playing);
            break;
        case MenuAction::StartLocalPvAI:
            m_playMode = PlayMode::LocalPvAI;
            m_ai.setColor(Cell::WHITE);
            m_ai.setDepth(getGameSettings().aiDepth);
            resetGame();
            startTransition(GameState::Playing);
            break;
        case MenuAction::HostLAN: {
            m_roomName = res.hostPrivate ? "Private Room" : "Public Room";
            m_selectedRoomIP.clear();
            m_selectedRoomPort = 55001;
            if (m_network.host(55001, m_roomName, res.hostPrivate, res.roomCode)) {
                m_playMode = PlayMode::NetworkHost;
                m_myNetworkColor = Cell::BLACK;
                m_clientConnected = false;
                m_roomName = (res.hostPrivate ? "Private Room" : "Public Room") + std::string(" (Code: ") + res.roomCode + ")";
                startTransition(GameState::Lobby);
            }
            break;
        }
        case MenuAction::JoinRoom:
            beginNetworkConnect(res.ipAddress, res.port, res.roomCode);
            break;
        case MenuAction::ConnectLAN:
            beginNetworkConnect(res.ipAddress, res.port, res.roomCode);
            break;
        case MenuAction::Quit:
            m_window.close();
            break;
        default: break;
        }
    }
    else if (m_state == GameState::Connecting) {
        updateConnecting(dt);
    }
    else if (m_state == GameState::SaveLoad) {
        sf::Vector2f mousePos(sf::Mouse::getPosition(m_window).x, sf::Mouse::getPosition(m_window).y + m_saveLoadScrollY);
        for (auto& btn : m_saveLoadButtons) btn->update(mousePos);
    }
    else if (m_state == GameState::Playing) {
        updateGameButtonStates();
        sf::Vector2f mousePos(sf::Mouse::getPosition(m_window).x, sf::Mouse::getPosition(m_window).y);
        for (std::size_t i = 0; i < m_gameButtons.size(); ++i) {
            if (!isGameButtonVisible(i)) {
                continue;
            }
            m_gameButtons[i]->update(mousePos);
        }

        m_timer.update();
        if (m_timer.isTimeUp()) {
            std::cout << "[Game] Time's up! Turn skipped.\n";
            m_soundMgr.play(SoundEffect::Error);
            m_currentTurn = (m_currentTurn == Cell::BLACK) ? Cell::WHITE : Cell::BLACK;
            m_timer.reset();
        }
        else if (m_timer.isWarning() && fmod(m_timer.getRemainingSeconds(), 1.0f) < 0.1f) {
            // 每秒逼逼一聲
            // m_soundMgr.play(SoundEffect::TimerWarning); 
        }

        if (m_playMode == PlayMode::LocalPvAI && m_currentTurn == m_ai.calculateBestMove(m_board).color) {
            // AI 思考與落子 (這裡用同步處理，真正實作通常會放 thread 免得卡頓)
            Move aiMove = m_ai.calculateBestMove(m_board);
            if (aiMove.row != -1) performMove(aiMove.row, aiMove.col);
        }

        if (m_playMode == PlayMode::NetworkHost || m_playMode == PlayMode::NetworkClient) {
            m_network.update();
        }

        if (m_pendingUndoRequest && m_btnUndoAccept && m_btnUndoReject) {
            m_btnUndoAccept->update(mousePos);
            m_btnUndoReject->update(mousePos);
        }
    }
    else if (m_state == GameState::GameOver) {
        updateGameButtonStates();
        sf::Vector2f mousePos(sf::Mouse::getPosition(m_window).x, sf::Mouse::getPosition(m_window).y);
        for (std::size_t i = 0; i < m_gameButtons.size(); ++i) {
            if (!isGameButtonVisible(i)) {
                continue;
            }
            m_gameButtons[i]->update(mousePos);
        }
        if (m_btnPlayAgain) m_btnPlayAgain->update(mousePos);
        if (m_pendingUndoRequest && m_btnUndoAccept && m_btnUndoReject) {
            m_btnUndoAccept->update(mousePos);
            m_btnUndoReject->update(mousePos);
        }
    }
    else if (m_state == GameState::ReplayMode) {
        m_replay.update(dt);
        sf::Vector2f mousePos(sf::Mouse::getPosition(m_window).x, sf::Mouse::getPosition(m_window).y);
        for (auto& btn : m_replayButtons) btn->update(mousePos);
    }
    else if (m_state == GameState::Lobby) {
        sf::Vector2f mousePos(sf::Mouse::getPosition(m_window).x, sf::Mouse::getPosition(m_window).y);
        for (auto& btn : m_lobbyButtons) btn->update(mousePos);

        if (m_playMode == PlayMode::NetworkHost || m_playMode == PlayMode::NetworkClient) {
            m_network.update();
        }
    }
}

void Game::render() {
    m_window.clear(sf::Color(40, 45, 55)); // 深藍灰色背景

    if (m_state == GameState::Menu) {
        m_menu.draw(m_window);

        if (!m_connectErrorMessage.empty() && m_renderer.isFontLoaded()) {
            sf::Text err(m_renderer.getFont(), m_connectErrorMessage, 28);
            err.setFillColor(sf::Color(255, 120, 120));
            auto bounds = err.getLocalBounds();
            err.setOrigin({ bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f });
            err.setPosition({ 600.f, 820.f });
            m_window.draw(err);
        }
    }
    else if (m_state == GameState::Connecting) {
        m_menu.draw(m_window);

        sf::RectangleShape dim(sf::Vector2f(m_window.getSize()));
        dim.setFillColor(sf::Color(0, 0, 0, 160));
        m_window.draw(dim);

        if (m_renderer.isFontLoaded()) {
            sf::Text statusText(m_renderer.getFont(), L"加入遊戲中...", 42);
            statusText.setFillColor(sf::Color::White);
            auto tBounds = statusText.getLocalBounds();
            statusText.setOrigin({ tBounds.position.x + tBounds.size.x / 2.f, tBounds.position.y + tBounds.size.y / 2.f });
            statusText.setPosition({ 600.f, 370.f });
            m_window.draw(statusText);

            sf::RectangleShape barBg({ 520.f, 30.f });
            barBg.setOrigin({ 260.f, 15.f });
            barBg.setPosition({ 600.f, 450.f });
            barBg.setFillColor(sf::Color(60, 70, 90));
            barBg.setOutlineThickness(2.f);
            barBg.setOutlineColor(sf::Color(180, 190, 220));
            m_window.draw(barBg);

            float progress = std::min(m_connectElapsed / 3.0f, 0.95f);
            sf::RectangleShape barFill({ 516.f * progress, 24.f });
            barFill.setPosition({ 600.f - 258.f, 450.f - 12.f });
            barFill.setFillColor(sf::Color(120, 220, 160));
            m_window.draw(barFill);
        }
    }
    else if (m_state == GameState::SaveLoad) {
        sf::Text title(m_renderer.getFont());
        title.setCharacterSize(40);
        title.setPosition({ 100.f, 30.f });
        title.setString(m_isSaving ? "Select Slot to Save" : "Select Slot to Load");
        title.setFillColor(sf::Color::White);
        m_window.draw(title);

        sf::View oldView = m_window.getView();
        sf::View scrollView = oldView;
        scrollView.move({0.f, m_saveLoadScrollY});
        m_window.setView(scrollView);

        for (auto& btn : m_saveLoadButtons) btn->draw(m_window);

        m_window.setView(oldView);
    }
    else if (m_state == GameState::ReplayMode) {
        m_renderer.drawBoardGrid(m_window, m_boardOffset, m_cellSize);
        m_renderer.drawStones(m_window, m_replay.getReplayBoard(), m_boardOffset, m_cellSize);
        for (auto& btn : m_replayButtons) btn->draw(m_window);

        if (m_renderer.isFontLoaded()) {
            sf::Text info(m_renderer.getFont());
            info.setCharacterSize(30);
            info.setPosition({ 900.f, 100.f });
            info.setString("Replay: " + std::to_string(m_replay.getCurrentStep()) + "/" + std::to_string(m_replay.getTotalSteps()));
            info.setFillColor(sf::Color::White);
            m_window.draw(info);
        }
    }
    else if (m_state == GameState::Lobby) {
        if (m_renderer.isFontLoaded()) {
            // Draw title
            sf::Text titleText(m_renderer.getFont(), "GAME LOBBY", 50);
            titleText.setFillColor(sf::Color(220, 220, 255));
            auto tBounds = titleText.getLocalBounds();
            titleText.setOrigin({tBounds.position.x + tBounds.size.x / 2.f, 0});
            titleText.setPosition({600.f, 50.f});
            m_window.draw(titleText);

            // Draw player info
            float centerX = 600.f;
            float infoY = 200.f;

            // Player 1 (Host)
            sf::Text p1Text(m_renderer.getFont(), "Player 1 (Host/BLACK)", 30);
            p1Text.setFillColor(sf::Color::White);
            p1Text.setPosition({centerX - 300.f, infoY});
            m_window.draw(p1Text);

            // Status circle for Player 1
            sf::CircleShape p1Indicator(15.f);
            p1Indicator.setFillColor(sf::Color(100, 200, 100));
            p1Indicator.setPosition({centerX - 330.f, infoY + 50.f});
            m_window.draw(p1Indicator);

            // Player 2 (Client) or waiting
            if (m_network.getStatus() == Network::Status::Connected) {
                sf::Text p2Text(m_renderer.getFont(), "Player 2 (Client/WHITE)", 30);
                p2Text.setFillColor(sf::Color::White);
                p2Text.setPosition({centerX + 50.f, infoY});
                m_window.draw(p2Text);

                // Status circle for Player 2
                sf::CircleShape p2Indicator(15.f);
                p2Indicator.setFillColor(sf::Color(100, 200, 100));
                p2Indicator.setPosition({centerX + 20.f, infoY + 50.f});
                m_window.draw(p2Indicator);

                m_clientConnected = true;
            } else {
                sf::Text waitingText(m_renderer.getFont(), "Waiting for Player 2...", 30);
                waitingText.setFillColor(sf::Color(150, 150, 150));
                waitingText.setPosition({centerX + 50.f, infoY});
                m_window.draw(waitingText);

                // Darkened status circle
                sf::CircleShape p2Indicator(15.f);
                p2Indicator.setFillColor(sf::Color(80, 80, 80));
                p2Indicator.setPosition({centerX + 20.f, infoY + 50.f});
                m_window.draw(p2Indicator);

                // Show instructions
                sf::Text instrText(m_renderer.getFont(), "Other players can connect using:", 20);
                instrText.setFillColor(sf::Color(200, 200, 200));
                instrText.setPosition({150.f, 400.f});
                m_window.draw(instrText);

                std::string ipInfo = "Your IP: " + m_network.getLocalIP() + " (Port: 55001)";
                sf::Text ipText(m_renderer.getFont(), ipInfo, 20);
                ipText.setFillColor(sf::Color(150, 200, 255));
                ipText.setPosition({150.f, 430.f});
                m_window.draw(ipText);

                sf::Text roomInfo(m_renderer.getFont(), "Room: " + m_roomName, 20);
                roomInfo.setFillColor(sf::Color(220, 220, 180));
                roomInfo.setPosition({150.f, 460.f});
                m_window.draw(roomInfo);
            }
        }

        // Draw buttons
        for (auto& btn : m_lobbyButtons) {
            btn->draw(m_window);
        }
    }
    else {
        m_renderer.drawBoardGrid(m_window, m_boardOffset, m_cellSize);
        m_renderer.drawStones(m_window, m_board, m_boardOffset, m_cellSize);
        m_renderer.drawHighlights(m_window, m_board, m_boardOffset, m_cellSize);

        // 畫 Hover 預覽
        if (m_state == GameState::Playing && m_playMode == PlayMode::LocalPvP ||
            (m_playMode == PlayMode::LocalPvAI && m_currentTurn == Cell::BLACK) ||
            ((m_playMode == PlayMode::NetworkHost || m_playMode == PlayMode::NetworkClient) && m_currentTurn == m_myNetworkColor))
        {
            auto mousePos = sf::Mouse::getPosition(m_window);
            float boardEndX = m_boardOffset.x + (BOARD_SIZE - 1) * m_cellSize;
            float boardEndY = m_boardOffset.y + (BOARD_SIZE - 1) * m_cellSize;
            if (mousePos.x >= m_boardOffset.x - m_cellSize / 2 && mousePos.x <= boardEndX + m_cellSize / 2 &&
                mousePos.y >= m_boardOffset.y - m_cellSize / 2 && mousePos.y <= boardEndY + m_cellSize / 2) {
                int c = std::round((mousePos.x - m_boardOffset.x) / m_cellSize);
                int r = std::round((mousePos.y - m_boardOffset.y) / m_cellSize);
                if (m_board.getCell(r, c) == Cell::EMPTY) {
                    m_renderer.drawHoverStone(m_window, r, c, m_currentTurn, m_boardOffset, m_cellSize);
                }
            }
        }

        // 畫 UI (Turn text)
        if (m_renderer.isFontLoaded()) {
            sf::Text info(m_renderer.getFont());
            info.setCharacterSize(30);
            info.setPosition({ 900.f, 100.f });
            if (m_state == GameState::Playing) {
                info.setString(m_currentTurn == Cell::BLACK ? "Turn: BLACK" : "Turn: WHITE");
                info.setFillColor(m_currentTurn == Cell::BLACK ? sf::Color(100, 100, 100) : sf::Color::White);
                m_window.draw(info);
            }
            else if (m_state == GameState::GameOver) {
                // Game Over will be drawn below as a central popup
            }

            // Timer
            if (getGameSettings().timerEnabled) {
                sf::Text timerText(m_renderer.getFont(), "Time: " + m_timer.getTimeString(), 30);
                timerText.setPosition({ 900.f, 150.f });
                if (m_timer.isWarning()) timerText.setFillColor(sf::Color::Red);
                else timerText.setFillColor(sf::Color::White);
                m_window.draw(timerText);
            }
        }

        // 隨時畫按鈕
        for (std::size_t i = 0; i < m_gameButtons.size(); ++i) {
            if (!isGameButtonVisible(i)) {
                continue;
            }
            m_gameButtons[i]->draw(m_window);
        }

        // GameOver 中央視窗
        if (m_state == GameState::GameOver && m_renderer.isFontLoaded()) {
            sf::RectangleShape overlay(sf::Vector2f(m_window.getSize()));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            m_window.draw(overlay);

            sf::RectangleShape panel(sf::Vector2f(400.f, 250.f));
            panel.setOrigin({ 200.f, 125.f });
            panel.setPosition({ 600.f, 400.f });
            panel.setFillColor(sf::Color(50, 50, 50, 240));
            panel.setOutlineThickness(2.f);
            panel.setOutlineColor(sf::Color::White);
            m_window.draw(panel);

            sf::Text winText(m_renderer.getFont());
            winText.setCharacterSize(45);
            if (m_winner == Cell::BLACK) { winText.setString("WINNER: BLACK"); winText.setFillColor(sf::Color::White); }
            else if (m_winner == Cell::WHITE) { winText.setString("WINNER: WHITE"); winText.setFillColor(sf::Color::Yellow); }
            else { winText.setString("DRAW!"); winText.setFillColor(sf::Color::Red); }

            sf::FloatRect bounds = winText.getLocalBounds();
            winText.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
            winText.setPosition({ 600.f, 340.f });
            m_window.draw(winText);

            if (m_btnPlayAgain) {
                m_btnPlayAgain->draw(m_window);
            }

            if (m_pendingUndoRequest && m_btnUndoAccept && m_btnUndoReject) {
                sf::Text undoText(m_renderer.getFont(), "Opponent requests undo", 28);
                undoText.setFillColor(sf::Color(240, 220, 140));
                auto undoBounds = undoText.getLocalBounds();
                undoText.setOrigin({ undoBounds.position.x + undoBounds.size.x / 2.f,
                                     undoBounds.position.y + undoBounds.size.y / 2.f });
                undoText.setPosition({ 600.f, 320.f });
                m_window.draw(undoText);

                m_btnUndoAccept->draw(m_window);
                m_btnUndoReject->draw(m_window);
            }
        }

        if (m_state == GameState::Playing && m_pendingUndoRequest && m_btnUndoAccept && m_btnUndoReject && m_renderer.isFontLoaded()) {
            sf::RectangleShape overlay(sf::Vector2f(m_window.getSize()));
            overlay.setFillColor(sf::Color(0, 0, 0, 100));
            m_window.draw(overlay);

            sf::RectangleShape panel(sf::Vector2f(520.f, 180.f));
            panel.setOrigin({ 260.f, 90.f });
            panel.setPosition({ 600.f, 360.f });
            panel.setFillColor(sf::Color(45, 45, 60, 230));
            panel.setOutlineThickness(2.f);
            panel.setOutlineColor(sf::Color(180, 180, 220));
            m_window.draw(panel);

            sf::Text undoText(m_renderer.getFont(), "Opponent requests undo", 30);
            undoText.setFillColor(sf::Color(240, 220, 140));
            auto undoBounds = undoText.getLocalBounds();
            undoText.setOrigin({ undoBounds.position.x + undoBounds.size.x / 2.f,
                                 undoBounds.position.y + undoBounds.size.y / 2.f });
            undoText.setPosition({ 600.f, 315.f });
            m_window.draw(undoText);

            m_btnUndoAccept->draw(m_window);
            m_btnUndoReject->draw(m_window);
        }
    }

    drawTransition();

    // 繪製自訂鼠標
    if (getGameSettings().customCursorEnabled && m_cursorSprite) {
        m_window.draw(*m_cursorSprite);
    }

    m_window.display();
}
