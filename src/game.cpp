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

#include <yaml-cpp/yaml.h>

using namespace std;
using namespace Inugami;
using namespace Component;

// Constructor

    Game::Game(RenderParams params)
        : Core(params)
        , rng(nd_rand())
    {
        params = getParams();

    // Configuration

        addCallback([&]{ tick(); draw(); }, 60.0);
        setWindowTitle("Escape", true);

        min_view.width = (params.width)/4.0;
        min_view.height = (params.height)/4.0;

    // Resources

        loadTextures();
        loadSprites();

    // Entities

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

    // Load Level

        Level lvl;

        #if 0
        vector<int> walls;
        walls.resize(lvl.width*lvl.height);

        auto wallAt = [&](int r, int c)-> int& {
            return walls[r*lvl.width+c];
        };
        #endif

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
                    //wallAt(i, j) = 1;
                    sprite.anim = "wall";

                    auto& solid = entities.newComponent<Solid>(tile);
                    solid.width = 8;
                    solid.height = 8;
                }
                else
                {
                    sprite.anim = "floor";
                }
            }
        }

        #if 0
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
        #endif
    }

// Resource and Configuration Functions

    void Game::loadTextures()
    {
        using namespace YAML;

        auto conf = YAML::LoadFile("data/textures.yaml");

        for (auto const& p : conf)
        {
            auto file_node   = p.second["file"];
            auto smooth_node = p.second["smooth"];
            auto clamp_node  = p.second["clamp"];

            auto name = p.first.as<string>();
            auto file = file_node.as<string>();
            auto smooth = (smooth_node? smooth_node.as<bool>() : false);
            auto clamp  = ( clamp_node?  clamp_node.as<bool>() : false);
            textures.create(name, Image::fromPNG("data/"+file), smooth, clamp);
        }
    }

    void Game::loadSprites()
    {
        using namespace YAML;

        auto conf = LoadFile("data/sprites.yaml");

        for (auto const& spr : conf)
        {
            auto name = spr.first.as<string>();
            SpriteData data;

            auto texname = spr.second["texture"].as<string>();
            auto width = spr.second["width"].as<int>();
            auto height = spr.second["height"].as<int>();
            auto const& anims = spr.second["anims"];

            data.sheet = Spritesheet(textures.get(texname), width, height);

            for (auto const& anim : anims)
            {
                auto anim_name = anim.first.as<string>();
                auto& frame_vec = data.anims.create(anim_name);

                for (auto const& frame : anim.second)
                {
                    auto r_node = frame["r"];
                    auto c_node = frame["c"];
                    auto dur_node = frame["dur"];

                    auto r = r_node.as<int>();
                    auto c = c_node.as<int>();
                    auto dur = dur_node.as<int>();

                    frame_vec.emplace_back(r, c, dur);
                }
            }

            sprites.create(name, move(data));
        }
    }

// Tick Functions

    void Game::tick()
    {
        iface->poll();

        auto ESC = iface->key(Interface::ivkFunc(0));

        if (ESC || shouldClose())
        {
            running = false;
            return;
        }

        procAIs();
        runPhysics();
    }

    void Game::procAIs()
    {
        for (auto& ent : entities.getEntities<AI>())
        {
            auto& e = get<0>(ent);
            auto& ai = *get<1>(ent);
            ai.proc(e);
        }
    }

    void Game::runPhysics()
    {
        auto getRect = [](Position const& pos, Solid const& solid)
        {
            Rect rv;
            rv.left   = pos.x - solid.width/2.0;
            rv.right  = pos.x + solid.width/2.0;
            rv.bottom = pos.y - solid.height/2.0;
            rv.top    = pos.y + solid.height/2.0;
            return rv;
        };

        for (auto& ent : entities.getEntities<Position,Velocity,Solid>())
        {
            auto& eid   =  get<0>(ent);
            auto& pos   = *get<1>(ent);
            auto& vel   = *get<2>(ent);
            auto& solid = *get<3>(ent);

            auto linearCollide = [&](double Position::*d,
                                     double Velocity::*vd,
                                     double Rect::*lower,
                                     double Rect::*upper)
            {
                bool hit = false;
                auto aabb = getRect(pos, solid);
                for (auto& other : entities.getEntities<Position,Solid>())
                {
                    auto& eid2   =  get<0>(other);
                    auto& pos2   = *get<1>(other);
                    auto& solid2 = *get<2>(other);

                    if (eid == eid2) continue;

                    auto aabb2 = getRect(pos2, solid2);

                    if (aabb.top > aabb2.bottom
                    and aabb.bottom < aabb2.top
                    and aabb.right > aabb2.left
                    and aabb.left < aabb2.right)
                    {
                        double overlap;
                        if (vel.*vd > 0.0)
                            overlap = aabb2.*lower-aabb.*upper;
                        else
                            overlap = aabb2.*upper-aabb.*lower;
                        pos.*d += overlap;
                        aabb = getRect(pos, solid);
                        hit = true;
                    }
                }
                if (hit)
                    vel.*vd = 0.0;
            };

            pos.x += vel.vx;
            linearCollide(&Position::x, &Velocity::vx, &Rect::left, &Rect::right);

            pos.y += vel.vy;
            linearCollide(&Position::y, &Velocity::vy, &Rect::bottom, &Rect::top);

            vel.vx *= 1.0-vel.friction;
            vel.vy *= 1.0-vel.friction;
        }
    }

// Draw Functions

    void Game::draw()
    {
        beginFrame();

        setupCamera();
        drawSprites();

        endFrame();
    }

    void Game::setupCamera()
    {
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
    }

    void Game::drawSprites()
    {
        Transform mat;

        auto getDrawfunc = [&](Sprite& spr)
        {
            auto const& sprdata = sprites.get(spr.name);
            auto const& anim = sprdata.anims.get(spr.anim);

            if (--spr.ticker <= 0)
            {
                ++spr.anim_frame;
                if (spr.anim_frame >= anim.size()) spr.anim_frame = 0;
                spr.ticker = anim[spr.anim_frame].duration;
            }

            auto const& frame = anim[spr.anim_frame];

            return [&]{sprdata.sheet.draw(frame.r, frame.c);};
        };

        for (auto& ent : entities.getEntities<Position, Sprite>())
        {
            auto _ = mat.scope_push();

            auto& pos = *get<1>(ent);
            auto& spr = *get<2>(ent);

            mat.translate(pos.x, pos.y);
            modelMatrix(mat);

            getDrawfunc(spr)();
        }

        #if 0
        for (auto& ent : entities.getEntities<Position, Solid>())
        {
            auto _ = mat.scope_push();

            auto& pos = *get<1>(ent);
            auto& solid = *get<2>(ent);

            mat.translate(pos.x, pos.y);
            mat.scale(solid.width/8.0, solid.height/8.0);
            modelMatrix(mat);

            auto const& sprdata = sprites.get("redx");
            auto const& anim = sprdata.anims.get("redx");
            auto const& frame = anim[0];

            sprdata.sheet.draw(frame.r, frame.c);
        }
        #endif
    }
