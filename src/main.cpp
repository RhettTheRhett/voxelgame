#include "raylib.h"
#include "raymath.h"
#include "chunk.h"



int main(){


    InitWindow(1080, 720, "Voxel Game");
    SetTargetFPS(200);

    float yaw   = -90.0f; // start facing -Z (into the scene)
    float pitch = 0.0f;
    float speed = 5.0f;
    float sensitivity = 0.1f;

    Material mat = LoadMaterialDefault();

    Camera3D camera = {};
    camera.fovy = 60.0f;
    camera.position = {0, 5, 10};
    camera.target = {0, 0, 0};
    camera.up = {0,1,0};
    camera.projection = CAMERA_PERSPECTIVE;

    Chunk newchunk = {};
    

    //loop through chunks
    //x
    for(int x = 0; x < CHUNK_SIZE; x++){
        //y
        for(int y = 0; y < CHUNK_SIZE; y++){
            //z
            for(int z = 0; z < CHUNK_SIZE; z++){
                if(y < 3){
                    newchunk.blocks[x][y][z] = 1; 
                    
                }else{
                    newchunk.blocks[x][y][z] = 0;
                }
            }
        }
    }
    
    Mesh chunkMesh = BuildChunkMesh(newchunk);

    int solidCount = 0;
    for(int x = 0; x < CHUNK_SIZE; x++)
        for(int y = 0; y < CHUNK_SIZE; y++)
            for(int z = 0; z < CHUNK_SIZE; z++)
                if(newchunk.blocks[x][y][z] == 1)
                    solidCount++;

    

    DisableCursor();

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
        

        camera.target = camera.position + forward;
        
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
            DrawGrid(20, 1.0f);
            DrawCube({0,0.5f,0}, 1, 1, 1, BLUE);
            DrawMesh(chunkMesh, mat, MatrixIdentity());
        EndMode3D();

        DrawText(TextFormat("dx: %.2f  dy: %.2f", delta.x, delta.y), 10, 10, 20, BLACK);
        DrawText(TextFormat("pitch: %.2f yaw: %.2f", pitch, yaw), 10, 35, 20, BLACK);
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 60, 20, BLACK);
        DrawText(TextFormat("x: %.2f  y: %.2f Z: %.2f", camera.position.x, camera.position.x, camera.position.y), 10, 85, 20, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}