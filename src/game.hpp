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

    // Configuration

        double tileWidth = 16.0;

        struct
        {
            double width;
            double height;
        } min_view;

    // Resources

        ResourcePool<Inugami::Texture> textures;
        ResourcePool<SpriteData> sprites;

    // Entities

        Ginseng::Database entities;
        Ginseng::Entity playerEID;

    // Support

        std::mt19937 rng;

public:
        Game(RenderParams params);

        void loadTextures();
        void loadSprites();

    // Tick Functions

        void tick();

        void procAIs();
        void runPhysics();

    // Draw Functions

        void draw();

        void setupCamera();
        void drawSprites();
};

#endif // GAME_HPP
