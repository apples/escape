#pragma once

#include "ginseng/ginseng.hpp"

#include <string>

namespace Component {

struct Sprite
    : Ginseng::Component<Sprite>
{
    std::string name;
    std::string anim;
    unsigned anim_frame = -1;
    int ticker = 0;

    struct
    {
        double x = 0;
        double y = 0;
    }
    offset;
};

} // namespace Component
