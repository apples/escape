#include "component.ai.playerai.hpp"

#include "components.hpp"

#include <tuple>
using namespace std;

namespace Component {

void PlayerAI::proc(Ginseng::Entity ent)
{
    auto comps = Ginseng::getComponents<Velocity>(ent);
    auto& vel = *get<0>(comps);

    if (inputs[LEFT]())  vel.vx -= 1;
    if (inputs[RIGHT]()) vel.vx += 1;

    if (inputs[UP]() and senses.onGround) vel.vy += 5;
    //if (inputs[DOWN]())  vel.vy -= 1;
}

void PlayerAI::setInput(Input i, std::function<bool()> func)
{
    inputs[i] = std::move(func);
}

} // namespace Component
