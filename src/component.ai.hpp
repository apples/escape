#ifndef COMPONENT_AI_HPP
#define COMPONENT_AI_HPP

#include "forward.hpp"
#include "types.hpp"

#include <functional>

namespace Component {

class AI
{
    using Brain = std::function<void(Game&, EntID, AI const&)>;
    Brain brain;

public:
    struct Senses
    {
        std::vector<EntID> hitsLeft;
        std::vector<EntID> hitsRight;
        std::vector<EntID> hitsBottom;
        std::vector<EntID> hitsTop;
    } senses;

    void clearSenses()
    {
        senses.hitsLeft.clear();
        senses.hitsRight.clear();
        senses.hitsBottom.clear();
        senses.hitsTop.clear();
    }

    AI(Brain b);
    void proc(Game& game, EntID ent);

    template <typename T>
    bool brainEq() const
    {
        return (brain.target_type() == typeid(T));
    }
};

} // namespace Component

#endif // COMPONENT_AI_HPP
