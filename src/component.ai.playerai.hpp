#ifndef COMPONENT_PLAYERAI_HPP
#define COMPONENT_PLAYERAI_HPP

#include "inugami/interface.hpp"

#include "component.ai.hpp"

namespace Component {

struct PlayerAI
{
    enum Input
    {
        LEFT,
        RIGHT,
        UP,
        DOWN,
        N_INPUT
    };

    using Func = std::function<bool()>;

    void operator()(Ginseng::Entity ent, AI const& ai);

    void setInput(Input i, Func func);

private:
    std::array<Func, Input::N_INPUT> inputs;
};

} // namespace Component

#endif // COMPONENT_PLAYERAI_HPP
