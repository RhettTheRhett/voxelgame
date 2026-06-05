#include "raylib.h"

int main() {

	float fps = 60.0;

	InitWindow(800, 600, "Voxel Game");
	SetTargetFPS(fps);

	float xPos = 0.0f;

	while (!WindowShouldClose()) {

		float deltaTime = GetFrameTime();
		xPos += 100.0f * deltaTime;
		if(xPos > 800) {
			xPos = 0;
		 }

		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawRectangle(xPos, 280, 40, 40, DARKBLUE);
		DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, DARKGRAY);
		DrawText(TextFormat("DeltaTime: %.4f", deltaTime), 10, 35, 20, DARKGRAY);
		EndDrawing();

	}

	CloseWindow();
	return 0;
}