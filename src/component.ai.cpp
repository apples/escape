#include "component.ai.hpp"

#include <utility>

namespace Component {

AI::AI(Brain b)
    : brain(std::move(b))
{}

void AI::proc(Game& game, EntID ent)
{
    return brain(game, ent, *this);
}

} // namespace Component
