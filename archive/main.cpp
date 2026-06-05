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

	/*// clamp x
	if (player.position.x < 0) player.position.x = 0;
	if (player.position.x + player.width > GetScreenWidth()) player.position.x = GetScreenWidth() - player.width;
	// clamp y
	if (player.position.y < 0) player.position.y = 0;
	if (player.position.y + player.height > GetScreenHeight()) player.position.y = GetScreenHeight() - player.height;
	*/
	
	
	
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
    player.position = {400, 300};
    player.velocity = {0, 0};
    player.speed = 200.0f;
    player.width = 40;
    player.height = 40;

    Camera2D camera = {0};
    // YOUR CODE: set camera offset to center of screen
	camera.offset.x = GetScreenWidth() / 2;
	camera.offset.y = GetScreenHeight() / 2;

    // YOUR CODE: set camera zoom to 1.0f
	camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        UpdatePlayer(player, deltaTime);

        // YOUR CODE: update camera target to follow player
		camera.target = { player.position.x + player.width / 2.0f, player.position.y + player.height / 2.0f};

		float wheel = GetMouseWheelMove();
		if (wheel != 0)
			camera.zoom += wheel * 0.1f;

		if (camera.zoom < 0.1f) camera.zoom = 0.1f;
		if (camera.zoom > 5.0f) camera.zoom = 5.0f;

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode2D(camera);
                // everything drawn here is in WORLD SPACE
                DrawPlayer(player);
                // draw world origin marker
				DrawCircle(0, 0, 8, RED);

				// draw some world space reference points
				for (int x = -500; x <= 500; x += 100)
					DrawLine(x, -500, x, 500, LIGHTGRAY);
				for (int y = -500; y <= 500; y += 100)
					DrawLine(-500, y, 500, y, LIGHTGRAY);
            EndMode2D();

            // everything drawn here is in SCREEN SPACE
            DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, DARKGRAY);
            DrawText(TextFormat("World Pos: %.0f %.0f",
                player.position.x, player.position.y), 10, 35, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}