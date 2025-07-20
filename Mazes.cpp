#include "defGameEngine.hpp"

#include <stack>
#include <chrono>
#include <thread>

class Example : public def::GameEngine
{
public:
    Example()
    {
        GetWindow()->SetTitle("Example");
    }

    const def::Vector2i mapSize = { 16, 16 };
    def::Vector2i tileSize;

    enum Direction : int
    {
        DIR_NORTH = 1 << 0,
        DIR_SOUTH = 1 << 1,
        DIR_EAST = 1 << 2,
        DIR_WEST = 1 << 3,
        DIR_VISITED = 1 << 4
    };

    int* map = nullptr;

    int visited = 0;
    std::stack<def::Vector2i> frontier;

protected:
    bool OnUserCreate() override
    {
        def::Vector2i screenSize = GetWindow()->GetScreenSize();

        tileSize = screenSize / mapSize;

        map = new int[screenSize.x * screenSize.y]{ 0 };

        visited = 1;
        frontier.push({ 0, 0 });

        return true;
    }

    void GetNeighbours(const def::Vector2i cell, std::vector<std::pair<int, def::Vector2i>>& out)
    {
        def::Vector2i coord;

        auto val = [&](int ox, int oy)
        {
            coord.x = cell.x + ox;
            coord.y = cell.y + oy;
            return map[(cell.y + oy) * mapSize.x + cell.x + ox];
        };

        if (cell.x - 1 >= 0 && val(-1, 0) == 0)
            out.push_back({ DIR_WEST, coord });

        if (cell.x + 1 < mapSize.x && val(1, 0) == 0)
            out.push_back({ DIR_EAST, coord });

        if (cell.y - 1 >= 0 && val(0, -1) == 0)
            out.push_back({ DIR_NORTH, coord });

        if (cell.y + 1 < mapSize.y && val(0, 1) == 0)
            out.push_back({ DIR_SOUTH, coord });
    }

    bool OnUserUpdate(float deltaTime) override
    {
        // Build a maze

        //using namespace std::chrono_literals;
        //std::this_thread::sleep_for(30ms);

        if (visited < mapSize.x * mapSize.y)
        {
            const def::Vector2i& cell = frontier.top();

            // Each neighbour has its direction relative to the current cell
            // and a coordinate
            std::vector<std::pair<int, def::Vector2i>> neighs;
            GetNeighbours(cell, neighs);
            
            if (neighs.empty())
            {
                // We've stucked so go backwards
                frontier.pop();
            }
            else
            {
                // We have some neighbours so let's randomly pick one of them
                const auto& next = neighs[rand() % (int)neighs.size()];

                auto get = [&](const def::Vector2i& p) -> int&
                {
                    return map[p.y * mapSize.x + p.x];
                };

                // Update the current cell
                get(cell) |= next.first | DIR_VISITED;

                int& cellValue = get(next.second);

                // Update its neighbour
                if (next.first == DIR_NORTH) cellValue |= DIR_SOUTH;
                if (next.first == DIR_SOUTH) cellValue |= DIR_NORTH;
                if (next.first == DIR_WEST)  cellValue |= DIR_EAST;
                if (next.first == DIR_EAST)  cellValue |= DIR_WEST;

                // Push the neighbour to the frontier
                frontier.push(next.second);

                visited++;
            }
        }

        // Draw the maze

        Clear(def::BLUE);

        auto draw_line = [&](int x1, int y1, int x2, int y2)
        {
            DrawLine(x1 * tileSize.x, y1 * tileSize.y, x2 * tileSize.x, y2 * tileSize.y, def::WHITE);
        };

        def::Vector2i screenSize = GetWindow()->GetScreenSize();

        for (int y = 0; y < mapSize.y; y++)
            for (int x = 0; x < mapSize.x; x++)
            {
                int cell = map[y * mapSize.x + x];

                if (cell & DIR_VISITED)
                {
                    if ((cell & DIR_NORTH) == 0)
                        draw_line(x, y, x + 1, y);

                    if ((cell & DIR_SOUTH) == 0)
                        draw_line(x, y + 1, x + 1, y + 1);

                    if ((cell & DIR_EAST) == 0)
                        draw_line(x + 1, y, x + 1, y + 1);

                    if ((cell & DIR_WEST) == 0)
                        draw_line(x, y, x, y + 1);
                }
            }

        // Draw our current position

        FillRectangle(frontier.top() * tileSize, tileSize, def::GREEN);

        return true;
    }

};

int main()
{
    Example demo;

    if (demo.Construct(256, 240, 4, 4, false, true))
        demo.Run();

    return 0;
}
