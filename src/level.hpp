#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <array>
#include <vector>

class Level
{
    using Tile = int;
    using Layer = std::vector<Tile>;
    std::array<Layer,4> data;

public:
    int width;
    int height;

    Level();

    Tile& at(int l, int r, int c);
};

#endif // LEVEL_HPP
