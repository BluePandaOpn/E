// color.cpp
// Logica real de conversion y secuencias ANSI.

#include <stdexcept>
#include <string>
#include <sstream>
#include <cctype>

namespace colorlib {

struct RGB {
    int r;
    int g;
    int b;
};

static bool isHexChar(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

bool isHexColor(const std::string& hex) {
    if (hex.size() != 7 || hex[0] != '#') {
        return false;
    }
    for (size_t i = 1; i < hex.size(); ++i) {
        if (!isHexChar(hex[i])) {
            return false;
        }
    }
    return true;
}

RGB hexToRgb(const std::string& hex) {
    if (!isHexColor(hex)) {
        throw std::invalid_argument("Invalid hex color format. Use #RRGGBB.");
    }

    auto toDec = [](const std::string& v) {
        return std::stoi(v, nullptr, 16);
    };

    RGB out;
    out.r = toDec(hex.substr(1, 2));
    out.g = toDec(hex.substr(3, 2));
    out.b = toDec(hex.substr(5, 2));
    return out;
}

std::string ansiForeground(const RGB& c) {
    std::ostringstream ss;
    ss << "\x1b[38;2;" << c.r << ';' << c.g << ';' << c.b << 'm';
    return ss.str();
}

std::string ansiBackground(const RGB& c) {
    std::ostringstream ss;
    ss << "\x1b[48;2;" << c.r << ';' << c.g << ';' << c.b << 'm';
    return ss.str();
}

std::string ansiReset() {
    return "\x1b[0m";
}

} // namespace colorlib
