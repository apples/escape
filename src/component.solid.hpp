#ifndef COMPONENT_SOLID_HPP
#define COMPONENT_SOLID_HPP

#include <ginseng/ginseng.hpp>

namespace Component {

struct Solid
    : Ginseng::Component<Solid>
{
    double width;
    double height;
};

} // namespace Component

#endif // COMPONENT_SOLID_HPP
