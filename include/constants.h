#pragma once
#include <cstdint>

// Lighting  
inline constexpr uint8_t MIN_LIGHT = 2;

// Simulation
inline constexpr float TICKS_PER_SECOND = 20.0f;

// Day cycle
inline constexpr float DAY_LENGTH_TICKS = 24000.0f;

//Hotbar
inline constexpr uint8_t HOTBAR_SIZE = 9;
inline constexpr int SLOT_SIZE = 48;
inline constexpr int SLOT_PADDING = 4;
inline constexpr int BOTTOM_MARGIN = 20;