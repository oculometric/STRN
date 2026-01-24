#include "util.h"

#include <algorithm>

using namespace STRN;

Vec2 STRN::clip(const Vec2 min_size, const Vec2 max_size, const Vec2 available_size)
{
    int x;
    if (max_size.x == -1) x = available_size.x;
    else x = std::min(available_size.x, max_size.x);
    int y;
    if (max_size.y == -1) y = available_size.y;
    else y = std::min(available_size.y, max_size.y);
    
    return Vec2{ x, y };
}