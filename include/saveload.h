#include <stdio.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstdint>
#include "saveformat.h"

void SaveWorldManifest(const WorldManifest& manifest, const std::string& path);

void LoadWorldManifest(const WorldManifest& manifest, const std::string& path);