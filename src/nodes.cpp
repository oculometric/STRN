#include "nodes.h"

using namespace STRN;
using namespace std;

Node::Node()
{
    transform.size = Vec2{ 16, 5 };
}

void Node::render(Context& ctx)
{
    ctx.drawBox(Vec2{ 0, 0 }, transform.size, 0);
    ctx.fill(Vec2{ 1, 1 }, Vec2{ transform.size.x - 2, transform.size.y - 2 }, ' ');
    ctx.fill(Vec2{ 1, 0 }, Vec2{ transform.size.x - 2, 1 }, ' ', 1);
    ctx.drawText(Vec2{ 2, 0 }, title, 1);
    int line = 1;
    for (const auto& element : elements)
    {
        switch (element.type)
        {
        case ELEMENT_TEXT:
            ctx.drawText(Vec2{ 1, line }, element.text, 0, 0, transform.size.x - 2);
            break;
        case ELEMENT_INPUT:
            ctx.drawText(Vec2{ 1, line }, element.text, 0, 0, transform.size.x - 2);
            ctx.draw(Vec2{ 0, line }, '>');
            break;
        case ELEMENT_OUTPUT:
            ctx.drawText(Vec2{ 1, line }, element.text, 0, 0, transform.size.x - 2);
            ctx.draw(Vec2{ transform.size.x - 1, line }, '>');
            break;
        case ELEMENT_SPACE:
            break;
        case ELEMENT_BLOCK:
            ctx.fill(Vec2{ 1, line }, Vec2{ transform.size.x - 2, 1 }, ' ', 1);
            ctx.drawText(Vec2{ 2, line }, element.text, 1, 0, transform.size.x - 3);
            break;
        }
        ++line;
    }
}

void Node::resetSize()
{
    transform.size = Vec2{ 16, static_cast<int>(elements.size()) + 2 };
}
