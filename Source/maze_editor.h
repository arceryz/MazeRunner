#ifndef MAZE_GUI_H
#define MAZE_GUI_H

#include <imgui.h>
#include <string>
#include <vector>
#include "arclib.h"
#include "maze.h"
#include "file_dialog.h"
#include "maze_renderer.h"

using namespace std;


#define KEY_SELECT IsMouseButtonPressed(MOUSE_LEFT_BUTTON)
#define KEY_REMOVE IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)
#define KEY_TUNNEL IsKeyPressed(KEY_F)
#define KEY_EXPORT IsKeyPressed(KEY_X)
#define KEY_PRUNE IsKeyPressed(KEY_P)
#define MODKEY IsKeyDown(KEY_LEFT_SHIFT)
#define ALTKEY IsKeyDown(KEY_LEFT_ALT)

using namespace std;
namespace Gui = ImGui;

// TODO: Clean up this class with states.
// TODO: Split file loading from Maze class to editor.

// This class is basically the entire editor.
class MazeEditor {
public:
    Maze maze;
    MazeRenderer mazeRenderer;
    float tileSize = 16;

    FileDialog fileDialog;
    fs::path filePath = "";
    
    Coord mouseCoord = {};
    Vector2 mousePos = {};
    Vector2 mouseWorld = {};

    Color clearColor = { 1, 14, 0, 255 };
    float junctionColorArr[3]; 
    float gridColorArr[3];      
    float tunnelColorArr[3];    
    float clearColorArr[3];     

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
    bool loadOnFile = false;

    bool showLabels = true;
    bool showJunctions = true;
    bool showTunnelLabels = true;
    bool showGrid = true;
    bool showSectors = true;
    bool showIdMap = false;
    bool showTags = true;

    MazeEditor()
    {
        CenterHome();
        ColorToFloat3(mazeRenderer.junctionColor, junctionColorArr);
        ColorToFloat3(mazeRenderer.gridColor, gridColorArr);
        ColorToFloat3(clearColor, clearColorArr);
        mazeRenderer.junctionFillColor = clearColor;
        ColorToFloat3(mazeRenderer.tunnelColor, tunnelColorArr);
        strcpy(mazeNameBuf, maze.name.c_str());
        mazeRenderer.SetMaze(&maze);
        LoadMaze("Examples/test.json");
    }

    //
    // Update methods.
    //
    void Update()
    {
        // Blackboard.
        mousePos = GetMousePosition();
        mouseWorld = GetScreenToWorld2D(mousePos, arcGlobal.camera);
        mouseCoord.x = (int)floorf(mouseWorld.x / mazeRenderer.tileSize);
        mouseCoord.y = (int)floorf(mouseWorld.y / mazeRenderer.tileSize);
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
    
    // 
    // Selection.
    //
    void SetMainJunction(JunctionID id)
    {
        mainJunctionID = id;
        Junction &j = maze.GetJunction(id);
        strncpy(nameBuf, j.name.c_str(), 128);

        Coord coords = maze.GetJunctionCoord(id);
        JunctionRect r = maze.GetJunctionRect(j.id);
        float tileSize = mazeRenderer.tileSize;
        topCornerWorld = {  (r.top.x+coords.x) * tileSize, (r.top.y+coords.y) * tileSize };
        botCornerWorld = {  (r.bot.x+coords.x) * tileSize, (r.bot.y+coords.y) * tileSize };
    }
    void SetSecondJunction(JunctionID id)
    {
        secondJunctionID = id;
    }
    void SelectCoord(Coord coord)
    {
        tagsList = maze.GetTagsAt(coord.x, coord.y);
        selectedCoord = coord;
        hasSelectedCoord = true;
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
            float tileSize = mazeRenderer.tileSize;
            Coord homeCoord = maze.GetJunctionCoord(homeId);
            arcGlobal.camera.target = { (float)homeCoord.x * tileSize, (float)homeCoord.y * tileSize };
            arcGlobal.camera.offset.x = GetScreenWidth() / 2;
            arcGlobal.camera.offset.y = GetScreenHeight() / 2;
        }
    }
    
    //
    // IO Methods.
    //
    void SaveMaze(fs::path path)
    {
        if (maze.ExportJson(path)) {
            filePath = path;
            cout << "Saved " << filePath << endl;
        } else {
            cout << "Invalid file " << filePath << endl;
        }
    }
    void LoadMaze(fs::path path)
    {
        if (maze.ImportJson(path)) {
            filePath = path;
            cout << "Loaded " << filePath << endl;
        } else {
            cout << "Invalid file " << filePath << endl;
        }
    }
    void NewMaze()
    {
        maze = Maze();
        filePath = "";
        cout << "New Maze" << endl;
    }
    bool HasValidFile()
    {
        ifstream stream(filePath);
        return stream.good();
    }

    //
    // Drawing methods.
    //
    void Draw()
    {
        BeginMode2D(arcGlobal.camera);
        ClearBackground(clearColor);

        if (showGrid) mazeRenderer.DrawGrid();
        mazeRenderer.DrawTunnels();
        if (showJunctions) mazeRenderer.DrawJunctions();
        DrawSelectionHighlight();

        // Junction corner gizmos.
        editingCorners = false;
        if (mainJunctionID > 0) {
            editingCorners |= PositionGizmo(topCornerWorld, topCornerRel, topCornerGrabbed, 3, GRAY, LIGHTGRAY, tileSize);
            editingCorners |= PositionGizmo(botCornerWorld, botCornerRel, botCornerGrabbed, 3, GRAY, LIGHTGRAY, tileSize);
        }
        if (mazeHasFocus && !editingCorners) DrawTileHighlight();

        EndMode2D();

        if (showTags) mazeRenderer.DrawTags();
        if (showIdMap) mazeRenderer.DrawIdMap();
        if (showLabels) mazeRenderer.DrawJunctionLabels();
        DrawFPS(5, GetScreenHeight() - 15);

        Gui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Once);
        Gui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_Once);
        Gui::Begin("Control Panel");
        Gui::PushItemWidth(-130);
        DrawGuiMenuBar();
        DrawGuiEditorSettings();
        DrawGuiMazeSettings();
        DrawGuiInspector();
        if (fileDialog.Update()) {
            if (loadOnFile) {
                LoadMaze(fileDialog.targetPath);
            } else {
                SaveMaze(fileDialog.targetPath);
            }
        }
        Gui::PopItemWidth();
        Gui::End();
    }
    void DrawTileHighlight()
    {
        float tileSize = mazeRenderer.tileSize;
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
            DrawRectangleLinesZ(mazeRenderer.GetJunctionRect(mainJunctionID), 1, MAGENTA, 2);
        } else if (hasSelectedCoord) {
            Rectangle r = { selectedCoord.x*tileSize, selectedCoord.y*tileSize, tileSize, tileSize };
            DrawRectangleLinesZ(r, 1, MAGENTA, 2);
        } 
        if (secondJunctionID > 0) {
            DrawRectangleLinesZ(mazeRenderer.GetJunctionRect(secondJunctionID), 1, GREEN, 3);
        }
    }
    void DrawGuiEditorSettings() 
    {
        if (Gui::TreeNode("Editor")) {
            Gui::SliderInt("Font Size", &mazeRenderer.fontSize, 10, 30);
            Gui::SliderInt("Tunnel Size", &mazeRenderer.tunnelSize, 1, tileSize);

            if (Gui::TreeNode("Colors")) {
                Gui::ColorEdit3("Junction", junctionColorArr);
                Gui::ColorEdit3("Grid", gridColorArr);
                Gui::ColorEdit3("Tunnel", tunnelColorArr);
                Gui::ColorEdit3("Clear", clearColorArr);
                mazeRenderer.junctionColor = ColorFromNormalized3(junctionColorArr);
                mazeRenderer.gridColor = ColorFromNormalized3(gridColorArr);
                mazeRenderer.tunnelColor = ColorFromNormalized3(tunnelColorArr);
                clearColor = ColorFromNormalized3(clearColorArr);
                mazeRenderer.junctionFillColor = clearColor;
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
            maze.name = string(mazeNameBuf);

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
                Gui::Text("Tag Position (%d, %d)", selectedCoord.x, selectedCoord.y);
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
                if (Gui::MenuItem("New")) {
                    NewMaze();
                }
                if (Gui::MenuItem("Save", "")) {
                    if (HasValidFile())
                        SaveMaze(filePath);
                    else {
                        fileDialog.Open();
                        loadOnFile = false;
                    }
                }
                if (Gui::MenuItem("Save As", "")) {
                    fileDialog.Open();
                    loadOnFile = false;
                }
                if (Gui::MenuItem("Load")) {
                    fileDialog.Open();
                    loadOnFile = true;
                }
                Gui::EndMenu();
            }
            Gui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3, 0.3, 0.3, 1));    
            Gui::Text(statusBarText.c_str());
            Gui::PopStyleColor();
            Gui::EndMainMenuBar();
        }

        int w = GetScreenWidth();
        Gui::SetNextWindowPos(ImVec2(w-400, 19));
        Gui::SetNextWindowSize(ImVec2(400, 0));
        Gui::Begin("Info bar", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
        Gui::TextWrapped("%s", filePath.string().c_str());
        Gui::End();
    }
};

#endif
