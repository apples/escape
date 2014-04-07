#include "level.hpp"

Level::Level()
{
    width = 32;
    height = 32;
    for (auto&& l : data) l.resize(width*height);
}

Level::Tile& Level::at(int l, int r, int c)
{
    return data[l][r*width+c];
}
