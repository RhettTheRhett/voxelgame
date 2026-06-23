#include "raylib.h"
#include "raymath.h"
#include "chunk.h"
#include "noise.h"
#include "world.h"
#include "raycast.h"
#include "block.h"
#include "saveformat.h"
#include "saveload.h"
#include <cmath>
#include <filesystem>
#include <chrono>

enum class GameState { MENU, PLAYING };

void HandleNoiseInput(World& world) {
    auto regen = [&]() {
        SetNoiseSeed(world.seed);
        world.chunks.clear();
        GenerateWorld(world, 3, 0, 0);
    };

    if (IsKeyPressed(KEY_UP))    { world.noiseScale *= 1.5f; regen(); }
    if (IsKeyPressed(KEY_DOWN))  { world.noiseScale /= 1.5f; regen(); }
    if (IsKeyPressed(KEY_RIGHT)) { world.noiseOctaves = __min(world.noiseOctaves+1, 8); regen(); }
    if (IsKeyPressed(KEY_LEFT))  { world.noiseOctaves = __max(world.noiseOctaves-1, 1); regen(); }
    if (IsKeyPressed(KEY_E))     { world.noisePersistence = __min(world.noisePersistence+0.05f, 0.95f); regen(); }
    if (IsKeyPressed(KEY_Q))     { world.noisePersistence = __max(world.noisePersistence-0.05f, 0.05f); regen(); }
    if (IsKeyPressed(KEY_N))     { world.seed++; regen(); }
    if (IsKeyPressed(KEY_B))     { world.seed--; regen(); }
}

void UpdatePlayer(Camera3D& camera, float& yaw, float& pitch, float speed, float sensitivity) {
    Vector2 delta = GetMouseDelta();
    yaw   += delta.x * sensitivity;
    pitch += delta.y * sensitivity * -1;

    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    float pitchRad = pitch * DEG2RAD;
    float yawRad   = yaw   * DEG2RAD;

    Vector3 forward;
    forward.x = cosf(pitchRad) * cosf(yawRad);
    forward.y = sinf(pitchRad);
    forward.z = cosf(pitchRad) * sinf(yawRad);

    Vector3 moveForward = {cosf(yawRad), 0, sinf(yawRad)};
    Vector3 moveSide    = {cosf(yawRad + 90.0f * DEG2RAD), 0, sinf(yawRad + 90.0f * DEG2RAD)};

    float dt = GetFrameTime();
    if (IsKeyDown(KEY_W)) camera.position += moveForward * speed * dt;
    if (IsKeyDown(KEY_S)) camera.position -= moveForward * speed * dt;
    if (IsKeyDown(KEY_A)) camera.position -= moveSide    * speed * dt;
    if (IsKeyDown(KEY_D)) camera.position += moveSide    * speed * dt;
    if (IsKeyDown(KEY_SPACE))        camera.position.y += speed * dt;
    if (IsKeyDown(KEY_LEFT_CONTROL)) camera.position.y -= speed * dt;
    

    camera.target = camera.position + forward;
}

void UpdateWorldStreaming(World& world, int playerChunkX, int playerChunkZ, float renderDistance, int& lastPlayerChunkX, int& lastPlayerChunkZ) {
    if (playerChunkX != lastPlayerChunkX || playerChunkZ != lastPlayerChunkZ) {
        UnloadDistantChunks(world, playerChunkX, playerChunkZ, renderDistance);
        GenerateWorld(world, renderDistance, playerChunkX, playerChunkZ);
        lastPlayerChunkX = playerChunkX;
        lastPlayerChunkZ = playerChunkZ;
    }
}

void DrawHUD(const World& world, const Camera3D& camera, bool showNoiseDebug) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    if (showNoiseDebug) {
        int previewSize = 256;
        for (int px = 0; px < previewSize; px++) {
            for (int py = 0; py < previewSize; py++) {
                float wx = px * world.noiseScale;
                float wy = py * world.noiseScale;
                float n  = FBm2D(wx, wy, world.noiseOctaves, world.noisePersistence);
                float t  = (n + 1.0f) / 2.0f;
                unsigned char c = (unsigned char)(t * 255);
                DrawPixel(px, py, {c, c, c, 255});
            }
        }
    }

    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, BLACK);
    DrawText(TextFormat("x: %.2f  y: %.2f  z: %.2f", camera.position.x, camera.position.y, camera.position.z), 10, 35, 20, BLACK);
    DrawText(TextFormat("Scale: %.4f  [UP/DOWN]",     world.noiseScale),       10, 60,  20, DARKGREEN);
    DrawText(TextFormat("Octaves: %d  [LEFT/RIGHT]",  world.noiseOctaves),     10, 85,  20, DARKGREEN);
    DrawText(TextFormat("Persist: %.2f  [Q/E]",       world.noisePersistence), 10, 110, 20, DARKGREEN);
    DrawText(TextFormat("Seed: %d  [B/N]",            world.seed),             10, 135, 20, DARKGREEN);
    DrawLine(screenWidth/2, screenHeight/2 - 10, screenWidth/2, screenHeight/2 + 10, WHITE);
    DrawLine(screenWidth/2 - 10, screenHeight/2, screenWidth/2 + 10, screenHeight/2, WHITE);
}

void DrawChunkBorders(int playerChunkX, int playerChunkZ, int radius) {
    for (int cx = playerChunkX - radius; cx <= playerChunkX + radius; cx++) {
        for (int cz = playerChunkZ - radius; cz <= playerChunkZ + radius; cz++) {
            float x = cx * CHUNK_SIZE;
            float z = cz * CHUNK_SIZE;

            // Draw 4 vertical edges of the chunk column
            DrawLine3D({x,             0, z},             {x,             CHUNK_HEIGHT, z},             GREEN);
            DrawLine3D({x + CHUNK_SIZE, 0, z},             {x + CHUNK_SIZE, CHUNK_HEIGHT, z},             GREEN);
            DrawLine3D({x,             0, z + CHUNK_SIZE}, {x,             CHUNK_HEIGHT, z + CHUNK_SIZE}, GREEN);
            DrawLine3D({x + CHUNK_SIZE, 0, z + CHUNK_SIZE}, {x + CHUNK_SIZE, CHUNK_HEIGHT, z + CHUNK_SIZE}, GREEN);
        }
    }
}

Texture2D LoadBlockAtlas() {
    Texture2D atlas = LoadTexture("assets/textures/blocks/blocksatlas.png");
    return atlas;
}

bool StartNewWorld(World& world, Camera3D& camera, std::string path) {
    std::filesystem::remove_all(path); // clear previous chunks
    std::filesystem::create_directories(path + "/chunks"); // built from the param, not hardcoded
    

    int32_t worldSeed = GetRandomValue(-99999999, 99999999);
    world.seed             = worldSeed;
    world.noiseScale       = 0.0044f;
    world.noiseOctaves     = 4;
    world.noisePersistence = 0.55f;

    WorldManifest manifest = {}; // zero-init everything first, so untouched fields aren't garbage
    manifest.worldSignature = WORLD_FILE_SIGNATURE;
    manifest.seed            = worldSeed;
    manifest.versionMajor    = WORLD_VERSION_MAJOR;
    manifest.versionMinor    = WORLD_VERSION_MINOR;
    manifest.versionPatch    = WORLD_VERSION_PATCH;

    strncpy(manifest.worldName, "world", sizeof(manifest.worldName) - 1);
    // strncpy: copies into the existing buffer, won't overrun it.
    // sizeof(...) - 1 reserves the last byte for a guaranteed null terminator,
    // since strncpy doesn't null-terminate if the source is >= the limit you give it.

    uint64_t nowSeconds = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    manifest.worldCreationTime = nowSeconds;

    manifest.spawnX = 0;
    manifest.spawnY = 0;
    manifest.spawnZ = 0;

    SetNoiseSeed(worldSeed);

    camera.fovy       = 70.0f;
    camera.position   = {manifest.spawnX, manifest.spawnY, manifest.spawnZ};
    camera.target     = {0, 0, 0};
    camera.up         = {0, 1, 0};
    camera.projection = CAMERA_PERSPECTIVE;

    bool saved = SaveWorldManifest(manifest, path + "/world.dat");
    // ask SaveWorldManifest directly whether it worked — don't re-derive
    // that answer from an unrelated filesystem check
    if (!saved) {
        printf("Failed to save new world manifest to %s\n", path.c_str());
    }
    return saved; // true = ready for PLAYING, false = main should NOT transition state
}

bool ContinueWorld(World& world, Camera3D& camera, const std::string& path) {
    std::optional<WorldManifest> result = LoadWorldManifest(path + "/world.dat");
    if (!result) {
        printf("No valid world manifest found at %s\n", path.c_str());
        return false;
    }

    WorldManifest manifest = result.value();

    world.seed             = manifest.seed;
    world.noiseScale       = 0.0044f;
    world.noiseOctaves     = 4;
    world.noisePersistence = 0.55f;

    SetNoiseSeed(manifest.seed);

    camera.fovy       = 70.0f;
    camera.position   = {manifest.spawnX, manifest.spawnY, manifest.spawnZ};
    camera.target     = {0, 0, 0};
    camera.up         = {0, 1, 0};
    camera.projection = CAMERA_PERSPECTIVE;

    return true;
}

void SaveAndQuit(World& world, Camera3D& camera, const std::string& path){
    for (auto& [coord, chunk] : world.chunks){
        if (chunk.needsSaving) {
            std::string chunkPath = GetChunkFilePath(CHUNK_PATH, coord.x, coord.z);
            printf("Saving chunk %d, %d to %s\n", coord.x, coord.z, chunkPath.c_str());
            SaveChunk(chunk, coord.x, coord.z, chunkPath);
        }
    }
    std::optional<WorldManifest> result = LoadWorldManifest(path + "/world.dat");
    if (!result) {
        printf("Failed to load manifest during SaveAndQuit\n");
        return;
    }
    WorldManifest manifest = result.value();

    manifest.spawnX = camera.position.x;
    manifest.spawnY = camera.position.y;
    manifest.spawnZ = camera.position.z;

    bool saved = SaveWorldManifest(manifest, path + "/world.dat");
}

bool DrawButton(Rectangle rect, const char* label, int fontSize, Color buttonColor, Color fontColor){

    Vector2 mousePoint = GetMousePosition();
    bool btnAction = false;

    DrawRectangle(rect.x,rect.y,rect.width,rect.height, buttonColor);
    DrawText(label, rect.x,rect.y,fontSize, fontColor);
    if (CheckCollisionPointRec(mousePoint, rect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return true;
    return false;
}

int main(){
    ChangeDirectory(GetApplicationDirectory());
    std::filesystem::create_directories("saves/world/chunks");
    GameState state = GameState::MENU;
    InitWindow(1080, 720, "Voxel Game");
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int lastPlayerChunkX = INT_MIN;
    int lastPlayerChunkZ = INT_MIN;

    float yaw         = -90.0f;
    float pitch       = 0.0f;
    float speed       = 15.0f;
    float sensitivity = 0.1f;
    float renderDistance = 4;
    bool showNoiseDebug = false;
    bool showChunkBorders = false; 

    Texture2D atlas = LoadBlockAtlas();
    Material mat = LoadMaterialDefault();
    mat.maps[MATERIAL_MAP_DIFFUSE].texture = atlas;

    Camera3D camera = {};
    World world = {};

    EnableCursor();

    while(!WindowShouldClose()){
        
        switch (state)
        {
        // MENU
        case GameState::MENU : {

            Rectangle newWorldButton = {(float)screenWidth / 2, (float)screenHeight / 4, 300 , 100};
            Rectangle continueButton = {(float)screenWidth / 2, (float)screenHeight / 2, 300 , 100};
            BeginDrawing();
            if(DrawButton(newWorldButton, "New World", 16, BROWN, LIGHTGRAY)){
                if(StartNewWorld(world,camera,CHUNK_PATH)){
                   state = GameState::PLAYING;
                    DisableCursor(); 
                }
                
            }
            if (std::filesystem::exists("saves/world/world.dat")) {
             if (DrawButton(continueButton, "Continue", 16, BROWN, LIGHTGRAY)){
                    if(ContinueWorld(world, camera, CHUNK_PATH)){
                     state = GameState::PLAYING;
                        DisableCursor();    
                    }
                }
            }
        }
            EndDrawing();
            break;
        
        // PLAYING
        case GameState::PLAYING : {

            UpdatePlayer(camera, yaw, pitch, speed, sensitivity);
            HandleNoiseInput(world);

            int playerChunkX = (int)floor(camera.position.x / CHUNK_SIZE);
            int playerChunkZ = (int)floor(camera.position.z / CHUNK_SIZE);
            UpdateWorldStreaming(world, playerChunkX, playerChunkZ, renderDistance, lastPlayerChunkX, lastPlayerChunkZ);

            Ray ray = GetMouseRay({screenWidth/2.0f, screenHeight/2.0f}, camera);
            RayHit hit = RayCast(ray, world, 8.0f);

            if(hit.didHit){
                int worldBlockX = (int)hit.position.x;
                int worldBlockY = (int)hit.position.y;
                int worldBlockZ = (int)hit.position.z;

                int placeX = (int)hit.position.x + FACE_DIRS[hit.faceHit][0];
                int placeY = (int)hit.position.y + FACE_DIRS[hit.faceHit][1];
                int placeZ = (int)hit.position.z + FACE_DIRS[hit.faceHit][2];

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    SetBlock(world, worldBlockX, worldBlockY, worldBlockZ, Block::AIR);

                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
                    SetBlock(world, placeX, placeY, placeZ, Block::LIGHT_STONE);
            }

            BeginDrawing();
                ClearBackground(SKYBLUE);
                BeginMode3D(camera);
                    DrawWorld(world, mat);
                    if (hit.didHit) {
                        DrawCubeWires(
                            {hit.position.x + 0.5f, hit.position.y + 0.5f, hit.position.z + 0.5f},
                            1.01f, 1.01f, 1.01f,
                            WHITE
                        );
                    }
                    if (showChunkBorders) {
                        DrawChunkBorders(playerChunkX, playerChunkZ, 3);
                    }
                EndMode3D();
                if (IsKeyPressed(KEY_TAB)) showNoiseDebug = !showNoiseDebug;
                if (IsKeyPressed(KEY_G)) showChunkBorders = ! showChunkBorders;
                DrawHUD(world, camera, showNoiseDebug);
            EndDrawing();

            break;
        }
    }
    }
    
    if (state == GameState::PLAYING) {
        SaveAndQuit(world, camera, CHUNK_PATH);
    }   
}