#ifndef COMPONENT_POSITION_HPP
#define COMPONENT_POSITION_HPP

#include <ginseng/ginseng.hpp>

namespace Component {

struct Position
    : Ginseng::Component<Position>
{
    double x = 0;
    double y = 0;
    double z = 0;
};

} // namespace Component

#endif // COMPONENT_POSITION_HPP
