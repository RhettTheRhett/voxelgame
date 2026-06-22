#include <stdio.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstdint>
#include <optional>
#include "saveformat.h"
#include "chunk.h"

bool SaveWorldManifest(const WorldManifest& manifest, const std::string& path);

std::optional<WorldManifest> LoadWorldManifest(const std::string& path);

bool SaveChunk(const Chunk& chunk, int32_t chunkX, int32_t chunkZ, const std::string& path);

bool LoadChunk(Chunk& outChunk, int32_t chunkX, int32_t chunkZ, const std::string& path);

std::string GetChunkFilePath(const std::string& worldFolder, int32_t chunkX, int32_t chunkZ);