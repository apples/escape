#pragma once

#include "ginseng/ginseng.hpp"

#include "rect.hpp"

namespace Component {

class Physical
    : public Ginseng::Component<Physical>
{
public:
    double x;
    double y;

    double vx = 0.0;
    double vy = 0.0;

    double friction = 1.0;

    bool solid = false;
    bool dynamic = true;

    double width;
    double height;

    Rect getAABB() const;
};

} // namespace Component
