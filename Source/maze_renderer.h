#ifndef MAZE_RENDERER_H
#define MAZE_RENDERER_H

#include "maze.h"
#include "arclib.h"

class MazeRenderer
{
private:
    Maze *maze = nullptr;
    Font raylibFont;

public:
    Color junctionColor = { 15, 255, 0, 255 };
    Color junctionFillColor = { 1, 14, 0, 255 };
    Color gridColor = { 13, 64, 0, 255 };
    Color tunnelColor = { 10, 68, 0, 255 };

    int tileSize = 16;
    int tunnelSize = 7;
    int fontSize = 20;

    MazeRenderer();
    void SetMaze(Maze *_maze);

    void DrawJunctionLabels();
    void DrawJunctions();
    void DrawTunnels();
    void DrawGrid();
    void DrawIdMap();
    void DrawTags();
    Rectangle GetJunctionRect(JunctionID id);
};

#endif