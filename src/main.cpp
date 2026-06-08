#include "raylib.h"
#include "raymath.h"
#include "chunk.h"
#include "noise.h"
#include "world.h"
#include <cmath>


int main(){


    InitWindow(1080, 720, "Voxel Game");
    SetTargetFPS(200);

    float yaw   = -90.0f; // start facing -Z (into the scene)
    float pitch = 0.0f;
    float speed = 15.0f;
    float sensitivity = 0.1f;

    Material mat = LoadMaterialDefault();

    Camera3D camera = {};
    camera.fovy = 60.0f;
    camera.position = {0, 5, 10};
    camera.target = {0, 0, 0};
    camera.up = {0,1,0};
    camera.projection = CAMERA_PERSPECTIVE;

    World world = {};
    world.seed             = GetRandomValue(0,999999999);
    world.noiseScale       = 0.0044f;
    world.noiseOctaves     = 4;
    world.noisePersistence = 0.55f;

    SetNoiseSeed(world.seed);
    //GenerateWorld(world, 3, 0, 0);

    bool showNoiseDebug = false;

    DisableCursor();

    // helper lambda to regenerate and rebuild
    auto regen = [&]() {
    SetNoiseSeed(world.seed);
    world.chunks.clear();
    GenerateWorld(world, 3, 0, 0);
    };

    while(!WindowShouldClose()){

        Vector2 delta = GetMouseDelta();

        yaw += delta.x * sensitivity;
        pitch += delta.y * sensitivity * -1;

        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        float pitchRad = pitch * DEG2RAD;
        float yawRad   = yaw   * DEG2RAD;

        Vector3 forward;
        forward.x = cosf(pitchRad) * cosf(yawRad);
        forward.y = sinf(pitchRad);
        forward.z = cosf(pitchRad) * sinf(yawRad);

        

        Vector3 moveForward = {};
        Vector3 moveSide = {};
        
        moveForward = {cosf(yawRad), 0, sinf(yawRad)};
        moveSide = {cosf(yawRad + 90.0f * DEG2RAD), 0, sinf(yawRad + 90.0f * DEG2RAD)};

        float deltaTime = GetFrameTime();

        if(IsKeyDown(KEY_W)) camera.position += moveForward * speed * deltaTime;
        if(IsKeyDown(KEY_S)) camera.position -= moveForward * speed * deltaTime;
        if(IsKeyDown(KEY_A)) camera.position -= moveSide * speed * deltaTime;
        if(IsKeyDown(KEY_D)) camera.position += moveSide * speed * deltaTime;

        if(IsKeyDown(KEY_SPACE)) camera.position.y += speed * deltaTime;
        if(IsKeyDown(KEY_LEFT_CONTROL)) camera.position.y -= speed * deltaTime;
        
        // Scale
        if (IsKeyPressed(KEY_UP))   { world.noiseScale *= 1.5f; regen(); }
        if (IsKeyPressed(KEY_DOWN)) { world.noiseScale /= 1.5f; regen(); }

        // Octaves
        if (IsKeyPressed(KEY_RIGHT)) { world.noiseOctaves = __min(world.noiseOctaves+1, 8); regen(); }
        if (IsKeyPressed(KEY_LEFT))  { world.noiseOctaves = __max(world.noiseOctaves-1, 1); regen(); }

        // Persistence
        if (IsKeyPressed(KEY_E)) { world.noisePersistence = __min(world.noisePersistence+0.05f, 0.95f); regen(); }
        if (IsKeyPressed(KEY_Q)) { world.noisePersistence = __max(world.noisePersistence-0.05f, 0.05f); regen(); }

        // Seed
        if (IsKeyPressed(KEY_N)) { world.seed++; regen(); }
        if (IsKeyPressed(KEY_B)) { world.seed--; regen(); }

        camera.target = camera.position + forward;

        int playerChunkX = (int)floor(camera.position.x / CHUNK_SIZE);
        int playerChunkZ = (int)floor(camera.position.z / CHUNK_SIZE);
        GenerateWorld(world, 12, playerChunkX, playerChunkZ);

        
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
            DrawGrid(20, 1.0f);
            DrawCube({0,0.5f,0}, 1, 1, 1, BLUE);
            DrawWorld(world, mat);
        EndMode3D();

        // in main.cpp, after EndMode3D() and before EndDrawing()
        if (IsKeyPressed(KEY_TAB)) showNoiseDebug = !showNoiseDebug;

        if (showNoiseDebug) {
            int previewSize = 256;
            for (int px = 0; px < previewSize; px++) {
                for (int py = 0; py < previewSize; py++) {
                    float wx = px * world.noiseScale;
                    float wy = py * world.noiseScale;
                    float n = FBm2D(wx, wy, world.noiseOctaves, world.noisePersistence);
                    float t = (n + 1.0f) / 2.0f;
                    unsigned char c = (unsigned char)(t * 255);
                    DrawPixel(px, py, {c, c, c, 255});
                }
            }
        }

        DrawText(TextFormat("dx: %.2f  dy: %.2f", delta.x, delta.y), 10, 10, 20, BLACK);
        DrawText(TextFormat("pitch: %.2f yaw: %.2f", pitch, yaw), 10, 35, 20, BLACK);
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 60, 20, BLACK);
        DrawText(TextFormat("x: %.2f  y: %.2f Z: %.2f", camera.position.x, camera.position.y, camera.position.z), 10, 85, 20, BLACK);

        DrawText(TextFormat("Scale: %.4f  [UP/DOWN]",    world.noiseScale),   10, 110, 20, DARKGREEN);
        DrawText(TextFormat("Octaves: %d  [LEFT/RIGHT]", world.noiseOctaves), 10, 135, 20, DARKGREEN);
        DrawText(TextFormat("Persist: %.2f  [Q/E]",      world.noisePersistence), 10, 160, 20, DARKGREEN);
        DrawText(TextFormat("Seed: %d  [B/N]", world.seed), 10, 185, 20, DARKGREEN);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}