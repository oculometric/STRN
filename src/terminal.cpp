#include "core.h"

#include <stdexcept>
#include <string>
#include <iostream>
#include <map>
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
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_EXTENDED_FLAGS);
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
    else fputs("\033[?25l", stdout);
#endif
}

void TerminalRasteriser::setCursorPosition(const Vec2 position)
{ printf("\033[%i;%iH", position.y, position.x); }

bool TerminalRasteriser::update()
{
    setSize(getScreenSize());

#if defined(_WIN32)
    // find available
    DWORD events_available;
    GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &events_available);
    if (events_available < 1) return false;

    // grab 32 of them
    INPUT_RECORD records[32] = { };
    DWORD records_read;

    if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), records, 32, &records_read) == 0)
        throw runtime_error("input error");

    for (size_t i = 0; i < records_read; ++i)
    {
        if (records[i].EventType == KEY_EVENT)
        { // && records[i].Event.KeyEvent.bKeyDown
            auto unicode = records[i].Event.KeyEvent.uChar.UnicodeChar;
            if (!(unicode == '\r' || unicode == '\t' || unicode < 32 || unicode == 127 || records[i].Event.KeyEvent.dwControlKeyState > VK_SHIFT))
            {
                // if the event is a unicode character, send it to the char queue
                if (records[i].Event.KeyEvent.bKeyDown)
                    pending_chars.push(unicode);
            }

            // otherwise send it to the keyevent queue
            static map<WORD, int> key_code_map = {
                { 13, 257 },
                { 27, 256 },
                { 37, 263 },
                { 38, 265 },
                { 40, 264 },
                { 39, 262 },
                { 8, 259 },
                { 46, 261 },
                { 220, '\\' },
                { 3, 259 },
            };
            KeyEvent evt;
            evt.pressed = records[i].Event.KeyEvent.bKeyDown;
            evt.key = records[i].Event.KeyEvent.wVirtualKeyCode;
            // process special characters to make sure they appear in the right way
            if (key_code_map.find(evt.key) != key_code_map.end())
                evt.key = key_code_map[evt.key];

            // process modifiers
            DWORD control_key = records[i].Event.KeyEvent.dwControlKeyState;
            if (control_key &       0b11) evt.modifiers = (KeyEvent::Modifier)(evt.modifiers | KeyEvent::ALT);
            if (control_key &     0b1100) evt.modifiers = (KeyEvent::Modifier)(evt.modifiers | KeyEvent::CTRL);
            if (control_key &    0b10000) evt.modifiers = (KeyEvent::Modifier)(evt.modifiers | KeyEvent::SHIFT);
            if (control_key &    0b10000) evt.modifiers = (KeyEvent::Modifier)(evt.modifiers | KeyEvent::SHIFT);
            if (control_key &   0b100000) evt.modifiers = (KeyEvent::Modifier)(evt.modifiers | KeyEvent::NUM);
            if (control_key & 0b10000000) evt.modifiers = (KeyEvent::Modifier)(evt.modifiers | KeyEvent::CAPS);
            pending_keys.push(evt);
        }
    }
#endif

    // FIXME: linux input not supported!!

    return false; // TODO: should return true if CTRL+C is pressed
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
        if (it.value != 0)
            result.push_back(it.value);
        else
            result.push_back(' ');
    }
    setCursorVisible(false);
    setCursorPosition({ 0, 0 });
    fputs(result.c_str(), stdout);
}
