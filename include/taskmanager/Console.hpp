#pragma once

#include <string>

namespace console
{
    enum class Color
    {
        None,
        Red,
        Green,
        Yellow,
        Cyan,
        Gray,
        Bold
    };

    // Enables ANSI colors when the output really is a terminal and the user did
    // not opt out through --no-color or the NO_COLOR environment variable.
    void initialize(bool userWantsColor);

    bool colorsEnabled();

    std::string colorize(const std::string& text, Color color);

    // Colorizes the text and pads it to the requested width, so that escape
    // sequences never break column alignment.
    std::string cell(const std::string& text, std::size_t width, Color color = Color::None);
}
