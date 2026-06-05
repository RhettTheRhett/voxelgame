#include "raylib.h"

struct Vec2 {
    float x;
    float y;
};

struct Player {
    Vec2 position;
    Vec2 velocity;
    float speed;
    int width;
    int height;
};

void UpdatePlayer(Player& player, float deltaTime)
{
    // YOUR CODE HERE
	player.velocity.x= 0;
	player.velocity.y = 0;
    // handle WASD input
	if(IsKeyDown(KEY_W)) player.velocity.y -= player.speed * deltaTime;
	if(IsKeyDown(KEY_S)) player.velocity.y += player.speed * deltaTime;
	if(IsKeyDown(KEY_A)) player.velocity.x -= player.speed * deltaTime;
	if(IsKeyDown(KEY_D)) player.velocity.x += player.speed * deltaTime;
    // apply velocity
	player.position.x += player.velocity.x;
	player.position.y += player.velocity.y;
    // clamp to screen bounds (fix the jitter problem you identified)
	// clamp x
	if (player.position.x < 0) player.position.x = 0;
	if (player.position.x + player.width > GetScreenWidth()) player.position.x = GetScreenWidth() - player.width;
	// clamp y
	if (player.position.y < 0) player.position.y = 0;
	if (player.position.y + player.height > GetScreenHeight()) player.position.y = GetScreenHeight() - player.height;
	
	
	
}

void DrawPlayer(const Player& player)
{
	DrawRectangle(player.position.x, player.position.y, player.width, player.height, DARKBLUE);
}

int main()
{
    InitWindow(800, 600, "Voxel Game");
    SetTargetFPS(60);

    Player player;
	player.position = {100, 100};
	player.velocity = {0, 0};
	player.speed = 200.0f;
	player.width = 40;
	player.height = 40;
	
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        UpdatePlayer(player, deltaTime);

        BeginDrawing();
            ClearBackground(RAYWHITE);
			DrawPlayer(player);
			DrawText(TextFormat("X: %.1f Y: %.1f", player.position.x, player.position.y), 10, 10, 20, DARKGRAY);
			DrawText(TextFormat("X: %.1f Y: %.1f", player.velocity.x, player.velocity.y), 10, 35, 20, DARKGRAY);
			DrawText(TextFormat("SPEED: %.1f", player.speed), 10, 60, 20, DARKGRAY);
            
        EndDrawing();
    }

    CloseWindow();
    return 0;
}