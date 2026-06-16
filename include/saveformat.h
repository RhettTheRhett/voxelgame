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

