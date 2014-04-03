#include "game.hpp"

#include "inugami/camera.hpp"
#include "inugami/interface.hpp"

#include "meta.hpp"
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

    logger->log("Window setup...");
    addCallback([&]{ tick(); draw(); }, 60.0);
    setWindowTitle("Escape", true);

    logger->log("Camera setup...");
    double orthox = (params.width)/4.0;
    double orthoy = (params.height)/4.0;
    cam_base.ortho(-orthox, orthox, -orthoy, orthoy, -10, 10);

    logger->log("Loading resources...");
    spritesheets.create("player", Image::fromNoise(64,64), 16, 16);

    logger->log("Registering components...");
    entities.registerComponent<Component::Physical>();
    entities.registerComponent<Component::Sprite>();

    logger->log("Creating base entity...");
    auto ent = entities.newEntity();
    playerEID = ent;

    auto sprite_com = entities.newComponent<Component::Sprite>(ent);
    sprite_com->name = "player";

    auto physical_com = entities.newComponent<Component::Physical>(ent);
    physical_com->x = 0;
    physical_com->y = 0;

    uniform_real_distribution<double> xdist (-orthox, orthox);
    uniform_real_distribution<double> ydist (-orthoy, orthoy);

    logger->log("Cloning entity...");
    for (int i=0; i<10; ++i)
    {
        ent = entities.cloneEntity(ent);
        std::tie(physical_com) = entities.getComponents<Component::Physical>(ent);
        physical_com->x = xdist(rng);
        physical_com->y = ydist(rng);
    }

    logger->log("Setup done.");
}

void Game::tick()
{
    iface->poll();

    auto ESC = iface->key(Interface::ivkFunc(0));
    auto left = iface->key(Interface::ivkArrow('L'));
    auto right = iface->key(Interface::ivkArrow('R'));
    auto up = iface->key(Interface::ivkArrow('U'));
    auto down = iface->key(Interface::ivkArrow('D'));
    auto space = iface->key(' ');

    if (ESC || shouldClose())
    {
        running = false;
        return;
    }

    Component::Physical* player_physical;
    tie(player_physical) = entities.getComponents<Component::Physical>(playerEID);

    if (left)  player_physical->x -= 1;
    if (right) player_physical->x += 1;
    if (down)  player_physical->y -= 1;
    if (up)    player_physical->y += 1;

    if (space)
    {
        for (int i=0; i<10; ++i)
        {
            auto ent = entities.cloneEntity(playerEID);
            Component::Physical* physical_com;
            std::tie(physical_com) = entities.getComponents<Component::Physical>(ent);
            double orthox = (getParams().width)/4.0;
            double orthoy = (getParams().height)/4.0;
            uniform_real_distribution<double> xdist (-orthox, orthox);
            uniform_real_distribution<double> ydist (-orthoy, orthoy);
            physical_com->x = xdist(rng);
            physical_com->y = ydist(rng);
        }
        setWindowTitle("Escape (" + to_string(entities.numEntities()) + ")", true);
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
        mat.push();

        auto& pos = *get<1>(ent);
        auto& spr = *get<2>(ent);

        mat.translate(pos.x, pos.y);
        modelMatrix(mat);

        spritesheets.get(spr.name).draw(0,0);

        mat.pop();
    }

    endFrame();
}
