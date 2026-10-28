#ifndef SAT_TEXT_INPUT_H
#define SAT_TEXT_INPUT_H

#include <SFML/Graphics.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>

class TextInput {
public:
    TextInput(const sf::Font& font, unsigned int characterSize);
    void setBounds(const sf::FloatRect& bounds);
    void handleTextEntered(std::uint32_t unicode);
    void handleKeyPressed(const sf::Event::KeyPressed& key);
    void handleMousePressed(sf::Vector2f position);
    void handleMouseMoved(sf::Vector2f position, bool leftButtonDown);
    void update(float elapsedSeconds);
    void draw(sf::RenderWindow& window) const;
    const std::string& value() const;
    void setValue(std::string value);
    std::string takeValue();
    bool empty() const;
    bool consumeSubmit();
    void clear();

private:
    static std::string encodeUtf8(std::uint32_t codepoint);
    static std::size_t previousCodepoint(const std::string& value, std::size_t position);
    static std::size_t nextCodepoint(const std::string& value, std::size_t position);
    std::size_t cursorFromMouse(float x) const;
    float textWidth(std::size_t end) const;
    void deleteSelection();
    void ensureCursorVisible();
    void updateSelection(std::size_t position);
    void selectWordAt(std::size_t position);

    const sf::Font& font_;
    unsigned int characterSize_;
    sf::FloatRect bounds_;
    std::string value_;
    std::size_t cursor_ = 0;
    std::size_t anchor_ = 0;
    float scrollX_ = 0.f;
    float blinkTime_ = 0.f;
    bool dragging_ = false;
    bool submit_ = false;
    std::chrono::steady_clock::time_point lastClick_{};
    sf::Vector2f lastClickPosition_{};
};

#endif
