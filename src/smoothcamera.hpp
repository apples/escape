#ifndef SMOOTHCAMERA_HPP
#define SMOOTHCAMERA_HPP

#include <vector>

class SmoothCamera
{
    struct State
    {
        double x = 0;
        double y = 0;
        double w = 0;
        double h = 0;
    };

    std::vector<State> history;

public:
    SmoothCamera() = default;
    SmoothCamera(int sz);

    void push(double x, double y, double w, double h);

    State get() const;
};

#endif // SMOOTHCAMERA_HPP
