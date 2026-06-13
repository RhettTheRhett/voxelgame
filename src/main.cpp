#include "raylib.h"
#include "raymath.h"
#include "chunk.h"
#include "noise.h"
#include "world.h"
#include "raycast.h"
#include "block.h"
#include <cmath>

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


int main() {
    InitWindow(1080, 720, "Voxel Game");
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    float yaw         = -90.0f;
    float pitch       = 0.0f;
    float speed       = 15.0f;
    float sensitivity = 0.1f;
    float renderDistance = 12;
    bool showNoiseDebug = false;
    bool showChunkBorders = false; 

    Material mat = LoadMaterialDefault();

    Camera3D camera = {};
    camera.fovy       = 70.0f;
    camera.position   = {0, 64, 0};
    camera.target     = {0, 0, 0};
    camera.up         = {0, 1, 0};
    camera.projection = CAMERA_PERSPECTIVE;

    World world = {};
    world.seed             = GetRandomValue(0, 999999999);
    world.noiseScale       = 0.0044f;
    world.noiseOctaves     = 4;
    world.noisePersistence = 0.55f;

    SetNoiseSeed(world.seed);

    int lastPlayerChunkX = INT_MIN;
    int lastPlayerChunkZ = INT_MIN;

    DisableCursor();

    while (!WindowShouldClose()) {
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
                SetBlock(world, placeX, placeY, placeZ, Block::STONE);
        }
        

        BeginDrawing();
            ClearBackground(RAYWHITE);
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
    }

    CloseWindow();
    return 0;
}