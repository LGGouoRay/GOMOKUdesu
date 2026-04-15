# Gomoku 房間列表和遊戲大廳實現指南

## 功能概述

此實現為 Gomoku 遊戲添加了以下功能：

### 1. 遊戲大廳 (Lobby) 狀態
- **新增的遊戲狀態**: `GameState::Lobby`
- 當玩家連接或開房間後，會進入大廳而不是直接開始遊戲
- Host 可以在大廳中看到 Player 2 的連接狀態
- 只有 Host 可以點擊 "Start Game" 按鈕開始遊戲

### 2. 房間列表菜單
- **新增菜單狀態**: `MenuState::RoomList`
- **新增菜單動作**: `MenuAction::JoinRoom`
- 多人遊戲菜單現在有三個選項：
  - "Host Game" - 創建房間並等待玩家加入
  - "Find Room" - 搜索區域網路中的可用房間
  - "Connect Manually" - 手動輸入 IP 地址連接

### 3. UDP 廣播基礎結構（已預留）
- `Network` 類已添加 UDP 廣播相關方法：
  - `startBroadcasting()` - Host 定期發送廣播
  - `stopBroadcasting()` - 停止廣播
  - `discoverRooms()` - Client 開始監聽廣播
  - `updateRoomDiscovery()` - 更新發現的房間列表
  - `RoomInfo` 結構體 - 存儲房間信息

## 實現詳情

### Network 類的升級
```cpp
struct RoomInfo {
    sf::IpAddress ip;
    unsigned short port;
    std::string roomName;
    std::chrono::steady_clock::time_point lastSeen;
};
```

- Host 使用新簽名：`host(port, roomName)`
- 自動啟動廣播（目前為 stub，等待 SFML 3.0 API 更新）

### Game 類的升級
新增狀態變數：
- `m_clientConnected` - 追蹤是否有客戶端連接
- `m_roomName` - 房間名稱
- `m_selectedRoomIP` 和 `m_selectedRoomPort` - 選中的房間信息
- `m_roomDiscoveryTimer` - 房間發現計時器

新增方法：
- `initLobbyUI()` - 初始化大廳按鈕（Start Game, Leave）
- `initRoomListUI()` - 初始化房間列表

### Menu 類的升級
- `initRoomListButtons()` - 根據發現的房間創建按鈕
- 房間列表菜單顯示 "No rooms found" 直到有房間被發現

## 當前限制和未來工作

### UDP 廣播（暫時禁用）
由於 SFML 3.0 的 API 變化，UDP 廣播功能目前已禁用。實現方式：
1. Host 定期發送 UDP 廣播 `"RoomName|LocalIP|Port"`
2. Client 監聽廣播端口，接收並解析房間信息
3. 自動移除超過 5 秒未見的房間

### 需要完成的工作
1. **UDP 廣播實現**: 一旦 SFML 3.0 文檔更新，實現 `sendBroadcastPacket()` 和 `receiveBroadcastPackets()`
2. **IP 輸入界面**: 為手動連接添加文本輸入框
3. **房間名稱輸入**: 允許 Host 自訂房間名稱
4. **玩家信息**: 在大廳顯示玩家準備狀態

## 使用流程

### Host 開房間
1. 進入主菜單 → 多人遊戲
2. 點擊 "Host Game"
3. 進入遊戲大廳，等待玩家加入
4. 玩家加入後，按鈕變為可用
5. 點擊 "Start Game" 開始遊戲
6. 點擊 "Leave" 返回菜單

### Client 加入房間
**方式 1: 搜索房間（需要 UDP 廣播）**
1. 進入主菜單 → 多人遊戲
2. 點擊 "Find Room"
3. 等待房間列表顯示
4. 點擊房間加入
5. 進入遊戲大廳等待開始

**方式 2: 手動連接**
1. 進入主菜單 → 多人遊戲
2. 點擊 "Connect Manually"
3. 輸入 Host IP 地址（目前預設為 127.0.0.1）
4. 連接成功後進入遊戲大廳

## 文件更改列表

### 修改的文件
1. **Network.h** - 添加 UDP 廣播相關成員和方法
2. **Network.cpp** - 實現房間發現邏輯（stub 實現）
3. **Game.h** - 添加新的遊戲狀態和大廳相關變數
4. **Game.cpp** - 實現 Lobby 狀態的更新和渲染
5. **Menu.h** - 添加 RoomList 菜單狀態
6. **Menu.cpp** - 實現房間列表菜單

## 測試建議

1. 在同一台機器上測試本地連接（127.0.0.1）
2. 使用區域網路測試多台機器連接
3. 驗證大廳中的玩家狀態顯示
4. 確保 Host 可以控制遊戲開始

## 注意事項

- 目前 UDP 廣播已禁用，但基礎結構已準備好
- 連接端口固定為 55001，廣播端口為 55002
- 房間信息會自動在 5 秒後過期
- Client 連接後自動進入大廳等待 Host 開始遊戲
