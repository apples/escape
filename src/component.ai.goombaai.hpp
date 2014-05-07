#ifndef COMPONENT_GOOMBAAI_HPP
#define COMPONENT_GOOMBAAI_HPP

#include "component.ai.hpp"

namespace Component {

struct GoombaAI
{
    int dir = 1;
    void operator()(Ginseng::Entity ent, AI const& ai);
};

} // namespace Component

#endif // COMPONENT_GOOMBAAI_HPP
