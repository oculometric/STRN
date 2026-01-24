#include "core.h"

#include <stdexcept>

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

void Context::draw(const Vec2 position, char value, const int palette_colour)
{ draw(position, { value, getPaletteColour(palette_colour) }); }

// TODO: make this actually platform-independent https://stackoverflow.com/questions/4494306/drawing-tables-in-terminal-using-ansi-box-characters
void Context::drawBox(const Vec2 start, const Vec2 size, const int palette_colour)
{
    const Colour col = getPaletteColour(palette_colour);
    if (size.x < 2 || size.y < 2)
        return;
    draw(start, { (char)0xDA, col });
    for (int a = start.x + 1; a < start.x + size.x - 1; ++a)
    {
        draw(Vec2{ a, start.y }, { (char)0xC4, col });
        draw(Vec2{ a, start.y + size.y - 1 }, { (char)0xC4, col });
    }
    draw(start + Vec2{ size.x - 1, 0 }, { (char)0xBF, col });
    draw(start + Vec2{ 0, size.y - 1 }, { (char)0xC0, col });
    for (int a = start.y + 1; a < start.y + size.y - 1; ++a)
    {
        draw(Vec2{ start.x, a }, { (char)0xB3, col });
        draw(Vec2{ start.x + size.x - 1, a }, { (char)0xB3, col });
    }
    draw(start + size - Vec2{ 1, 1 }, { (char)0xD9, col });
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

void Context::fill(const Vec2 start, const Vec2 size, char value, const int palette_colour)
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
        backing[offset] = { text[text_pos], colour };
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

#include <iostream>
#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <poll.h>
#endif

TerminalRasteriser::TerminalRasteriser()
{
    std::ios_base::sync_with_stdio(false);
#if defined(_WIN32)
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
#elif defined(__linux__)
    termios new_termios;
    tcgetattr(STDIN_FILENO, &new_termios);

    new_termios.c_iflag &= ~(IGNBRK | BRKINT | IXON);
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VSUSP] = 255;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
#endif
    setSize(getScreenSize());
    setCursorVisible(false);
}

Vec2 TerminalRasteriser::getScreenSize()
{
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return Vec2{ (info.srWindow.Right - info.srWindow.Left) + 1, (info.srWindow.Bottom - info.srWindow.Top) + 1 };
#elif defined(__linux__)
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    return Vec2{ (int)size.ws_col, (int)size.ws_row };
#endif
}

void TerminalRasteriser::setCursorVisible(const bool visible)
{
#if defined(_WIN32)
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    info.bVisible = visible;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
#elif defined(__linux__)
    if (visible) fputs("\033[?25h", stdout);
    else fputs(std::cout << "\033[?25l", stdout);
#endif
}

void TerminalRasteriser::setCursorPosition(const Vec2 position)
{ printf("\033[%i;%iH", position.y, position.x); }

void TerminalRasteriser::update()
{
    setSize(getScreenSize());
    // TODO: handle input
}

static const char* getANSIColourFromBits(const Colour c, const bool high)
{
    static const char* low_table[16] = 
    {
        "30", "31", "32", "33", "34", "35", "36", "37",
        "90", "91", "92", "93", "94", "95", "96", "97"
    };
    static const char* high_table[16] =
    {
        "40", "41", "42", "43", "44", "45", "46", "47",
        "100", "101", "102", "103", "104", "105", "106", "107"
    };
    if (!high)
        return low_table[c];
    return high_table[c >> 4];
}

void TerminalRasteriser::present()
{
    std::string result;
    result.reserve(static_cast<size_t>(getSize().x) * static_cast<size_t>(getSize().y));
    Colour last_colour_bits = BG_BLACK;
    Colour last_high_bits = BG_BLACK;
    Colour last_low_bits = BG_BLACK;
    bool first_pass = true;
    for (const auto it : getContext())
    {
        if (first_pass || (last_colour_bits != it.colour_bits))
        {
            result.push_back('\033');
            result.push_back('[');
            const Colour high_bits = static_cast<Colour>(it.colour_bits & 0xF0);
            const Colour low_bits = static_cast<Colour>(it.colour_bits & 0x0F);
            if (first_pass || (last_high_bits != high_bits))
                result.append(getANSIColourFromBits(high_bits, true));
            if (first_pass || (last_high_bits != high_bits && last_low_bits != low_bits))
            {
                result.push_back(';');
                last_high_bits = high_bits;
            }
            if (first_pass || (last_low_bits != low_bits))
            {
                result.append(getANSIColourFromBits(low_bits, false));
                last_low_bits = low_bits;
            }
            last_colour_bits = it.colour_bits;
            result.push_back('m');
            first_pass = false;
        }
        result.push_back(it.value);
    }
    setCursorVisible(false);
    fputs(result.c_str(), stdout);
    setCursorPosition({ 0, 0 });
}