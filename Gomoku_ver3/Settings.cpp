#pragma execution_character_set("utf-8")
#include "Settings.h"





Settings& getGameSettings() {
    static Settings instance;
    return instance;
}

