#include "game.hpp"

#include "inugami/camera.hpp"
#include "inugami/interface.hpp"

#include "meta.hpp"
#include "rect.hpp"
#include "level.hpp"
#include "component.ai.hpp"
#include "component.ai.playerai.hpp"
#include "component.physical.hpp"
#include "component.sprite.hpp"

#include <tuple>
#include <random>
#include <string>

using namespace std;
using namespace Inugami;

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

    double orthox = (params.width)/8.0;
    double orthoy = (params.height)/8.0;
    cam_base.ortho(-orthox, orthox, -orthoy, orthoy, -10, 10);

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

    auto ent = entities.newEntity();
    playerEID = ent;

    auto& sprite_com = entities.newComponent<Component::Sprite>(ent);
    sprite_com.name = "player";
    sprite_com.anim = "idle";

    auto& physical_com = entities.newComponent<Component::Physical>(ent);
    physical_com.x = 0;
    physical_com.y = 0;

    auto& ai_com = entities.newComponent<Component::PlayerAI>(ent);
    ai_com.setInput(Component::PlayerAI::LEFT, iface->key(Interface::ivkArrow('L')));
    ai_com.setInput(Component::PlayerAI::RIGHT, iface->key(Interface::ivkArrow('R')));
    ai_com.setInput(Component::PlayerAI::DOWN, iface->key(Interface::ivkArrow('D')));
    ai_com.setInput(Component::PlayerAI::UP, iface->key(Interface::ivkArrow('U')));

    Level lvl;

    vector<Rect> walls;
    walls.reserve(lvl.width*lvl.height);

    for (int i=0; i<lvl.height; ++i)
    {
        for (int j=0; j<lvl.width; ++j)
        {
            if (lvl.at(0,i,j) == 1)
            {
                Rect r;
                r.left = j*32;
                r.right = r.left+32;
                r.bottom = i*32;
                r.top = r.bottom+32;
                walls.push_back(r);

                auto tile = entities.newEntity();
                auto& tile_sprite = entities.newComponent<Component::Sprite>(tile);
                auto& tile_pos = entities.newComponent<Component::Physical>(tile);
                tile_sprite.name = "tile";
                tile_sprite.anim = "wall";
                tile_pos.y = i*8+4;
                tile_pos.x = j*8+4;
                tile_pos.dynamic = false;
            }
            else
            {
                auto tile = entities.newEntity();
                auto& tile_sprite = entities.newComponent<Component::Sprite>(tile);
                auto& tile_pos = entities.newComponent<Component::Physical>(tile);
                tile_sprite.name = "tile";
                tile_sprite.anim = "floor";
                tile_pos.x = j*8+4;
                tile_pos.y = i*8+4;
                tile_pos.dynamic = false;
            }
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

    for (auto& ent : entities.getEntities<Component::AI>())
    {
        auto& e = get<0>(ent);
        auto& ai = *get<1>(ent);
        ai.proc(e);
    }

    for (auto& ent : entities.getEntities<Component::Physical>())
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

    auto cam = cam_base;
    applyCam(cam);

    Transform mat;

    for (auto& ent : entities.getEntities<Component::Physical, Component::Sprite>())
    {
        auto _ = mat.scope_push();

        auto& pos = *get<1>(ent);
        auto& spr = *get<2>(ent);
logger->log(pos.x, ",", pos.y);
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
