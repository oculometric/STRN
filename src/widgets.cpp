#include "widgets.h"

#include "strn.h"

using namespace STRN;
using namespace std;

// Widget ==================================================

void Widget::arrange(const Vec2 available_area)
{ transform.size = clip(getMinSize(), getMaxSize(), available_area); }

void Widget::setVisible(const bool value)
{ visible = value; dirty(); }

void Widget::setTransform(const Transform2& value)
{ transform = value; dirty(); }

void Widget::setPosition(const Vec2& value)
{ transform.position = value; dirty(); }

void Widget::setSize(const Vec2& value)
{ transform.size = value; dirty(); }

int Widget::addChild(Widget* child)
{
    children.push_back(child);
    child->parent = this;
    child->window = window;
    dirty();
    return static_cast<int>(children.size());
}

void Widget::setWindow(Window* _window)
{
    window = _window;
    for (const auto& c : children)
        c->setWindow(window);
    dirty();
}

void Widget::dirty() const
{
    if (window)
        window->dirty();
}


// Label ====================================================

void Label::render(Context& ctx) const
{ ctx.drawText(Vec2{ 0, 0 }, text); }

void Label::setText(const std::string& value)
{ text = value; dirty(); }


// ArrangedBox ==============================================

void ArrangedBox::render(Context& ctx) const
{
    for (const auto c : children)
    {
        ctx.pushBounds(c->getTransform().position, c->getTransform().position + c->getTransform().size);
        c->render(ctx);
        ctx.popBounds();
    }
}

static int genericArrange(const int budget, const vector<int>& min_constraints, const vector<int>& max_constraints, vector<int>& calculated_constraints)
{
    int consumed = 0;
    calculated_constraints.resize(min_constraints.size());
    for (size_t i = 0; i < min_constraints.size(); ++i)
    {
        calculated_constraints[i] = min_constraints[i];
        consumed += calculated_constraints[i];
    }

    if (consumed > budget)
        return budget;

    while (consumed < budget)
    {
        bool changed = false;
        for (size_t i = 0; i < min_constraints.size(); ++i)
        {
            if (max_constraints[i] != -1 && calculated_constraints[i] >= max_constraints[i]) continue;

            ++calculated_constraints[i];
            changed = true;

            ++consumed;
            if (consumed == budget) break;
        }
        if (!changed) break;
    }
    
    return consumed;
}


// VerticalBox ============================================

void VerticalBox::arrange(const Vec2 available_area)
{
    vector<int> min_heights(children.size());
    vector<int> max_heights(children.size());
    for (size_t i = 0; i < children.size(); ++i)
    {
        min_heights[i] = children[i]->getMinSize().y;
        max_heights[i] = children[i]->getMaxSize().y;
    }
    vector<int> calculated_heights(children.size());
    int total_height = genericArrange(available_area.y, min_heights, max_heights, calculated_heights);

    int y_offset = 0;
    for (size_t i = 0; i < children.size(); ++i)
    {
        children[i]->arrange(Vec2{ available_area.x, calculated_heights[i] });
        children[i]->setPosition(Vec2{ 0, y_offset });
        y_offset += calculated_heights[i];
    }
    setSize(Vec2{ available_area.x, total_height });
}


// HorizontalBox ============================================

void HorizontalBox::arrange(const Vec2 available_area)
{
    vector<int> min_widths(children.size());
    vector<int> max_widths(children.size());
    for (size_t i = 0; i < children.size(); ++i)
    {
        min_widths[i] = children[i]->getMinSize().x;
        max_widths[i] = children[i]->getMaxSize().x;
    }
    vector<int> calculated_widths(children.size());
    int total_width = genericArrange(available_area.x, min_widths, max_widths, calculated_widths);
    
    int x_offset = 0;
    for (size_t i = 0; i < children.size(); ++i)
    {
        children[i]->arrange(Vec2{ calculated_widths[i], available_area.y });
        children[i]->setPosition(Vec2{ x_offset, 0 });
        x_offset += calculated_widths[i];
    }
    setSize(Vec2{ total_width, available_area.y });
}


// ArtBlock =================================================

void ArtBlock::render(Context& ctx) const
{
    size_t offset = 0;
    int line = 0;
    while (offset < ascii.size())
    {
        ctx.drawText(Vec2{ 0, line }, ascii, DEFAULT_COLOUR, offset, pitch);
        offset += pitch;
        ++line;
    }
}

void ArtBlock::setData(const std::string& data, const int _pitch)
{ ascii = data; pitch = _pitch; dirty(); }


// SizeLimiter =================================================

void SizeLimiter::arrange(const Vec2 available_area)
{
    setSize(clip(min_size, max_size, available_area));
    if (!children.empty())
    {
        children[0]->arrange(available_area);
        children[0]->setPosition(Vec2{ 0, 0 });
    }
}

void SizeLimiter::render(Context& ctx) const
{
    if (!children.empty())
    {
        const auto c = children[0];
        ctx.pushBounds(c->getTransform().position, c->getTransform().position + c->getTransform().size);
        c->render(ctx);
        ctx.popBounds();
    }
}

void SizeLimiter::setMaxSize(const Vec2& value)
{ max_size = value; }

void SizeLimiter::setMinSize(const Vec2& value)
{ min_size = maxi(Vec2{ 0, 0 }, value); }


// Builder =================================================

Label* Builder::label(const std::string& text)
{ return insertWidget(new Label(text)); }

ArtBlock* Builder::artBlock(const std::string& ascii, const int pitch)
{ return insertWidget(new ArtBlock(ascii, pitch)); }

HorizontalBox* Builder::beginHorizontalBox()
{ return beginWidget(new HorizontalBox({}), -1); }

void Builder::endHorizontalBox()
{ endWidget<HorizontalBox>(); }

VerticalBox* Builder::beginVerticalBox()
{ return beginWidget(new VerticalBox({}), -1); }

void Builder::endVerticalBox()
{ endWidget<VerticalBox>(); }

SizeLimiter* Builder::sizeLimiter(const Vec2& max_size, const Vec2& min_size)
{ return beginWidget(new SizeLimiter(max_size, min_size, nullptr), 1); }

Widget* Builder::end()
{
    Widget* tmp = root;
    reset();
    return tmp;
}

void Builder::reset()
{
    root = new VerticalBox({ });
    current_parent_stack.push_back({ root, -1 });
}
