#ifndef GAME_HPP
#define GAME_HPP

#include "inugami/core.hpp"
#include "inugami/camera.hpp"
#include "inugami/texture.hpp"
#include "inugami/spritesheet.hpp"

#include "ginseng/ginseng.hpp"

#include "resourcepool.hpp"

#include <random>

class Game
	: public Inugami::Core
{
    double tileWidth = 16.0;

    Inugami::Camera cam_base;

    ResourcePool<Inugami::Spritesheet> spritesheets;

    Ginseng::Database entities;

    Ginseng::Entity playerEID;

    std::mt19937 rng;

public:
	Game(RenderParams params);

	void tick();
	void draw();
};

#endif // GAME_HPP
