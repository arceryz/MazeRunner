#ifndef MAZE_H
#define MAZE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

using namespace std;

typedef uint32_t JunctionID;
typedef string CoordID;

// Structure to represent simply an int vector.
struct Coord {
    int x;
    int y;

    Coord();
    Coord(int x, int y);
    Coord(CoordID key);
    Coord operator+(const Coord &other);
    Coord operator-(const Coord &other);
    bool operator==(const Coord &other);
    CoordID ToKey();
};

// Structure to represent a junction with its associated data.
struct Junction {
    string name;
    JunctionID id;

    Junction();
    Junction(string _name, JunctionID _id);
};

struct JunctionRect {
    Coord top;
    Coord bot;
    JunctionRect();
    JunctionRect(Coord top, Coord bot);
    bool ContainsPoint(int x, int y);
};

struct Tunnel {
    JunctionID from;
    JunctionID to;
};

struct TagCoord {
    TagCoord();
    TagCoord(vector<string> &tags, Coord coord);
    vector<string> tags;
    Coord coord;
};

class Maze {
public:
    string name;

    // Natural bijections of data access:
    // junctionID -> Junction
    // junctionID -> JunctionRect
    // junctionID -> Coord
    // Coord -> JunctionID
    unordered_map<JunctionID, Junction> id_to_junction;
    unordered_map<JunctionID, JunctionRect> id_to_rect;
    unordered_map<JunctionID, Coord> id_to_coord;
    unordered_map<CoordID, JunctionID> coord_to_id;
    
    unordered_map<JunctionID, unordered_map<JunctionID, int>> tunnel_map;
    unordered_map<CoordID, vector<string>> coord_to_tags;

    Maze();

    // Junction methods.
    void AddJunction(int x, int y, string name, JunctionID id=0);
    void RemoveJunction(JunctionID id);
    bool JunctionExists(JunctionID id);

    JunctionID GetJunctionAt(int x, int y);
    Junction& GetJunction(JunctionID id);
    Coord GetJunctionCoord(JunctionID id);
    JunctionID FindJunction(string name);
    vector<JunctionID> GetJunctionList();
    vector<JunctionID> GetConnectedJunctions(JunctionID source);
    void PruneJunctions();

    // JunctionRect methods.
    bool SetJunctionRect(JunctionID id, JunctionRect rect);
    JunctionRect GetJunctionRect(JunctionID id);

    // Tunnel methods.
    void AddTunnel(JunctionID from, JunctionID to);
    void RemoveTunnel(Tunnel t);
    bool IsValidTunnel(Tunnel t);
    bool TunnelExists(Tunnel t);
    Tunnel GetTunnelAt(int x, int y);
    vector<Tunnel> GetTunnelList();

    // Tag methods.
    vector<string> GetTagsAt(int x, int y);
    void SetTagsAt(int x, int y, vector<string> &tags);
    vector<TagCoord> GetTagsList(); 

    // IO Methods.
    void ExportJson(string filename);
    void ImportJson(string filename);
};

#endif