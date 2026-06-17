#include <stdio.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstdint>
#include "saveload.h"



void SaveWorldManifest(const WorldManifest& manifest, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        printf("Failed to save world manifest to %s\n", path.c_str());
        return;
    }
    file.write(reinterpret_cast<const char*>(&manifest), sizeof(manifest));
    file.close();
}

bool LoadWorldManifest(const std::string& path, WorldManifest& outManifest){
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        printf("Failed to save world manifest to %s\n", path.c_str());
        return false;
    }
    file.read(reinterpret_cast<char*>(&outManifest), sizeof(outManifest));
    file.close();
    return true;
}

std::string GetChunkFilePath(const std::string& worldFolder, int32_t chunkX, int32_t chunkZ) {
    return worldFolder + "/chunks/" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".chunk";
}

bool SaveChunk(const Chunk& chunk, int32_t chunkX, int32_t chunkZ, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        printf("Failed to save chunk to %s\n", path.c_str());
        return false;
    }

    ChunkHeader header;
    header.chunkSignature = 0x564F5843;
    header.chunkX = chunkX;
    header.chunkZ = chunkZ;
    header.chunkSize = CHUNK_SIZE;
    header.chunkHeight = CHUNK_HEIGHT;
    header.bytesPerBlock = sizeof(chunk.blocks[0][0][0]);
    header.version = 1;

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(chunk.blocks), sizeof(chunk.blocks));

    file.close();
    return true;
}
bool LoadChunk(Chunk& outChunk, int32_t chunkX, int32_t chunkZ, const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        printf("Failed to save chunk to %s\n", path.c_str());
        return false;
    }

    ChunkHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if(header.chunkSignature != CHUNK_FILE_SIGNATURE){
        printf("Chunk signature not valid");
        return false;
    }
    if(header.version != CHUNK_FILE_VERSION){
        printf("Chunk version not valid");
        return false;
    }
    if(header.chunkX != chunkX || header.chunkZ != chunkZ){
        printf("Chunk position not valid");
        return false;
    }
    if( header.chunkSize != CHUNK_SIZE || header.chunkHeight != CHUNK_HEIGHT){
        printf("Chunk boundaries not valid");
        return false;
    }
    if(header.bytesPerBlock != sizeof(outChunk.blocks[0][0][0])){
        printf("Chunk bytesperblock not valid");
        return false;
    }
                   
    file.read(reinterpret_cast<char*>(outChunk.blocks), sizeof(outChunk.blocks));
    outChunk.meshDirty = true;

    file.close();
    return true;
}