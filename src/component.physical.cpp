#include "component.physical.hpp"

namespace Component {

Rect Physical::getAABB() const
{
    Rect rv;
    rv.left = x-width/2.0;
    rv.right = x+width/2.0;
    rv.bottom = y-height/2.0;
    rv.top = y+height/2.0;
    return rv;
}

}
