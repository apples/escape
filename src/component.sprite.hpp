#ifndef COMPONENT_SPRITE_HPP
#define COMPONENT_SPRITE_HPP

#include <string>

namespace Component {

struct Sprite
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

#endif // COMPONENT_SPRITE_HPP
