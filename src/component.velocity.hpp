#ifndef COMPONENT_VELOCITY_HPP
#define COMPONENT_VELOCITY_HPP

#include <ginseng/ginseng.hpp>

namespace Component {

struct Velocity
    : Ginseng::Component<Velocity>
{
    double vx = 0.0;
    double vy = 0.0;
    double friction = 1.0;
};

} // namespace Component

#endif // COMPONENT_VELOCITY_HPP
