#ifndef SPRITEDATA_HPP
#define SPRITEDATA_HPP

#include "inugami/spritesheet.hpp"

#include "resourcepool.hpp"

#include <utility>
#include <vector>

class SpriteData
{
public:
    Inugami::Spritesheet sheet;

    ResourcePool<std::vector<std::pair<int,int>>> anims;
};

#endif // SPRITEDATA_HPP
