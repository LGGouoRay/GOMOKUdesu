#pragma once




struct Settings {
    
    float soundVolume = 70.f;
    bool soundMuted = false;

    bool timerEnabled = true;
    float timerLimit = 30.f;

    
    bool undoEnabled = true;
    bool showLastMoveStatus = true;
    
    // 自訂鼠標選項
    bool customCursorEnabled = false;
    
    int aiDepth = 3;  
};

Settings& getGameSettings();







