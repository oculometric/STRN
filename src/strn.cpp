#include "strn.h"

#include <stdexcept>
#include <iostream>
#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
	#include <sys/ioctl.h>
	#include <signal.h>
	#include <termios.h>
	#include <poll.h>
#endif

using namespace STRN;
using namespace std;

void Window::setSize(const Vec2 value)
{ size = value; dirty(); }

void Window::setPosition(const Vec2 value)
{ position = value; dirty(); }

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
    Vec2 content_area = size;
    Vec2 content_start = position;
    if (!borderless)
    {
        ctx.drawBox(position, size);
        ctx.fill(position + Vec2{ 1, size.y - 1 }, Vec2{ size.x - 2, 1 }, { ' ', DEFAULT_INVERTED });
        ctx.drawText(position + Vec2{ 2, size.y - 1 }, title, DEFAULT_INVERTED, 0, size.x - 3);
        ctx.draw(position + Vec2{ size.x - 2, size.y - 1 }, { (char)0xFE, DEFAULT_INVERTED });
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
    
    is_dirty = false;
}

#pragma region COMPOSITOR

Window* Compositor::newWindow(const std::string& title, const bool borderless, const bool start_minimised)
{
    Window* window = new Window(title, borderless, start_minimised);
    windows.push_back(window);
    
    return window;
}

void Compositor::renderWindows()
{
    for (Window* window : windows)
        if (window->getDirty() && !window->getMinimised())
            window->render(context);
}

void Compositor::clearContext()
{ context.clear({ ' ' }); }

void Compositor::setSize(const Vec2 new_size)
{
    if (size == new_size)
        return;
    size = new_size;
    context.resize(new_size, { ' ' });
    clearContext();
    for (const auto window : windows)
        window->dirty();
}

TerminalCompositor::TerminalCompositor()
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

Vec2 TerminalCompositor::getScreenSize()
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

void TerminalCompositor::setCursorVisible(const bool visible)
{
#if defined(_WIN32)
        CONSOLE_CURSOR_INFO info;
        GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
        info.bVisible = visible;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
#elif defined(__linux__)
        if (visible) std::cout << "\033[?25h";
        else std::cout << "\033[?25l";
#endif
}

void TerminalCompositor::setCursorPosition(const Vec2 position)
{ printf("\033[%i;%iH", position.y, position.x); }

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

void TerminalCompositor::update()
{
    // TODO: the base compositor/rasteriser should rasterise all the renderables (ie execute draw commands), *then* the platform-specific stuff should happen
    // TODO: only clear areas which are going to be updated!
    //clearContext();
    setSize(getScreenSize());
    setCursorVisible(false);
    // TODO: handle input
    renderWindows();
    
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
    fputs(result.c_str(), stdout);
    setCursorPosition({ 0, 0 });
}

#pragma endregion
