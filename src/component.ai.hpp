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
    struct Senses
    {
        std::vector<Ginseng::Entity> hitsLeft;
        std::vector<Ginseng::Entity> hitsRight;
        std::vector<Ginseng::Entity> hitsBottom;
        std::vector<Ginseng::Entity> hitsTop;
    } senses;

    void clearSenses()
    {
        senses.hitsLeft.clear();
        senses.hitsRight.clear();
        senses.hitsBottom.clear();
        senses.hitsTop.clear();
    }

    AI(Brain b);
    void proc(Ginseng::Entity ent);

    template <typename T>
    bool brainEq() const
    {
        return (brain.target_type() == typeid(T));
    }
};

} // namespace Component

#endif // COMPONENT_AI_HPP
