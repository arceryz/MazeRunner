#include <format>
#include <iostream>
#include <fstream>

#define ARCLIB_IMPLEMENTATION
#include "arclib.h"

#include "editor.h"
#include "maze.h"
#include "rlImGui.h"

int main() {
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(144);
    InitWindow(1280, 720, "MazeRunner");
    rlImGuiSetup(true);
    ImGui::GetIO().IniFilename = NULL;

    Maze maze;
    maze.ImportJson("Mazes/test.json");
    MazeEditor editor(maze);

    while (!WindowShouldClose()) {
        editor.Update();
        DragCameraUpdate(0.1, 10, 0.5, 20, 400);

        BeginDrawing();
        rlImGuiBegin();
        
        editor.Draw();

        rlImGuiEnd();
        EndDrawing();
    }

    rlImGuiShutdown();
}