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
inline constexpr uint8_t SLOT_SIZE = 48;
inline constexpr uint8_t SLOT_PADDING = 4;
inline constexpr uint8_t BOTTOM_MARGIN = 20;

//Blocks
inline constexpr uint8_t TILE_SIZE = 16;

//World Version
inline constexpr uint8_t WORLD_VERSION_MAJOR = 1;
inline constexpr uint8_t WORLD_VERSION_MINOR = 2;
inline constexpr uint8_t WORLD_VERSION_PATCH = 0;

//Save Format
inline constexpr uint32_t WORLD_FILE_SIGNATURE = 0x564F4C44;

//Player
inline constexpr float PLAYER_HEIGHT = 1.8f;
inline constexpr float PLAYER_WIDTH = 0.6f;
inline constexpr float PLAYER_EYE_HEIGHT = 1.5f;
