// chunkcoord.h
#pragma once
#include <unordered_map>

struct ChunkCoord {
    int x, z;
    bool operator==(const ChunkCoord& o) const {
        return x == o.x && z == o.z;
    }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.z) << 16);
    }
};