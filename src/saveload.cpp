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