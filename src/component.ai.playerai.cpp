#include "component.ai.playerai.hpp"

#include "component.physical.hpp"

#include <tuple>
using namespace std;

namespace Component {

void PlayerAI::proc(Ginseng::Entity ent)
{
    Physical* phys;
    tie(phys) = Ginseng::getComponents<Physical>(ent);

    if (inputs[LEFT]())  phys->vx -= 1;
    if (inputs[RIGHT]()) phys->vx += 1;

    if (inputs[UP]())    phys->vy += 1;
    if (inputs[DOWN]())  phys->vy -= 1;
}

void PlayerAI::setInput(Input i, std::function<bool()> func)
{
    inputs[i] = std::move(func);
}

} // namespace Component
