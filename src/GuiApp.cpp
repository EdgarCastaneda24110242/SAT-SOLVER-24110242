#include "GuiApp.h"

#include "AtomTable.h"
#include "Parser.h"
#include "SATSolver.h"
#include "SoundManager.h"
#include "TextInput.h"
#include "UITheme.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <optional>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
struct Analysis {
    FormulaPtr formula;
    SATResult result;
    std::string naturalFormula;
    std::string error;
};

std::string naturalFormula(const FormulaPtr& formula, const AtomTable& atomTable) {
    const std::vector<FormulaPtr> children = formula->graphChildren();
    if (children.empty()) return "(" + atomTable.text(formula->graphLabel()) + ")";
    if (children.size() == 1) return "¬" + naturalFormula(children[0], atomTable);
    const std::string label = formula->graphLabel();
    const std::string symbol = label == "∧" ? " ∧ " :
                               label == "∨" ? " ∨ " :
                               label == "→" ? " → " : " ↔ ";
    return "(" + naturalFormula(children[0], atomTable) + symbol +
           naturalFormula(children[1], atomTable) + ")";
}

void drawText(sf::RenderWindow& window, const sf::Font& font, const std::string& value,
              float x, float y, unsigned int size, sf::Color color) {
    sf::Text text(font, sf::String::fromUtf8(value.begin(), value.end()), size);
    text.setPosition({x, y});
    text.setFillColor(color);
    window.draw(text);
}

void drawTreeEdge(sf::RenderWindow& window, sf::Vector2f from, sf::Vector2f to, sf::Color color) {
    sf::VertexArray edge(sf::PrimitiveType::Lines, 2);
    edge[0].position = from;
    edge[0].color = color;
    edge[1].position = to;
    edge[1].color = color;
    window.draw(edge);
}

std::string assignmentText(const SemanticTree::Assignment& assignment) {
    std::string text;
    for (const auto& [atom, value] : assignment) {
        if (!text.empty()) text += ", ";
        text += atom + "=" + (value ? "V" : "F");
    }
    return text;
}

void drawSemanticTreeNode(sf::RenderWindow& window, const sf::Font& font,
                          const SemanticTree::NodePtr& node, float x, float y,
                          float spread, float scale, const sf::Vector2f& offset) {
    if (!node) return;
    const float screenX = offset.x + x * scale;
    const float screenY = offset.y + y * scale;
    const float radius = (node->isLeaf ? 23.f : 25.f) * scale;
    if (node->isLeaf) {
        sf::CircleShape leaf(radius);
        leaf.setOrigin({radius, radius});
        leaf.setPosition({screenX, screenY});
        leaf.setFillColor(node->open ? sf::Color(35, 122, 82) : sf::Color(105, 57, 64));
        leaf.setOutlineThickness(std::max(1.f, 2.f * scale));
        leaf.setOutlineColor(node->open ? sf::Color(120, 235, 165) : sf::Color(220, 125, 135));
        window.draw(leaf);
        drawText(window, font, node->open ? "V" : "X", screenX - 7.f * scale,
                 screenY - 10.f * scale, static_cast<unsigned int>(std::max(10.f, 15.f * scale)),
                 sf::Color::White);
        drawText(window, font, assignmentText(node->assignment), screenX + radius + 6.f,
                 screenY - 8.f * scale, static_cast<unsigned int>(std::max(9.f, 12.f * scale)),
                 node->open ? sf::Color(180, 245, 205) : sf::Color(225, 175, 180));
        return;
    }

    const float childY = y + 76.f;
    const float childSpread = std::max(58.f, spread * .54f);
    const float leftX = x - childSpread;
    const float rightX = x + childSpread;
    const float childRadius = 25.f * scale;
    drawTreeEdge(window, {screenX, screenY + radius}, {offset.x + leftX * scale, offset.y + childY * scale - childRadius},
                 sf::Color(125, 135, 145));
    drawTreeEdge(window, {screenX, screenY + radius}, {offset.x + rightX * scale, offset.y + childY * scale - childRadius},
                 sf::Color(125, 135, 145));

    sf::CircleShape treeNode(25.f * scale);
    treeNode.setOrigin({25.f * scale, 25.f * scale});
    treeNode.setPosition({screenX, screenY});
    treeNode.setFillColor(sf::Color(35, 91, 112));
    treeNode.setOutlineThickness(std::max(1.f, 2.f * scale));
    treeNode.setOutlineColor(sf::Color(210, 225, 230));
    window.draw(treeNode);
    drawText(window, font, node->atom, screenX - 12.f * scale, screenY - 11.f * scale,
             static_cast<unsigned int>(std::max(10.f, 15.f * scale)), sf::Color::White);
    drawText(window, font, "F", offset.x + leftX * scale - 24.f * scale,
             screenY + 27.f * scale, static_cast<unsigned int>(std::max(10.f, 14.f * scale)),
             sf::Color(220, 220, 225));
    drawText(window, font, "V", offset.x + rightX * scale + 10.f * scale,
             screenY + 27.f * scale, static_cast<unsigned int>(std::max(10.f, 14.f * scale)),
             sf::Color(220, 220, 225));
    drawSemanticTreeNode(window, font, node->falseBranch, leftX, childY, childSpread, scale, offset);
    drawSemanticTreeNode(window, font, node->trueBranch, rightX, childY, childSpread, scale, offset);
}

Analysis analyze(const std::string& input, AtomTable& atomTable, Parser& parser, SATSolver& solver) {
    Analysis analysis;
    try {
        const bool symbolic = input.find('&') != std::string::npos || input.find('|') != std::string::npos ||
                              input.find("->") != std::string::npos || input.find('!') != std::string::npos ||
                              input.find('~') != std::string::npos;
        analysis.formula = symbolic ? parser.parseSymbolic(input) : parser.parseNaturalLanguage(input);
        analysis.result = solver.solve(*analysis.formula);
        analysis.naturalFormula = naturalFormula(analysis.formula, atomTable);
    } catch (const std::exception& exception) {
        analysis.error = exception.what();
    }
    return analysis;
}

bool contains(const sf::FloatRect& rectangle, sf::Vector2i position) {
    return rectangle.contains({static_cast<float>(position.x), static_cast<float>(position.y)});
}

void drawRoundedRectangle(sf::RenderWindow& window, const sf::FloatRect& bounds,
                          float radius, sf::Color color) {
    const float clampedRadius = std::min(radius, std::min(bounds.size.x, bounds.size.y) * .5f);
    sf::RectangleShape center({bounds.size.x - clampedRadius * 2.f, bounds.size.y});
    center.setPosition({bounds.position.x + clampedRadius, bounds.position.y});
    center.setFillColor(color);
    window.draw(center);
    sf::RectangleShape middle({bounds.size.x, bounds.size.y - clampedRadius * 2.f});
    middle.setPosition({bounds.position.x, bounds.position.y + clampedRadius});
    middle.setFillColor(color);
    window.draw(middle);
    const std::array<sf::Vector2f, 4> centers{{
        {bounds.position.x + clampedRadius, bounds.position.y + clampedRadius},
        {bounds.position.x + bounds.size.x - clampedRadius, bounds.position.y + clampedRadius},
        {bounds.position.x + clampedRadius, bounds.position.y + bounds.size.y - clampedRadius},
        {bounds.position.x + bounds.size.x - clampedRadius, bounds.position.y + bounds.size.y - clampedRadius}
    }};
    for (const sf::Vector2f centerPosition : centers) {
        sf::CircleShape corner(clampedRadius, 24);
        corner.setOrigin({clampedRadius, clampedRadius});
        corner.setPosition(centerPosition);
        corner.setFillColor(color);
        window.draw(corner);
    }
}

void drawShadow(sf::RenderWindow& window, const sf::FloatRect& bounds, float radius) {
    drawRoundedRectangle(window,
        {{bounds.position.x + 3.f, bounds.position.y + 5.f}, bounds.size},
        radius, sf::Color(0, 0, 0, 70));
}

void drawButton(sf::RenderWindow& window, const sf::Font& font, const sf::FloatRect& bounds,
                const std::string& label, sf::Vector2i mousePosition, bool pressed) {
    const bool hovered = contains(bounds, mousePosition);
    drawShadow(window, bounds, 8.f);
    drawRoundedRectangle(window, bounds, 8.f,
                         pressed ? sf::Color(78, 42, 49) :
                         hovered ? sf::Color(155, 72, 81) : sf::Color(120, 55, 62));
    drawText(window, font, label, bounds.position.x + 13.f, bounds.position.y + 6.f, 13, sf::Color::White);
}

void drawGradientPanel(sf::RenderWindow& window, const sf::FloatRect& bounds,
                       sf::Color top, sf::Color bottom) {
    drawShadow(window, bounds, 12.f);
    drawRoundedRectangle(window, bounds, 12.f, bottom);
    const sf::FloatRect highlight(
        {bounds.position.x + 1.f, bounds.position.y + 1.f},
        {bounds.size.x - 2.f, std::max(1.f, bounds.size.y * .38f)});
    drawRoundedRectangle(window, highlight, 11.f,
                         sf::Color(top.r, top.g, top.b, 150));
}

std::filesystem::path dataDirectory(int argc, char** argv) {
    const std::filesystem::path working = std::filesystem::current_path() / "data";
    if (std::filesystem::exists(working)) return working;
    if (argc > 0 && argv && argv[0]) {
        const std::filesystem::path executable = std::filesystem::absolute(argv[0]).parent_path();
        const std::filesystem::path beside = executable / "data";
        if (std::filesystem::exists(beside)) return beside;
        const std::filesystem::path parent = executable / ".." / "data";
        if (std::filesystem::exists(parent)) return std::filesystem::weakly_canonical(parent);
    }
    std::filesystem::create_directories(working);
    return working;
}

bool savePhrases(const std::filesystem::path& path, const std::vector<std::string>& phrases) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    for (const std::string& phrase : phrases) output << phrase << '\n';
    return static_cast<bool>(output);
}

std::vector<std::string> loadPhrases(const std::filesystem::path& path) {
    std::vector<std::string> phrases;
    std::ifstream input(path, std::ios::binary);
    std::string phrase;
    while (std::getline(input, phrase)) if (!phrase.empty()) phrases.push_back(phrase);
    return phrases;
}
}

int GuiApp::run(int argc, char** argv) {
    sf::RenderWindow window(sf::VideoMode({1200, 900}), "SAT Solver", sf::Style::Default);
    window.setFramerateLimit(60);
    const UITheme theme;
    bool fullscreen = false;

    sf::Font font;
    const bool fontLoaded = font.openFromFile("assets/OpenSans.ttf");
    if (!fontLoaded) return 1;

    AtomTable atomTable;
    Parser parser(atomTable);
    SATSolver solver;
    SoundManager soundManager;
    Analysis analysis;
    std::vector<std::string> phrases;
    TextInput textInput(font, 17);
    textInput.setBounds(sf::FloatRect({30.f, 98.f}, {1140.f, 44.f}));
    std::string status = "Escribe una frase y presiona Enter";
    float phraseScroll = 0.f;
    std::optional<std::size_t> selectedPhrase;
    const std::filesystem::path data = dataDirectory(argc, argv);
    const std::filesystem::path phrasesPath = data / "frases.txt";
    const std::filesystem::path atomsPath = data / "atomos.txt";
    atomTable.load(atomsPath.string());
    phrases = loadPhrases(phrasesPath);
    for (const std::string& phrase : phrases) {
        Analysis loaded = analyze(phrase, atomTable, parser, solver);
        if (loaded.error.empty()) analysis = std::move(loaded);
    }
    status = "Se cargaron " + std::to_string(phrases.size()) + " frases";
    bool confirmClear = false;
    sf::Vector2i mousePosition;
    bool mousePressed = false;
    bool treeDragging = false;
    sf::Vector2i lastTreeMouse;
    float treeZoom = 0.72f;
    sf::Vector2f treeOffset{0.f, 0.f};

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                window.setView(sf::View(sf::FloatRect({0.f, 0.f},
                    {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)})));
            }
            if (const auto* moved = event->getIf<sf::Event::MouseMoved>())
                mousePosition = moved->position;
            if (treeDragging) {
                treeOffset += sf::Vector2f(
                    static_cast<float>(mousePosition.x - lastTreeMouse.x),
                    static_cast<float>(mousePosition.y - lastTreeMouse.y));
                lastTreeMouse = mousePosition;
            }
            if (const auto* button = event->getIf<sf::Event::MouseButtonPressed>()) {
                mousePosition = button->position;
                if (button->button == sf::Mouse::Button::Left) mousePressed = true;
                if (button->button == sf::Mouse::Button::Middle) {
                    treeDragging = true;
                    lastTreeMouse = button->position;
                }
            }
            if (const auto* released = event->getIf<sf::Event::MouseButtonReleased>()) {
                mousePosition = released->position;
                if (released->button == sf::Mouse::Button::Left) mousePressed = false;
                if (released->button == sf::Mouse::Button::Middle) treeDragging = false;
            }
            if (const auto* text = event->getIf<sf::Event::TextEntered>()) {
                textInput.handleTextEntered(text->unicode);
                soundManager.playKey();
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) window.close();
                if (key->code == sf::Keyboard::Key::F11) {
                    fullscreen = !fullscreen;
                    window.create(sf::VideoMode::getDesktopMode(), "SAT Solver",
                                  sf::Style::Default,
                                  fullscreen ? sf::State::Fullscreen : sf::State::Windowed);
                    window.setFramerateLimit(60);
                }
                textInput.handleKeyPressed(*key);
                if (textInput.consumeSubmit() && !textInput.empty()) {
                    const std::string input = textInput.takeValue();
                    analysis = analyze(input, atomTable, parser, solver);
                    if (analysis.error.empty()) {
                        soundManager.playSubmit();
                        if (selectedPhrase.has_value()) {
                            phrases[*selectedPhrase] = input;
                            selectedPhrase.reset();
                        } else {
                            phrases.push_back(input);
                        }
                        savePhrases(phrasesPath, phrases);
                        atomTable.save(atomsPath.string());
                        status = "Frase procesada correctamente";
                        if (analysis.result.satisfiable) soundManager.playSuccess();
                        else soundManager.playError();
                    } else {
                        status = "Error: " + analysis.error;
                        soundManager.playError();
                    }
                }
            }
            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>())
                textInput.handleMousePressed({static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)});
            if (const auto* mouse = event->getIf<sf::Event::MouseMoved>())
                textInput.handleMouseMoved({static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)},
                                           sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));
            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                const float currentWidth = static_cast<float>(window.getSize().x);
                const float currentHeight = static_cast<float>(window.getSize().y);
                const float currentLeftWidth = std::clamp(currentWidth * .29f, 300.f, 430.f);
                const float currentContentHeight = std::max(240.f, currentHeight - 168.f - 48.f);
                const sf::FloatRect phraseBounds({24.f, 200.f}, {currentLeftWidth, currentContentHeight - 95.f});
                const sf::FloatRect deleteBounds({24.f, 168.f + currentContentHeight - 40.f}, {140.f, 30.f});
                const sf::FloatRect clearBounds({174.f, 168.f + currentContentHeight - 40.f}, {150.f, 30.f});
                const sf::FloatRect muteBounds({currentWidth - 250.f, 16.f}, {92.f, 28.f});
                const sf::FloatRect volumeDownBounds({currentWidth - 150.f, 16.f}, {28.f, 28.f});
                const sf::FloatRect volumeUpBounds({currentWidth - 116.f, 16.f}, {28.f, 28.f});
                if (mouse->button == sf::Mouse::Button::Left && mouse->position.x >= 30 &&
                    phraseBounds.contains({static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)})) {
                    const float row = (static_cast<float>(mouse->position.y) - 198.f + phraseScroll) / 22.f;
                    if (row >= 0.f && static_cast<std::size_t>(row) < phrases.size()) {
                        selectedPhrase = static_cast<std::size_t>(row);
                        textInput.setValue(phrases[*selectedPhrase]);
                        status = "Frase seleccionada; edita y presiona Enter";
                    }
                }
                textInput.update(1.f / 60.f);
                if (mouse->button == sf::Mouse::Button::Left && deleteBounds.contains(
                    {static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)}) &&
                    !phrases.empty()) {
                    const std::size_t index = selectedPhrase.value_or(phrases.size() - 1);
                    phrases.erase(phrases.begin() + static_cast<std::ptrdiff_t>(index));
                    selectedPhrase.reset();
                    textInput.clear();
                    savePhrases(phrasesPath, phrases);
                    atomTable.save(atomsPath.string());
                    status = "Ultima frase eliminada";
                    soundManager.playClick();
                }
                if (mouse->button == sf::Mouse::Button::Left && clearBounds.contains(
                    {static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)})) {
                    if (confirmClear) {
                        phrases.clear();
                        analysis = {};
                        selectedPhrase.reset();
                        textInput.clear();
                        atomTable.clear();
                        savePhrases(phrasesPath, phrases);
                        atomTable.save(atomsPath.string());
                        status = "Base limpiada";
                        confirmClear = false;
                        soundManager.playClick();
                    } else {
                        confirmClear = true;
                        status = "Pulsa Limpiar otra vez para confirmar";
                    }
                }
                if (mouse->button == sf::Mouse::Button::Left && muteBounds.contains(
                    {static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)})) {
                    soundManager.toggleMuted();
                }
                if (mouse->button == sf::Mouse::Button::Left && volumeDownBounds.contains(
                    {static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)})) {
                    soundManager.setVolume(soundManager.volume() - 5.f);
                    soundManager.playClick();
                }
                if (mouse->button == sf::Mouse::Button::Left && volumeUpBounds.contains(
                    {static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)})) {
                    soundManager.setVolume(soundManager.volume() + 5.f);
                    soundManager.playClick();
                }
            }
            if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                const float wheelX = static_cast<float>(wheel->position.x);
                const float wheelY = static_cast<float>(wheel->position.y);
                const float currentWidth = static_cast<float>(window.getSize().x);
                const float currentHeight = static_cast<float>(window.getSize().y);
                const float currentLeftWidth = std::clamp(currentWidth * .29f, 300.f, 430.f);
                const float currentRightX = theme.padding + currentLeftWidth + theme.gap;
                const float currentRightWidth = std::max(360.f, currentWidth - currentRightX - theme.padding);
                const float currentContentTop = 168.f;
                const float currentContentHeight = std::max(240.f, currentHeight - currentContentTop - 48.f);
                const sf::FloatRect treeBounds(
                    {currentRightX + 8.f, currentContentTop + 178.f},
                    {currentRightWidth - 16.f, currentContentHeight - 190.f});
                if (treeBounds.contains({wheelX, wheelY})) {
                    treeZoom = std::clamp(treeZoom + wheel->delta * .08f, .35f, 1.65f);
                } else {
                    phraseScroll = std::clamp(phraseScroll - wheel->delta * 24.f, 0.f,
                                              std::max(0.f, static_cast<float>(phrases.size() * 22) - 100.f));
                }
            }
        }

        const sf::Vector2u windowSize = window.getSize();
        const float width = static_cast<float>(windowSize.x);
        const float height = static_cast<float>(windowSize.y);
        const float margin = theme.padding;
        const float leftWidth = std::clamp(width * .29f, 300.f, 430.f);
        const float contentTop = 168.f;
        const float contentHeight = std::max(240.f, height - contentTop - 48.f);
        const float rightX = margin + leftWidth + theme.gap;
        const float rightWidth = std::max(360.f, width - rightX - margin);
        const sf::FloatRect inputBounds({margin, 98.f}, {width - margin * 2.f, 44.f});
        textInput.setBounds(inputBounds);

        window.clear(theme.background);
        drawGradientPanel(window, {{0.f, 0.f}, {width, 84.f}}, theme.primary, sf::Color(24, 61, 78));
        drawText(window, font, "SAT SOLVER", margin, 14.f, theme.titleSize, theme.text);
        drawText(window, font, "Frases en lenguaje natural - Enter para analizar - F11 pantalla completa",
                 margin + 2.f, 52.f, theme.bodySize, theme.text);
        drawButton(window, font, sf::FloatRect({width - 250.f, 16.f}, {92.f, 28.f}),
                   soundManager.muted() ? "Sonido OFF" : "Sonido ON", mousePosition, mousePressed);
        drawButton(window, font, sf::FloatRect({width - 150.f, 16.f}, {28.f, 28.f}),
                   "-", mousePosition, mousePressed);
        drawButton(window, font, sf::FloatRect({width - 116.f, 16.f}, {28.f, 28.f}),
                   "+", mousePosition, mousePressed);

        textInput.draw(window);

        drawGradientPanel(window, {{margin, contentTop}, {leftWidth, contentHeight}},
                          theme.panelAlt, theme.panel);
        drawText(window, font, "Frases acumuladas", 44.f, 168.f, 17, sf::Color::White);
        for (std::size_t i = 0; i < phrases.size(); ++i) {
            const float y = 198.f + static_cast<float>(i) * 22.f - phraseScroll;
            if (y >= contentTop + 32.f && y < contentTop + contentHeight - 52.f)
                drawText(window, font, std::to_string(i + 1) + ". " + phrases[i], 44.f, y, 13,
                         theme.text);
        }
        drawButton(window, font, sf::FloatRect({margin, contentTop + contentHeight - 40.f}, {140.f, 30.f}),
                   "Borrar ultima", mousePosition, mousePressed);
        drawButton(window, font, sf::FloatRect({margin + 150.f, contentTop + contentHeight - 40.f}, {150.f, 30.f}),
                   "Limpiar todo", mousePosition, mousePressed);

        drawGradientPanel(window, {{rightX, contentTop}, {rightWidth, contentHeight}},
                          theme.panel, sf::Color(22, 30, 42));
        drawText(window, font, status, rightX + 18.f, contentTop + 14.f, theme.bodySize, theme.muted);
        if (analysis.formula) {
            drawText(window, font, "Formula: " + analysis.formula->print(), rightX + 18.f, contentTop + 44.f, 17, theme.text);
            drawText(window, font, "En lenguaje natural: " + analysis.naturalFormula,
                     rightX + 18.f, contentTop + 70.f, theme.bodySize, sf::Color(220, 220, 180));
            drawText(window, font, "Atomos: " + std::to_string(analysis.result.atoms.size()) +
                     "   Modelos: " + std::to_string(analysis.result.modelCount) +
                     "   Descartadas: " + std::to_string(analysis.result.discardedAssignments),
                     rightX + 18.f, contentTop + 98.f, theme.bodySize, sf::Color(175, 220, 190));
            const sf::Color resultColor = analysis.result.satisfiable ? theme.success : theme.danger;
            const sf::FloatRect resultBadge(
                {rightX + rightWidth - 270.f, contentTop + 10.f}, {250.f, 32.f});
            drawRoundedRectangle(window, resultBadge, 10.f,
                                 sf::Color(resultColor.r, resultColor.g, resultColor.b, 210));
            drawText(window, font, analysis.result.satisfiable ? "SATISFACIBLE" : "INSATISFACIBLE / CONTRADICCION",
                     rightX + rightWidth - 258.f, contentTop + 16.f, 15, sf::Color::White);
            drawText(window, font, "Leyenda:", rightX + 18.f, contentTop + 126.f, theme.bodySize, theme.text);
            for (std::size_t i = 0; i < analysis.result.atoms.size() && i < 5; ++i)
                drawText(window, font, analysis.result.atoms[i] + " = " +
                         atomTable.text(analysis.result.atoms[i]), rightX + 18.f, contentTop + 150.f + static_cast<float>(i) * 20.f,
                         13, theme.text);

            if (!analysis.result.atoms.empty()) {
                const sf::FloatRect treeBounds(
                    {rightX + 8.f, contentTop + 178.f},
                    {rightWidth - 16.f, contentHeight - 190.f});
                drawRoundedRectangle(window, treeBounds, 10.f, sf::Color(19, 27, 38, 235));
                drawText(window, font, "Arbol semantico (rueda: zoom, boton central: desplazar)",
                         treeBounds.position.x + 10.f, treeBounds.position.y + 8.f, 12, theme.muted);
                const sf::Vector2f defaultOffset{
                    treeBounds.position.x + treeBounds.size.x * .50f + treeOffset.x,
                    treeBounds.position.y + 54.f + treeOffset.y};
                drawSemanticTreeNode(window, font, analysis.result.tree, 0.f, 0.f,
                                     std::max(90.f, treeBounds.size.x * .24f), treeZoom, defaultOffset);
            }
        }

        drawText(window, font, "Verde = modelo   Rojo = descartada   Esc = salir   Rueda = frases",
                 margin, height - 28.f, 14, theme.muted);
        window.display();
    }
    return 0;
}
