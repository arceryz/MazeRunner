#ifndef MAZE_GUI_H
#define MAZE_GUI_H

#include "arclib.h"
#include "maze.h"
#include "imgui.h"
#include <string>
#include <vector>

#define KEY_SELECT IsMouseButtonPressed(MOUSE_LEFT_BUTTON)
#define KEY_REMOVE IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)
#define KEY_TUNNEL IsKeyPressed(KEY_F)
#define KEY_EXPORT IsKeyPressed(KEY_X)
#define KEY_PRUNE IsKeyPressed(KEY_P)
#define MODKEY IsKeyDown(KEY_LEFT_SHIFT)
#define ALTKEY IsKeyDown(KEY_LEFT_ALT)

using namespace std;
namespace Gui = ImGui;

// This class is basically the entire editor.
class MazeEditor {
public:
    Maze &maze;

    float tileSize = 16.0f;
    int fontSize = 20;
    int tunnelSize = 7;
    Font raylibFont;
    Coord mouseCoord = {};
    Vector2 mousePos = {};
    Vector2 mouseWorld = {};

    float junctionColorArr[3];  Color junctionColor = { 15, 255, 0, 255 };
    float gridColorArr[3];      Color gridColor = { 13, 64, 0, 255 };
    float tunnelColorArr[3];    Color tunnelColor = { 10, 68, 0, 255 };
    float clearColorArr[3];     Color clearColor = { 1, 14, 0, 255 };

    JunctionID mouseJunction = 0;
    Tunnel mouseTunnel = {};
    Coord selectedCoord = {};
    bool hasSelectedCoord = false;
    JunctionID mainJunctionID = 0;
    JunctionID secondJunctionID = 0;

    Vector2 topCornerWorld = {};
    Vector2 botCornerWorld = {};
    Coord topCorner = {};
    Coord botCorner = {};
    bool topCornerGrabbed = false;
    bool botCornerGrabbed = false;
    Vector2 topCornerRel = {};
    Vector2 botCornerRel = {};

    string statusBarText;
    char nameBuf[128] = "";
    char tagBuf[128] = "";
    char mazeNameBuf[128] = "";
    int tagsItem;
    vector<string> tagsList;
    bool mazeHasFocus = false;
    bool editingCorners = false;

    bool showLabels = true;
    bool showJunctions = true;
    bool showTunnelLabels = true;
    bool showGrid = true;
    bool showSectors = true;
    bool showIdMap = false;
    bool showTags = true;

    MazeEditor(Maze &targetMaze): maze(targetMaze)
    {
        raylibFont = GetFontDefault();
        CenterHome();
        ColorToFloat3(junctionColor, junctionColorArr);
        ColorToFloat3(gridColor, gridColorArr);
        ColorToFloat3(clearColor, clearColorArr);
        ColorToFloat3(tunnelColor, tunnelColorArr);
    }

    //
    // Update methods.
    //
    void Update()
    {
        // Blackboard.
        mousePos = GetMousePosition();
        mouseWorld = GetScreenToWorld2D(mousePos, arcGlobal.camera);
        mouseCoord.x = (int)floorf(mouseWorld.x / tileSize);
        mouseCoord.y = (int)floorf(mouseWorld.y / tileSize);
        mouseJunction = maze.GetJunctionAt(mouseCoord.x, mouseCoord.y);
        mouseTunnel = maze.GetTunnelAt(mouseCoord.x, mouseCoord.y);
        mazeHasFocus = !Gui::GetIO().WantCaptureMouse;
        ConfigureMainJunction();
        SetStatusBar();

        // Selection stuff only when mouse is in the maze area.
        if (mazeHasFocus && !editingCorners) {
            if(MODKEY && KEY_SELECT && mouseJunction == 0) {
                ClearSelections();
                SelectCoord(mouseCoord);
            }
            if((!MODKEY || mainJunctionID == 0) && KEY_SELECT && mouseJunction > 0) {
                SetMainJunction(mouseJunction);
                SelectCoord(mouseCoord);
            }
            else if(MODKEY && KEY_SELECT && mouseJunction > 0 && mainJunctionID > 0 && mouseJunction != mainJunctionID) SetSecondJunction(mouseJunction);
            if(!MODKEY && KEY_SELECT && mouseJunction == 0) ClearSelections();
        }

        // Creation / Deletion
        if(ALTKEY && KEY_SELECT && mouseJunction == 0)      maze.AddJunction(mouseCoord.x, mouseCoord.y, "x");
        if(ALTKEY && KEY_REMOVE && mouseJunction > 0)       maze.RemoveJunction(mouseJunction);
        if(ALTKEY && KEY_REMOVE && mouseTunnel.from > 0)    maze.RemoveTunnel(mouseTunnel);
        if(KEY_TUNNEL && mainJunctionID > 0 && secondJunctionID > 0)  {
            maze.AddTunnel(mainJunctionID, secondJunctionID);
            ClearSelections();
        }

        // Utility
        if(MODKEY && KEY_EXPORT)    maze.ExportJson("Mazes/test.json");
        if(MODKEY && KEY_PRUNE)     maze.PruneJunctions();
    }
    void SetStatusBar()
    {
        const char *str = TextFormat(
            "Mouse=(%3.0f,%3.0f) [%d, %d] | "
            "Hover=%d, Tunnel=(%d, %d) | "
            "Main=(%d,%d)? %s | "
            "Sel j1=%d, j2=%d | "
            "Focus=%s Corners=%s | "
            "Zoom=%1.2f | "
            "Corners=[%d,%d],[%d,%d] | ",
            mousePos.x, mousePos.y, mouseCoord.x, mouseCoord.y,
            mouseJunction, mouseTunnel.from, mouseTunnel.to,
            selectedCoord.x, selectedCoord.y, hasSelectedCoord ? "Yes" : "No",
            mainJunctionID, secondJunctionID,
            mazeHasFocus ? "Yes" : "No", editingCorners ? "Yes" : "No",
            arcGlobal.camera.zoom,
            topCorner.x, topCorner.y, botCorner.x, botCorner.y);
        statusBarText = string(str);
    }
    void ConfigureMainJunction() 
    {
        if (mainJunctionID == 0)
            return;
        // Transfer all GUI parameters to the selected junction.
        // It is essentially "docked" at the GUI and the GUI unloads its cargo.
        Junction &j = maze.GetJunction(mainJunctionID);
        j.name = string(nameBuf);

        Coord coords = maze.GetJunctionCoord(j.id);
        topCorner =  { (int)floorf(topCornerWorld.x/tileSize+0.5)-coords.x, (int)floorf(topCornerWorld.y/tileSize+0.5)-coords.y };
        botCorner =  { (int)floorf(botCornerWorld.x/tileSize+0.5)-coords.x, (int)floorf(botCornerWorld.y/tileSize+0.5)-coords.y };
        if (!maze.SetJunctionRect(j.id, JunctionRect(topCorner, botCorner))) {
            JunctionRect r = maze.GetJunctionRect(j.id);
            topCornerWorld = {  (r.top.x+coords.x) * tileSize, (r.top.y+coords.y) * tileSize };
            botCornerWorld = {  (r.bot.x+coords.x) * tileSize, (r.bot.y+coords.y) * tileSize };
        }
    }
    void SelectCoord(Coord coord)
    {
        tagsList = maze.GetTagsAt(coord.x, coord.y);
        selectedCoord = coord;
        hasSelectedCoord = true;
    }
    void ConfigureTags()
    {
        if (!hasSelectedCoord)
            return;

        // Trim whitespace.
        for (int i = 0; i < tagsList.size(); i++) {
            string &s = tagsList[i];
            s.erase(s.find_last_not_of(' ')+1);
            s.erase(0, s.find_first_not_of(' '));
            
            // Erase empty trash.
            if (s.size() == 0)
                tagsList.erase(tagsList.begin()+i);
        }
        maze.SetTagsAt(selectedCoord.x, selectedCoord.y, tagsList);
    }
    void SetMainJunction(JunctionID id)
    {
        mainJunctionID = id;
        Junction &j = maze.GetJunction(id);
        strncpy(nameBuf, j.name.c_str(), 128);

        Coord coords = maze.GetJunctionCoord(id);
        JunctionRect r = maze.GetJunctionRect(j.id);
        topCornerWorld = {  (r.top.x+coords.x) * tileSize, (r.top.y+coords.y) * tileSize };
        botCornerWorld = {  (r.bot.x+coords.x) * tileSize, (r.bot.y+coords.y) * tileSize };
    }
    void SetSecondJunction(JunctionID id)
    {
        secondJunctionID = id;
    }
    void ClearSelections()
    {
        mainJunctionID = 0;
        secondJunctionID = 0;
        topCorner = {};
        botCorner = {};
        topCornerWorld = {};
        botCornerWorld = {};
        hasSelectedCoord = false;
        selectedCoord = {};
        hasSelectedCoord = false;
        tagsList.clear();
    }
    void CenterHome()
    {
        vector<string> options = { "H", "Home", "Haven", "Start", "Center", "S", "C", "G" };

        // Center to the home junction.
        JunctionID homeId = 0;
        for (string s: options) {
            homeId = maze.FindJunction(s);
            if (homeId > 0)
                break;
        }
        if (homeId > 0) {
            Coord homeCoord = maze.GetJunctionCoord(homeId);
            arcGlobal.camera.target = { (float)homeCoord.x * tileSize, (float)homeCoord.y * tileSize };
            arcGlobal.camera.offset.x = GetScreenWidth() / 2;
            arcGlobal.camera.offset.y = GetScreenHeight() / 2;
        }
    }

    //
    // Drawing methods.
    //
    void Draw()
    {
        BeginMode2D(arcGlobal.camera);
        ClearBackground(clearColor);

        if (showGrid) DrawGrid();
        DrawTunnels();
        if (showJunctions) DrawJunctions();
        DrawSelectionHighlight();

        // Junction corner gizmos.
        editingCorners = false;
        if (mainJunctionID > 0) {
            editingCorners |= PositionGizmo(topCornerWorld, topCornerRel, topCornerGrabbed, 3, LIGHTGRAY, tileSize);
            editingCorners |= PositionGizmo(botCornerWorld, botCornerRel, botCornerGrabbed, 3, GRAY, tileSize);
        }
        if (mazeHasFocus && !editingCorners) DrawTileHighlight();

        EndMode2D();

        if (showTags) DrawTags();
        if (showIdMap) DrawIdMap();
        if (showLabels) DrawJunctionLabels();
        DrawFPS(5, GetScreenHeight() - 15);


        Gui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Once);
        Gui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_Once);
        Gui::Begin("Control Panel");
        Gui::PushItemWidth(-130);
        DrawGuiMenuBar();
        DrawGuiEditorSettings();
        DrawGuiMazeSettings();
        DrawGuiInspector();
        Gui::PopItemWidth();
        Gui::End();
    }
    void DrawJunctionLabels() 
    {
        vector<JunctionID> junctions = maze.GetJunctionList();
        for (JunctionID id: junctions){
            Junction &j = maze.GetJunction(id);
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
    void DrawJunctions()
    {
        vector<JunctionID> junctions = maze.GetJunctionList();
        for (JunctionID id: junctions){
            Junction &j = maze.GetJunction(id);
            Rectangle rect = GetJunctionRect(id);
            DrawRectangleRec(rect, clearColor);
            DrawRectangleLinesZ(rect, 1.0, junctionColor, 0, 1);
        }
    }
    void DrawTunnels()
    {
        vector<Tunnel> tunnels = maze.GetTunnelList();
        for(Tunnel t: tunnels){
            Coord coord1 = maze.GetJunctionCoord(t.from);
            Coord coord2 = maze.GetJunctionCoord(t.to);
            Vector2 pos1 = Vector2Scale({ coord1.x+0.5f, coord1.y+0.5f }, tileSize);
            Vector2 pos2 = Vector2Scale({ coord2.x+0.5f, coord2.y+0.5f }, tileSize);
            DrawLineZ(pos1, pos2, tunnelColor, tunnelSize, 0.5);
        }
    }
    void DrawGrid()
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
    void DrawIdMap()
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
                JunctionID id = maze.GetJunctionAt(gx, gy);

                const char *text = TextFormat("%d", id);
                int w = MeasureText(text, 1); 
                Vector2 screenPos = GetWorldToScreen2D({ gx*tileSize, gy*tileSize }, arcGlobal.camera);

                DrawRectangle(screenPos.x-2, screenPos.y, w+4, 10, clearColor);
                DrawText(text, screenPos.x, screenPos.y, 1, DARKGRAY);
            }
        }
    }
    void DrawTags()
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
                
                vector<string> tags = maze.GetTagsAt(gx, gy);

                for (int i = 0; i < tags.size(); i++) {
                    const char *text = tags[i].c_str();
                    int w = MeasureText(text, 15); 
                    Vector2 screenPos = GetWorldToScreen2D({ gx*tileSize, gy*tileSize }, arcGlobal.camera);
                    screenPos.y += spacing*i;
                    DrawRectangle(screenPos.x-2, screenPos.y, w+4, 15, clearColor);
                    DrawText(text, screenPos.x, screenPos.y, 15, BLUE);
                }
            }
        }
    }
    void DrawTileHighlight()
    {
        Rectangle rect = {
            (float)mouseCoord.x * tileSize,
            (float)mouseCoord.y * tileSize,
            tileSize, tileSize
        };
        DrawRectangleLinesZ(rect, 0.5, WHITE, 3*fabsf(sin(GetTime()*8)));
    }
    void DrawSelectionHighlight()
    {
        if (mainJunctionID > 0) {
            DrawRectangleLinesZ(GetJunctionRect(mainJunctionID), 1, MAGENTA, 2);
        } else if (hasSelectedCoord) {
            Rectangle r = { selectedCoord.x*tileSize, selectedCoord.y*tileSize, tileSize, tileSize };
            DrawRectangleLinesZ(r, 1, MAGENTA, 2);
        } 
        if (secondJunctionID > 0) {
            DrawRectangleLinesZ(GetJunctionRect(secondJunctionID), 1, GREEN, 3);
        }
    }
    
    //
    // Gui drawing.
    //
    void DrawGuiEditorSettings() 
    {
        if (Gui::TreeNode("Editor")) {
            Gui::SliderInt("Font Size", &fontSize, 10, 30);
            Gui::SliderInt("Tunnel Size", &tunnelSize, 1, tileSize);

            if (Gui::TreeNode("Colors")) {
                Gui::ColorEdit3("Junction", junctionColorArr);
                Gui::ColorEdit3("Grid", gridColorArr);
                Gui::ColorEdit3("Tunnel", tunnelColorArr);
                Gui::ColorEdit3("Clear", clearColorArr);
                junctionColor = ColorFromNormalized3(junctionColorArr);
                gridColor = ColorFromNormalized3(gridColorArr);
                clearColor = ColorFromNormalized3(clearColorArr);
                tunnelColor = ColorFromNormalized3(tunnelColorArr);
                Gui::TreePop();
            }
            Gui::TreePop();
            Gui::Spacing();
        }
    }
    void DrawGuiMazeSettings()
    {
        if (Gui::TreeNode("Maze")) {
            Gui::InputText("Maze Name", mazeNameBuf, IM_ARRAYSIZE(mazeNameBuf));

            Gui::Text("View Toggles");
            if (Gui::BeginTable("Split", 3)) {
                Gui::TableNextColumn(); Gui::Checkbox("Junctions", &showJunctions);
                Gui::TableNextColumn(); Gui::Checkbox("Grid", &showGrid);
                Gui::TableNextColumn(); Gui::Checkbox("Tags", &showTags);

                Gui::TableNextColumn(); Gui::Checkbox("Sectors", &showSectors);
                Gui::TableNextColumn(); Gui::Checkbox("Tunnel Labels", &showTunnelLabels);
                Gui::TableNextColumn(); Gui::Checkbox("ID map", &showIdMap);
                Gui::EndTable();
            }
            
            Gui::TreePop();
            Gui::Spacing();
        }
    }
    void DrawGuiInspector()
    {
        if (Gui::TreeNode("Inspector")){
            if (mainJunctionID == 0 && !hasSelectedCoord) {
                Gui::BulletText("Select first junction with ");
                Gui::SameLine(); Gui::TextColored(ImVec4(0, 1, 0, 1), "LMB");

                Gui::BulletText("Select second junction with ");
                Gui::SameLine(); Gui::TextColored(ImVec4(0, 1, 0, 1), "Shift+LMB");

                Gui::BulletText("Select non-junction with ");
                Gui::SameLine(); Gui::TextColored(ImVec4(0, 1, 0, 1), "Shift+LMB");
            }
            if (mainJunctionID > 0) 
                Gui::InputText("Junction Name", nameBuf, IM_ARRAYSIZE(nameBuf));
            // The two buttons.
            if (hasSelectedCoord) {
                Gui::Text(TextFormat("Tag Position (%d, %d)", selectedCoord.x, selectedCoord.y));
                Gui::InputText("Tag", tagBuf, IM_ARRAYSIZE(tagBuf));
                Gui::SameLine();
                if (Gui::Button("Add")) {
                    tagsList.push_back(string(tagBuf));
                    strcpy(tagBuf, "");
                    ConfigureTags();
                }
                Gui::SameLine();
                if (Gui::Button("Remove")) {
                    tagsList.erase(tagsList.begin()+tagsItem);
                    ConfigureTags();
                }

                const char *tagsPtr[tagsList.size()];
                for (int i = 0; i < tagsList.size(); i++) {
                    tagsPtr[i] = tagsList[i].c_str();
                }
                Gui::ListBox("", &tagsItem, tagsPtr, tagsList.size());
            }
            Gui::TreePop();
        }
    }
    void DrawGuiMenuBar()
    {
        // The top bar.
         if (Gui::BeginMainMenuBar()) {
            if (Gui::BeginMenu("File")) {
                if (Gui::MenuItem("New")) {}
                if (Gui::MenuItem("Save", "Ctrl+S")) {}
                if (Gui::MenuItem("Load")) {}
                Gui::EndMenu();
            }
            Gui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3, 0.3, 0.3, 1));    
            Gui::Text(statusBarText.c_str());
            Gui::PopStyleColor();
            Gui::EndMainMenuBar();
        }
    }
    
    //
    // Utility methods.
    //
    Rectangle GetJunctionRect(JunctionID id)
    {
        JunctionRect r = maze.GetJunctionRect(id);
        Coord coord = maze.GetJunctionCoord(id);
        int width = r.bot.x - r.top.x;
        int height = r.bot.y - r.top.y;
        Rectangle rect = {
            (coord.x+r.top.x) * tileSize,
            (coord.y+r.top.y) * tileSize,
            width*tileSize, height*tileSize
        };
        return rect;
    } 
};

#endif
