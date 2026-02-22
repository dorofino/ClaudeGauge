#pragma once

#include <TFT_eSPI.h>
#include "data_models.h"

namespace UIRenderer {
    void drawOverview(TFT_eSprite& spr, const AppState& state, uint32_t countdown);
    void drawModels(TFT_eSprite& spr, const AppState& state, uint32_t countdown);
    void drawMonthlyModels(TFT_eSprite& spr, const AppState& state, uint32_t countdown);
    void drawClaudeCode(TFT_eSprite& spr, const AppState& state, uint32_t countdown);
    void drawMonthlyCode(TFT_eSprite& spr, const AppState& state, uint32_t countdown);
    void drawStatus(TFT_eSprite& spr, const AppState& state, uint32_t countdown);
    void drawClaudeAi(TFT_eSprite& spr, const AppState& state, uint32_t countdown);
}
