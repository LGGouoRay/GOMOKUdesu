#pragma execution_character_set("utf-8")
#pragma once
#include <SFML/Graphics.hpp>
#include <future>
#include "Board.h"
#include "Renderer.h"
#include "Menu.h"
#include "AI.h"
#include "Network.h"
#include "SoundManager.h"
#include "Timer.h"
#include "SaveLoad.h"
#include "Replay.h"

// ============================================================
//  Game.h Game logic and class
// ============================================================

enum class GameState { Menu, Playing, GameOver, ReplayMode, SaveLoad, RoomList, Lobby, Connecting };
enum class PlayMode { LocalPvP, LocalPvAI, NetworkHost, NetworkClient };

class Game {
public:
	Game();
	void run();

private:
	void processEvents();
	void update(float dt);
	void render();

	// Game logic
	void handleBoardClick(int row, int col);
	void performMove(int row, int col);
	void applySkill(int skillId, int playerId, const std::vector<sf::Vector2i>& targets);
	void switchTurn();
	void resetGame();
	bool applyUndoMove();

	// Save/Load UI state variables
	bool m_isSaving = true;
	int m_maxSaveSlots = 5;
	float m_saveLoadScrollY = 0.0f;
	void incrementSaveSlots();
	void initSaveLoadUI();
	std::vector<std::unique_ptr<Button>> m_saveLoadButtons;

	// UI
	void initGameUI();
	void initReplayUI();
	void initLobbyUI();
	void initRoomListUI();
	void beginNetworkConnect(const std::string& ip, unsigned short port, const std::string& roomCode);
	void updateConnecting(float dt);

	// Settings / Actions
	void toggleAI();
	void saveCurrentGame();
	void loadSavedGame();
	void enterReplayMode();
	bool isNetworkMode() const;
	bool canUseSaveLoad() const;
	bool isGameButtonVisible(std::size_t index) const;
	void updateGameButtonStates();

	// Transition
	float m_fadeAlpha = 0.0f;
	int m_fadeState = 0;
	GameState m_nextState = GameState::Menu;
	void startTransition(GameState targetState);
	void updateTransition(float dt);
	void drawTransition();

    // SFML Window
    sf::RenderWindow m_window;
    Renderer m_renderer;
    Board m_board;
    Menu m_menu;
    AI m_ai;
    Network m_network;
    SoundManager m_soundMgr;
    Timer m_timer;
    
    // Game State
    GameState m_state = GameState::Menu;
    PlayMode m_playMode = PlayMode::LocalPvP;
    bool m_isExperimentalMode = false;
    Cell m_currentTurn = Cell::BLACK;
    Cell m_myNetworkColor = Cell::BLACK;
    Cell m_winner = Cell::EMPTY;
    bool m_hostIsSavingLoading = false;

    // UI positions
    sf::Vector2f m_boardOffset{100.f, 100.f};
    float m_cellSize = 45.f;
    std::vector<std::unique_ptr<Button>> m_gameButtons;
    std::vector<std::unique_ptr<Button>> m_replayButtons;
    std::vector<std::unique_ptr<Button>> m_lobbyButtons;
	std::vector<std::unique_ptr<Button>> m_roomListButtons;
	std::vector<std::unique_ptr<Button>> m_chatButtons;

	struct EmojiMsg {
		int emojiId;
		float x, y;
		float life;
	};
	std::vector<EmojiMsg> m_emojiMessages;

	std::unique_ptr<Button> m_btnPlayAgain;
    std::unique_ptr<Button> m_btnUndoAccept;
    std::unique_ptr<Button> m_btnUndoReject;
    Button* m_btnToggleAIPtr = nullptr;
    bool m_pendingUndoRequest = false;
    Replay m_replay;

	// Room List and Lobby
	std::string m_selectedRoomIP;
	unsigned short m_selectedRoomPort;
	std::string m_roomName = "Room";
	bool m_clientConnected = false;
	std::future<bool> m_connectFuture;
	bool m_connectInProgress = false;
	float m_connectElapsed = 0.f;
	std::wstring m_connectErrorMessage;
	float m_connectErrorTimer = 0.f;
	float m_roomDiscoveryTimer = 0.f;
	float m_roomListRefreshTimer = 0.f;
	std::size_t m_lastRoomListHash = 0;

	sf::Clock m_clock;

	bool m_aiThinking = false;

	// Custom Cursor
	std::vector<sf::Texture> m_cursorTextures;
	std::unique_ptr<sf::Sprite> m_cursorSprite;

	// Skills
	int m_skillPoints[2] = {0, 0};
	bool m_extraTurnActive = false;
	float m_spFlashTime[2] = {0.0f, 0.0f};
	int m_spFlashLevel[2] = {0, 0};

	int m_cursorFrame = 0;
	float m_cursorAnimTime = 0.0f;
	const float m_cursorAnimMaxTime = 0.1f;
	bool m_cursorVisibilityApplied = true;
	sf::Vector2i m_lastMousePos{ -1, -1 };
};

