#include <chrono>
#include <iostream>
#include <thread>

#include "strn.h"

const char* saturn_ascii = 
" .             "
" .##           "
"  #  ..        "
"     #####-    "
"   #########   "
"   ###.#####.  "
"    ###-####   "
"     .### .    "
"          # -# "
"            ## ";

int main()
{
    STRN::NativeRasteriser comp;
    STRN::Window* window = new STRN::Window("performance", false, false);
    comp.insertDrawable(window);
    window->setSize({ 20, 4 });
    STRN::Window* window2 = new STRN::Window("lucylucy", false, false);
    comp.insertDrawable(window2);
    window2->setPosition({ 16, 8 });
    window2->setSize({ 26, 6 });

    STRN::Window* window3 = new STRN::Window("big guy", false, false);
    comp.insertDrawable(window3);
    window3->setPosition({ 50, 2 });
    window3->setSize({ 60, 16 });
    STRN::Builder w3_builder;
    w3_builder.beginHorizontalBox();
    w3_builder.artBlock(saturn_ascii, 15);
    w3_builder.beginVerticalBox();
    w3_builder.label("i didnt want your little bitchass anyway!~");
    w3_builder.label("busy woman");
    w3_builder.label("but if you need my love");
    w3_builder.sizeLimiter(STRN::Vec2{ 10, 1 }, STRN::Vec2{ 1, 1 });
    w3_builder.label("my clothes are off im coming over to your place");
    w3_builder.label("the quick brown fox jumps over the lazy dog. oh yeah, and shes a really cute fox too actually!");
    w3_builder.endVerticalBox();
    w3_builder.endHorizontalBox();
    window3->setRoot(w3_builder.end());
    
    STRN::Node* node = new STRN::Node("blibblob", 
        { 
            { "an input", STRN::Node::ELEMENT_INPUT },
            { "another input", STRN::Node::ELEMENT_INPUT },
            { "output!", STRN::Node::ELEMENT_OUTPUT },
            { "spacer?", STRN::Node::ELEMENT_SPACE },
            { "block", STRN::Node::ELEMENT_BLOCK },
            { "just a bit of text", STRN::Node::ELEMENT_TEXT },
            { "final output", STRN::Node::ELEMENT_OUTPUT },
        });
    comp.insertDrawable(node);
    node->setPosition({ 3, 16 });
    
    STRN::Label perf_label("fps: 0");
    STRN::Label perf_label2("dt: 0");
    window->setRoot(new STRN::VerticalBox({ &perf_label, &perf_label2 }));
    
    constexpr std::chrono::duration<double> ideal_frame_time(1.0 / 60.0);
    
    while (true)
    {
        auto last = std::chrono::high_resolution_clock::now();
        if (comp.update())
            break;
        STRN::KeyEvent key = comp.getKeyEvent();
        while (key.key != 0)
        {
            if (/*key.key >= 'A' && key.key <= 'Z' && */key.pressed)
                window2->setTitle(window2->getTitle() + (char)key.key);
            key = comp.getKeyEvent();
        }
        comp.render();
        comp.present();
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = now - last;        
        perf_label.setText("fps: " + std::to_string(1.0 / duration.count()));
        perf_label2.setText("dt: " + std::to_string(duration.count() * 1000.0) + "ms");
        auto sleep_duration = ideal_frame_time - duration;
        std::this_thread::sleep_for(sleep_duration); // prevents us from spinning too hard
    }
}