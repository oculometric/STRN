#include "core.h"

#include <stdexcept>
#include <string>
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
