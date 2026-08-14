// chunkcoord.h
#pragma once
#include <cstdint>
#include <unordered_map>

struct ChunkCoord {
    int x, z;
    bool operator==(const ChunkCoord& o) const {
        return x == o.x && z == o.z;
    }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const {
        // Shift-XOR collides for pairs like (0,1) vs (65536,0). Multiply-hash
        // stays unique for the chunk coords we actually load.
        const size_t x = static_cast<size_t>(static_cast<uint32_t>(c.x));
        const size_t z = static_cast<size_t>(static_cast<uint32_t>(c.z));
        return x * 73856093u ^ z * 19349663u;
    }
};