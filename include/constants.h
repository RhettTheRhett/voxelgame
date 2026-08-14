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

inline constexpr float PLAYER_GRAVITY = 28.0f;
inline constexpr float PLAYER_TERMINAL_VELOCITY = 50.0f;

inline constexpr float PLAYER_JUMP_HEIGHT = 1.25f;
inline constexpr float PLAYER_JUMP_SPEED  = 8.37f;  // sqrt(2 * PLAYER_GRAVITY * PLAYER_JUMP_HEIGHT)

inline constexpr float PLAYER_MOVE_SPEED = 4.3f;
inline constexpr float PLAYER_RUN_MULTIPLIER = 1.45f;

inline constexpr float PLAYER_JUMP_BUFFER = 0.12f;

inline constexpr float PLAYER_STEP_HEIGHT = 0.5f;

inline constexpr float PLAYER_DEBUG_EYE_Y = 80.0f;
