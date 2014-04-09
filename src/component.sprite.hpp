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
    int anim_frame = -1;
    int ticker = 0;
};

} // namespace Component
