#include "game.hpp"

#include "inugami/camera.hpp"
#include "inugami/interface.hpp"

#include "meta.hpp"
#include "rect.hpp"
#include "level.hpp"
#include "components.hpp"

#include <tuple>
#include <random>
#include <string>
#include <limits>

using namespace std;
using namespace Inugami;
using namespace Component;

#ifdef __MINGW32__
#include <windows.h>
#include <Ntsecapi.h>
static mt19937::result_type rng_seed()
{
    mt19937::result_type rv;
    RtlGenRandom(&rv, sizeof(rv));
    return rv;
}
#else
static mt19937::result_type rng_seed()
{
    return random_device{}();
}
#endif // __MINGW32__

Game::Game(RenderParams params)
    : Core(params)
    , rng(rng_seed())
{
    params = getParams();

    addCallback([&]{ tick(); draw(); }, 60.0);
    setWindowTitle("Escape", true);

    min_view.width = (params.width)/4.0;
    min_view.height = (params.height)/4.0;

    Texture enemies (Image::fromPNG("data/enemies.png"), false, false);

    {
        auto& sheet = spritesheets.create("player");
        sheet.sheet = {enemies, 8, 8};
        auto& anim = sheet.anims.create("idle");
        anim.emplace_back(0,0,10);
        anim.emplace_back(0,1,10);
    }

    {
        auto& sheet = spritesheets.create("tile");
        sheet.sheet = {enemies, 8, 8};
        {
            auto& anim = sheet.anims.create("floor");
            anim.emplace_back(1,1,10);
        }
        {
            auto& anim = sheet.anims.create("wall");
            anim.emplace_back(0,0,10);
        }
    }

    {
        auto ent = entities.newEntity();
        playerEID = ent;

        auto& sprite = entities.newComponent<Sprite>(ent);
        sprite.name = "player";
        sprite.anim = "idle";

        auto& pos = entities.newComponent<Position>(ent);
        pos.x = 0;
        pos.y = 0;

        auto& vel = entities.newComponent<Velocity>(ent);

        auto& solid = entities.newComponent<Solid>(ent);
        solid.width = 8;
        solid.height = 8;

        auto& ai = entities.newComponent<PlayerAI>(ent);
        ai.setInput(PlayerAI::LEFT,  iface->key(Interface::ivkArrow('L')));
        ai.setInput(PlayerAI::RIGHT, iface->key(Interface::ivkArrow('R')));
        ai.setInput(PlayerAI::DOWN,  iface->key(Interface::ivkArrow('D')));
        ai.setInput(PlayerAI::UP,    iface->key(Interface::ivkArrow('U')));

        auto& cam = entities.newComponent<CamLook>(ent);
        cam.aabb.left   = -4;
        cam.aabb.right  =  4;
        cam.aabb.bottom = -4;
        cam.aabb.top    =  4;
    }

    Level lvl;

    vector<int> walls;
    walls.resize(lvl.width*lvl.height);

    auto wallAt = [&](int r, int c)-> int& {
        return walls[r*lvl.width+c];
    };

    for (int i=0; i<lvl.height; ++i)
    {
        for (int j=0; j<lvl.width; ++j)
        {
            auto tile = entities.newEntity();

            auto& pos = entities.newComponent<Position>(tile);
            pos.y = i*8+4;
            pos.x = j*8+4;

            auto& sprite = entities.newComponent<Sprite>(tile);
            sprite.name = "tile";

            if (lvl.at(0,i,j) == 1)
            {
                wallAt(i, j) = 1;
                sprite.anim = "wall";
            }
            else
            {
                sprite.anim = "floor";
            }
        }
    }

    for (int i=0; i<lvl.height; ++i)
    {
        for (int j=0; j<lvl.width; ++j)
        {
            for (int k=j+1; k<lvl.width and wallAt(i,k)!=0; ++k)
            {
                ++wallAt(i,j);
                wallAt(i,k) = 0;
            }
            j += wallAt(i,j);
        }
    }

    for (int i=0; i<lvl.width; ++i)
    {
        for (int j=0; j<lvl.height; ++j)
        {
            if (wallAt(i,j) == 0) continue;

            double cx = i*8 + 4;
            double cy = j*8 + 4;
            double w = 8;
            double h = 8;

            for (int k=j+1; k<lvl.height and wallAt(i,k)==w; ++k)
            {
                cy += 4;
                h += 8;
                wallAt(i,k) = 0;
                ++j;
            }

            auto wall = entities.newEntity();

            auto& pos = entities.newComponent<Position>(wall);
            pos.x = cx;
            pos.y = cy;

            auto& solid = entities.newComponent<Solid>(wall);
            solid.width = w;
            solid.height = h;
        }
    }
}

void Game::tick()
{
    iface->poll();

    auto ESC = iface->key(Interface::ivkFunc(0));

    if (ESC || shouldClose())
    {
        running = false;
        return;
    }

    for (auto& ent : entities.getEntities<AI>())
    {
        auto& e = get<0>(ent);
        auto& ai = *get<1>(ent);
        ai.proc(e);
    }

    for (auto& ent : entities.getEntities<Position,Velocity>())
    {
        auto& pos = *get<1>(ent);
        auto& vel = *get<2>(ent);

        pos.x += vel.vx;
        pos.y += vel.vy;

        vel.vx -= vel.vx*vel.friction;
        vel.vy -= vel.vy*vel.friction;
    }
}

void Game::draw()
{
    beginFrame();

    Camera cam;
    {
        Rect view;
        view.left = numeric_limits<decltype(view.left)>::max();
        view.bottom = view.left;
        view.right = numeric_limits<decltype(view.right)>::lowest();
        view.top = view.right;

        for (auto& ent : entities.getEntities<Position, CamLook>())
        {
            auto& pos = *get<1>(ent);
            auto& cam = *get<2>(ent);

            view.left   = min(view.left,   pos.x + cam.aabb.left);
            view.bottom = min(view.bottom, pos.y + cam.aabb.bottom);
            view.right  = max(view.right,  pos.x + cam.aabb.right);
            view.top    = max(view.top,    pos.y + cam.aabb.top);
        }

        struct
        {
            double cx;
            double cy;
            double w;
            double h;
        } camloc =
        {     (view.left+view.right)/2.0
            , (view.bottom+view.top)/2.0
            , (view.right-view.left)
            , (view.top-view.bottom)
        };

        double rat = camloc.w/camloc.h;
        double trat = (min_view.width)/(min_view.height);

        if (rat < trat)
        {
            if (camloc.h < min_view.height)
            {
                double s = min_view.height / camloc.h;
                camloc.h = min_view.height;
                camloc.w *= s;
            }

            camloc.w = trat*camloc.h;
        }
        else
        {
            if (camloc.w < min_view.width)
            {
                double s = min_view.width / camloc.w;
                camloc.w = min_view.width;
                camloc.h *= s;
            }

            camloc.h = camloc.w/trat;
        }

        double hw = camloc.w/2.0;
        double hh = camloc.h/2.0;
        cam.ortho(camloc.cx-hw, camloc.cx+hw, camloc.cy-hh, camloc.cy+hh, -10, 10);
    }
    applyCam(cam);

    Transform mat;

    for (auto& ent : entities.getEntities<Position, Sprite>())
    {
        auto _ = mat.scope_push();

        auto& pos = *get<1>(ent);
        auto& spr = *get<2>(ent);

        mat.translate(pos.x, pos.y);
        modelMatrix(mat);

        auto const& sprdata = spritesheets.get(spr.name);
        auto const& anim = sprdata.anims.get(spr.anim);

        if (--spr.ticker <= 0)
        {
            ++spr.anim_frame;
            if (spr.anim_frame >= anim.size()) spr.anim_frame = 0;
            spr.ticker = anim[spr.anim_frame].duration;
        }

        auto const& frame = anim[spr.anim_frame];

        sprdata.sheet.draw(frame.r, frame.c);
    }

    endFrame();
}
