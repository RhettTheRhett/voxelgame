#pragma once
#include <cstdint>



// ============================================================
// Note on design for future Rhett:
// On-disk save format structs.
// These are binary contracts — fixed-size types only, no
// engine/runtime types (Vector3, bool, etc.) allowed here.
// Changing field order or types breaks old saves; bump the
// relevant version field instead.
// ============================================================

inline constexpr uint8_t WORLD_VERSION_MAJOR = 1;
inline constexpr uint8_t WORLD_VERSION_MINOR = 0;
inline constexpr uint8_t WORLD_VERSION_PATCH = 0;

inline constexpr uint32_t WORLD_FILE_SIGNATURE = 0x564F4C44;


struct ChunkHeader {
    uint32_t chunkSignature;
    int32_t  chunkX;
    int32_t  chunkZ;
    uint8_t  chunkSize;
    uint16_t chunkHeight;
    uint8_t  bytesPerBlock;
    uint8_t  version;
};

struct WorldManifest {
    uint32_t worldSignature;
    char     worldName[64];
    int32_t  seed;
    uint8_t  versionMajor;
    uint8_t  versionMinor;
    uint8_t  versionPatch;
    uint64_t worldCreationTime;
    float    spawnX, spawnY, spawnZ;
    uint8_t  pvpEnabled;
    uint8_t  cheatsEnabled;
};

struct PlayerData {
    float posX, posY, posZ;
    float yaw, pitch;
};

