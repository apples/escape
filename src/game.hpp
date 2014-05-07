#ifndef GAME_HPP
#define GAME_HPP

#include "inugami/core.hpp"
#include "inugami/texture.hpp"
#include "inugami/spritesheet.hpp"

#include "ginseng/ginseng.hpp"

#include "resourcepool.hpp"
#include "spritedata.hpp"
#include "rect.hpp"
#include "smoothcamera.hpp"

#include <random>

class Game
	: public Inugami::Core
{
    // Configuration

        int tileWidth = 32;

        struct
        {
            double width;
            double height;
        } min_view;

        SmoothCamera smoothcam;

    // Resources

        ResourcePool<Inugami::Texture> textures;
        ResourcePool<SpriteData> sprites;

    // Entities

        Ginseng::Database entities;

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
        void slaughter();

    // Draw Functions

        void draw();

        Rect setupCamera();
        void drawSprites(Rect view);
};

#endif // GAME_HPP
