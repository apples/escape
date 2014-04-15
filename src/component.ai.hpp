#ifndef COMPONENT_AI_HPP
#define COMPONENT_AI_HPP

#include "ginseng/ginseng.hpp"

namespace Component {

class AI
    : public Ginseng::Component<AI, false>
{
public:
    virtual void proc(Ginseng::Entity ent) = 0;

    struct
    {
        bool onGround;
        int wallHit;
        std::vector<Ginseng::Entity> hits;
    } senses;
};

template <typename Child>
class AIFactory
    : public AI
{
public:
    virtual ::std::unique_ptr<ComponentBase> clone() const override
    {
        return ::std::unique_ptr<ComponentBase>(new Child(reinterpret_cast<const Child&>(*this)));
    }
};

} // namespace Component

#endif // COMPONENT_AI_HPP
