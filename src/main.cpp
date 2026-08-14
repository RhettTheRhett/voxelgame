#include "raylib.h"
#include "raymath.h"
#include "chunk.h"
#include "noise.h"
#include "world.h"
#include "raycast.h"
#include "block.h"
#include "saveformat.h"
#include "saveload.h"
#include "atmosphere.h"
#include "constants.h"
#include "player.h"
#include "collision.h"
#include "settings.h"
#include <cmath>
#include <filesystem>
#include <chrono>
#include <cstring>
#include <cstdio>

enum class GameState { MENU, PLAYING, PAUSED };

enum class BindSlot {
    None = -1,
    Forward = 0,
    Back,
    Left,
    Right,
    Jump,
    Sprint,
    Count
};

void UpdatePlayerLook(float& yaw, float& pitch, float sensitivity) {
    Vector2 delta = GetMouseDelta();
    yaw   += delta.x * sensitivity;
    pitch += delta.y * sensitivity * -1;

    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void ChangeHotbarSlot(Player& player){
    if(IsKeyPressed(KEY_ONE)){
        player.SelectSlot(0);
    }
    if(IsKeyPressed(KEY_TWO)){
        player.SelectSlot(1);
    }
    if(IsKeyPressed(KEY_THREE)){
        player.SelectSlot(2);
    }
    if(IsKeyPressed(KEY_FOUR)){
    player.SelectSlot(3);
    }
    if(IsKeyPressed(KEY_FIVE)){
    player.SelectSlot(4);
    }
    if(IsKeyPressed(KEY_SIX)){
    player.SelectSlot(5);
    }
    if(IsKeyPressed(KEY_SEVEN)){
        player.SelectSlot(6);
    }
    if(IsKeyPressed(KEY_EIGHT)){
        player.SelectSlot(7);
    }
    if(IsKeyPressed(KEY_NINE)){
        player.SelectSlot(8);
    }
    float mouseWheelMovement = GetMouseWheelMove();
    if(mouseWheelMovement){
        player.ScrollSlot((int)mouseWheelMovement * -1);
    }
}

void UpdateWorldStreaming(World& world, int playerChunkX, int playerChunkZ, int renderDistance, int& lastPlayerChunkX, int& lastPlayerChunkZ) {
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
    DrawText(TextFormat("Time Of Day: %.1f", world.manifest.timeOfDay), 10, 60, 20, DARKGREEN);
    DrawLine(screenWidth/2, screenHeight/2 - 10, screenWidth/2, screenHeight/2 + 10, WHITE);
    DrawLine(screenWidth/2 - 10, screenHeight/2, screenWidth/2 + 10, screenHeight/2, WHITE);
}

void DrawChunkBorders(int playerChunkX, int playerChunkZ, int radius) {
    for (int cx = playerChunkX - radius; cx <= playerChunkX + radius; cx++) {
        for (int cz = playerChunkZ - radius; cz <= playerChunkZ + radius; cz++) {
            float x = cx * CHUNK_SIZE;
            float z = cz * CHUNK_SIZE;

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
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path + "/chunks");
    
    world.noiseScale = 0.0044f;
    world.noiseOctaves = 4;
    world.noisePersistence = 0.55f;

    WorldManifest manifest = {};
    world.manifest = manifest; 
    manifest.worldSignature = WORLD_FILE_SIGNATURE;
    manifest.seed = GetRandomValue(-99999999, 99999999);;
    manifest.versionMajor = WORLD_VERSION_MAJOR;
    manifest.versionMinor = WORLD_VERSION_MINOR;
    manifest.versionPatch = WORLD_VERSION_PATCH;
    manifest.timeOfDay = 0.0f;

    strncpy(manifest.worldName, "world", sizeof(manifest.worldName) - 1);

    uint64_t nowSeconds = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    manifest.worldCreationTime = nowSeconds;

    manifest.spawnX = 0;
    manifest.spawnY = 80;
    manifest.spawnZ = 0;

    SetNoiseSeed(manifest.seed);

    camera.fovy       = DEFAULT_CAMERA_FOV;
    camera.position   = {manifest.spawnX, manifest.spawnY, manifest.spawnZ};
    camera.target     = {0, 0, 0};
    camera.up         = {0, 1, 0};
    camera.projection = CAMERA_PERSPECTIVE;

    bool saved = SaveWorldManifest(manifest, path + "/world.dat");
    if (!saved) {
        printf("Failed to save new world manifest to %s\n", path.c_str());
    }
    return saved;
}

bool ContinueWorld(World& world, Camera3D& camera, const std::string& path) {
    std::optional<WorldManifest> result = LoadWorldManifest(path + "/world.dat");
    if (!result) {
        printf("No valid world manifest found at %s\n", path.c_str());
        return false;
    }

    world.manifest = result.value();
    world.noiseScale = 0.0044f;
    world.noiseOctaves  = 4;
    world.noisePersistence = 0.55f;

    SetNoiseSeed(world.manifest.seed);

    camera.fovy = DEFAULT_CAMERA_FOV;
    camera.position = {world.manifest.spawnX, world.manifest.spawnY, world.manifest.spawnZ};
    camera.target = {0, 0, 0};
    camera.up = {0, 1, 0};
    camera.projection = CAMERA_PERSPECTIVE;

    return true;
}

void SaveWorld(World& world, Camera3D& camera, const std::string& path){
    std::filesystem::create_directories(path + "/chunks");

    for (auto& [coord, chunk] : world.chunks){
        std::string chunkPath = GetChunkFilePath(CHUNK_PATH, coord.x, coord.z);
        SaveChunk(chunk, coord.x, coord.z, chunkPath);
        chunk.needsSaving = false;
    }

    WorldManifest manifest = world.manifest;
    manifest.spawnX = camera.position.x;
    manifest.spawnY = camera.position.y;
    manifest.spawnZ = camera.position.z;
    world.manifest = manifest;

    SaveWorldManifest(manifest, path + "/world.dat");
}

void LeaveWorld(World& world, int& lastPlayerChunkX, int& lastPlayerChunkZ) {
    UnloadAllChunks(world);
    lastPlayerChunkX = INT_MIN;
    lastPlayerChunkZ = INT_MIN;
}

bool DrawButton(Rectangle rect, const char* label, int fontSize, Color buttonColor, Color fontColor){
    Vector2 mousePoint = GetMousePosition();

    DrawRectangle(rect.x,rect.y,rect.width,rect.height, buttonColor);
    DrawText(label, (int)rect.x, (int)rect.y, fontSize, fontColor);
    if (CheckCollisionPointRec(mousePoint, rect) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return true;
    return false;
}

void DrawHotbar(const Player& player, const Texture2D atlasTexture)
{
    const int hotbarWidth = HOTBAR_SIZE * SLOT_SIZE + (HOTBAR_SIZE - 1) * SLOT_PADDING;

    const int firstX = (GetScreenWidth() - hotbarWidth) / 2;
    const int firstY = GetScreenHeight() - SLOT_SIZE - BOTTOM_MARGIN;

    for (int i = 0; i < HOTBAR_SIZE; i++)
    {
        int x = firstX + i * (SLOT_SIZE + SLOT_PADDING);
        int y = firstY;

        DrawRectangle(x, y, SLOT_SIZE, SLOT_SIZE, DARKGRAY);

        int iconMargin = 4;

        BlockId blockInSlot = player.GetData().blockSlot[i];
        const BlockDefinition& def = GetBlockDef(blockInSlot);
        Vector2 faceTex = def.FACE_TEX[0];

        Rectangle sourceRect = {
            (float)(faceTex.x * TILE_SIZE),
            (float)(faceTex.y * TILE_SIZE),
            (float)(TILE_SIZE),
            (float)(TILE_SIZE)
        };

        Rectangle destRect = {
            (float)(x + iconMargin),
            (float)(y + iconMargin),
            (float)(SLOT_SIZE - iconMargin * 2),
            (float)(SLOT_SIZE - iconMargin * 2)
        };

        DrawTexturePro(atlasTexture, sourceRect, destRect, {0,0}, 0.0f, WHITE);

        Color borderColor = (i == player.GetData().currentSlot) ? YELLOW : LIGHTGRAY;

        DrawRectangleLines(x, y, SLOT_SIZE, SLOT_SIZE, borderColor);
    }
}

void AdvanceTime(float deltaTime, World& world){
    float deltaTicks = deltaTime * TICKS_PER_SECOND;
    world.manifest.timeOfDay += deltaTicks;
    world.manifest.timeOfDay = std::fmod(world.manifest.timeOfDay, DAY_LENGTH_TICKS);
}

static int* GetBindKeyPtr(GameSettings& settings, BindSlot slot) {
    switch (slot) {
        case BindSlot::Forward: return &settings.keyForward;
        case BindSlot::Back:    return &settings.keyBack;
        case BindSlot::Left:    return &settings.keyLeft;
        case BindSlot::Right:   return &settings.keyRight;
        case BindSlot::Jump:    return &settings.keyJump;
        case BindSlot::Sprint:  return &settings.keySprint;
        default: return nullptr;
    }
}

static const char* GetBindLabel(BindSlot slot) {
    switch (slot) {
        case BindSlot::Forward: return "Forward";
        case BindSlot::Back:    return "Back";
        case BindSlot::Left:    return "Left";
        case BindSlot::Right:   return "Right";
        case BindSlot::Jump:    return "Jump";
        case BindSlot::Sprint:  return "Sprint";
        default: return "?";
    }
}

static void FormatKeyName(int key, char* out, int outSize) {
    // Avoid GetKeyName + nested TextFormat (raylib TextFormat uses a tiny rotating
    // static buffer — nesting it crashes / corrupts strings).
    const char* name = nullptr;
    switch (key) {
        case KEY_SPACE:         name = "Space"; break;
        case KEY_LEFT_SHIFT:    name = "L-Shift"; break;
        case KEY_RIGHT_SHIFT:   name = "R-Shift"; break;
        case KEY_LEFT_CONTROL:  name = "L-Ctrl"; break;
        case KEY_RIGHT_CONTROL: name = "R-Ctrl"; break;
        case KEY_LEFT_ALT:      name = "L-Alt"; break;
        case KEY_RIGHT_ALT:     name = "R-Alt"; break;
        case KEY_TAB:           name = "Tab"; break;
        case KEY_ENTER:         name = "Enter"; break;
        case KEY_BACKSPACE:     name = "Backspace"; break;
        case KEY_UP:            name = "Up"; break;
        case KEY_DOWN:          name = "Down"; break;
        case KEY_LEFT:          name = "Left"; break;
        case KEY_RIGHT:         name = "Right"; break;
        default: break;
    }
    if (!name && key >= KEY_A && key <= KEY_Z) {
        snprintf(out, outSize, "%c", (char)key);
        return;
    }
    if (!name && key >= KEY_ZERO && key <= KEY_NINE) {
        snprintf(out, outSize, "%c", (char)key);
        return;
    }
    if (name) {
        snprintf(out, outSize, "%s", name);
        return;
    }
    snprintf(out, outSize, "Key %d", key);
}

// Returns true if caller should leave PAUSED (Keep Playing).
bool DrawPauseMenu(GameSettings& settings, BindSlot& waitingBind, bool& outSaveAndQuit)
{
    outSaveAndQuit = false;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.45f));

    // Compact panel so everything fits on 720p without off-screen buttons.
    const float panelW = 320.0f;
    float x = ((float)screenWidth - panelW) * 0.5f;
    float y = 24.0f;

    DrawText("Paused", (int)x, (int)y, 28, LIGHTGRAY);
    y += 36.0f;

    DrawText("Options", (int)x, (int)y, 20, LIGHTGRAY);
    y += 26.0f;

    {
        char line[64];
        snprintf(line, sizeof(line), "Mouse Sensitivity: %.2f", settings.mouseSensitivity);
        DrawText(line, (int)x, (int)y, 16, LIGHTGRAY);
    }
    y += 20.0f;
    Rectangle sensDown = { x, y, 150, 36 };
    Rectangle sensUp   = { x + 170, y, 150, 36 };
    if (DrawButton(sensDown, "- Sensitivity", 14, BROWN, LIGHTGRAY)) {
        settings.mouseSensitivity -= 0.02f;
        settings.Clamp();
    }
    if (DrawButton(sensUp, "+ Sensitivity", 14, BROWN, LIGHTGRAY)) {
        settings.mouseSensitivity += 0.02f;
        settings.Clamp();
    }
    y += 44.0f;

    {
        char line[64];
        snprintf(line, sizeof(line), "Render Distance: %d", settings.renderDistance);
        DrawText(line, (int)x, (int)y, 16, LIGHTGRAY);
    }
    y += 20.0f;
    Rectangle rdDown = { x, y, 150, 36 };
    Rectangle rdUp   = { x + 170, y, 150, 36 };
    if (DrawButton(rdDown, "- Distance", 14, BROWN, LIGHTGRAY)) {
        settings.renderDistance -= 1;
        settings.Clamp();
    }
    if (DrawButton(rdUp, "+ Distance", 14, BROWN, LIGHTGRAY)) {
        settings.renderDistance += 1;
        settings.Clamp();
    }
    y += 44.0f;

    {
        char line[64];
        snprintf(line, sizeof(line), "FOV: %.0f", settings.cameraFov);
        DrawText(line, (int)x, (int)y, 16, LIGHTGRAY);
    }
    y += 20.0f;
    Rectangle fovDown = { x, y, 150, 36 };
    Rectangle fovUp   = { x + 170, y, 150, 36 };
    if (DrawButton(fovDown, "- FOV", 14, BROWN, LIGHTGRAY)) {
        settings.cameraFov -= 5.0f;
        settings.Clamp();
    }
    if (DrawButton(fovUp, "+ FOV", 14, BROWN, LIGHTGRAY)) {
        settings.cameraFov += 5.0f;
        settings.Clamp();
    }
    y += 44.0f;

    DrawText("Controls (click to rebind)", (int)x, (int)y, 16, LIGHTGRAY);
    y += 22.0f;
    if (waitingBind != BindSlot::None) {
        char line[96];
        snprintf(line, sizeof(line), "Press a key for %s (Esc cancel)", GetBindLabel(waitingBind));
        DrawText(line, (int)x, (int)y, 14, YELLOW);
        y += 18.0f;
    }

    for (int i = 0; i < (int)BindSlot::Count; i++) {
        BindSlot slot = (BindSlot)i;
        int* keyPtr = GetBindKeyPtr(settings, slot);
        if (!keyPtr) continue;

        char keyName[32];
        FormatKeyName(*keyPtr, keyName, sizeof(keyName));

        char label[64];
        if (waitingBind == slot) {
            snprintf(label, sizeof(label), "%s: ...", GetBindLabel(slot));
        } else {
            snprintf(label, sizeof(label), "%s: %s", GetBindLabel(slot), keyName);
        }

        Rectangle bindBtn = { x, y, panelW, 32 };
        Color color = (waitingBind == slot) ? DARKBROWN : BROWN;
        if (DrawButton(bindBtn, label, 14, color, LIGHTGRAY)) {
            waitingBind = slot;
        }
        y += 36.0f;
    }

    y += 8.0f;

    Rectangle keepPlayingBtn = { x, y, panelW, 48 };
    if (DrawButton(keepPlayingBtn, "Keep Playing", 16, BROWN, LIGHTGRAY)) {
        waitingBind = BindSlot::None;
        return true;
    }
    y += 56.0f;

    Rectangle saveQuitBtn = { x, y, panelW, 48 };
    if (DrawButton(saveQuitBtn, "Save and Quit", 16, BROWN, LIGHTGRAY)) {
        waitingBind = BindSlot::None;
        outSaveAndQuit = true;
        return false;
    }

    return false;
}

void HandleBindCapture(GameSettings& settings, BindSlot& waitingBind) {
    if (waitingBind == BindSlot::None) return;

    if (IsKeyPressed(KEY_ESCAPE)) {
        waitingBind = BindSlot::None;
        return;
    }

    // Only poll known bindable keys — avoid scanning the entire keycode range.
    static const int kBindableKeys[] = {
        KEY_SPACE, KEY_TAB, KEY_ENTER,
        KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT,
        KEY_LEFT_CONTROL, KEY_RIGHT_CONTROL,
        KEY_LEFT_ALT, KEY_RIGHT_ALT,
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
        KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
        KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
        KEY_ZERO, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
        KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE,
    };

    for (int key : kBindableKeys) {
        if (IsKeyPressed(key)) {
            int* dest = GetBindKeyPtr(settings, waitingBind);
            if (dest) *dest = key;
            waitingBind = BindSlot::None;
            return;
        }
    }
}

int main(){
    ChangeDirectory(GetApplicationDirectory());
    InitBlockRegistry();
    GameState state = GameState::MENU;
    InitWindow(1080, 720, "Voxel Game");
    SetExitKey(KEY_NULL); // Escape opens pause menu instead of closing the window

    SetTraceLogLevel(LOG_WARNING);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    Shader shader = LoadShader("assets/shaders/chunk.vert", "assets/shaders/chunk.frag");
    int sunBrightnessLocation = GetShaderLocation(shader, "sunBrightness"); 

    int lastPlayerChunkX = INT_MIN;
    int lastPlayerChunkZ = INT_MIN;

    float yaw = -90.0f;
    float pitch = 0.0f;
    bool showNoiseDebug = false;
    bool showChunkBorders = false;

    GameSettings settings;
    BindSlot waitingBind = BindSlot::None;

    Texture2D atlas = LoadBlockAtlas();
    Material mat = LoadMaterialDefault();
    mat.maps[MATERIAL_MAP_DIFFUSE].texture = atlas;
    mat.shader = shader;

    Camera3D camera = {};
    World world = {};
    Player player;

    EnableCursor();
    bool appQuit = false;

    while(!WindowShouldClose() && !appQuit){    
        bool pauseOpenedThisFrame = false;

        switch (state)
        {
        case GameState::MENU : {

            Rectangle newWorldButton = {(float)screenWidth / 2, (float)screenHeight / 4, 300 , 80};
            Rectangle continueButton = {(float)screenWidth / 2, (float)screenHeight / 2, 300 , 80};
            Rectangle quitButton     = {(float)screenWidth / 2, (float)screenHeight * 0.75f, 300 , 80};
            bool quitGame = false;
            BeginDrawing();
            ClearBackground(DARKGRAY);
            if(DrawButton(newWorldButton, "New World", 16, BROWN, LIGHTGRAY)){
                if(StartNewWorld(world,camera,CHUNK_PATH)){
                    camera.fovy = settings.cameraFov;
                    state = GameState::PLAYING;
                    player.SetPositionFromEye(camera.position);
                    lastPlayerChunkX = INT_MIN;
                    lastPlayerChunkZ = INT_MIN;
                    DisableCursor(); 
                }
                
            }
            if (std::filesystem::exists("saves/world/world.dat")) {
             if (DrawButton(continueButton, "Continue", 16, BROWN, LIGHTGRAY)){
                    if(ContinueWorld(world, camera, CHUNK_PATH)){
                        camera.fovy = settings.cameraFov;
                        state = GameState::PLAYING;
                        player.SetPositionFromEye(camera.position);
                        lastPlayerChunkX = INT_MIN;
                        lastPlayerChunkZ = INT_MIN;
                        DisableCursor();    
                    }
                }
            }
            if (DrawButton(quitButton, "Quit Game", 16, BROWN, LIGHTGRAY)) {
                quitGame = true;
            }
            EndDrawing();
            if (quitGame) {
                appQuit = true;
            }
            break;
        
        }
            
        case GameState::PLAYING : {

            if (IsKeyPressed(KEY_ESCAPE)) {
                state = GameState::PAUSED;
                waitingBind = BindSlot::None;
                pauseOpenedThisFrame = true;
                EnableCursor();
                // Fall through to the pause draw so EndDrawing still runs
                // (Raylib polls input there). Same-frame Escape must not close.
            }

            if (state == GameState::PLAYING) {
            float dt = GetFrameTime();
            UpdatePlayerLook(yaw, pitch, settings.mouseSensitivity);
            player.ApplyInput(yaw, dt, settings);
            player.ApplyDebugInput();

            int playerChunkX = DivFloor((int)floorf(player.GetPosition().x), CHUNK_SIZE);
            int playerChunkZ = DivFloor((int)floorf(player.GetPosition().z), CHUNK_SIZE);
            UpdateWorldStreaming(world, playerChunkX, playerChunkZ, settings.renderDistance, lastPlayerChunkX, lastPlayerChunkZ);

            player.UpdatePhysics(world, dt);
            player.SyncCamera(camera, yaw, pitch);
            camera.fovy = settings.cameraFov;
            
            ChangeHotbarSlot(player);

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
                    SetBlock(world, placeX, placeY, placeZ, player.GetHeldItem());
            }

            AdvanceTime(dt, world);
            float sunBrightness = CalculateSunBrightness(world.manifest.timeOfDay);
            SetShaderValue(shader, sunBrightnessLocation, &sunBrightness, SHADER_UNIFORM_FLOAT);

            BeginDrawing();
                ClearBackground(CalculateSkyColor(world.manifest.timeOfDay));
                BeginMode3D(camera);
                    DrawWorld(world, mat);
                    
                    DrawBoundingBox(player.GetBounds(), GREEN);
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
                if (IsKeyPressed(KEY_GRAVE)) world.manifest.timeOfDay += 1000;
                DrawHUD(world, camera, showNoiseDebug);
                DrawHotbar(player, atlas);
            EndDrawing();

            break;
            }
            // Opened pause this frame: draw the paused overlay (falls through).
        }

        case GameState::PAUSED : {

            const bool wasCapturingBind = (waitingBind != BindSlot::None);
            HandleBindCapture(settings, waitingBind);

            bool saveAndQuit = false;
            bool resume = false;

            // Open-frame fallthrough still has IsKeyPressed(ESCAPE)==true.
            // HandleBindCapture already used Escape to cancel a remap.
            if (!pauseOpenedThisFrame && !wasCapturingBind && IsKeyPressed(KEY_ESCAPE)) {
                resume = true;
            }

            float sunBrightness = CalculateSunBrightness(world.manifest.timeOfDay);
            SetShaderValue(shader, sunBrightnessLocation, &sunBrightness, SHADER_UNIFORM_FLOAT);

            BeginDrawing();
                ClearBackground(CalculateSkyColor(world.manifest.timeOfDay));
                BeginMode3D(camera);
                    DrawWorld(world, mat);
                    DrawBoundingBox(player.GetBounds(), GREEN);
                EndMode3D();
                DrawHotbar(player, atlas);

                if (DrawPauseMenu(settings, waitingBind, saveAndQuit)) {
                    resume = true;
                }
            EndDrawing();

            camera.fovy = settings.cameraFov;

            if (resume) {
                waitingBind = BindSlot::None;
                state = GameState::PLAYING;
                DisableCursor();
                lastPlayerChunkX = INT_MIN;
            } else if (saveAndQuit) {
                SaveWorld(world, camera, CHUNK_PATH);
                LeaveWorld(world, lastPlayerChunkX, lastPlayerChunkZ);
                state = GameState::MENU;
                EnableCursor();
            }

            break;
        }
    }
    }
    
    if (state == GameState::PLAYING || state == GameState::PAUSED) {
        SaveWorld(world, camera, CHUNK_PATH);
        UnloadAllChunks(world);
    }
    UnloadShader(shader);
    UnloadTexture(atlas);
    CloseWindow();
}
