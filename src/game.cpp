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
#include <functional>
#include <vector>

#include <yaml-cpp/yaml.h>

using namespace std;
using namespace Inugami;
using namespace Component;

// Constructor

    Game::Game(RenderParams params)
        : Core(params)
        , rng(nd_rand())
    {
        auto _ = profiler->scope("Game::<constructor>()");

        params = getParams();

    // Configuration

        addCallback([&]{ tick(); draw(); }, 60.0);
        setWindowTitle("Escape", true);

        min_view.width = (params.width);
        min_view.height = (params.height);

        smoothcam = SmoothCamera(10);

    // Resources

        loadTextures();
        loadSprites();

    // Entities

        // Player

            {
                auto ent = entities.newEntity();

                auto& sprite = entities.addComponent(ent, make_shared<Sprite>());
                sprite.name = "player";
                sprite.anim = "idle";

                auto& pos = entities.addComponent(ent, make_shared<Position>());
                pos.x = 0;
                pos.y = 0;

                auto& vel = entities.addComponent(ent, make_shared<Velocity>());

                auto& solid = entities.addComponent(ent, make_shared<Solid>());
                solid.rect.left = -14;
                solid.rect.right = solid.rect.left + 28;
                solid.rect.bottom = -16;
                solid.rect.top = solid.rect.bottom + 28;

                PlayerAI ai;
                ai.setInput(PlayerAI::LEFT,  iface->key(Interface::ivkArrow('L')));
                ai.setInput(PlayerAI::RIGHT, iface->key(Interface::ivkArrow('R')));
                ai.setInput(PlayerAI::DOWN,  iface->key(Interface::ivkArrow('D')));
                ai.setInput(PlayerAI::UP,    iface->key(Interface::ivkArrow('U')));
                auto& aic = entities.addComponent(ent, make_shared<AI>(move(ai)));

                auto& cam = entities.addComponent(ent, make_shared<CamLook>());
                cam.aabb = solid.rect;
            }

        // Fire Pots

            for (int i=0; i<500; ++i)
            {
                auto ent = entities.newEntity();

                auto& sprite = entities.addComponent(ent, make_shared<Sprite>());
                sprite.name = "tile";
                sprite.anim = "firepot";

                auto& pos = entities.addComponent(ent, make_shared<Position>());
                pos.x = (rng()%149+1)*16;
                pos.y = (rng()%149+1)*16;
                pos.z = -0.5;
            }

        // Goombas

            for (int i=0; i<500; ++i)
            {
                auto ent = entities.newEntity();

                auto& sprite = entities.addComponent(ent, make_shared<Sprite>());
                sprite.name = "player";
                sprite.anim = "idle";

                auto& pos = entities.addComponent(ent, make_shared<Position>());
                pos.x = (rng()%149+1)*16;
                pos.y = (rng()%149+1)*16;

                auto& vel = entities.addComponent(ent, make_shared<Velocity>());

                auto& solid = entities.addComponent(ent, make_shared<Solid>());
                solid.rect.left = -14;
                solid.rect.right = solid.rect.left + 28;
                solid.rect.bottom = -16;
                solid.rect.top = solid.rect.bottom + 28;

                auto& ai = entities.addComponent(ent, make_shared<AI>(GoombaAI{}));
            }

    // Load Level

        Level lvl;

        for (int i=0; i<lvl.height; ++i)
        {
            for (int j=0; j<lvl.width; ++j)
            {
                auto tile = entities.newEntity();

                auto& pos = entities.addComponent(tile, make_shared<Position>());
                pos.y = i*tileWidth+tileWidth/2;
                pos.x = j*tileWidth+tileWidth/2;

                auto& sprite = entities.addComponent(tile, make_shared<Sprite>());
                sprite.name = "tile";

                if (lvl.at(0,i,j) == 1)
                {
                    sprite.anim = "bricks";

                    auto& solid = entities.addComponent(tile, make_shared<Solid>());
                    solid.rect.left = -tileWidth/2;
                    solid.rect.right = tileWidth/2;
                    solid.rect.bottom = -tileWidth/2;
                    solid.rect.top = tileWidth/2;
                }
                else
                {
                    sprite.anim = "background";

                    pos.z = -1;
                }
            }
        }
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
            data.width = width;
            data.height = height;

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
        auto _ = profiler->scope("Game::tick()");

        iface->poll();

        auto ESC = iface->key(Interface::ivkFunc(0));

        if (ESC || shouldClose())
        {
            running = false;
            return;
        }

        for (auto&& ent : entities.getEntities<AI>())
        {
            auto& ai = *get<1>(ent);
            ai.clearSenses();
        }

        runPhysics();
        procAIs();
        slaughter();
    }

    void Game::procAIs()
    {
        auto _ = profiler->scope("Game::procAIs()");

        for (auto&& ent : entities.getEntities<AI>())
        {
            auto& e = get<0>(ent);
            auto& ai = *get<1>(ent);
            ai.proc(e);
        }
    }

    void Game::runPhysics()
    {
        using AIHitVec = vector<Ginseng::Entity> AI::Senses::*;

        auto _ = profiler->scope("Game::runPhysics()");

        auto getRect = [](Position const& pos, Solid const& solid)
        {
            Rect rv;
            rv.left   = pos.x + solid.rect.left;
            rv.right  = pos.x + solid.rect.right;
            rv.bottom = pos.y + solid.rect.bottom;
            rv.top    = pos.y + solid.rect.top;
            return rv;
        };
        #if 0
        struct BucketID
        {
            int x;
            int y;
        };

        unordered_map<Ginseng::Entity*, vector<BucketID>> bucketmap;
        map<BucketID, Ginseng::Entity*>

        auto setBuckets = [&](Ginseng::Entity& ent, Rect const& rect)
        {
            constexpr double bucketlength = (24.0*32.0);
            auto& buckets = bucketmap[&ent];

            buckets.clear();

            int bxb = int(rect.left/bucketlength);
            int bxe = int(rect.right/bucketlength);
            int byb = int(rect.bottom/bucketlength);
            int bye = int(rect.top/bucketlength);

            for (int x=bxb; x<=bxe; ++x)
                for (int y=byb; y<=bye; ++y)
                    buckets.emplace_back(x,y);
        };
#endif
        for (auto& ent : entities.getEntities<Velocity,Solid>())
        {
            auto _ = profiler->scope("Gravity");

            auto& vel = *get<1>(ent);
            vel.vy -= 0.5;
        }

        for (auto& ent : entities.getEntities<Position,Velocity,Solid>())
        {
            auto _ = profiler->scope("Collision");

            auto& eid   =  get<0>(ent);
            auto& pos   = *get<1>(ent);
            auto& vel   = *get<2>(ent);
            auto& solid = *get<3>(ent);

            AI* ai;
            tie(ai) = Ginseng::getComponents<AI>(eid);

            auto linearCollide = [&](double Position::*d,
                                     double Velocity::*v,
                                     double Rect::*lower,
                                     double Rect::*upper,
                                     AIHitVec lhits,
                                     AIHitVec uhits)
            {
                int hit = 0;
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
                        AI* ai2;
                        tie(ai2) = Ginseng::getComponents<AI>(eid2);

                        double overlap;

                        if (vel.*v > 0.0)
                            overlap = aabb2.*lower-aabb.*upper;
                        else
                            overlap = aabb2.*upper-aabb.*lower;

                        pos.*d += overlap;
                        aabb = getRect(pos, solid);
                        hit = (overlap>0?-1:1);

                        if (ai)
                        {
                            if (hit<0)
                            {
                                if (ai2)
                                    (ai2->senses.*uhits).emplace_back(eid);
                                (ai->senses.*lhits).emplace_back(eid2);
                            }
                            else
                            {
                                if (ai2)
                                    (ai2->senses.*lhits).emplace_back(eid);
                                (ai->senses.*uhits).emplace_back(eid2);
                            }
                        }
                    }
                }

                if (hit != 0)
                    vel.*v = 0.0;

                return hit;
            };

            pos.x += vel.vx;
            int xhit = linearCollide(&Position::x,
                                     &Velocity::vx,
                                     &Rect::left,
                                     &Rect::right,
                                     &AI::Senses::hitsLeft,
                                     &AI::Senses::hitsRight);

            pos.y += vel.vy;
            int yhit = linearCollide(&Position::y,
                                     &Velocity::vy,
                                     &Rect::bottom,
                                     &Rect::top,
                                     &AI::Senses::hitsBottom,
                                     &AI::Senses::hitsTop);

            if (yhit != 0)
                vel.vx *= 1.0-vel.friction;
        }
    }

    void Game::slaughter()
    {
        auto const& ents = entities.getEntities<KillMe>();
        vector<Ginseng::Entity> dead;

        dead.reserve(ents.size());

        for (auto& ent : ents)
            dead.emplace_back(get<0>(ent));

        for (auto& ent : dead)
            entities.eraseEntity(ent);
    }

// Draw Functions

    void Game::draw()
    {
        auto _ = profiler->scope("Game::draw()");

        beginFrame();

        Rect view = setupCamera();

        drawSprites(view);

        endFrame();
    }

    Rect Game::setupCamera()
    {
        auto _ = profiler->scope("Game::setupCamera()");

        Camera cam;
        cam.depthTest = true;

        Rect rv;

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

            smoothcam.push(camloc.cx, camloc.cy, camloc.w, camloc.h);
            auto scam = smoothcam.get();
            scam.x = int(scam.x*8.0)/8.0;
            scam.y = int(scam.y*8.0)/8.0;

            double hw = scam.w/2.0;
            double hh = scam.h/2.0;
            rv.left = scam.x-hw;
            rv.right = scam.x+hw;
            rv.bottom = scam.y-hh;
            rv.top = scam.y+hh;

            cam.ortho(rv.left, rv.right, rv.bottom, rv.top, -10, 10);
        }

        applyCam(cam);

        return rv;
    }

    void Game::drawSprites(Rect view)
    {
        auto _ = profiler->scope("Game::drawSprites()");

        Transform mat;

        auto const& ents = entities.getEntities<Position, Sprite>();
        using Ent = decltype(&ents[0]);

        struct DrawItem
        {
            Ent ent;
            function<void()> draw;

            DrawItem(Ent e, function<void()> f)
                : ent(e)
                , draw(move(f))
            {}
        };

        vector<DrawItem> items;

        for (auto const& ent : ents)
        {
            auto& pos = *get<1>(ent);
            auto& spr = *get<2>(ent);

            auto const& sprdata = sprites.get(spr.name);

            Rect aabb;
            aabb.left   = pos.x-sprdata.width/2;
            aabb.right  = pos.x+sprdata.width/2;
            aabb.bottom = pos.y-sprdata.height/2;
            aabb.top    = pos.y+sprdata.height/2;

            if( aabb.left<view.right
            and aabb.right>view.left
            and aabb.bottom<view.top
            and aabb.top>view.bottom)
            {
                items.emplace_back(&ent, [&]
                {
                    auto _ = mat.scope_push();

                    auto const& anim = sprdata.anims.get(spr.anim);

                    mat.translate(int(pos.x+spr.offset.x), int(pos.y+spr.offset.y), pos.z);
                    modelMatrix(mat);

                    if (--spr.ticker <= 0)
                    {
                        ++spr.anim_frame;
                        if (spr.anim_frame >= anim.size())
                            spr.anim_frame = 0;
                        spr.ticker = anim[spr.anim_frame].duration;
                    }

                    auto const& frame = anim[spr.anim_frame];

                    sprdata.sheet.draw(frame.r, frame.c);
                });
            }
        }

        sort(begin(items), end(items), [](DrawItem const& a, DrawItem const& b)
        {
            auto& posa = *get<1>(*a.ent);
            auto& posb = *get<1>(*b.ent);
            return (tie(posa.z,posa.x,posa.y) < tie(posb.z,posb.x,posb.y));
        });

        for (auto const& item : items)
            item.draw();

        #if 0
        for (auto& ent : entities.getEntities<Position, Solid>())
        {
            auto _ = mat.scope_push();

            auto& pos = *get<1>(ent);
            auto& solid = *get<2>(ent);

            mat.translate(pos.x, pos.y, pos.x+1);
            //mat.scale(solid.width/8.0, solid.height/8.0);
            modelMatrix(mat);

            auto const& sprdata = sprites.get("redx");
            auto const& anim = sprdata.anims.get("redx");
            auto const& frame = anim[0];

            sprdata.sheet.draw(frame.r, frame.c);
        }
        #endif
    }
