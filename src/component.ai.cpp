#include "component.ai.hpp"

#include <utility>

namespace Component {

AI::AI(Brain b)
    : brain(std::move(b))
{}

void AI::proc(Ginseng::Entity ent)
{
    return brain(ent,*this);
}

} // namespace Component
