#include "maze_renderer.h"
#include "arclib.h"

MazeRenderer::MazeRenderer()
{
    raylibFont = GetFontDefault();
}
void MazeRenderer::SetMaze(Maze *_maze)
{
        maze = _maze;
}

void MazeRenderer::DrawJunctionLabels() 
{
    vector<JunctionID> junctions = maze->GetJunctionList();
    for (JunctionID id: junctions){
        Junction &j = maze->GetJunction(id);
        Rectangle rect = GetJunctionRect(id);
        const char *text = j.name.c_str();

        Vector2 targetPosWorld = { rect.x - 5, rect.y - 5 };
        Vector2 tilePosWorld = { rect.x, rect.y };
        Vector2 targetPos = GetWorldToScreen2D(targetPosWorld, arcGlobal.camera);
        Vector2 tilePos = GetWorldToScreen2D(tilePosWorld, arcGlobal.camera);

        int textWidth = MeasureText(text, fontSize);
        Vector2 textPos = { targetPos.x - textWidth, targetPos.y - fontSize };
        
        float w = 2.0;
        textPos.y -= w;

        DrawLineZ(tilePos, targetPos, WHITE, w, 1.0);
        DrawLineZ(targetPos, { textPos.x - 3, targetPos.y }, WHITE, w, 1.0);
        DrawText(text, textPos.x, textPos.y, fontSize, WHITE);
    }
}
void MazeRenderer::DrawJunctions()
{
    vector<JunctionID> junctions = maze->GetJunctionList();
    for (JunctionID id: junctions){
        Junction &j = maze->GetJunction(id);
        Rectangle rect = GetJunctionRect(id);
        DrawRectangleRec(rect, junctionFillColor);
        DrawRectangleLinesZ(rect, 1.0, junctionColor, 0, 1);
    }
}
void MazeRenderer::DrawTunnels()
{
    vector<Tunnel> tunnels = maze->GetTunnelList();
    for(Tunnel t: tunnels){
        Coord coord1 = maze->GetJunctionCoord(t.from);
        Coord coord2 = maze->GetJunctionCoord(t.to);
        Vector2 pos1 = Vector2Scale({ coord1.x+0.5f, coord1.y+0.5f }, tileSize);
        Vector2 pos2 = Vector2Scale({ coord2.x+0.5f, coord2.y+0.5f }, tileSize);
        DrawLineZ(pos1, pos2, tunnelColor, tunnelSize, 0.5);
    }
}
void MazeRenderer::DrawGrid()
{
    float f = Clamp(1-0.3/arcGlobal.camera.zoom, 0, 1);
    if (f < 0.01)
        return;
    Color color = LerpColor(BLACK, gridColor, f);

    Rectangle worldRect = GetCameraWorldRect(arcGlobal.camera);
    int w = worldRect.width / tileSize;
    int h = worldRect.height / tileSize;
    
    Vector2 c = SnapGrid({ worldRect.x, worldRect.y }, tileSize);
    int cx = (int)c.x;
    int cy = (int)c.y;

    for (int x = 0; x < w+10; x++) {
        DrawLine(cx+x*tileSize, cy, cx+x*tileSize, cy+worldRect.height+tileSize, color);
    }

    for (int y = 0; y < h+10; y++) {
        DrawLine(cx, cy+y*tileSize, cx+worldRect.width+tileSize, cy+y*tileSize, color);
    }
}
void MazeRenderer::DrawIdMap()
{
    if (arcGlobal.camera.zoom < 2)
        return;
    Rectangle worldRect = GetCameraWorldRect(arcGlobal.camera);
    int tx = worldRect.x / tileSize;
    int ty = worldRect.y / tileSize;
    int w = worldRect.width / tileSize;
    int h = worldRect.height / tileSize;

    for (int x = 0; x < w+4; x++) {
        for (int y = 0; y < h+4; y++) {
            int gx = tx+x;
            int gy = ty+y;
            JunctionID id = maze->GetJunctionAt(gx, gy);

            const char *text = TextFormat("%d", id);
            int w = MeasureText(text, 1); 
            Vector2 screenPos = GetWorldToScreen2D({ gx*tileSize, gy*tileSize }, arcGlobal.camera);

            DrawRectangle(screenPos.x-2, screenPos.y, w+4, 10, junctionFillColor);
            DrawText(text, screenPos.x, screenPos.y, 1, DARKGRAY);
        }
    }
}
void MazeRenderer::DrawTags()
{
    if (arcGlobal.camera.zoom < 2)
        return;
    Rectangle worldRect = GetCameraWorldRect(arcGlobal.camera);
    int tx = worldRect.x / tileSize;
    int ty = worldRect.y / tileSize;
    int w = worldRect.width / tileSize;
    int h = worldRect.height / tileSize;
    int spacing = 15;

    for (int x = 0; x < w+4; x++) {
        for (int y = 0; y < h+4; y++) {
            int gx = tx+x;
            int gy = ty+y;
            
            vector<string> tags = maze->GetTagsAt(gx, gy);

            for (int i = 0; i < tags.size(); i++) {
                const char *text = tags[i].c_str();
                int w = MeasureText(text, 15); 
                Vector2 screenPos = GetWorldToScreen2D({ gx*tileSize, gy*tileSize }, arcGlobal.camera);
                screenPos.y += spacing*i;
                DrawRectangle(screenPos.x-2, screenPos.y, w+4, 15, junctionFillColor);
                DrawText(text, screenPos.x, screenPos.y, 15, BLUE);
            }
        }
    }
}

Rectangle MazeRenderer::GetJunctionRect(JunctionID id)
{
    JunctionRect r = maze->GetJunctionRect(id);
    Coord coord = maze->GetJunctionCoord(id);
    int width = r.bot.x - r.top.x;
    int height = r.bot.y - r.top.y;
    Rectangle rect = {
        (coord.x+r.top.x) * tileSize,
        (coord.y+r.top.y) * tileSize,
        width*tileSize, height*tileSize
    };
    return rect;
} 