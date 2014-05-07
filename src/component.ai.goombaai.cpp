#include "component.ai.goombaai.hpp"

#include "component.ai.playerai.hpp"
#include "component.killme.hpp"
#include "component.velocity.hpp"

#include <algorithm>
#include <tuple>
#include <typeinfo>
#include <memory>
using namespace std;

namespace Component {

void GoombaAI::operator()(Ginseng::Entity ent, AI const& ai)
{
    auto xHit = int(ai.senses.hitsRight.size()) - int(ai.senses.hitsLeft.size());
    if (xHit != 0)
        dir = (xHit>0? -1 : 1);

    Velocity* vel;
    tie(vel) = Ginseng::getComponents<Velocity>(ent);

    if (vel)
    {
        if (dir < 0 and vel->vx>-5.0) vel->vx -= min(vel->vx+5.0,5.0);
        if (dir > 0 and vel->vx< 5.0) vel->vx += min(5.0-vel->vx,5.0);
        if (!ai.senses.hitsBottom.empty() and
            (!ai.senses.hitsLeft.empty() or !ai.senses.hitsRight.empty()))
        {
            vel->vy += 15;
        }
    }

    if (!ai.senses.hitsTop.empty())
    {
        AI* ai2;

        for (auto&& ent2 : ai.senses.hitsTop)
        {
            tie(ai2) = Ginseng::getComponents<AI>(ent2);
            if (ai2 && (ai2->brainEq<GoombaAI>() || ai2->brainEq<PlayerAI>()))
            {
                ent.getDB()->addComponent(ent, make_shared<KillMe>());
                break;
            }
        }
    }
}

} // namespace Component
