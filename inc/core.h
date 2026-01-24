#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "util.h"

namespace STRN
{

enum Colour : uint8_t
{
    FG_BLACK        = 0b0000,
    FG_DARK_RED     = 0b0001,
    FG_DARK_GREEN   = 0b0010,
    FG_DARK_YELLOW  = 0b0011,
    FG_DARK_BLUE    = 0b0100,
    FG_DARK_MAGENTA = 0b0101,
    FG_DARK_CYAN    = 0b0110,
    FG_LIGHT_GREY   = 0b0111,
    FG_DARK_GREY    = 0b1000,
    FG_RED          = 0b1001,
    FG_GREEN        = 0b1010,
    FG_YELLOW       = 0b1011,
    FG_BLUE         = 0b1100,
    FG_MAGENTA      = 0b1101,
    FG_CYAN         = 0b1110,
    FG_WHITE        = 0b1111,
    
    BG_BLACK        = FG_BLACK << 4,
    BG_DARK_RED     = FG_DARK_RED << 4,
    BG_DARK_GREEN   = FG_DARK_GREEN << 4,
    BG_DARK_YELLOW  = FG_DARK_YELLOW << 4,
    BG_DARK_BLUE    = FG_DARK_BLUE << 4,
    BG_DARK_MAGENTA = FG_DARK_MAGENTA << 4,
    BG_DARK_CYAN    = FG_DARK_CYAN << 4,
    BG_LIGHT_GREY   = FG_LIGHT_GREY << 4,
    BG_DARK_GREY    = FG_DARK_GREY << 4,
    BG_RED          = FG_RED << 4,
    BG_GREEN        = FG_GREEN << 4,
    BG_YELLOW       = FG_YELLOW << 4,
    BG_BLUE         = FG_BLUE << 4,
    BG_MAGENTA      = FG_MAGENTA << 4,
    BG_CYAN         = FG_CYAN << 4,
    BG_WHITE        = FG_WHITE << 4
};

inline Colour operator|(const Colour a, const Colour b)
{ return static_cast<Colour>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }

inline Colour foreground(const Colour c)
{ return static_cast<Colour>(c & 0b00001111); }

inline Colour background(const Colour c)
{ return static_cast<Colour>(c & 0b11110000); }

inline Colour invert(const Colour c)
{ return static_cast<Colour>((foreground(c) << 4) | (background(c) >> 4)); }

// TODO: make colours a palette instead!
#define DEFAULT_COLOUR (BG_BLACK | FG_WHITE)
#define DEFAULT_INVERTED (FG_BLACK | BG_WHITE)

struct Char
{
    char value = ' ';
    Colour colour_bits = DEFAULT_COLOUR;
    
    Char() = default;
    Char(const char c) : value(c) { }
    Char(const int c) { value = static_cast<char>(c); }
    Char(const char chr, const Colour col) : value(chr), colour_bits(col) { }
};

class Context
{
    std::vector<Char> backing;
    int pitch = 0;
    std::vector<Box2> bounds_stack;
    Box2 permitted_bounds;
    
public:
    Context(Vec2 size, Char fill_value);
    Context() : Context({1, 1}, ' ') { }
    Context(const Context& other) = delete;
    Context(Context&& other) = delete;
    void operator=(const Context& other) = delete;
    void operator=(Context&& other) = delete;
    ~Context() = default;
    
    Vec2 getSize() const { return permitted_bounds.size(); }
    void draw(Vec2 position, Char value);
    void drawBox(Vec2 start, Vec2 size);
    void fill(Vec2 start, Vec2 size, Char value);
    void drawText(Vec2 start, const std::string& text, Colour colour = DEFAULT_COLOUR, size_t text_offset = 0, size_t max_length = -1);
    
    std::vector<Char>::const_iterator begin() const;
    std::vector<Char>::const_iterator end() const;
    
    void pushBounds(const Vec2& min, const Vec2& max);
    void popBounds();
    
    void clear(Char fill_value);
    void resize(Vec2 new_size, Char fill_value);
};

}