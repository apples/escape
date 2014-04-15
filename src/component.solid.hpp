#ifndef COMPONENT_SOLID_HPP
#define COMPONENT_SOLID_HPP

#include <ginseng/ginseng.hpp>

#include "rect.hpp"

namespace Component {

struct Solid
    : Ginseng::Component<Solid>
{
    Rect rect;
};

} // namespace Component

#endif // COMPONENT_SOLID_HPP
