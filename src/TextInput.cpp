#include "TextInput.h"

#include <algorithm>

TextInput::TextInput(const sf::Font& font, unsigned int characterSize)
    : font_(font), characterSize_(characterSize) {}

void TextInput::setBounds(const sf::FloatRect& bounds) { bounds_ = bounds; ensureCursorVisible(); }

std::string TextInput::encodeUtf8(std::uint32_t codepoint) {
    if (codepoint <= 0x7f) return std::string(1, static_cast<char>(codepoint));
    if (codepoint <= 0x7ff) return {
        static_cast<char>(0xc0 | (codepoint >> 6)),
        static_cast<char>(0x80 | (codepoint & 0x3f))};
    if (codepoint <= 0xffff) return {
        static_cast<char>(0xe0 | (codepoint >> 12)),
        static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)),
        static_cast<char>(0x80 | (codepoint & 0x3f))};
    if (codepoint <= 0x10ffff) return {
        static_cast<char>(0xf0 | (codepoint >> 18)),
        static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)),
        static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)),
        static_cast<char>(0x80 | (codepoint & 0x3f))};
    return {};
}

std::size_t TextInput::previousCodepoint(const std::string& value, std::size_t position) {
    if (position == 0) return 0;
    --position;
    while (position > 0 && (static_cast<unsigned char>(value[position]) & 0xc0) == 0x80) --position;
    return position;
}

std::size_t TextInput::nextCodepoint(const std::string& value, std::size_t position) {
    if (position >= value.size()) return value.size();
    ++position;
    while (position < value.size() && (static_cast<unsigned char>(value[position]) & 0xc0) == 0x80) ++position;
    return position;
}

void TextInput::handleTextEntered(std::uint32_t unicode) {
    if (unicode < 32 || unicode == 127) return;
    deleteSelection();
    const std::string encoded = encodeUtf8(unicode);
    value_.insert(cursor_, encoded);
    cursor_ += encoded.size();
    anchor_ = cursor_;
    blinkTime_ = 0.f;
    ensureCursorVisible();
}

void TextInput::handleKeyPressed(const sf::Event::KeyPressed& key) {
    const bool shift = key.shift;
    const std::size_t oldCursor = cursor_;
    if (key.control) {
        if (key.code == sf::Keyboard::Key::C) {
            const std::size_t start = std::min(anchor_, cursor_);
            const std::size_t end = std::max(anchor_, cursor_);
            if (start != end)
                sf::Clipboard::setString(sf::String::fromUtf8(value_.begin() + static_cast<std::ptrdiff_t>(start),
                                                              value_.begin() + static_cast<std::ptrdiff_t>(end)));
            return;
        }
        if (key.code == sf::Keyboard::Key::V) {
            deleteSelection();
            const auto pastedUtf8 = sf::Clipboard::getString().toUtf8();
            const std::string pasted(pastedUtf8.begin(), pastedUtf8.end());
            value_.insert(cursor_, pasted);
            cursor_ += pasted.size();
            anchor_ = cursor_;
            ensureCursorVisible();
            return;
        }
        if (key.code == sf::Keyboard::Key::X) {
            const std::size_t start = std::min(anchor_, cursor_);
            const std::size_t end = std::max(anchor_, cursor_);
            if (start != end) {
                sf::Clipboard::setString(sf::String::fromUtf8(value_.begin() + static_cast<std::ptrdiff_t>(start),
                                                              value_.begin() + static_cast<std::ptrdiff_t>(end)));
                deleteSelection();
            }
            return;
        }
    }
    switch (key.code) {
    case sf::Keyboard::Key::Left:
        cursor_ = previousCodepoint(value_, cursor_);
        break;
    case sf::Keyboard::Key::Right:
        cursor_ = nextCodepoint(value_, cursor_);
        break;
    case sf::Keyboard::Key::Home:
        cursor_ = 0;
        break;
    case sf::Keyboard::Key::End:
        cursor_ = value_.size();
        break;
    case sf::Keyboard::Key::Backspace:
        if (anchor_ != cursor_) deleteSelection();
        else if (cursor_ > 0) {
            const std::size_t start = previousCodepoint(value_, cursor_);
            value_.erase(start, cursor_ - start);
            cursor_ = start;
            anchor_ = cursor_;
        }
        break;
    case sf::Keyboard::Key::Delete:
        if (anchor_ != cursor_) deleteSelection();
        else if (cursor_ < value_.size()) value_.erase(cursor_, nextCodepoint(value_, cursor_) - cursor_);
        break;
    case sf::Keyboard::Key::Enter:
        submit_ = true;
        break;
    case sf::Keyboard::Key::A:
        if (key.control) { anchor_ = 0; cursor_ = value_.size(); }
        break;
    default:
        return;
    }
    if (!shift && key.code != sf::Keyboard::Key::Backspace && key.code != sf::Keyboard::Key::Delete &&
        key.code != sf::Keyboard::Key::Enter && !(key.code == sf::Keyboard::Key::A && key.control))
        anchor_ = cursor_;
    if (!shift && (key.code == sf::Keyboard::Key::Backspace || key.code == sf::Keyboard::Key::Delete))
        anchor_ = cursor_;
    if (oldCursor != cursor_ || key.code == sf::Keyboard::Key::Backspace || key.code == sf::Keyboard::Key::Delete)
        blinkTime_ = 0.f;
    ensureCursorVisible();
}

void TextInput::handleMousePressed(sf::Vector2f position) {
    if (!bounds_.contains(position)) return;
    cursor_ = cursorFromMouse(position.x);
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastClick_).count();
    const float dx = position.x - lastClickPosition_.x;
    const float dy = position.y - lastClickPosition_.y;
    if (elapsed < 450 && dx * dx + dy * dy < 64.f) selectWordAt(cursor_);
    else anchor_ = cursor_;
    lastClick_ = now;
    lastClickPosition_ = position;
    dragging_ = true;
    blinkTime_ = 0.f;
    ensureCursorVisible();
}

void TextInput::selectWordAt(std::size_t position) {
    if (value_.empty()) { anchor_ = cursor_ = 0; return; }
    if (position == value_.size()) position = previousCodepoint(value_, position);
    const auto isSpace = [this](std::size_t index) {
        return value_[index] == ' ' || value_[index] == '\t';
    };
    std::size_t start = position;
    while (start > 0) {
        const std::size_t previous = previousCodepoint(value_, start);
        if (isSpace(previous)) break;
        start = previous;
    }
    std::size_t end = nextCodepoint(value_, position);
    while (end < value_.size() && !isSpace(end)) end = nextCodepoint(value_, end);
    anchor_ = start;
    cursor_ = end;
}

void TextInput::handleMouseMoved(sf::Vector2f position, bool leftButtonDown) {
    if (!dragging_ || !leftButtonDown) { dragging_ = false; return; }
    updateSelection(cursorFromMouse(position.x));
}

void TextInput::update(float elapsedSeconds) {
    blinkTime_ += elapsedSeconds;
    if (blinkTime_ > 1.f) blinkTime_ -= 1.f;
}

std::size_t TextInput::cursorFromMouse(float x) const {
    const float target = x - bounds_.position.x + scrollX_ - 8.f;
    for (std::size_t position = 0; position <= value_.size();) {
        const std::size_t next = nextCodepoint(value_, position);
        const float midpoint = (textWidth(position) + textWidth(next)) * .5f;
        if (target < midpoint) return position;
        if (next == position) break;
        position = next;
    }
    return value_.size();
}

float TextInput::textWidth(std::size_t end) const {
    sf::Text text(font_, sf::String::fromUtf8(value_.begin(), value_.begin() + static_cast<std::ptrdiff_t>(end)), characterSize_);
    return text.getLocalBounds().size.x;
}

void TextInput::deleteSelection() {
    if (anchor_ == cursor_) return;
    const std::size_t start = std::min(anchor_, cursor_);
    const std::size_t end = std::max(anchor_, cursor_);
    value_.erase(start, end - start);
    cursor_ = start;
    anchor_ = start;
}

void TextInput::ensureCursorVisible() {
    const float width = std::max(1.f, bounds_.size.x - 16.f);
    const float cursorX = textWidth(cursor_);
    if (cursorX - scrollX_ > width) scrollX_ = cursorX - width;
    if (cursorX - scrollX_ < 0.f) scrollX_ = cursorX;
    scrollX_ = std::max(0.f, scrollX_);
}

void TextInput::updateSelection(std::size_t position) {
    cursor_ = position;
    blinkTime_ = 0.f;
    ensureCursorVisible();
}

void TextInput::draw(sf::RenderWindow& window) const {
    sf::RectangleShape box({bounds_.size.x, bounds_.size.y});
    box.setPosition(bounds_.position);
    box.setFillColor(sf::Color(245, 247, 248));
    window.draw(box);

    const std::size_t start = std::min(anchor_, cursor_);
    const std::size_t end = std::max(anchor_, cursor_);
    if (start != end) {
        sf::RectangleShape selection({textWidth(end) - textWidth(start), bounds_.size.y - 10.f});
        selection.setPosition({bounds_.position.x + 8.f + textWidth(start) - scrollX_, bounds_.position.y + 5.f});
        selection.setFillColor(sf::Color(125, 180, 220));
        window.draw(selection);
    }

    sf::Text text(font_, sf::String::fromUtf8(value_.begin(), value_.end()), characterSize_);
    text.setPosition({bounds_.position.x + 8.f - scrollX_, bounds_.position.y + 7.f});
    text.setFillColor(sf::Color(30, 35, 40));
    window.draw(text);

    if (blinkTime_ < .5f) {
        sf::RectangleShape cursor({2.f, bounds_.size.y - 14.f});
        cursor.setPosition({bounds_.position.x + 8.f + textWidth(cursor_) - scrollX_, bounds_.position.y + 7.f});
        cursor.setFillColor(sf::Color(35, 91, 112));
        window.draw(cursor);
    }
}

const std::string& TextInput::value() const { return value_; }
void TextInput::setValue(std::string value) {
    value_ = std::move(value);
    cursor_ = anchor_ = value_.size();
    ensureCursorVisible();
}
std::string TextInput::takeValue() {
    std::string result = value_;
    value_.clear();
    cursor_ = anchor_ = 0;
    scrollX_ = 0.f;
    return result;
}
bool TextInput::empty() const { return value_.empty(); }
void TextInput::clear() {
    value_.clear();
    cursor_ = anchor_ = 0;
    scrollX_ = 0.f;
    blinkTime_ = 0.f;
}
bool TextInput::consumeSubmit() {
    const bool result = submit_;
    submit_ = false;
    return result;
}
