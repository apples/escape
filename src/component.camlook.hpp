#ifndef COMPONENT_CAMLOOK_HPP
#define COMPONENT_CAMLOOK_HPP

#include "ginseng/ginseng.hpp"

#include "rect.hpp"

namespace Component {

struct CamLook
    : Ginseng::Component<CamLook>
{
    Rect aabb;
};

} // namespace Component

#endif // COMPONENT_CAMLOOK_HPP
