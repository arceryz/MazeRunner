#include <fstream>
#include <nlohmann/json.hpp>
using namespace nlohmann;

#include "maze.h"
#include "arclib.h"

//
// Coord methods.
//
Coord::Coord()
{

}
Coord::Coord(int _x, int _y)
{
    x = _x;
    y = _y;
}
Coord::Coord(CoordID key)
{
    sscanf(key.c_str(), "%i,%i", &x, &y);
}
Coord Coord::operator+(const Coord &other)
{
    Coord ret = { x+other.x, y+other.y };
    return ret;
}
Coord Coord::operator-(const Coord &other)
{
    Coord ret = { x-other.x, y-other.y };
    return ret;
}
bool Coord::operator==(const Coord &other)
{
    return x == other.x && y == other.y;
}
CoordID Coord::ToKey()
{
    // Makes a key for indexing the position->data maps.
    CoordID key = to_string(x) + string(",") + to_string(y);
    return key;
}

//
// Junction class methods.
//
Junction::Junction()
{
}
Junction::Junction(string _name, JunctionID _id) 
{
    name = _name;
    id = _id;
}

//
// JunctionRect methods.
//
JunctionRect::JunctionRect()
{
}
JunctionRect::JunctionRect(Coord _top, Coord _bot)
: top(_top), bot(_bot)
{
}
bool JunctionRect::ContainsPoint(int x, int y)
{
    return top.x <= x && x < bot.x && top.y <= y && y < bot.y;
}

//
// TagCoord methods
//
TagCoord::TagCoord() 
{

}
TagCoord::TagCoord(vector<string> &_tags, Coord _coord)
: tags(_tags), coord(_coord)
{

}

//
// Maze methods.
//
Maze::Maze()
{
    name = "Unnamed Maze";
}

//
// Junction methods.
//
void Maze::AddJunction(int x, int y, string name, JunctionID id) 
{
    if (GetJunctionAt(x, y) != 0) {
        printf("Junction at %i %i already exists", x, y);
        return;
    }
    Coord coord = Coord(x, y);
    CoordID key = coord.ToKey();

    // Ensure uniqueness.
    if (id == 0) {
        id = rand();
        while (id != 0 && JunctionExists(id))
            id = rand();
    }
    Junction j = Junction(name, id);
    printf("Added junction %s (%i) at key %s (%d, %d)\n", name.c_str(), j.id, key.c_str(), x, y);

    id_to_junction[id] = j;
    id_to_rect[id] = { { 0, 0 }, { 1, 1 } };
    id_to_coord[id] = coord;
    coord_to_id[key] = id;

    // Split a tunnel if inserting on a tunnel.
    Tunnel tunnel = GetTunnelAt(x, y);
    if (tunnel.from != 0 && tunnel.to != 0) {
        RemoveTunnel(tunnel);
        AddTunnel(tunnel.from, id);
        AddTunnel(tunnel.to, id);
        printf("Split tunnel at %i, %i", x, y);
    }
}
void Maze::RemoveJunction(JunctionID id)
{
    // Remove all tunnels attached.
    for (auto kv: tunnel_map[id]) {
        tunnel_map[kv.first].erase(id);
        printf("Erasing tunnel %i->%i\n", id, kv.first);
    }

    Coord coord = id_to_coord[id];

    tunnel_map.erase(id);
    id_to_junction.erase(id);
    id_to_coord.erase(id);
    coord_to_id.erase(coord.ToKey());
    printf("Junction (%i) at %i %i removed\n", id, coord.x, coord.y);
}
bool Maze::JunctionExists(JunctionID id)
{
    return id_to_junction.find(id) != id_to_junction.end();
}

JunctionID Maze::GetJunctionAt(int x, int y)
{
    CoordID key = Coord(x, y).ToKey();
    if (coord_to_id.find(key) != coord_to_id.end()) {
        return coord_to_id[key];
    }
    return 0;
}
Junction& Maze::GetJunction(JunctionID id) 
{
    return id_to_junction[id];
}
Coord Maze::GetJunctionCoord(JunctionID id)
{
    if (JunctionExists(id)) {
        return id_to_coord[id];
    }
    return Coord {0, 0};
}
JunctionID Maze::FindJunction(string name)
{
    vector<JunctionID> junctions = GetJunctionList();
    for (JunctionID id: junctions) {
        Junction &j = GetJunction(id);
        if (j.name == name)
            return id;
    }
    return 0;
}
vector<JunctionID> Maze::GetJunctionList()
{
    vector<JunctionID> junctions;
    for (auto kv: id_to_junction) {
        junctions.push_back(kv.first);
    };
    return junctions;
}
vector<JunctionID> Maze::GetConnectedJunctions(JunctionID source)
{
    vector<JunctionID> junctions;
    for (auto kv: tunnel_map[source]) {
        junctions.push_back(kv.first);
    }
    return junctions;
}
void Maze::PruneJunctions()
{
    // Prune all colinear junctions.
    vector<JunctionID> junctions = GetJunctionList();

    for (JunctionID j: junctions) {
        Coord pos = GetJunctionCoord(j);
        vector<JunctionID> connected = GetConnectedJunctions(j);
        int num = connected.size();
        
        // Prune junctions on a tunnel.
        if (num == 2) {
            Coord c1 = GetJunctionCoord(connected[0]);
            Coord c2 = GetJunctionCoord(connected[1]);

            if (c1.x == pos.x && c2.x == pos.x || c1.y == pos.y && c2.y == pos.y) {
                RemoveJunction(j);
                AddTunnel(connected[0], connected[1]);
                printf("Pruned node %d due to colinear degree 2", j);
            }
        }

        // Prune loose junctions.
        if (num == 0) {
            RemoveJunction(j);
        }
    }
}

//
// JunctionRect methods.
//
bool Maze::SetJunctionRect(JunctionID id, JunctionRect rect)
{
    // Validate rectangle.
    if (!(rect.top.x < rect.bot.x && rect.top.y < rect.bot.y)) {
        printf("Rectangle top must be < bot\n");
        return false;
    }

    Coord c = GetJunctionCoord(id);

    // Validate if no other junctions exist in rectangle.
    for (int x = rect.top.x; x < rect.bot.x; x++) {
        for (int y = rect.top.y; y < rect.bot.y; y++) {
            JunctionID other = GetJunctionAt(c.x+x, c.y+y);
            if (other > 0 && other != id) {
                printf("Existing junction under rect, abort.\n");
                return false;
            }
        }
    }

    // First set the rectangle.
    // Afterwards remesh the coord_to_id map.
    JunctionRect old = id_to_rect[id];

    // Iterate old.
    for (int x = old.top.x; x < old.bot.x; x++) {
        for (int y = old.top.y; y < old.bot.y; y++) {
            // Remove if not in new.
            if (!rect.ContainsPoint(x, y)) {
                coord_to_id.erase(Coord(c.x+x, c.y+y).ToKey());
                printf("Removed point (%d, %d)\n", x, y);
            }
        }
    }

    // Iterate new. Abort if existing junction at point.
    for (int x = rect.top.x; x < rect.bot.x; x++) {
        for (int y = rect.top.y; y < rect.bot.y; y++) {
            // Add if not in old.
            if (!old.ContainsPoint(x, y)) {
                coord_to_id[Coord(c.x+x, c.y+y).ToKey()] = id;
                printf("Added point (%d, %d)\n", x, y);
            }
        }
    }
    
    // This way we deleted all the excess points and added the new points,
    // without having to add and remove all points.
    id_to_rect[id] = rect;
    return true;
}
JunctionRect Maze::GetJunctionRect(JunctionID id)
{
    if (id_to_rect.find(id) != id_to_rect.end())
        return id_to_rect[id];
    return {};
}

//
// Tunnel methods.
//
void Maze::AddTunnel(JunctionID from, JunctionID to)
{
    Tunnel t { from, to };
    if (!IsValidTunnel(t)) {
        printf("Tunnel between %i and %i is not valid (existent or invalid)", t);
        return;
    }

    // Attempt to create a tunnel.
    Coord coord1 = GetJunctionCoord(from);
    Coord coord2 = GetJunctionCoord(to);

    printf("Added tunnel from %i-%i\n", from, to);
    tunnel_map[from][to] = 1;
    tunnel_map[to][from] = 1;
}
void Maze::RemoveTunnel(Tunnel t)
{
    tunnel_map[t.from].erase(t.to);
    tunnel_map[t.to].erase(t.from);
    printf("Removed tunnel from %i-%i\n", t.from, t.to);
}
bool Maze::IsValidTunnel(Tunnel t) {
    // Tunnel is valid when:
    // 1. Junctions are colinear to each other.
    // 2. Tunnels does not overlap an existing tunnel.
    if (TunnelExists(t) || t.from == t.to)
        return false;

    Coord coord1 = GetJunctionCoord(t.from);
    Coord coord2 = GetJunctionCoord(t.to);

    Coord diff = coord1 - coord2;
    if (diff.x != 0 && diff.y != 0)
        return false;

    Coord coordmax = Coord(max(coord1.x, coord2.x), max(coord1.y, coord2.y));
    Coord coordmin = Coord(min(coord1.x, coord2.x), min(coord1.y, coord2.y));

    vector<Tunnel> tunnels = GetTunnelList();
    for (Tunnel t: tunnels) {
        Coord other1 = GetJunctionCoord(t.from);
        Coord other2 = GetJunctionCoord(t.to);

        Coord othermax = Coord(max(other1.x, other2.x), max(other1.y, other2.y));
        Coord othermin = Coord(min(other1.x, other2.x), min(other1.y, other2.y));

        if (coord1 == other1 || coord2 == other1 || coord1 == other2 || coord2 == other2)
            continue;

        bool fail1 = othermin.x <= coordmin.x && coordmin.x <= othermax.x &&
            coordmin.y <= othermin.y && othermin.y <= coordmax.y;
        bool fail2 = coordmin.x <= othermin.x && othermin.x <= coordmax.x &&
            othermin.y <= coordmin.y && coordmin.y <= othermax.y;

        if (fail1 || fail2)
            return false;
    }

    return true;
}
bool Maze::TunnelExists(Tunnel t)
{
    if (tunnel_map.find(t.to) != tunnel_map.end()) {
        auto map2 = tunnel_map[t.from];
        return map2.find(t.to) != map2.end();
    }
    return false;
}
Tunnel Maze::GetTunnelAt(int x, int y)
{
    vector<Tunnel> tunnels = GetTunnelList();
    for (Tunnel t: tunnels) {
        Coord other1 = GetJunctionCoord(t.from);
        Coord other2 = GetJunctionCoord(t.to);

        Coord othermax = Coord(max(other1.x, other2.x), max(other1.y, other2.y));
        Coord othermin = Coord(min(other1.x, other2.x), min(other1.y, other2.y));
        
        if (x == other1.x && x == other2.x && othermin.y < y && y < othermax.y ||
             y == other1.y && y == other2.y && othermin.x < x && x < othermax.x) {
            return t;
        } 
    }
    return { 0, 0 };
}
vector<Tunnel> Maze::GetTunnelList()
{
    vector<Tunnel> tunnels;
    for (auto pair1: tunnel_map) {
        for (auto pair2: pair1.second) {
            if (pair1.first < pair2.first) {
                tunnels.push_back({ pair1.first, pair2.first });
            }
        }
    }
    return tunnels;
}

//
// Tag methods.
//
vector<string> Maze::GetTagsAt(int x, int y)
{
    // Return if it exists, otherwise empty. We can't create tags with 
    // every query because queries will be numerous.
    // Our system only includes non-empty tag lists.
    CoordID key = Coord(x, y).ToKey();
    if (coord_to_tags.find(key) != coord_to_tags.end()) {
            return coord_to_tags[key];
    }
    return vector<string>();
}
void Maze::SetTagsAt(int x, int y, vector<string> &tags)
{
    // Set the tags at this location.
    // If the tag list is empty, the tags will be removed.
    CoordID key = Coord(x, y).ToKey();
    if (tags.size() == 0) {
        printf("Pruning empty tags at (%d, %d)\n", x, y);
        coord_to_tags.erase(key);
        return;
    }

    // Now we can set the tags, newly created if nonexistent key.
    printf("Updating tags at (%d, %d)\n", x, y);
    coord_to_tags[key] = tags;
}
vector<TagCoord> Maze::GetTagsList()
{
    vector<TagCoord> tagCoords;
    for (auto pair: coord_to_tags) {
        tagCoords.push_back(TagCoord(pair.second, pair.first));
    }
    return tagCoords;
}

//
// IO methods.
//
void Maze::ExportJson(string filename)
{
    // Here we get all vertices and push them back.
    json junclist;
    vector<JunctionID> junctions = GetJunctionList();
    for (JunctionID id: junctions) {
        Junction &j = id_to_junction[id];
        Coord coords = id_to_coord[id];
        junclist.push_back({ coords.x, coords.y, j.id, j.name } );
    };

    // Here we get all edges and push them back.
    json edgelist;
    vector<Tunnel> tunnels = GetTunnelList();
    for (Tunnel t: tunnels) {
        Junction j1 = id_to_junction[t.from];
        Junction j2 = id_to_junction[t.to];
        edgelist.push_back(to_string(j1.id) + string(",") + to_string(j2.id));
    }

    // Here we get all the edges and push them back.
    json tags;
    vector<TagCoord> tagCoords = GetTagsList();
    for (TagCoord tc: tagCoords) {
        tags.push_back({ tc.coord.x, tc.coord.y, tc.tags });
    }

    ordered_json jfile = {
        {"mazeName", name },
        {"junctions", junclist },
        {"tunnels", edgelist },
        {"tags", tags }
    };

    ofstream file(filename);
    file << jfile.dump(4);
    printf("Written maze \"%s\" to json \"%s\"", name.c_str(), filename.c_str());
}
void Maze::ImportJson(string filename)
{
    ifstream stream(filename);
    if (!stream.good())
        return;
    json imported = json::parse(stream);
    name = imported["mazeName"];

    json juncs = imported["junctions"];
    for (json junction: juncs) {
        string s = junction.at(3);
        JunctionID id = junction.at(2);
        int x = junction.at(0);
        int y = junction.at(1);
        AddJunction(x, y, s, id);
    }

    json tunnels = imported["tunnels"];
    for (string tunnel: tunnels) {
        int mid = tunnel.find(",");
        string s1 = tunnel.substr(0, mid);
        string s2 = tunnel.substr(mid+1, tunnel.length());
        JunctionID j1 = stoi(s1);
        JunctionID j2 = stoi(s2);
        AddTunnel(j1, j2);
    }

    json tags = imported["tags"];
    for (json tagCoord: tags) {
        int tx = tagCoord.at(0);
        int ty =  tagCoord.at(1);
        vector<string> vec = tagCoord.at(2).get<vector<string>>();
        SetTagsAt(tx, ty, vec);
    }
}