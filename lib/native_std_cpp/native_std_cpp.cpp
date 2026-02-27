#include "epp_native.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
thread_local std::string g_string_result;
std::mt19937_64 g_rng{std::random_device{}()};

EppNativeValue makeNull() {
    EppNativeValue v{};
    v.type = EPP_NATIVE_NULL;
    return v;
}

EppNativeValue makeNumber(double n) {
    EppNativeValue v{};
    v.type = EPP_NATIVE_NUMBER;
    v.number_value = n;
    return v;
}

EppNativeValue makeBool(bool b) {
    EppNativeValue v{};
    v.type = EPP_NATIVE_BOOL;
    v.bool_value = b ? 1 : 0;
    return v;
}

EppNativeValue makeString(const std::string& s) {
    g_string_result = s;
    EppNativeValue v{};
    v.type = EPP_NATIVE_STRING;
    v.string_value = g_string_result.c_str();
    return v;
}

bool isNumber(const EppNativeValue& v) { return v.type == EPP_NATIVE_NUMBER; }
bool isString(const EppNativeValue& v) { return v.type == EPP_NATIVE_STRING && v.string_value != nullptr; }
bool isBool(const EppNativeValue& v) { return v.type == EPP_NATIVE_BOOL; }

int asInt(const EppNativeValue& v, int fallback) {
    if (v.type == EPP_NATIVE_NUMBER) return static_cast<int>(v.number_value);
    return fallback;
}

std::string asString(const EppNativeValue& v, const std::string& fallback) {
    if (v.type == EPP_NATIVE_STRING && v.string_value) return std::string(v.string_value);
    return fallback;
}

bool asBool(const EppNativeValue& v, bool fallback) {
    if (v.type == EPP_NATIVE_BOOL) return v.bool_value != 0;
    if (v.type == EPP_NATIVE_NUMBER) return v.number_value != 0.0;
    return fallback;
}

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
#endif

void closeSocket(SocketHandle s) {
#ifdef _WIN32
    if (s != INVALID_SOCKET) closesocket(s);
#else
    if (s >= 0) close(s);
#endif
}

bool socketStartup() {
#ifdef _WIN32
    static bool initialized = false;
    static bool ok = false;
    if (!initialized) {
        WSADATA wsa{};
        ok = (WSAStartup(MAKEWORD(2, 2), &wsa) == 0);
        initialized = true;
    }
    return ok;
#else
    return true;
#endif
}

struct HttpRequestParts {
    std::string requestLine;
    std::string method;
    std::string path;
    std::string version;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

struct HttpServerCtx {
    SocketHandle socket = kInvalidSocket;
    std::string host;
    int port = 0;
};

struct HttpClientCtx {
    SocketHandle socket = kInvalidSocket;
    HttpRequestParts req;
};

std::mutex g_httpMutex;
int g_nextHttpServerId = 1;
int g_nextHttpClientId = 1;
std::unordered_map<int, HttpServerCtx> g_httpServers;
std::unordered_map<int, HttpClientCtx> g_httpClients;

enum class TkWidgetType { Label, Button, Entry };

struct TkWidgetCtx {
    int id = -1;
    TkWidgetType type = TkWidgetType::Label;
    std::string text;
};

struct TkWindowCtx {
    int id = -1;
    std::string title;
    int width = 640;
    int height = 420;
    std::string bg = "#0f172a";
    std::string fg = "#e2e8f0";
    std::string accent = "#38bdf8";
    int nextWidgetId = 1;
    int lastClickedButton = 0;
    std::unordered_map<int, TkWidgetCtx> widgets;
    std::vector<int> widgetOrder;
};

std::mutex g_tkMutex;
int g_nextTkWindowId = 1;
std::unordered_map<int, TkWindowCtx> g_tkWindows;

std::string toLowerCopy(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

std::string trimCopy(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

std::string statusTextFromCode(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 422: return "Unprocessable Entity";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "OK";
    }
}

std::string buildHttpResponse(int statusCode,
                              const std::string& statusText,
                              const std::string& contentType,
                              const std::string& body) {
    std::ostringstream out;
    out << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    out << "Content-Type: " << contentType << "\r\n";
    out << "Connection: close\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "\r\n";
    out << body;
    return out.str();
}

bool sendAll(SocketHandle socket, const std::string& payload) {
    const char* data = payload.c_str();
    int total = static_cast<int>(payload.size());
    int sent = 0;
    while (sent < total) {
        const int n = send(socket, data + sent, total - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

HttpRequestParts parseHttpRequest(const std::string& raw) {
    HttpRequestParts out;
    const size_t lineEnd = raw.find("\r\n");
    if (lineEnd == std::string::npos) return out;
    out.requestLine = raw.substr(0, lineEnd);

    {
        std::istringstream line(out.requestLine);
        line >> out.method >> out.path >> out.version;
    }

    size_t headerEnd = raw.find("\r\n\r\n");
    size_t headerStart = lineEnd + 2;
    if (headerEnd == std::string::npos) {
        headerEnd = raw.size();
        headerStart = lineEnd + 2;
    }

    size_t cur = headerStart;
    while (cur < headerEnd) {
        size_t next = raw.find("\r\n", cur);
        if (next == std::string::npos || next > headerEnd) break;
        const std::string line = raw.substr(cur, next - cur);
        const size_t sep = line.find(':');
        if (sep != std::string::npos) {
            std::string key = trimCopy(line.substr(0, sep));
            std::string value = trimCopy(line.substr(sep + 1));
            if (!key.empty()) out.headers[toLowerCopy(key)] = value;
        }
        cur = next + 2;
    }

    if (headerEnd + 4 <= raw.size()) {
        out.body = raw.substr(headerEnd + 4);
    }
    return out;
}

bool bindAndListen(SocketHandle& outServer, const std::string& host, int port) {
    if (!socketStartup()) return false;
    outServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (outServer == kInvalidSocket) return false;

    int yes = 1;
#ifdef _WIN32
    setsockopt(outServer, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
#else
    setsockopt(outServer, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));

    if (host.empty() || host == "0.0.0.0" || host == "*") {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        const int ok = inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
        if (ok != 1) {
            closeSocket(outServer);
            outServer = kInvalidSocket;
            return false;
        }
    }

    if (bind(outServer, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        closeSocket(outServer);
        outServer = kInvalidSocket;
        return false;
    }
    if (listen(outServer, 64) < 0) {
        closeSocket(outServer);
        outServer = kInvalidSocket;
        return false;
    }
    return true;
}

int httpServerListen(const std::string& host, int port) {
    if (port <= 0 || port > 65535) return -1;
    SocketHandle s = kInvalidSocket;
    if (!bindAndListen(s, host, port)) return -1;

    std::lock_guard<std::mutex> lock(g_httpMutex);
    const int id = g_nextHttpServerId++;
    g_httpServers[id] = HttpServerCtx{s, host, port};
    return id;
}

int httpServerAccept(int serverId) {
    SocketHandle server = kInvalidSocket;
    {
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpServers.find(serverId);
        if (it == g_httpServers.end()) return -1;
        server = it->second.socket;
    }

    sockaddr_in clientAddr{};
#ifdef _WIN32
    int clientLen = sizeof(clientAddr);
#else
    socklen_t clientLen = static_cast<socklen_t>(sizeof(clientAddr));
#endif
    SocketHandle client = accept(server, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
    if (client == kInvalidSocket) return -1;

    std::string raw;
    std::vector<char> buffer(8192);
    const int received = recv(client, buffer.data(), static_cast<int>(buffer.size() - 1), 0);
    if (received > 0) {
        buffer[received] = '\0';
        raw.assign(buffer.data(), static_cast<size_t>(received));
    }
    HttpRequestParts req = parseHttpRequest(raw);
    if (req.requestLine.empty()) req.requestLine = "(sin request-line)";

    std::lock_guard<std::mutex> lock(g_httpMutex);
    const int clientId = g_nextHttpClientId++;
    g_httpClients[clientId] = HttpClientCtx{client, std::move(req)};
    return clientId;
}

void httpClientClose(int clientId) {
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpClients.find(clientId);
    if (it == g_httpClients.end()) return;
    closeSocket(it->second.socket);
    g_httpClients.erase(it);
}

bool httpResponseSend(int clientId,
                      int statusCode,
                      const std::string& statusText,
                      const std::string& contentType,
                      const std::string& body) {
    SocketHandle socket = kInvalidSocket;
    {
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(clientId);
        if (it == g_httpClients.end()) return false;
        socket = it->second.socket;
    }

    const std::string response = buildHttpResponse(statusCode, statusText, contentType, body);
    const bool ok = sendAll(socket, response);
    httpClientClose(clientId);
    return ok;
}

bool httpServerClose(int serverId) {
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpServers.find(serverId);
    if (it == g_httpServers.end()) return false;
    closeSocket(it->second.socket);
    g_httpServers.erase(it);
    return true;
}

std::string nowIsoLocal() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

std::string jsonQuote(const std::string& in) {
    std::ostringstream out;
    out << '"';
    for (char c : in) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    out << '"';
    return out.str();
}

std::string urlEncode(const std::string& in) {
    std::ostringstream out;
    out << std::hex << std::uppercase;
    for (unsigned char c : in) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            out << static_cast<char>(c);
        } else {
            out << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }
    return out.str();
}

int clamp255Int(int n) {
    if (n < 0) return 0;
    if (n > 255) return 255;
    return n;
}

std::string ansiReset() { return "\033[0m"; }
std::string ansiBold() { return "\033[1m"; }
std::string ansiDim() { return "\033[2m"; }
std::string ansiUnderline() { return "\033[4m"; }

std::string ansiFgCode(std::string name) {
    name = toLowerCopy(trimCopy(name));
    if (name.empty()) return "";
    if (name == "red" || name == "rojo") return "\033[31m";
    if (name == "green" || name == "verde") return "\033[32m";
    if (name == "blue" || name == "azul") return "\033[34m";
    if (name == "yellow" || name == "amarillo") return "\033[33m";
    if (name == "magenta") return "\033[35m";
    if (name == "cyan") return "\033[36m";
    if (name == "white" || name == "blanco") return "\033[37m";
    return "";
}

std::string ansiBgCode(std::string name) {
    name = toLowerCopy(trimCopy(name));
    if (name.empty()) return "";
    if (name == "red" || name == "rojo") return "\033[41m";
    if (name == "green" || name == "verde") return "\033[42m";
    if (name == "blue" || name == "azul") return "\033[44m";
    if (name == "yellow" || name == "amarillo") return "\033[43m";
    if (name == "magenta") return "\033[45m";
    if (name == "cyan") return "\033[46m";
    if (name == "white" || name == "blanco") return "\033[47m";
    return "";
}

std::string ansiRgbCode(int r, int g, int b) {
    std::ostringstream out;
    out << "\033[38;2;" << clamp255Int(r) << ';' << clamp255Int(g) << ';' << clamp255Int(b) << 'm';
    return out.str();
}

std::string ansiBgRgbCode(int r, int g, int b) {
    std::ostringstream out;
    out << "\033[48;2;" << clamp255Int(r) << ';' << clamp255Int(g) << ';' << clamp255Int(b) << 'm';
    return out.str();
}

std::string colorApply(const std::string& text, const std::string& code) { return code + text + ansiReset(); }

std::string colorApplyStyle(const std::string& text, const std::string& colorCode, const std::string& extraCode) {
    return extraCode + colorCode + text + ansiReset();
}

std::string ansiStrip(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size();) {
        if (in[i] == '\x1B' && i + 1 < in.size() && in[i + 1] == '[') {
            i += 2;
            while (i < in.size() && in[i] != 'm') ++i;
            if (i < in.size()) ++i;
            continue;
        }
        out.push_back(in[i]);
        ++i;
    }
    return out;
}

bool colorStdoutIsConsole() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

bool colorConsoleInit() {
#ifdef _WIN32
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr) return false;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return false;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, mode)) return false;
    return true;
#else
    return colorStdoutIsConsole();
#endif
}

void tkTryApplyScss(TkWindowCtx& w, const std::string& path) {
    if (path.empty()) return;
    std::ifstream in(path);
    if (!in.is_open()) return;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string text = buffer.str();
    const std::string lower = toLowerCopy(text);

    auto takeValue = [&](const std::string& key, std::string* out) {
        size_t pos = lower.find(key);
        if (pos == std::string::npos) return;
        pos += key.size();
        size_t end = text.find(';', pos);
        if (end == std::string::npos) return;
        std::string v = trimCopy(text.substr(pos, end - pos));
        if (!v.empty()) *out = v;
    };

    takeValue("$bg:", &w.bg);
    takeValue("$fg:", &w.fg);
    takeValue("$accent:", &w.accent);

    size_t windowPos = lower.find("window");
    if (windowPos != std::string::npos) {
        size_t start = lower.find('{', windowPos);
        size_t stop = lower.find('}', start == std::string::npos ? windowPos : start + 1);
        if (start != std::string::npos && stop != std::string::npos && stop > start) {
            std::string block = text.substr(start + 1, stop - start - 1);
            std::string bLower = toLowerCopy(block);

            auto prop = [&](const std::string& name, std::string* out) {
                size_t p = bLower.find(name);
                if (p == std::string::npos) return;
                p += name.size();
                size_t e = block.find(';', p);
                if (e == std::string::npos) return;
                std::string v = trimCopy(block.substr(p, e - p));
                if (!v.empty()) *out = v;
            };

            prop("background:", &w.bg);
            prop("background-color:", &w.bg);
            prop("color:", &w.fg);
            prop("--accent:", &w.accent);
        }
    }
}

int tkAddWidget(TkWindowCtx& w, TkWidgetType type, const std::string& text) {
    const int id = w.nextWidgetId++;
    w.widgets[id] = TkWidgetCtx{id, type, text};
    w.widgetOrder.push_back(id);
    return id;
}

bool tkIsButton(const TkWindowCtx& w, int id) {
    auto it = w.widgets.find(id);
    if (it == w.widgets.end()) return false;
    return it->second.type == TkWidgetType::Button;
}

void tkRender(const TkWindowCtx& w) {
    std::cout << "=== " << w.title << " (" << w.width << "x" << w.height << ") ===" << std::endl;
    for (int id : w.widgetOrder) {
        auto it = w.widgets.find(id);
        if (it == w.widgets.end()) continue;
        const TkWidgetCtx& wd = it->second;
        if (wd.type == TkWidgetType::Label) std::cout << "[Label #" << id << "] " << wd.text << std::endl;
        if (wd.type == TkWidgetType::Entry) std::cout << "[Entry #" << id << "] " << wd.text << std::endl;
        if (wd.type == TkWidgetType::Button) std::cout << "[Button #" << id << "] " << wd.text << std::endl;
    }
}

std::string decodeEscapesForConsole(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '\\' && i + 1 < in.size()) {
            if (in.compare(i, 4, "\\033") == 0) {
                out.push_back('\x1B');
                i += 3;
                continue;
            }
            if (in.compare(i, 4, "\\x1b") == 0 || in.compare(i, 4, "\\x1B") == 0) {
                out.push_back('\x1B');
                i += 3;
                continue;
            }
        }
        out.push_back(in[i]);
    }
    return out;
}

std::string nativeValueToString(const EppNativeValue& v) {
    switch (v.type) {
        case EPP_NATIVE_STRING:
            return v.string_value ? std::string(v.string_value) : std::string();
        case EPP_NATIVE_NUMBER: {
            std::ostringstream out;
            out << v.number_value;
            return out.str();
        }
        case EPP_NATIVE_BOOL:
            return v.bool_value ? "true" : "false";
        case EPP_NATIVE_NULL:
        default:
            return "null";
    }
}

EppNativeValue fn_time_time(const EppNativeValue*, int) {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return makeNumber(std::chrono::duration<double>(now).count());
}

EppNativeValue fn_time_monotonic(const EppNativeValue*, int) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return makeNumber(std::chrono::duration<double>(now).count());
}

EppNativeValue fn_time_sleep(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    const double seconds = std::max(0.0, args[0].number_value);
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
    return makeNull();
}

EppNativeValue fn_datetime_now_iso(const EppNativeValue*, int) { return makeString(nowIsoLocal()); }

EppNativeValue fn_datetime_year(const EppNativeValue*, int) {
    std::time_t tt = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return makeNumber(static_cast<double>(tm.tm_year + 1900));
}

EppNativeValue fn_datetime_month(const EppNativeValue*, int) {
    std::time_t tt = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return makeNumber(static_cast<double>(tm.tm_mon + 1));
}

EppNativeValue fn_datetime_day(const EppNativeValue*, int) {
    std::time_t tt = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return makeNumber(static_cast<double>(tm.tm_mday));
}

EppNativeValue fn_math_pi(const EppNativeValue*, int) { return makeNumber(3.14159265358979323846); }
EppNativeValue fn_math_e(const EppNativeValue*, int) { return makeNumber(2.71828182845904523536); }

EppNativeValue fn_math_sqrt(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    return makeNumber(std::sqrt(args[0].number_value));
}

EppNativeValue fn_math_pow(const EppNativeValue* args, int argc) {
    if (argc < 2 || !isNumber(args[0]) || !isNumber(args[1])) return makeNull();
    return makeNumber(std::pow(args[0].number_value, args[1].number_value));
}

EppNativeValue fn_math_floor(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    return makeNumber(std::floor(args[0].number_value));
}

EppNativeValue fn_math_ceil(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    return makeNumber(std::ceil(args[0].number_value));
}

EppNativeValue fn_math_sin(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    return makeNumber(std::sin(args[0].number_value));
}

EppNativeValue fn_math_cos(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    return makeNumber(std::cos(args[0].number_value));
}

EppNativeValue fn_math_tan(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    return makeNumber(std::tan(args[0].number_value));
}

EppNativeValue fn_math_log(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    return makeNumber(std::log(args[0].number_value));
}

EppNativeValue fn_math_exp(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNull();
    return makeNumber(std::exp(args[0].number_value));
}

EppNativeValue fn_random_seed(const EppNativeValue* args, int argc) {
    if (argc >= 1 && isNumber(args[0])) {
        g_rng.seed(static_cast<std::uint64_t>(args[0].number_value));
    } else {
        g_rng.seed(std::random_device{}());
    }
    return makeNull();
}

EppNativeValue fn_random_random(const EppNativeValue*, int) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return makeNumber(dist(g_rng));
}

EppNativeValue fn_random_randint(const EppNativeValue* args, int argc) {
    if (argc < 2 || !isNumber(args[0]) || !isNumber(args[1])) return makeNull();
    long long a = static_cast<long long>(args[0].number_value);
    long long b = static_cast<long long>(args[1].number_value);
    if (a > b) std::swap(a, b);
    std::uniform_int_distribution<long long> dist(a, b);
    return makeNumber(static_cast<double>(dist(g_rng)));
}

EppNativeValue fn_random_uniform(const EppNativeValue* args, int argc) {
    if (argc < 2 || !isNumber(args[0]) || !isNumber(args[1])) return makeNull();
    double a = args[0].number_value;
    double b = args[1].number_value;
    if (a > b) std::swap(a, b);
    std::uniform_real_distribution<double> dist(a, b);
    return makeNumber(dist(g_rng));
}

EppNativeValue fn_sys_platform(const EppNativeValue*, int) {
#ifdef _WIN32
    return makeString("windows");
#elif __APPLE__
    return makeString("macos");
#elif __linux__
    return makeString("linux");
#else
    return makeString("unknown");
#endif
}

EppNativeValue fn_sys_version(const EppNativeValue*, int) { return makeString("epp-std-0.2.3"); }
EppNativeValue fn_sys_executable(const EppNativeValue*, int) { return makeString("epp"); }

EppNativeValue fn_os_getcwd(const EppNativeValue*, int) {
    return makeString(std::filesystem::current_path().string());
}

EppNativeValue fn_os_exists(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeBool(false);
    return makeBool(std::filesystem::exists(std::filesystem::path(args[0].string_value)));
}

EppNativeValue fn_os_is_file(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeBool(false);
    return makeBool(std::filesystem::is_regular_file(std::filesystem::path(args[0].string_value)));
}

EppNativeValue fn_os_is_dir(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeBool(false);
    return makeBool(std::filesystem::is_directory(std::filesystem::path(args[0].string_value)));
}

EppNativeValue fn_os_mkdir(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeBool(false);
    std::error_code ec;
    bool ok = std::filesystem::create_directories(std::filesystem::path(args[0].string_value), ec);
    return makeBool(ok && !ec);
}

EppNativeValue fn_os_remove(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeBool(false);
    std::error_code ec;
    bool ok = std::filesystem::remove(std::filesystem::path(args[0].string_value), ec);
    return makeBool(ok && !ec);
}

EppNativeValue fn_json_quote(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeString("\"\"");
    return makeString(jsonQuote(args[0].string_value));
}

EppNativeValue fn_json_is_valid_number(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeBool(false);
    try {
        std::string s = args[0].string_value;
        size_t idx = 0;
        (void)std::stod(s, &idx);
        return makeBool(idx == s.size());
    } catch (...) {
        return makeBool(false);
    }
}

EppNativeValue fn_json_parse_number(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeNull();
    try {
        return makeNumber(std::stod(std::string(args[0].string_value)));
    } catch (...) {
        return makeNull();
    }
}

EppNativeValue fn_csv_join2(const EppNativeValue* args, int argc) {
    if (argc < 2 || !isString(args[0]) || !isString(args[1])) return makeString("");
    return makeString(jsonQuote(args[0].string_value) + "," + jsonQuote(args[1].string_value));
}

EppNativeValue fn_urllib_quote(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isString(args[0])) return makeString("");
    return makeString(urlEncode(args[0].string_value));
}

EppNativeValue fn_color_reset(const EppNativeValue*, int) { return makeString(ansiReset()); }
EppNativeValue fn_color_bold(const EppNativeValue*, int) { return makeString(ansiBold()); }
EppNativeValue fn_color_dim(const EppNativeValue*, int) { return makeString(ansiDim()); }
EppNativeValue fn_color_underline(const EppNativeValue*, int) { return makeString(ansiUnderline()); }

EppNativeValue fn_color_apply(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeString("");
    return makeString(colorApply(asString(args[0], ""), asString(args[1], "")));
}

EppNativeValue fn_color_apply_style(const EppNativeValue* args, int argc) {
    if (argc < 3) return makeString("");
    return makeString(colorApplyStyle(asString(args[0], ""), asString(args[1], ""), asString(args[2], "")));
}

EppNativeValue fn_color_strip_ansi(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(ansiStrip(asString(args[0], "")));
}

EppNativeValue fn_color_fg_code(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(ansiFgCode(asString(args[0], "")));
}

EppNativeValue fn_color_bg_code(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(ansiBgCode(asString(args[0], "")));
}

EppNativeValue fn_color_colorize(const EppNativeValue* args, int argc) {
    if (argc < 6) return makeString("");
    const std::string text = asString(args[0], "");
    const std::string fg = asString(args[1], "");
    const std::string bg = asString(args[2], "");
    const bool bold = asBool(args[3], false);
    const bool underline = asBool(args[4], false);
    const bool dim = asBool(args[5], false);

    std::string pref;
    if (bold) pref += ansiBold();
    if (underline) pref += ansiUnderline();
    if (dim) pref += ansiDim();
    pref += ansiFgCode(fg);
    pref += ansiBgCode(bg);
    return makeString(pref + text + ansiReset());
}

EppNativeValue fn_color_paint(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeString("");
    const std::string text = asString(args[0], "");
    const std::string fg = asString(args[1], "");
    return makeString(ansiFgCode(fg) + text + ansiReset());
}

EppNativeValue fn_color_highlight(const EppNativeValue* args, int argc) {
    if (argc < 3) return makeString("");
    const std::string text = asString(args[0], "");
    const std::string fg = asString(args[1], "");
    const std::string bg = asString(args[2], "");
    return makeString(ansiFgCode(fg) + ansiBgCode(bg) + text + ansiReset());
}

EppNativeValue fn_color_clamp255(const EppNativeValue* args, int argc) {
    if (argc < 1 || !isNumber(args[0])) return makeNumber(0);
    return makeNumber(static_cast<double>(clamp255Int(static_cast<int>(args[0].number_value))));
}

EppNativeValue fn_color_rgb_code(const EppNativeValue* args, int argc) {
    if (argc < 3) return makeString("");
    return makeString(ansiRgbCode(asInt(args[0], 0), asInt(args[1], 0), asInt(args[2], 0)));
}

EppNativeValue fn_color_bg_rgb_code(const EppNativeValue* args, int argc) {
    if (argc < 3) return makeString("");
    return makeString(ansiBgRgbCode(asInt(args[0], 0), asInt(args[1], 0), asInt(args[2], 0)));
}

EppNativeValue fn_color_rgb(const EppNativeValue* args, int argc) {
    if (argc < 4) return makeString("");
    const std::string text = asString(args[0], "");
    const std::string code = ansiRgbCode(asInt(args[1], 0), asInt(args[2], 0), asInt(args[3], 0));
    return makeString(code + text + ansiReset());
}

EppNativeValue fn_color_bg_rgb(const EppNativeValue* args, int argc) {
    if (argc < 4) return makeString("");
    const std::string text = asString(args[0], "");
    const std::string code = ansiBgRgbCode(asInt(args[1], 0), asInt(args[2], 0), asInt(args[3], 0));
    return makeString(code + text + ansiReset());
}

EppNativeValue fn_color_rgb_style(const EppNativeValue* args, int argc) {
    if (argc < 7) return makeString("");
    const std::string text = asString(args[0], "");
    const std::string fg = ansiRgbCode(asInt(args[1], 0), asInt(args[2], 0), asInt(args[3], 0));
    const std::string bg = ansiBgRgbCode(asInt(args[4], 0), asInt(args[5], 0), asInt(args[6], 0));
    return makeString(fg + bg + text + ansiReset());
}

EppNativeValue fn_color_rojo(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[31m"));
}
EppNativeValue fn_color_verde(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[32m"));
}
EppNativeValue fn_color_azul(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[34m"));
}
EppNativeValue fn_color_amarillo(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[33m"));
}
EppNativeValue fn_color_magenta(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[35m"));
}
EppNativeValue fn_color_cyan(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[36m"));
}
EppNativeValue fn_color_blanco(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[37m"));
}

EppNativeValue fn_color_bg_rojo(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[41m"));
}
EppNativeValue fn_color_bg_verde(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[42m"));
}
EppNativeValue fn_color_bg_azul(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[44m"));
}
EppNativeValue fn_color_bg_amarillo(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[43m"));
}
EppNativeValue fn_color_bg_magenta(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[45m"));
}
EppNativeValue fn_color_bg_cyan(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[46m"));
}
EppNativeValue fn_color_bg_blanco(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApply(asString(args[0], ""), "\033[47m"));
}

EppNativeValue fn_color_negrita(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApplyStyle(asString(args[0], ""), "", ansiBold()));
}

EppNativeValue fn_color_subrayado(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApplyStyle(asString(args[0], ""), "", ansiUnderline()));
}

EppNativeValue fn_color_tenue(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApplyStyle(asString(args[0], ""), "", ansiDim()));
}

EppNativeValue fn_color_alerta(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApplyStyle(asString(args[0], ""), "\033[31m", ansiBold()));
}

EppNativeValue fn_color_exito(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApplyStyle(asString(args[0], ""), "\033[32m", ansiBold()));
}

EppNativeValue fn_color_info(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    return makeString(colorApplyStyle(asString(args[0], ""), "\033[36m", ansiBold()));
}

EppNativeValue fn_color_error(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    const std::string text = asString(args[0], "");
    return makeString(ansiBold() + ansiFgCode("red") + text + ansiReset());
}

EppNativeValue fn_color_success(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    const std::string text = asString(args[0], "");
    return makeString(ansiBold() + ansiFgCode("green") + text + ansiReset());
}

EppNativeValue fn_color_warning(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    const std::string text = asString(args[0], "");
    return makeString(ansiBold() + ansiFgCode("yellow") + text + ansiReset());
}

EppNativeValue fn_color_console_init(const EppNativeValue*, int) {
    return makeBool(colorConsoleInit());
}

EppNativeValue fn_color_stdout_is_console(const EppNativeValue*, int) {
    return makeBool(colorStdoutIsConsole());
}

EppNativeValue fn_color_print(const EppNativeValue* args, int argc) {
    (void)colorConsoleInit();
    for (int i = 0; i < argc; ++i) {
        if (i > 0) std::cout << ' ';
        std::cout << decodeEscapesForConsole(nativeValueToString(args[i]));
    }
    std::cout << std::endl;
    return makeNull();
}

EppNativeValue fn_http_get(const EppNativeValue*, int) {
    return makeString("http.get no disponible en runtime actual");
}

EppNativeValue fn_http_server_listen(const EppNativeValue* args, int argc) {
    const std::string host = (argc >= 1) ? asString(args[0], "0.0.0.0") : "0.0.0.0";
    const int port = (argc >= 2) ? asInt(args[1], 8080) : 8080;
    return makeNumber(static_cast<double>(httpServerListen(host, port)));
}

EppNativeValue fn_http_server_accept(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeNumber(-1);
    const int serverId = asInt(args[0], -1);
    return makeNumber(static_cast<double>(httpServerAccept(serverId)));
}

EppNativeValue fn_http_server_close(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeBool(false);
    const int serverId = asInt(args[0], -1);
    return makeBool(httpServerClose(serverId));
}

EppNativeValue fn_http_client_close(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeBool(false);
    const int clientId = asInt(args[0], -1);
    httpClientClose(clientId);
    return makeBool(true);
}

EppNativeValue fn_http_request_line(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    const int clientId = asInt(args[0], -1);
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpClients.find(clientId);
    if (it == g_httpClients.end()) return makeString("");
    return makeString(it->second.req.requestLine);
}

EppNativeValue fn_http_request_method(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    const int clientId = asInt(args[0], -1);
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpClients.find(clientId);
    if (it == g_httpClients.end()) return makeString("");
    return makeString(it->second.req.method);
}

EppNativeValue fn_http_request_path(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    const int clientId = asInt(args[0], -1);
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpClients.find(clientId);
    if (it == g_httpClients.end()) return makeString("");
    return makeString(it->second.req.path);
}

EppNativeValue fn_http_request_version(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    const int clientId = asInt(args[0], -1);
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpClients.find(clientId);
    if (it == g_httpClients.end()) return makeString("");
    return makeString(it->second.req.version);
}

EppNativeValue fn_http_request_body(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeString("");
    const int clientId = asInt(args[0], -1);
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpClients.find(clientId);
    if (it == g_httpClients.end()) return makeString("");
    return makeString(it->second.req.body);
}

EppNativeValue fn_http_request_header(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeString("");
    const int clientId = asInt(args[0], -1);
    const std::string key = toLowerCopy(asString(args[1], ""));
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpClients.find(clientId);
    if (it == g_httpClients.end()) return makeString("");
    auto h = it->second.req.headers.find(key);
    if (h == it->second.req.headers.end()) return makeString("");
    return makeString(h->second);
}

EppNativeValue fn_http_response_send(const EppNativeValue* args, int argc) {
    if (argc < 5) return makeBool(false);
    const int clientId = asInt(args[0], -1);
    const int statusCode = asInt(args[1], 200);
    std::string statusText = asString(args[2], "");
    const std::string contentType = asString(args[3], "text/plain; charset=utf-8");
    const std::string body = asString(args[4], "");
    if (statusText.empty()) statusText = statusTextFromCode(statusCode);
    return makeBool(httpResponseSend(clientId, statusCode, statusText, contentType, body));
}

EppNativeValue fn_http_server_once(const EppNativeValue* args, int argc) {
    const int port = (argc >= 1) ? asInt(args[0], 8080) : 8080;
    const std::string body = (argc >= 2) ? asString(args[1], "Hola desde E++") : "Hola desde E++";
    const std::string status = (argc >= 3) ? asString(args[2], "200 OK") : "200 OK";
    if (port <= 0 || port > 65535) return makeString("ERROR: puerto invalido");
    const int serverId = httpServerListen("0.0.0.0", port);
    if (serverId <= 0) return makeString("ERROR: no se pudo abrir el servidor");
    const int clientId = httpServerAccept(serverId);
    if (clientId <= 0) {
        httpServerClose(serverId);
        return makeString("ERROR: no se pudo aceptar cliente");
    }

    std::string requestLine = "(sin request-line)";
    {
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(clientId);
        if (it != g_httpClients.end() && !it->second.req.requestLine.empty()) requestLine = it->second.req.requestLine;
    }

    int statusCode = 200;
    std::string statusText = status;
    {
        std::istringstream iss(status);
        if (!(iss >> statusCode)) statusCode = 200;
        std::string rest;
        std::getline(iss, rest);
        rest = trimCopy(rest);
        statusText = rest.empty() ? statusTextFromCode(statusCode) : rest;
    }
    (void)httpResponseSend(clientId, statusCode, statusText, "text/plain; charset=utf-8", body);
    httpServerClose(serverId);
    return makeString(requestLine);
}

EppNativeValue fn_http_server_loop(const EppNativeValue* args, int argc) {
    const int port = (argc >= 1) ? asInt(args[0], 8080) : 8080;
    const std::string body = (argc >= 2) ? asString(args[1], "Hola desde E++") : "Hola desde E++";
    const int maxRequests = (argc >= 3) ? asInt(args[2], 0) : 0;
    const std::string status = (argc >= 4) ? asString(args[3], "200 OK") : "200 OK";
    if (port <= 0 || port > 65535) return makeNumber(-1);
    const int serverId = httpServerListen("0.0.0.0", port);
    if (serverId <= 0) return makeNumber(-1);

    int statusCode = 200;
    std::string statusText = status;
    {
        std::istringstream iss(status);
        if (!(iss >> statusCode)) statusCode = 200;
        std::string rest;
        std::getline(iss, rest);
        rest = trimCopy(rest);
        statusText = rest.empty() ? statusTextFromCode(statusCode) : rest;
    }

    int handled = 0;
    while (maxRequests <= 0 || handled < maxRequests) {
        const int clientId = httpServerAccept(serverId);
        if (clientId <= 0) break;
        if (!httpResponseSend(clientId, statusCode, statusText, "text/plain; charset=utf-8", body)) break;
        ++handled;
    }
    httpServerClose(serverId);
    return makeNumber(static_cast<double>(handled));
}

EppNativeValue fn_sqlite3_open(const EppNativeValue*, int) {
    return makeString("sqlite3 no disponible en runtime actual");
}

EppNativeValue fn_threading_cpu_count(const EppNativeValue*, int) {
    return makeNumber(static_cast<double>(std::thread::hardware_concurrency()));
}

EppNativeValue fn_asyncio_sleep(const EppNativeValue* args, int argc) { return fn_time_sleep(args, argc); }

EppNativeValue fn_tkinter_available(const EppNativeValue*, int) { return makeBool(true); }

EppNativeValue fn_tkinter_window_create(const EppNativeValue* args, int argc) {
    const std::string title = (argc >= 1) ? asString(args[0], "E++ App") : "E++ App";
    const int width = (argc >= 2) ? asInt(args[1], 640) : 640;
    const int height = (argc >= 3) ? asInt(args[2], 420) : 420;

    std::lock_guard<std::mutex> lock(g_tkMutex);
    const int id = g_nextTkWindowId++;
    TkWindowCtx w;
    w.id = id;
    w.title = title;
    w.width = std::max(220, width);
    w.height = std::max(160, height);
    g_tkWindows[id] = w;
    return makeNumber(static_cast<double>(id));
}

EppNativeValue fn_tkinter_window_set_title(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeBool(false);
    const int windowId = asInt(args[0], -1);
    const std::string title = asString(args[1], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeBool(false);
    it->second.title = title;
    return makeBool(true);
}

EppNativeValue fn_tkinter_label_add(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeNumber(-1);
    const int windowId = asInt(args[0], -1);
    const std::string text = asString(args[1], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeNumber(-1);
    return makeNumber(static_cast<double>(tkAddWidget(it->second, TkWidgetType::Label, text)));
}

EppNativeValue fn_tkinter_button_add(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeNumber(-1);
    const int windowId = asInt(args[0], -1);
    const std::string text = asString(args[1], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeNumber(-1);
    return makeNumber(static_cast<double>(tkAddWidget(it->second, TkWidgetType::Button, text)));
}

EppNativeValue fn_tkinter_entry_add(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeNumber(-1);
    const int windowId = asInt(args[0], -1);
    const std::string text = asString(args[1], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeNumber(-1);
    return makeNumber(static_cast<double>(tkAddWidget(it->second, TkWidgetType::Entry, text)));
}

EppNativeValue fn_tkinter_widget_set_text(const EppNativeValue* args, int argc) {
    if (argc < 3) return makeBool(false);
    const int windowId = asInt(args[0], -1);
    const int widgetId = asInt(args[1], -1);
    const std::string text = asString(args[2], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeBool(false);
    auto wt = it->second.widgets.find(widgetId);
    if (wt == it->second.widgets.end()) return makeBool(false);
    wt->second.text = text;
    return makeBool(true);
}

EppNativeValue fn_tkinter_widget_get_text(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeString("");
    const int windowId = asInt(args[0], -1);
    const int widgetId = asInt(args[1], -1);
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeString("");
    auto wt = it->second.widgets.find(widgetId);
    if (wt == it->second.widgets.end()) return makeString("");
    return makeString(wt->second.text);
}

EppNativeValue fn_tkinter_window_set_bg(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeBool(false);
    const int windowId = asInt(args[0], -1);
    const std::string value = asString(args[1], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeBool(false);
    it->second.bg = value;
    return makeBool(true);
}

EppNativeValue fn_tkinter_window_set_fg(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeBool(false);
    const int windowId = asInt(args[0], -1);
    const std::string value = asString(args[1], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeBool(false);
    it->second.fg = value;
    return makeBool(true);
}

EppNativeValue fn_tkinter_window_set_accent(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeBool(false);
    const int windowId = asInt(args[0], -1);
    const std::string value = asString(args[1], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeBool(false);
    it->second.accent = value;
    return makeBool(true);
}

EppNativeValue fn_tkinter_window_apply_scss(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeBool(false);
    const int windowId = asInt(args[0], -1);
    const std::string path = asString(args[1], "");
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeBool(false);
    tkTryApplyScss(it->second, path);
    return makeBool(true);
}

EppNativeValue fn_tkinter_button_clicked(const EppNativeValue* args, int argc) {
    if (argc < 2) return makeNumber(0);
    const int windowId = asInt(args[0], -1);
    const int buttonId = asInt(args[1], -1);
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeNumber(0);
    if (it->second.lastClickedButton == buttonId) {
        it->second.lastClickedButton = 0;
        return makeNumber(1);
    }
    return makeNumber(0);
}

EppNativeValue fn_tkinter_window_show(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeNull();
    const int windowId = asInt(args[0], -1);
    std::lock_guard<std::mutex> lock(g_tkMutex);
    auto it = g_tkWindows.find(windowId);
    if (it == g_tkWindows.end()) return makeNull();
    tkRender(it->second);
    return makeNull();
}

EppNativeValue fn_tkinter_mainloop(const EppNativeValue* args, int argc) {
    if (argc < 1) return makeBool(false);
    const int windowId = asInt(args[0], -1);

    TkWindowCtx snapshot;
    {
        std::lock_guard<std::mutex> lock(g_tkMutex);
        auto it = g_tkWindows.find(windowId);
        if (it == g_tkWindows.end()) return makeBool(false);
        snapshot = it->second;
    }

    tkRender(snapshot);

    std::unordered_map<int, std::string> entryUpdates;
    std::vector<int> buttons;
    for (int id : snapshot.widgetOrder) {
        auto it = snapshot.widgets.find(id);
        if (it == snapshot.widgets.end()) continue;
        const TkWidgetCtx& wd = it->second;
        if (wd.type == TkWidgetType::Entry) {
            std::cout << "Input for Entry #" << id << " (empty = keep): ";
            std::string line;
            std::getline(std::cin, line);
            if (!line.empty()) entryUpdates[id] = line;
        } else if (wd.type == TkWidgetType::Button) {
            buttons.push_back(id);
        }
    }

    int click = 0;
    if (!buttons.empty()) {
        std::cout << "Click button id (0 = none): ";
        std::string raw;
        std::getline(std::cin, raw);
        try {
            click = std::stoi(trimCopy(raw));
        } catch (...) {
            click = 0;
        }
    }

    {
        std::lock_guard<std::mutex> lock(g_tkMutex);
        auto it = g_tkWindows.find(windowId);
        if (it == g_tkWindows.end()) return makeBool(false);
        for (const auto& kv : entryUpdates) {
            auto wt = it->second.widgets.find(kv.first);
            if (wt != it->second.widgets.end() && wt->second.type == TkWidgetType::Entry) wt->second.text = kv.second;
        }
        if (click > 0 && tkIsButton(it->second, click)) {
            it->second.lastClickedButton = click;
        } else {
            it->second.lastClickedButton = 0;
        }
    }
    return makeBool(true);
}
} // namespace

extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
int epp_register_v2(EppNativeEntryV2* out_entries, int max_entries) {
    if (!out_entries || max_entries <= 0) return 0;

    EppNativeEntryV2 entries[] = {
        {"time_time", 0, fn_time_time},
        {"time_monotonic", 0, fn_time_monotonic},
        {"time_sleep", 1, fn_time_sleep},
        {"datetime_now_iso", 0, fn_datetime_now_iso},
        {"datetime_year", 0, fn_datetime_year},
        {"datetime_month", 0, fn_datetime_month},
        {"datetime_day", 0, fn_datetime_day},
        {"math_pi", 0, fn_math_pi},
        {"math_e", 0, fn_math_e},
        {"math_sqrt", 1, fn_math_sqrt},
        {"math_pow", 2, fn_math_pow},
        {"math_floor", 1, fn_math_floor},
        {"math_ceil", 1, fn_math_ceil},
        {"math_sin", 1, fn_math_sin},
        {"math_cos", 1, fn_math_cos},
        {"math_tan", 1, fn_math_tan},
        {"math_log", 1, fn_math_log},
        {"math_exp", 1, fn_math_exp},
        {"random_seed", -1, fn_random_seed},
        {"random_random", 0, fn_random_random},
        {"random_randint", 2, fn_random_randint},
        {"random_uniform", 2, fn_random_uniform},
        {"sys_platform", 0, fn_sys_platform},
        {"sys_version", 0, fn_sys_version},
        {"sys_executable", 0, fn_sys_executable},
        {"os_getcwd", 0, fn_os_getcwd},
        {"os_exists", 1, fn_os_exists},
        {"os_is_file", 1, fn_os_is_file},
        {"os_is_dir", 1, fn_os_is_dir},
        {"os_mkdir", 1, fn_os_mkdir},
        {"os_remove", 1, fn_os_remove},
        {"json_quote", 1, fn_json_quote},
        {"json_is_valid_number", 1, fn_json_is_valid_number},
        {"json_parse_number", 1, fn_json_parse_number},
        {"csv_join2", 2, fn_csv_join2},
        {"urllib_quote", 1, fn_urllib_quote},
        {"color_reset", 0, fn_color_reset},
        {"color_bold", 0, fn_color_bold},
        {"color_dim", 0, fn_color_dim},
        {"color_underline", 0, fn_color_underline},
        {"color_apply", 2, fn_color_apply},
        {"color_apply_style", 3, fn_color_apply_style},
        {"color_strip_ansi", 1, fn_color_strip_ansi},
        {"color_fg_code", 1, fn_color_fg_code},
        {"color_bg_code", 1, fn_color_bg_code},
        {"color_colorize", 6, fn_color_colorize},
        {"color_paint", 2, fn_color_paint},
        {"color_highlight", 3, fn_color_highlight},
        {"color_clamp255", 1, fn_color_clamp255},
        {"color_rgb_code", 3, fn_color_rgb_code},
        {"color_bg_rgb_code", 3, fn_color_bg_rgb_code},
        {"color_rgb", 4, fn_color_rgb},
        {"color_bg_rgb", 4, fn_color_bg_rgb},
        {"color_rgb_style", 7, fn_color_rgb_style},
        {"color_rojo", 1, fn_color_rojo},
        {"color_verde", 1, fn_color_verde},
        {"color_azul", 1, fn_color_azul},
        {"color_amarillo", 1, fn_color_amarillo},
        {"color_magenta", 1, fn_color_magenta},
        {"color_cyan", 1, fn_color_cyan},
        {"color_blanco", 1, fn_color_blanco},
        {"color_bg_rojo", 1, fn_color_bg_rojo},
        {"color_bg_verde", 1, fn_color_bg_verde},
        {"color_bg_azul", 1, fn_color_bg_azul},
        {"color_bg_amarillo", 1, fn_color_bg_amarillo},
        {"color_bg_magenta", 1, fn_color_bg_magenta},
        {"color_bg_cyan", 1, fn_color_bg_cyan},
        {"color_bg_blanco", 1, fn_color_bg_blanco},
        {"color_negrita", 1, fn_color_negrita},
        {"color_subrayado", 1, fn_color_subrayado},
        {"color_tenue", 1, fn_color_tenue},
        {"color_alerta", 1, fn_color_alerta},
        {"color_exito", 1, fn_color_exito},
        {"color_info", 1, fn_color_info},
        {"color_error", 1, fn_color_error},
        {"color_success", 1, fn_color_success},
        {"color_warning", 1, fn_color_warning},
        {"color_console_init", 0, fn_color_console_init},
        {"color_stdout_is_console", 0, fn_color_stdout_is_console},
        {"color_print", -1, fn_color_print},
        {"http_get", 0, fn_http_get},
        {"http_server_listen", 2, fn_http_server_listen},
        {"http_server_accept", 1, fn_http_server_accept},
        {"http_server_close", 1, fn_http_server_close},
        {"http_client_close", 1, fn_http_client_close},
        {"http_request_line", 1, fn_http_request_line},
        {"http_request_method", 1, fn_http_request_method},
        {"http_request_path", 1, fn_http_request_path},
        {"http_request_version", 1, fn_http_request_version},
        {"http_request_body", 1, fn_http_request_body},
        {"http_request_header", 2, fn_http_request_header},
        {"http_response_send", 5, fn_http_response_send},
        {"http_server_once", -1, fn_http_server_once},
        {"http_server_loop", -1, fn_http_server_loop},
        {"sqlite3_open", 0, fn_sqlite3_open},
        {"threading_cpu_count", 0, fn_threading_cpu_count},
        {"asyncio_sleep", 1, fn_asyncio_sleep},
        {"tkinter_available", 0, fn_tkinter_available},
        {"tkinter_window_create", 3, fn_tkinter_window_create},
        {"tkinter_window_set_title", 2, fn_tkinter_window_set_title},
        {"tkinter_label_add", 6, fn_tkinter_label_add},
        {"tkinter_button_add", 6, fn_tkinter_button_add},
        {"tkinter_entry_add", 6, fn_tkinter_entry_add},
        {"tkinter_widget_set_text", 3, fn_tkinter_widget_set_text},
        {"tkinter_widget_get_text", 2, fn_tkinter_widget_get_text},
        {"tkinter_window_set_bg", 2, fn_tkinter_window_set_bg},
        {"tkinter_window_set_fg", 2, fn_tkinter_window_set_fg},
        {"tkinter_window_set_accent", 2, fn_tkinter_window_set_accent},
        {"tkinter_window_apply_scss", 2, fn_tkinter_window_apply_scss},
        {"tkinter_button_clicked", 2, fn_tkinter_button_clicked},
        {"tkinter_window_show", 1, fn_tkinter_window_show},
        {"tkinter_mainloop", 1, fn_tkinter_mainloop},
    };

    const int total = static_cast<int>(sizeof(entries) / sizeof(entries[0]));
    const int n = std::min(total, max_entries);
    for (int i = 0; i < n; ++i) out_entries[i] = entries[i];
    return n;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int epp_register(EppNativeEntry* out_entries, int max_entries) {
    if (!out_entries || max_entries < 2) return 0;

    auto legacy_sqrt = [](const double* args, int argc) -> double {
        if (argc < 1) return 0.0;
        return std::sqrt(args[0]);
    };

    auto legacy_pow = [](const double* args, int argc) -> double {
        if (argc < 2) return 0.0;
        return std::pow(args[0], args[1]);
    };

    out_entries[0].name = "math_sqrt";
    out_entries[0].arity = 1;
    out_entries[0].fn = legacy_sqrt;

    out_entries[1].name = "math_pow";
    out_entries[1].arity = 2;
    out_entries[1].fn = legacy_pow;
    return 2;
}
}
