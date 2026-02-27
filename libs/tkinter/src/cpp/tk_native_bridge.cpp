#include "tk_native_bridge.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
enum class WidgetType { Label, Button, Entry };

struct Widget {
    int id = -1;
    WidgetType type = WidgetType::Label;
    std::string text;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct Window {
    int id = -1;
    std::string title;
    int width = 640;
    int height = 420;
    std::string bg = "#0f172a";
    std::string fg = "#e2e8f0";
    std::string accent = "#38bdf8";
    std::string scss_path;
    int next_widget_id = 1;
    int last_clicked_button = 0;
    bool shown = false;
    std::unordered_map<int, Widget> widgets;
    std::vector<int> order;
};

int g_next_window_id = 1;
std::unordered_map<int, Window> g_windows;
thread_local std::string g_last_text_result;

std::string trimCopy(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])) != 0) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0) --e;
    return s.substr(b, e - b);
}

std::string lowerCopy(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

void replaceAll(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string stripComments(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size();) {
        if (i + 1 < in.size() && in[i] == '/' && in[i + 1] == '*') {
            i += 2;
            while (i + 1 < in.size() && !(in[i] == '*' && in[i + 1] == '/')) ++i;
            if (i + 1 < in.size()) i += 2;
            continue;
        }
        if (i + 1 < in.size() && in[i] == '/' && in[i + 1] == '/') {
            i += 2;
            while (i < in.size() && in[i] != '\n') ++i;
            continue;
        }
        out.push_back(in[i]);
        ++i;
    }
    return out;
}

std::unordered_map<std::string, std::string> parseScssVariables(const std::string& scss) {
    std::unordered_map<std::string, std::string> vars;
    std::istringstream in(scss);
    std::string line;
    while (std::getline(in, line)) {
        const std::string t = trimCopy(line);
        if (t.size() < 4 || t[0] != '$') continue;
        const size_t sep = t.find(':');
        const size_t end = t.rfind(';');
        if (sep == std::string::npos || end == std::string::npos || end <= sep + 1) continue;
        const std::string name = trimCopy(t.substr(0, sep));
        const std::string value = trimCopy(t.substr(sep + 1, end - sep - 1));
        if (!name.empty() && !value.empty()) vars[name] = value;
    }
    return vars;
}

std::vector<std::pair<std::string, std::string>> parseBlocks(const std::string& css) {
    std::vector<std::pair<std::string, std::string>> blocks;
    size_t pos = 0;
    while (pos < css.size()) {
        const size_t open = css.find('{', pos);
        if (open == std::string::npos) break;
        const std::string selector = trimCopy(css.substr(pos, open - pos));
        const size_t close = css.find('}', open + 1);
        if (close == std::string::npos) break;
        const std::string body = css.substr(open + 1, close - open - 1);
        if (!selector.empty()) blocks.push_back({lowerCopy(selector), body});
        pos = close + 1;
    }
    return blocks;
}

std::unordered_map<std::string, std::string> parseProperties(const std::string& body) {
    std::unordered_map<std::string, std::string> props;
    std::istringstream in(body);
    std::string line;
    while (std::getline(in, line, ';')) {
        const std::string t = trimCopy(line);
        if (t.empty()) continue;
        const size_t sep = t.find(':');
        if (sep == std::string::npos) continue;
        const std::string name = lowerCopy(trimCopy(t.substr(0, sep)));
        const std::string value = trimCopy(t.substr(sep + 1));
        if (!name.empty() && !value.empty()) props[name] = value;
    }
    return props;
}

void applyScss(Window& w, const std::string& scss_path) {
    std::ifstream f(scss_path);
    if (!f.is_open()) return;
    std::ostringstream buffer;
    buffer << f.rdbuf();
    std::string scss = stripComments(buffer.str());

    const auto vars = parseScssVariables(scss);
    for (const auto& kv : vars) replaceAll(scss, kv.first, kv.second);

    const auto blocks = parseBlocks(scss);
    for (const auto& block : blocks) {
        const std::string selector = block.first;
        const auto props = parseProperties(block.second);
        if (selector == "window") {
            auto itBg = props.find("background");
            if (itBg != props.end()) w.bg = itBg->second;
            auto itBg2 = props.find("background-color");
            if (itBg2 != props.end()) w.bg = itBg2->second;
            auto itFg = props.find("color");
            if (itFg != props.end()) w.fg = itFg->second;
            auto itAccent = props.find("accent");
            if (itAccent != props.end()) w.accent = itAccent->second;
            auto itAccent2 = props.find("--accent");
            if (itAccent2 != props.end()) w.accent = itAccent2->second;
        }
        if (selector == "button") {
            auto it = props.find("background");
            if (it != props.end()) w.accent = it->second;
        }
    }
}

bool isHexColor(const std::string& c) {
    if (c.size() != 7 || c[0] != '#') return false;
    for (size_t i = 1; i < c.size(); ++i) {
        const char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(c[i])));
        const bool ok = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
        if (!ok) return false;
    }
    return true;
}

int fromHex2(char a, char b) {
    auto nibble = [](char c) -> int {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return 0;
    };
    return nibble(a) * 16 + nibble(b);
}

std::string ansiReset() { return "\033[0m"; }

std::string ansiFg(const std::string& color) {
    if (!isHexColor(color)) return "";
    const int r = fromHex2(color[1], color[2]);
    const int g = fromHex2(color[3], color[4]);
    const int b = fromHex2(color[5], color[6]);
    std::ostringstream out;
    out << "\033[38;2;" << r << ';' << g << ';' << b << 'm';
    return out.str();
}

std::string ansiBg(const std::string& color) {
    if (!isHexColor(color)) return "";
    const int r = fromHex2(color[1], color[2]);
    const int g = fromHex2(color[3], color[4]);
    const int b = fromHex2(color[5], color[6]);
    std::ostringstream out;
    out << "\033[48;2;" << r << ';' << g << ';' << b << 'm';
    return out.str();
}

int addWidget(Window& w, WidgetType type, const char* text, int x, int y, int width, int height) {
    Widget widget;
    widget.id = w.next_widget_id++;
    widget.type = type;
    widget.text = text ? std::string(text) : std::string();
    widget.x = x;
    widget.y = y;
    widget.w = width;
    widget.h = height;
    w.widgets[widget.id] = widget;
    w.order.push_back(widget.id);
    return widget.id;
}

bool hasButton(const Window& w, int id) {
    auto it = w.widgets.find(id);
    return it != w.widgets.end() && it->second.type == WidgetType::Button;
}

void renderWindow(const Window& w) {
    const std::string fg = ansiFg(w.fg);
    const std::string bg = ansiBg(w.bg);
    const std::string accent = ansiFg(w.accent);

    std::cout << bg << fg << "=== " << w.title << " (" << w.width << "x" << w.height << ") ===" << ansiReset()
              << std::endl;
    for (int id : w.order) {
        auto it = w.widgets.find(id);
        if (it == w.widgets.end()) continue;
        const Widget& wd = it->second;
        if (wd.type == WidgetType::Label) {
            std::cout << fg << "  [Label #" << id << "] " << wd.text << ansiReset() << std::endl;
        } else if (wd.type == WidgetType::Entry) {
            std::cout << fg << "  [Entry #" << id << "] " << wd.text << ansiReset() << std::endl;
        } else {
            std::cout << accent << "  [Button #" << id << "] " << wd.text << ansiReset() << std::endl;
        }
    }
}
} // namespace

extern "C" {
int tkinter_available() { return 1; }

int tkinter_window_create(const char* title, int width, int height) {
    Window w;
    w.id = g_next_window_id++;
    w.title = title ? std::string(title) : std::string("E++ App");
    w.width = std::max(220, width);
    w.height = std::max(160, height);
    g_windows[w.id] = w;
    return w.id;
}

int tkinter_window_set_title(int window_id, const char* title) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return 0;
    it->second.title = title ? std::string(title) : std::string();
    return 1;
}

int tkinter_label_add(int window_id, const char* text, int x, int y, int w, int h) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return -1;
    return addWidget(it->second, WidgetType::Label, text, x, y, w, h);
}

int tkinter_button_add(int window_id, const char* text, int x, int y, int w, int h) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return -1;
    return addWidget(it->second, WidgetType::Button, text, x, y, w, h);
}

int tkinter_entry_add(int window_id, const char* text, int x, int y, int w, int h) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return -1;
    return addWidget(it->second, WidgetType::Entry, text, x, y, w, h);
}

int tkinter_widget_set_text(int window_id, int widget_id, const char* text) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return 0;
    auto wt = it->second.widgets.find(widget_id);
    if (wt == it->second.widgets.end()) return 0;
    wt->second.text = text ? std::string(text) : std::string();
    return 1;
}

const char* tkinter_widget_get_text(int window_id, int widget_id) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) {
        g_last_text_result.clear();
        return g_last_text_result.c_str();
    }
    auto wt = it->second.widgets.find(widget_id);
    if (wt == it->second.widgets.end()) {
        g_last_text_result.clear();
        return g_last_text_result.c_str();
    }
    g_last_text_result = wt->second.text;
    return g_last_text_result.c_str();
}

int tkinter_window_set_bg(int window_id, const char* color_hex) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return 0;
    if (color_hex) it->second.bg = color_hex;
    return 1;
}

int tkinter_window_set_fg(int window_id, const char* color_hex) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return 0;
    if (color_hex) it->second.fg = color_hex;
    return 1;
}

int tkinter_window_set_accent(int window_id, const char* color_hex) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return 0;
    if (color_hex) it->second.accent = color_hex;
    return 1;
}

int tkinter_window_apply_scss(int window_id, const char* scss_path) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end() || !scss_path) return 0;
    it->second.scss_path = scss_path;
    applyScss(it->second, it->second.scss_path);
    return 1;
}

int tkinter_button_clicked(int window_id, int button_id) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return 0;
    if (it->second.last_clicked_button == button_id) {
        it->second.last_clicked_button = 0;
        return 1;
    }
    return 0;
}

void tkinter_window_show(int window_id) {
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return;
    it->second.shown = true;
    renderWindow(it->second);
}

int tkinter_mainloop(int window_id) {
    Window snapshot;
    auto it = g_windows.find(window_id);
    if (it == g_windows.end()) return 0;
    it->second.shown = true;
    snapshot = it->second;

    renderWindow(snapshot);

    std::unordered_map<int, std::string> entry_updates;
    std::vector<int> buttons;
    for (int id : snapshot.order) {
        auto it = snapshot.widgets.find(id);
        if (it == snapshot.widgets.end()) continue;
        const Widget& wd = it->second;
        if (wd.type == WidgetType::Entry) {
            std::cout << "Input for Entry #" << id << " (leave empty to keep value): ";
            std::string input;
            std::getline(std::cin, input);
            if (!input.empty()) entry_updates[id] = input;
        } else if (wd.type == WidgetType::Button) {
            buttons.push_back(id);
        }
    }

    int clicked = 0;
    if (!buttons.empty()) {
        std::cout << "Click button id (0 = none): ";
        std::string raw;
        std::getline(std::cin, raw);
        try {
            clicked = std::stoi(trimCopy(raw));
        } catch (...) {
            clicked = 0;
        }
    }

    auto it2 = g_windows.find(window_id);
    if (it2 == g_windows.end()) return 0;
    for (const auto& kv : entry_updates) {
        auto wt = it2->second.widgets.find(kv.first);
        if (wt != it2->second.widgets.end() && wt->second.type == WidgetType::Entry) wt->second.text = kv.second;
    }
    if (clicked > 0 && hasButton(it2->second, clicked)) {
        it2->second.last_clicked_button = clicked;
    } else {
        it2->second.last_clicked_button = 0;
    }
    return 1;
}
}
