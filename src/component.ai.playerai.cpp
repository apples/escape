#include "component.ai.playerai.hpp"

#include "components.hpp"

#include "component.position.hpp"
#include "component.velocity.hpp"

#include <tuple>
using namespace std;

namespace Component {

void PlayerAI::proc(Ginseng::Entity ent)
{
    auto comps = Ginseng::getComponents<Position,Velocity>(ent);
    auto& pos = *get<0>(comps);
    auto& vel = *get<1>(comps);

    if (inputs[LEFT]())  vel.vx -= 5;
    if (inputs[RIGHT]()) vel.vx += 5;

    if (inputs[UP]() and senses.onGround) vel.vy += 15;
}

void PlayerAI::setInput(Input i, std::function<bool()> func)
{
    inputs[i] = std::move(func);
}

} // namespace Component
