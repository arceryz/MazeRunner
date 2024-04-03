/* 
 * Arclib is a raylib toolset for my personal projects.
 * @Arceryz.
 */

#ifndef ARCLIB_H
#define ARCLIB_H 

#include <raylib.h>
#include <raygui.h>
#include <raymath.h>
#include <vector>
#include <string>
using namespace std;

struct ArcGlobal {
    Camera2D camera;
    float zoomTarget;
};

extern ArcGlobal arcGlobal;

#define CYAN (Color){71, 224, 230, 255}
#define DEEPBLUE (Color){0, 30, 130, 255}

enum AnchorMode {
    ANCHOR_TOP_LEFT,
    ANCHOR_TOP_RIGHT,
    ANCHOR_BOT_LEFT,
    ANCHOR_BOT_RIGHT,
    ANCHOR_MID_RIGHT,
    ANCHOR_MID_LEFT,
};
Rectangle SRect(AnchorMode mode, float xoff, float yoff, float width, float height);
Rectangle ARect(Rectangle relative, AnchorMode mode, float xoff, float yoff, float width, float height);

void DragCameraUpdate(float zoomMin=0.1f, float zoomMax=2.0f, float zoomSpeed=0.12f, float lerpSpeed=1, float moveSpeed=0);
void DrawRectangleLinesZ(Rectangle rect, float lineWidth, Color color, float offset = 0, float zoomLOD=2);
void DrawLineZ(Vector2 from, Vector2 to, Color color, float thickness, float zoomLOD=2);
Rectangle GetCameraWorldRect(Camera2D camera);
Vector2 SnapGrid(Vector2 vec, float tileSize, float offset=0);
bool PositionGizmo(Vector2 &position, Vector2 &anchor, bool &grabbed, float size, Color baseColor, Color hoverColor, float tileSize=-1.0);
Color LerpColor(Color from, Color to, float factor);
Color ColorFromNormalized3(float arr[3]);
void ColorToFloat3(Color col, float arr[3]);
void StringsToPointers(vector<string> &vec, const char **pointers, int n);

#endif

// Here start the implementation.
#ifdef ARCLIB_IMPLEMENTATION
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

ArcGlobal arcGlobal = {
    { {0, 0}, {0, 0}, 0.0, 1.0},
    1.0
};

Rectangle SRect(AnchorMode mode, float xoff, float yoff, float width, float height) {
    return ARect({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }, 
        mode, xoff, yoff, width, height);
}
Rectangle ARect(Rectangle parent, AnchorMode mode, float xoff, float yoff, float width, float height) {
    float x = parent.x;
    float y = parent.y;
    float w = parent.width;
    float h = parent.height;

    Vector2 anchors[] = {
        [ANCHOR_TOP_LEFT] = { x, y },
        [ANCHOR_TOP_RIGHT] = { x+w, y },
        [ANCHOR_BOT_LEFT] = { x, y+h },
        [ANCHOR_BOT_RIGHT] = { x+w, y+h } ,
        [ANCHOR_MID_RIGHT] = { x+w, y+h/2 },
        [ANCHOR_MID_LEFT] = { x, y+h/2}
    };

    Vector2 anchor = anchors[mode];
    Rectangle rect = { anchor.x+xoff, anchor.y+yoff, 
        width >= 0 ? width: parent.width, 
        height >= 0 ? height: parent.height };
    return rect;
}

void DragCameraUpdate(float zoomMin, float zoomMax, float zoomSpeed, float lerpSpeed, float moveSpeed) {
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = Vector2Scale(GetMouseDelta(), 1.0f / arcGlobal.camera.zoom);
        arcGlobal.camera.target = Vector2Subtract(arcGlobal.camera.target, delta);
    }

    float zoom = arcGlobal.camera.zoom;
    float dt = GetFrameTime();
    arcGlobal.camera.target.y -= dt*moveSpeed/zoom * IsKeyDown(KEY_UP);
    arcGlobal.camera.target.y += dt*moveSpeed/zoom * IsKeyDown(KEY_DOWN);
    arcGlobal.camera.target.x += dt*moveSpeed/zoom * IsKeyDown(KEY_RIGHT);
    arcGlobal.camera.target.x -= dt*moveSpeed/zoom * IsKeyDown(KEY_LEFT);

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), arcGlobal.camera);
        // We first set the target to the mouse for proper zooming.
        arcGlobal.camera.target = mouseWorldPos;
        // Now set the mouse position (=distance from top left) as offset
        // to restore the original view. Scrolling is now center around the mouse.
        arcGlobal.camera.offset = GetMousePosition();

        arcGlobal.zoomTarget = Clamp(arcGlobal.camera.zoom * (1+wheel*zoomSpeed), zoomMin, zoomMax);
    }

    arcGlobal.camera.zoom = Lerp(arcGlobal.camera.zoom, arcGlobal.zoomTarget, lerpSpeed*dt);
}
void DrawRectangleLinesZ(Rectangle rect, float lineWidth, Color color, float offset, float zoomLOD)
{
    // Add offset.
    rect.x -= offset;
    rect.y -= offset;
    rect.width += 2*offset;
    rect.height += 2*offset;
    
    if (arcGlobal.camera.zoom < zoomLOD) {
        DrawLine(rect.x, rect.y, rect.x+rect.width, rect.y, color); // Horizontal top.
        DrawLine(rect.x+rect.width, rect.y+rect.height, rect.x, rect.y+rect.height, color); // Horizontal bot.
        DrawLine(rect.x, rect.y+rect.height, rect.x, rect.y, color); // Vertical left.
        DrawLine(rect.x+rect.width, rect.y, rect.x+rect.width, rect.y+rect.height, color); // Vertical right.
    } else {
        DrawRectangleLinesEx(rect, lineWidth, color);
    }
}
void DrawLineZ(Vector2 from, Vector2 to, Color color, float thickness, float zoomLOD)
{
    if (arcGlobal.camera.zoom < zoomLOD) {
        DrawLineV(from, to, color);
    } else {
        DrawLineEx(from, to, thickness, color);
    }
}
Rectangle GetCameraWorldRect(Camera2D camera)
{
    Vector2 worldA = GetScreenToWorld2D({ 0, 0 }, camera);
    Vector2 worldB = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, camera);
    Rectangle rect = { worldA.x, worldA.y, worldB.x-worldA.x, worldB.y-worldA.y};
    return rect;
}
Vector2 SnapGrid(Vector2 vec, float tileSize, float offset)
{
    return { floorf(vec.x/ tileSize + offset) * tileSize, floorf(vec.y / tileSize + offset) * tileSize };
}
bool PositionGizmo(Vector2 &position, Vector2 &anchor, bool &grabbed, float size, Color baseColor, Color hoverColor, float tileSize)
{
    Vector2 circlePos = tileSize > 0 ? SnapGrid(position, tileSize, 0.5) : position;
    Color color = baseColor;
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), arcGlobal.camera);

    Vector2 d = Vector2Subtract(position, mouseWorld);
    bool isHovering = d.x*d.x + d.y*d.y < size*size;

    if (grabbed) {
        position = Vector2Add(anchor, mouseWorld);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))   
            grabbed = false;
        isHovering = true;
        color = hoverColor;
    } 
    else {
        if (isHovering) {
            color = hoverColor;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                grabbed = true;
                anchor = Vector2Subtract(position, mouseWorld);
            }
        }
        if (tileSize > 0)
            position = SnapGrid(position, tileSize, 0.5);
    }
    DrawCircleV(position, 2, color);
    DrawRing(circlePos, size, size+1, 0, 360, 32, color);
    return isHovering;
}

Color LerpColor(Color from, Color to, float factor)
{
    return (Color){
        (unsigned char)(from.r + factor * (to.r - from.r)),
        (unsigned char)(from.g + factor * (to.g - from.g)),
        (unsigned char)(from.b + factor * (to.b - from.b)),
        (unsigned char)(from.a + factor * (to.a - from.a)),
    };
}
Color ColorFromNormalized3(float arr[3])
{
    return { 
        (unsigned char)(arr[0]*255), 
        (unsigned char)(arr[1]*255), 
        (unsigned char)(arr[2]*255), 
        255 
    };
}
void ColorToFloat3(Color col, float arr[3])

{
    arr[0] = ((float)col.r)/255.0;
    arr[1] = ((float)col.g)/255.0;
    arr[2] = ((float)col.b)/255.0;
}
void StringsToPointers(vector<string> &vec, const char **pointers, int n)
{
    for (int i = 0; i < vec.size() && i < n; i++) {
        pointers[i] = vec[i].c_str();
    }
}

#undef ARCLIB_IMPLEMENTATION
#endif