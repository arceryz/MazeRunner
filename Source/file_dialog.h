#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <string>
#include <stdio.h>
#include <imgui.h>
#include <vector>
#include <filesystem>
using namespace std;
namespace Gui = ImGui;
namespace fs = filesystem;

// FileDialog is an ImGui tool for getting file paths to write to
// in the system.
class FileDialog {
private:
    bool open = false;
    vector<fs::directory_entry> paths;
    vector<string> fileTypes;
    char fileBuf[256] = "";
    fs::path currentPath = "";
    float lastClick = 0;
    int lastItem = 0;

    void LoadOptions()
    {
        targetPath = "";
        strcpy(fileBuf, "");
        paths.clear();
        paths.push_back(fs::directory_entry(".."));
        for (const auto &entry: fs::directory_iterator(currentPath)) {
            paths.push_back(entry);
        }
    }

public:
    fs::path targetPath = "";

    FileDialog()
    {
    }
    void Open() 
    {
        currentPath = fs::current_path();
        lastClick = 0;
        lastItem = 0;
        fileTypes.clear();
        paths.clear();
        LoadOptions();
        open = true;
    }
    bool Update()
    {
        if (!open)
            return false;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        Gui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);

        targetPath = currentPath;
        targetPath.append(string(fileBuf));

        Gui::Begin("File Dialog", &open, flags);
        Gui::SetNextItemWidth(-80);
        Gui::InputText("", fileBuf, IM_ARRAYSIZE(fileBuf));
        Gui::SameLine();
        if (Gui::Button("Confirm", ImVec2(70, 20)) && targetPath.has_filename())
            open = false;
        Gui::Spacing();
        
        if (Gui::BeginTable("FilesTable", 2)) {
            Gui::TableSetupColumn("Type", 0, 0.1);
            Gui::TableSetupColumn("Paths");
            Gui::TableHeadersRow();

            Gui::TableNextRow();
            Gui::TableSetColumnIndex(0);

            for (int i = 0; i < paths.size(); i++) {
                fs::directory_entry entry = paths[i];
                fs::path path = entry.path();
                string pathName = path.filename().string();

                Gui::TableNextRow();
                Gui::TableSetColumnIndex(0);
                string type = "File";
                if (entry.is_directory()) {
                    Gui::TableSetBgColor(ImGuiTableBgTarget_CellBg, Gui::GetColorU32(ImVec4(0.1, 0.2, 0.3, 1)));
                    type = "Dir";
                }
                if (path.extension() == ".json") {
                     Gui::TableSetBgColor(ImGuiTableBgTarget_CellBg, Gui::GetColorU32(ImVec4(0.1, 0.3, 0.1, 1)));
                     type = "JSON";
                }
                Gui::Text("%s", type.c_str());
                
                Gui::TableSetColumnIndex(1);
                if (Gui::Selectable(TextFormat("%s", pathName.c_str()), false, ImGuiSelectableFlags_SpanAllColumns)) {
                    float time = GetTime();
                    if (time - lastClick < 0.3 && i == lastItem && entry.is_directory()) {
                        if (path == "..") {
                            if (currentPath.has_parent_path())
                                currentPath = currentPath.parent_path();
                        } else {
                            currentPath = path;
                        }
                        LoadOptions();
                    }
                    if (entry.is_regular_file()) {
                        targetPath = path;
                        strcpy(fileBuf, path.filename().string().c_str());
                    }
                    lastClick = time;
                    lastItem = i;
                }      
            }

            Gui::TableNextColumn();
            Gui::EndTable();
        }
        Gui::TextWrapped(currentPath.string().c_str());
        Gui::End();
        return !open;
    }
};

#endif