#include "smoothcamera.hpp"

SmoothCamera::SmoothCamera(int sz)
    : history(sz)
{}

void SmoothCamera::push(double x, double y, double w, double h)
{
    State st;
    st.x = x;
    st.y = y;
    st.w = w;
    st.h = h;
    history.erase(history.begin());
    history.push_back(st);
}

SmoothCamera::State SmoothCamera::get() const
{
    State avg;

    for (auto const& s : history)
    {
        avg.x += s.x;
        avg.y += s.y;
        avg.w += s.w;
        avg.h += s.h;
    }

    avg.x /= history.size();
    avg.y /= history.size();
    avg.w /= history.size();
    avg.h /= history.size();

    return avg;
}
