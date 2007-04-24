#include <iostream>
#include <string>
#include <vector>

class MapGenerator {
    enum Direction { up, down, left, right };
    enum Symmetry { asymmetric, rotational, horizontal, vertical };

    class SimpleRoom {
    public:
        SimpleRoom(bool walls = false): top(walls), bottom(walls), left(walls), right(walls), visited(false), checked_through(false), flag(false) { }

        bool top, bottom, left, right;  // walls
        bool visited;
        bool checked_through;
        bool flag;
    };

    class Node {
    public:
        Node(): cost(INT_MAX), score(INT_MAX) { }
        int cost, score;
    };

    struct Dist { std::pair<int, int> coords; int dist; };

public:
    void generate(int w, int h, bool allow_over_edge = false);
    void draw(std::ostream& out) const;
    void save_map(std::ostream& out, const std::string& title, const std::string& author) const;

private:
    bool remove_wall(int rx, int ry, int dx, int dy, bool mirror = false);

    std::pair<int, int> max_distance();
    int distance(int sx, int sy, int gx, int gy);
    const std::pair<int, int>& find_best(const std::vector<std::vector<Node> >& node, const std::vector<std::pair<int, int> >& open);

    int width() const { return room.size(); }
    int height() const { return room.front().size(); }

    std::vector<std::vector<SimpleRoom> > room; // accessible by room[x][y]
    Symmetry symmetry;
    bool over_edge;
};
