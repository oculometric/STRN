#pragma once

#include <string>
#include <vector>

#include "util.h"
#include "core.h"
#include "widgets.h"

namespace STRN
{

class Window
{
    friend class Compositor;
private:
    Widget* root = nullptr;
    
    Vec2 size = { 10, 10 };
    Vec2 position = { 5, 5 };
    std::string title = "window";
    bool borderless = false;
    bool is_minimised = false;
    bool allows_minimise = true;
    
    bool is_dirty = true;
    
public:
    Window(const Window& other) = delete;
    Window(Window&& other) = delete;
    void operator=(const Window& other) = delete;
    void operator=(Window&& other) = delete;
    
    Vec2 getSize() const { return size; }
    void setSize(Vec2 value);
    Vec2 getPosition() const { return position; }
    void setPosition(Vec2 value);
    std::string getTitle() const { return title; }
    void setTitle(const std::string& value);
    void setAllowsMinimise(bool value);
    void setBorderless(bool value);
    bool getMinimised() const { return is_minimised; }
    void setRoot(Widget* value);

    void render(Context& ctx);
    void dirty() { is_dirty = true; }
    bool getDirty() const { return is_dirty; }
    
private:
    Window(const std::string& _title, const bool _borderless, const bool _minimised) : title(_title), borderless(_borderless), is_minimised(_minimised) { }
    ~Window() = default;
};

class Compositor
{
private:
    std::vector<Window*> windows;
    Context context;
    Vec2 size;
    
public:
    Compositor(const Compositor& other) = delete;
    Compositor(Compositor&& other) = delete;
    void operator=(const Compositor& other) = delete;
    void operator=(Compositor&& other) = delete;
    virtual ~Compositor() = default;
    
    virtual void update() { }
    Window* newWindow(const std::string& title, bool borderless = false, bool start_minimised = false);
    Vec2 getSize() const { return size; }
    
protected:
    Compositor() { }
    
    void renderWindows();
    const Context& getContext() const { return context; }
    void clearContext();
    void setSize(Vec2 new_size);
}; 

class TerminalCompositor : public Compositor
{
public:
    TerminalCompositor();
    TerminalCompositor(const TerminalCompositor& other) = delete;
    TerminalCompositor(TerminalCompositor&& other) = delete;
    void operator=(const TerminalCompositor& other) = delete;
    void operator=(TerminalCompositor&& other) = delete;
    ~TerminalCompositor() override = default;
    
    static Vec2 getScreenSize();
    static void setCursorVisible(bool visible);
    static void setCursorPosition(Vec2 position);
    
    void update() override;
};

}
