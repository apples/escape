#pragma once

#include "ginseng/ginseng.hpp"

#include <string>

namespace Component {

class Sprite
    : public Ginseng::Component<Sprite>
{
public:
    std::string name;
    std::string anim;
    int anim_frame;
    int delay;
    int ticker = 0;
};

} // namespace Component
