#ifndef SAT_UI_THEME_H
#define SAT_UI_THEME_H

#include <SFML/Graphics/Color.hpp>

struct UITheme {
    sf::Color background{18, 24, 34};
    sf::Color panel{29, 39, 52};
    sf::Color panelAlt{35, 48, 64};
    sf::Color primary{35, 91, 112};
    sf::Color accent{75, 190, 220};
    sf::Color text{232, 240, 244};
    sf::Color muted{165, 184, 194};
    sf::Color success{42, 145, 94};
    sf::Color danger{145, 52, 59};
    float padding = 24.f;
    float gap = 16.f;
    float radius = 8.f;
    unsigned int titleSize = 28;
    unsigned int bodySize = 15;
};

#endif
