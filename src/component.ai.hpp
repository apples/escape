#ifndef COMPONENT_AI_HPP
#define COMPONENT_AI_HPP

#include "ginseng/ginseng.hpp"

#include <functional>

namespace Component {

class AI
{
    using Brain = std::function<void(Ginseng::Entity, AI const&)>;
    Brain brain;

public:
    struct
    {
        bool onGround;
        int wallHit;
        std::vector<Ginseng::Entity> hits;
    } senses;

    AI(Brain b);
    void proc(Ginseng::Entity ent);
};

} // namespace Component

#endif // COMPONENT_AI_HPP
