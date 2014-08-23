#include "level.hpp"

#include "meta.hpp"

#include <random>
using namespace std;

Level::Level()
{
    width = 75;
    height = 75;
    for (auto&& l : data) l.resize(width*height);

    mt19937 rng(nd_rand());
    uniform_int_distribution<int> dist(0,4);

    for (int i=0; i<height; ++i)
    {
        for (int j=0; j<width; ++j)
        {
            if (i==0 || j==0 || i==height-1 || j==width-1)
                at(0, i, j) = 1;
            else
                at(0, i, j) = (dist(rng)==0);
        }
    }
}

Level::Tile& Level::at(int l, int r, int c)
{
    return data[l][r*width+c];
}
