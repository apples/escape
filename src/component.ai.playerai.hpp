#ifndef COMPONENT_PLAYERAI_HPP
#define COMPONENT_PLAYERAI_HPP

#include "inugami/interface.hpp"

#include "component.ai.hpp"

namespace Component {

class PlayerAI
    : public AIFactory<PlayerAI>
{
public:
    enum Input
    {
        LEFT,
        RIGHT,
        UP,
        DOWN,
        N_INPUT
    };

    using Func = std::function<bool()>;

    virtual void proc(Ginseng::Entity ent) override;

    void setInput(Input i, Func func);

private:
    std::array<Func, Input::N_INPUT> inputs;
};

} // namespace Component

#endif // COMPONENT_PLAYERAI_HPP
