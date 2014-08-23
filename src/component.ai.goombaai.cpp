#include "component.ai.goombaai.hpp"

#include "component.ai.playerai.hpp"
#include "component.killme.hpp"
#include "component.velocity.hpp"

#include "game.hpp"

#include <algorithm>
#include <tuple>
#include <typeinfo>
#include <memory>
using namespace std;

namespace Component {

void GoombaAI::operator()(Game& game, EntID ent, AI const& ai)
{
    auto xHit = int(ai.senses.hitsRight.size()) - int(ai.senses.hitsLeft.size());
    if (xHit != 0)
        dir = (xHit>0? -1 : 1);

    Velocity& vel = ent.get<Velocity>().data();

    if (dir < 0 && vel.vx>-5.0) vel.vx -= min(vel.vx+5.0,5.0);
    if (dir > 0 && vel.vx< 5.0) vel.vx += min(5.0-vel.vx,5.0);
    if (!ai.senses.hitsBottom.empty() &&
        (!ai.senses.hitsLeft.empty() || !ai.senses.hitsRight.empty()))
    {
        vel.vy += 15;
    }

    #if 0
    if (!ai.senses.hitsTop.empty())
    {
        for (auto&& ent2 : ai.senses.hitsTop)
        {
            auto ai2 = ent2.get<AI>();
            if (ai2 && (ai2.data().brainEq<GoombaAI>() || ai2.data().brainEq<PlayerAI>()))
            {
                game.entities.makeComponent(ent, KillMe{});
                break;
            }
        }
    }
    #endif
}

} // namespace Component
