#include "window.h"

#include "widgets.h"

using namespace STRN;
using namespace std;

void Window::setSize(const Vec2 value)
{ transform.size = value; dirty(); }

void Window::setPosition(const Vec2 value)
{ transform.position = value; dirty(); }

void Window::setTitle(const std::string& value)
{ title = value; dirty(); }

void Window::setAllowsMinimise(const bool value)
{
    allows_minimise = value;
    if (!allows_minimise)
        is_minimised = false;
    dirty();
}

void Window::setBorderless(const bool value)
{ borderless = value; dirty(); }

void Window::setRoot(Widget* value)
{
    root = value;
    root->setWindow(this);
    dirty();
}

void Window::render(Context& ctx)
{
    Vec2 content_area = transform.size;
    Vec2 content_start = { 0, 0 };
    if (!borderless)
    {
        ctx.drawBox(content_start, content_area);
        ctx.fill(content_start + Vec2{ 1, content_area.y - 1 },
            Vec2{ content_area.x - 2, 1 },
            ' ', 1);
        ctx.drawText(content_start + Vec2{ 2, content_area.y - 1 },
            title, 1,
            0, content_area.x - 3);
        ctx.draw(content_start + Vec2{ content_area.x - 2, content_area.y - 1 },
            static_cast<char>(0xFE), 1);
        content_area -= Vec2{ 2, 2 };
        content_start += Vec2{ 1, 1 };
    }
    if (root == nullptr)
    {
        is_dirty = false;
        return;
    }
    
    root->arrange(content_area);
    
    ctx.pushBounds(content_start, content_area + content_start);
    root->render(ctx);
    ctx.popBounds();
    
    // TODO: restore dirty flag functionality
    is_dirty = false;
}
