#ifndef GAME_HPP
#define GAME_HPP

#include "inugami/core.hpp"
#include "inugami/texture.hpp"
#include "inugami/spritesheet.hpp"

#include "ginseng/ginseng.hpp"

#include "resourcepool.hpp"
#include "spritedata.hpp"
#include "rect.hpp"

#include <random>

class Game
	: public Inugami::Core
{
    double tileWidth = 16.0;

    struct
    {
        double width;
        double height;
    } min_view;

    ResourcePool<SpriteData> spritesheets;

    Ginseng::Database entities;

    Ginseng::Entity playerEID;

    std::mt19937 rng;

public:
	Game(RenderParams params);

	void tick();
	void draw();
};

#endif // GAME_HPP
