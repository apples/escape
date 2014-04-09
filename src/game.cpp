#include "game.hpp"

#include "inugami/camera.hpp"
#include "inugami/interface.hpp"

#include "meta.hpp"
#include "rect.hpp"
#include "level.hpp"
#include "component.ai.hpp"
#include "component.ai.playerai.hpp"
#include "component.camlook.hpp"
#include "component.physical.hpp"
#include "component.sprite.hpp"

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

        auto& sprite_com = entities.newComponent<Sprite>(ent);
        sprite_com.name = "player";
        sprite_com.anim = "idle";

        auto& physical_com = entities.newComponent<Physical>(ent);
        physical_com.x = 0;
        physical_com.y = 0;
        physical_com.solid = true;
        physical_com.width = 8;
        physical_com.height = 8;

        auto& ai_com = entities.newComponent<PlayerAI>(ent);
        ai_com.setInput(PlayerAI::LEFT,  iface->key(Interface::ivkArrow('L')));
        ai_com.setInput(PlayerAI::RIGHT, iface->key(Interface::ivkArrow('R')));
        ai_com.setInput(PlayerAI::DOWN,  iface->key(Interface::ivkArrow('D')));
        ai_com.setInput(PlayerAI::UP,    iface->key(Interface::ivkArrow('U')));

        entities.newComponent<CamLook>(ent);
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
            auto& tile_sprite = entities.newComponent<Sprite>(tile);
            auto& tile_pos = entities.newComponent<Physical>(tile);
            tile_sprite.name = "tile";
            tile_pos.y = i*8+4;
            tile_pos.x = j*8+4;
            tile_pos.dynamic = false;

            if (lvl.at(0,i,j) == 1)
            {
                wallAt(i, j) = 1;

                tile_sprite.anim = "wall";
            }
            else
            {
                tile_sprite.anim = "floor";
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
            int w = wallAt(i,j);

            if (w == 0) continue;

            Rect rect;
            rect.left = i*8;
            rect.right = rect.left + 8;
            rect.bottom = j*8;
            rect.top = rect.bottom + 8;

            for (int k=j+1; k<lvl.height and wallAt(i,k)==w; ++k)
            {
                rect.top += 8;
                wallAt(i,k) = 0;
                ++j;
            }

            auto wall = entities.newEntity();
            auto& wall_pos = entities.newComponent<Physical>(wall);
            wall_pos.x = (rect.left+rect.right)/2.0;
            wall_pos.y = (rect.bottom+rect.top)/2.0;
            wall_pos.solid = true;
            wall_pos.width = rect.right-rect.left;
            wall_pos.height = rect.top-rect.bottom;
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

    for (auto& ent : entities.getEntities<Physical>())
    {
        auto& phys = *get<1>(ent);

        if (!phys.dynamic) continue;

        phys.x += phys.vx;
        phys.y += phys.vy;

        phys.vx -= phys.vx*phys.friction;
        phys.vy -= phys.vy*phys.friction;
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

        for (auto& ent : entities.getEntities<Physical, CamLook>())
        {
            auto& pos = *get<1>(ent);
            Rect aabb = pos.getAABB();

            view.left   = min(view.left,   aabb.left);
            view.bottom = min(view.bottom, aabb.bottom);
            view.right  = max(view.right,  aabb.right);
            view.top    = max(view.top,    aabb.top);
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

    for (auto& ent : entities.getEntities<Physical, Sprite>())
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
