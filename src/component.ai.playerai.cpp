#include "component.ai.playerai.hpp"

#include "components.hpp"

#include "component.position.hpp"
#include "component.velocity.hpp"

#include <tuple>
using namespace std;

namespace Component {

void PlayerAI::operator()(Game& game, EntID ent, AI const& ai)
{
    auto comps = ent.getComs<Position,Velocity>();
    auto& pos = *get<0>(comps).first;
    auto& vel = *get<1>(comps).first;

    if (inputs[LEFT]() and vel.vx>-5.0) vel.vx -= min(vel.vx+5.0,5.0);
    if (inputs[RIGHT]() and vel.vx<5.0) vel.vx += min(5.0-vel.vx,5.0);

    if (inputs[UP]() and !ai.senses.hitsBottom.empty()) vel.vy += 15;
}

void PlayerAI::setInput(Input i, std::function<bool()> func)
{
    inputs[i] = std::move(func);
}

} // namespace Component
