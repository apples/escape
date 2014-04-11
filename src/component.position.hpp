#ifndef COMPONENT_POSITION_HPP
#define COMPONENT_POSITION_HPP

#include <ginseng/ginseng.hpp>

namespace Component {

struct Position
    : Ginseng::Component<Position>
{
    double x;
    double y;
};

} // namespace Component

#endif // COMPONENT_POSITION_HPP
