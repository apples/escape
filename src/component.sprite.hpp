#pragma once

#include "ginseng/ginseng.hpp"

#include <string>

namespace Component {

class Sprite
    : public Ginseng::Component<Sprite>
{
public:
    std::string name;
};

} // namespace Component
