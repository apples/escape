#ifndef SPRITEDATA_HPP
#define SPRITEDATA_HPP

#include "inugami/spritesheet.hpp"

#include "resourcepool.hpp"

#include <utility>
#include <vector>

class SpriteData
{
public:
    struct Frame
    {
        int r;
        int c;
        int duration;

        Frame(int x, int y, int z)
            : r(x)
            , c(y)
            , duration(z)
        {}
    };

    Inugami::Spritesheet sheet;

    ResourcePool<std::vector<Frame>> anims;
};

#endif // SPRITEDATA_HPP
