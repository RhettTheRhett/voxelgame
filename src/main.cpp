#include "raylib.h"

int main() {
	InitWindow(800, 600, "Voxel Game");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("It works.", 350, 280, 20, DARKGRAY);
		EndDrawing();
	}

	CloseWindow();
	return 0;
}