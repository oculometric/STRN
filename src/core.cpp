#include "core.h"

#include <stdexcept>
#include <string>

using namespace STRN;
using namespace std;

Context::Context(const Vec2 size, const Char fill_value)
{
    if (size.x <= 0 || size.y <= 0)
        throw invalid_argument("context size must be greater than zero");
    backing.resize(static_cast<size_t>(size.x) * static_cast<size_t>(size.y), fill_value);
    pitch = size.x;
    permitted_bounds = { { 0, 0 }, size };
}

void Context::draw(const Vec2 position, const Char value)
{
    const Vec2 real_position = position + permitted_bounds.min;
    if (real_position.x >= permitted_bounds.max.x
        || real_position.y >= permitted_bounds.max.y)
        return;
    const size_t offset = real_position.x + (real_position.y * pitch);
    backing[offset] = value;
}

void Context::draw(const Vec2 position, unsigned char value, const int palette_colour)
{ draw(position, { value, getPaletteColour(palette_colour) }); }

void Context::drawBox(const Vec2 start, const Vec2 size, const int palette_colour)
{
    const Colour col = getPaletteColour(palette_colour);
    if (size.x < 2 || size.y < 2)
        return;
    draw(start, 0xDA, col);
    for (int a = start.x + 1; a < start.x + size.x - 1; ++a)
    {
        draw(Vec2{ a, start.y }, 0xC4, col);
        draw(Vec2{ a, start.y + size.y - 1 }, 0xC4, col);
    }
    draw(start + Vec2{ size.x - 1, 0 }, 0xBF, col);
    draw(start + Vec2{ 0, size.y - 1 }, 0xC0, col);
    for (int a = start.y + 1; a < start.y + size.y - 1; ++a)
    {
        draw(Vec2{ start.x, a }, 0xB3, col);
        draw(Vec2{ start.x + size.x - 1, a }, 0xB3, col);
    }
    draw(start + size - Vec2{ 1, 1 }, 0xD9, col);
}

void Context::fill(const Vec2 start, const Vec2 size, const Char value)
{
    if (size.x <= 0 || size.y <= 0)
        return;
    const Vec2 actual_start = maxi(permitted_bounds.min, start + permitted_bounds.min);
    const Vec2 end = mini(permitted_bounds.max, start + permitted_bounds.min + size);
    const Vec2 actual_size = end - actual_start;
    if (actual_size.x <= 0)
        return;
    
    size_t offset = actual_start.x + (actual_start.y * pitch);
    size_t line_start = offset;
    for (int y = 0; y < actual_size.y; ++y)
    {
        for (int x = 0; x < actual_size.x; ++x, ++offset)
            backing[offset] = value;
        line_start += pitch;
        offset = line_start;
    }
}

void Context::fill(const Vec2 start, const Vec2 size, unsigned char value, const int palette_colour)
{ fill(start, size, { value, getPaletteColour(palette_colour) }); }

void Context::drawText(const Vec2 start, const string& text, Colour colour, size_t text_offset, size_t max_length)
{
    if (start.x + permitted_bounds.min.x >= permitted_bounds.max.x)
        return;
    if (start.y < 0 || start.y + permitted_bounds.min.y >= permitted_bounds.max.y)
        return;
    
    Vec2 actual_start = start;
    if (actual_start.x < 0)
    {
        text_offset += -actual_start.x;
        max_length -= -actual_start.x;
        actual_start.x = 0;
    }
    actual_start += permitted_bounds.min;
    const size_t max_bound = static_cast<size_t>(permitted_bounds.max.x);
    max_length = min(max_length, max_bound);
    if (static_cast<size_t>(actual_start.x) + max_length > max_bound)
        max_length -= static_cast<size_t>(actual_start.x) + max_length - max_bound;
    
    size_t offset = actual_start.x + (actual_start.y * pitch);
    
    for (size_t text_pos = text_offset; text_pos < text_offset + max_length; ++text_pos)
    {
        if (text_pos >= text.size())
            return;
        if (text[text_pos] == '\n')
            return;
        backing[offset] = { static_cast<unsigned char>(text[text_pos]), colour };
        ++offset;
    }
}

void Context::drawText(const Vec2 start, const std::string& text, const int palette_colour, const size_t text_offset, const size_t max_length)
{ drawText(start, text, getPaletteColour(palette_colour), text_offset, max_length); }

vector<Char>::const_iterator Context::begin() const
{
    return backing.begin();
}

vector<Char>::const_iterator Context::end() const
{
    return backing.end();
}

void Context::pushBounds(const Vec2& min, const Vec2& max)
{
    bounds_stack.push_back(permitted_bounds);
    
    permitted_bounds = { maxi(min, 0) + permitted_bounds.min, mini(max + permitted_bounds.min, permitted_bounds.max) };
}

void Context::popBounds()
{
    if (bounds_stack.empty())
        permitted_bounds = { Vec2{ 0, 0 }, Vec2{ pitch, static_cast<int>(backing.size() / pitch) } };
    else
    {
        permitted_bounds = *(bounds_stack.end() - 1);
        bounds_stack.pop_back();
    }
}

void Context::pushPalette(const Palette& p)
{
    palette_stack.push_back(p);
}

void Context::popPalette()
{
    if (!palette_stack.empty())
        palette_stack.pop_back();
}

void Context::setBasePalette(const Palette& p)
{
    base_palette = p;
}

Colour Context::getPaletteColour(int c) const
{
    if (palette_stack.empty())
        return base_palette[c];
    return (palette_stack[palette_stack.size() - 1])[c];
}

void Context::clear(const Char fill_value)
{
    for (auto& it : backing)
        it = fill_value;
}

void Context::resize(const Vec2 new_size, const Char fill_value)
{
    if (new_size.x <= 0 || new_size.y <= 0)
        throw invalid_argument("context size must be greater than zero");
    backing.resize(static_cast<size_t>(new_size.x) * static_cast<size_t>(new_size.y), fill_value);
    pitch = new_size.x;
    permitted_bounds = { { 0, 0 }, new_size };
}

void Drawable::setSize(const Vec2 value)
{ transform.size = value; }

void Drawable::setPosition(const Vec2 value)
{ transform.position = value; }

void Rasteriser::render()
{
    vector<Drawable*> commands;
    commands.insert(commands.end(), drawables.begin(), drawables.end());
    sort(commands.begin(), commands.end(), 
        [](const Drawable* a, const Drawable* b) -> bool { return a->getTransform().z < b->getTransform().z; });
    context.clear(clear_value);
    for (const auto it : commands)
    {
        auto trans = it->getTransform();
        context.pushBounds(trans.position, trans.position + trans.size);
        it->render(context);
        context.popBounds();
    }
}

void Rasteriser::setSize(Vec2 new_size)
{
    if (size == new_size)
        return;
    size = new_size;
    context.resize(new_size, clear_value);
    clearContext();
}

bool Rasteriser::insertDrawable(Drawable* d)
{
    if (drawables.count(d) > 0)
        return false;
    drawables.insert(d);
    return true;
}

bool Rasteriser::eraseDrawable(Drawable* d)
{
    if (drawables.count(d) == 0)
        return false;
    drawables.erase(d);
    return true;
}

void Rasteriser::setClearValue(const Char value)
{ clear_value = value; }

void Rasteriser::setPalette(const Palette& p)
{ context.setBasePalette(p); }

void Rasteriser::clearContext()
{ context.clear(clear_value); }
