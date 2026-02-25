#include "runtime.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "epp_native.h"

#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <array>
#include <thread>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dlfcn.h>
#endif

namespace {
struct ReturnSignal : std::runtime_error {
    explicit ReturnSignal(const Value& v) : std::runtime_error("return"), value(v) {}
    Value value;
};

bool hasPathHints(const std::string& moduleName) {
    return moduleName.find('/') != std::string::npos || moduleName.find('\\') != std::string::npos ||
           (!moduleName.empty() && moduleName[0] == '.');
}

std::string trimInline(std::string s) {
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

std::string parsePackageIdFromLine(const std::string& rawLine) {
    const std::string line = trimInline(rawLine);
    if (line.rfind("@package", 0) != 0) return "";

    std::string rest = trimInline(line.substr(8));
    if (rest.empty()) return "";
    if (!rest.empty() && rest[0] == '"') {
        const size_t endQuote = rest.find('"', 1);
        if (endQuote == std::string::npos || endQuote <= 1) return "";
        return rest.substr(1, endQuote - 1);
    }

    size_t end = 0;
    while (end < rest.size()) {
        const char c = rest[end];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.' ||
            c == '-') {
            ++end;
            continue;
        }
        break;
    }
    if (end == 0) return "";
    return rest.substr(0, end);
}

std::string readPackageIdFromModule(const std::filesystem::path& moduleFile) {
    std::ifstream in(moduleFile, std::ios::binary);
    if (!in) return "";
    std::string line;
    for (int i = 0; i < 48 && std::getline(in, line); ++i) {
        const std::string id = parsePackageIdFromLine(line);
        if (!id.empty()) return id;
    }
    return "";
}

std::filesystem::path moduleLexemeToPath(const std::string& moduleLexeme) {
    if (hasPathHints(moduleLexeme)) return std::filesystem::path(moduleLexeme);
    std::string normalized = moduleLexeme;
    for (char& c : normalized) {
        if (c == '.') c = '/';
    }
    return std::filesystem::path(normalized);
}

std::vector<std::filesystem::path> modulePathCandidates(const std::filesystem::path& modulePath) {
    std::vector<std::filesystem::path> out;
    out.push_back(modulePath);
    if (modulePath.extension() != ".epp") {
        out.push_back(modulePath.string() + ".epp");
    }
    out.push_back(modulePath / "__init__");
    out.push_back(modulePath / "__init__.epp");
    out.push_back(modulePath / ".__init__");
    out.push_back(modulePath / ".__init__.epp");
    return out;
}

std::vector<std::filesystem::path> buildAncestorDirs(const std::filesystem::path& start) {
    std::vector<std::filesystem::path> dirs;
    if (start.empty()) return dirs;

    std::filesystem::path current = start;
    while (!current.empty()) {
        dirs.push_back(current);
        auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return dirs;
}

std::vector<std::filesystem::path> importPrefixes(const std::filesystem::path& dir) {
    return {
        dir,
        dir / "libs",
        dir / "libs" / "stdlib",
        dir / "lib" / "libs",
        dir / "lib" / "libs" / "stdlib",
        dir / "examples" / "libs",
        dir / "examples" / "libs" / "stdlib",
    };
}

std::filesystem::path resolveImportPath(const std::string& moduleLexeme,
                                        const std::string& currentFilePath,
                                        const std::unordered_map<std::string, std::string>& packageIdToModulePath) {
    const auto known = packageIdToModulePath.find(moduleLexeme);
    if (known != packageIdToModulePath.end()) {
        std::filesystem::path knownPath(known->second);
        if (std::filesystem::exists(knownPath)) return knownPath;
    }

    std::filesystem::path modulePath = moduleLexemeToPath(moduleLexeme);
    const bool explicitPath = hasPathHints(moduleLexeme) || modulePath.is_absolute();
    const std::vector<std::filesystem::path> relativeCandidates = modulePathCandidates(modulePath);

    std::vector<std::filesystem::path> roots;
    if (!currentFilePath.empty()) {
        roots.push_back(std::filesystem::path(currentFilePath).parent_path());
    }
    roots.push_back(std::filesystem::current_path());

    std::vector<std::filesystem::path> searchDirs;
    for (const auto& root : roots) {
        const auto ancestors = buildAncestorDirs(std::filesystem::absolute(root));
        searchDirs.insert(searchDirs.end(), ancestors.begin(), ancestors.end());
    }

    for (const auto& dir : searchDirs) {
        if (explicitPath) {
            for (const auto& rel : relativeCandidates) {
                const std::filesystem::path candidate = std::filesystem::absolute(dir / rel);
                if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate)) return candidate;
            }
            continue;
        }

        for (const auto& prefix : importPrefixes(dir)) {
            for (const auto& rel : relativeCandidates) {
                const std::filesystem::path candidate = std::filesystem::absolute(prefix / rel);
                if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate)) return candidate;
            }
        }
    }

    if (!explicitPath) {
        std::error_code ec;
        for (const auto& dir : searchDirs) {
            for (const auto& prefix : importPrefixes(dir)) {
                if (!std::filesystem::exists(prefix, ec) || !std::filesystem::is_directory(prefix, ec)) continue;
                for (std::filesystem::recursive_directory_iterator it(prefix,
                                                                       std::filesystem::directory_options::skip_permission_denied,
                                                                       ec),
                     end;
                     it != end;
                     it.increment(ec)) {
                    if (ec) {
                        ec.clear();
                        continue;
                    }
                    if (!it->is_regular_file(ec)) continue;
                    const std::string fileName = it->path().filename().string();
                    if (fileName != "__init__" && fileName != "__init__.epp" && fileName != ".__init__" &&
                        fileName != ".__init__.epp") {
                        continue;
                    }
                    if (readPackageIdFromModule(it->path()) == moduleLexeme) {
                        return std::filesystem::absolute(it->path());
                    }
                }
            }
        }
    }

    const std::filesystem::path fallbackBase =
        !currentFilePath.empty() ? std::filesystem::path(currentFilePath).parent_path()
                                 : std::filesystem::current_path();
    const std::filesystem::path fallbackLeaf = modulePath.extension() == ".epp" ? modulePath : (modulePath / "__init__");
    return std::filesystem::absolute(fallbackBase / fallbackLeaf);
}

bool blockNeedsOwnScope(const BlockStmt& block) {
    for (const auto& stmt : block.statements) {
        if (std::dynamic_pointer_cast<VarDeclStmt>(stmt) ||
            std::dynamic_pointer_cast<FuncDeclStmt>(stmt) ||
            std::dynamic_pointer_cast<ClassDeclStmt>(stmt)) {
            return true;
        }
    }
    return false;
}

double toNumberOrDefault(const Value& v, double fallback) {
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1.0 : 0.0;
    return fallback;
}

std::string toStringOrDefault(const Value& v, const std::string& fallback) {
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<std::monostate>(v)) return fallback;
    return valueToString(v);
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

struct HttpRequestParts {
    std::string requestLine;
    std::string method;
    std::string path;
    std::string version;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

#ifdef _WIN32
using HttpSocket = SOCKET;
constexpr HttpSocket kInvalidHttpSocket = INVALID_SOCKET;
#else
using HttpSocket = int;
constexpr HttpSocket kInvalidHttpSocket = -1;
#endif

struct HttpServerCtx {
    HttpSocket socket = kInvalidHttpSocket;
    std::string host;
    int port = 0;
};

struct HttpClientCtx {
    HttpSocket socket = kInvalidHttpSocket;
    HttpRequestParts req;
};

std::mutex g_httpMutex;
int g_nextHttpServerId = 1;
int g_nextHttpClientId = 1;
std::unordered_map<int, HttpServerCtx> g_httpServers;
std::unordered_map<int, HttpClientCtx> g_httpClients;
std::mt19937_64 g_rng{std::random_device{}()};

void httpCloseSocket(HttpSocket s) {
#ifdef _WIN32
    if (s != INVALID_SOCKET) closesocket(s);
#else
    if (s >= 0) close(s);
#endif
}

bool httpSocketStartup() {
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

std::string httpStatusTextFromCode(int code) {
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

bool httpSendAll(HttpSocket socket, const std::string& payload) {
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

bool httpBindAndListen(HttpSocket& outServer, const std::string& host, int port) {
    if (!httpSocketStartup()) return false;
    outServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (outServer == kInvalidHttpSocket) return false;

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
            httpCloseSocket(outServer);
            outServer = kInvalidHttpSocket;
            return false;
        }
    }

    if (bind(outServer, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        httpCloseSocket(outServer);
        outServer = kInvalidHttpSocket;
        return false;
    }
    if (listen(outServer, 64) < 0) {
        httpCloseSocket(outServer);
        outServer = kInvalidHttpSocket;
        return false;
    }
    return true;
}

int httpServerListen(const std::string& host, int port) {
    if (port <= 0 || port > 65535) return -1;
    HttpSocket s = kInvalidHttpSocket;
    if (!httpBindAndListen(s, host, port)) return -1;
    std::lock_guard<std::mutex> lock(g_httpMutex);
    const int id = g_nextHttpServerId++;
    g_httpServers[id] = HttpServerCtx{s, host, port};
    return id;
}

int httpServerAccept(int serverId) {
    HttpSocket server = kInvalidHttpSocket;
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
    HttpSocket client = accept(server, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
    if (client == kInvalidHttpSocket) return -1;

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
    httpCloseSocket(it->second.socket);
    g_httpClients.erase(it);
}

bool httpResponseSend(int clientId,
                      int statusCode,
                      const std::string& statusText,
                      const std::string& contentType,
                      const std::string& body) {
    HttpSocket socket = kInvalidHttpSocket;
    {
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(clientId);
        if (it == g_httpClients.end()) return false;
        socket = it->second.socket;
    }

    const std::string response = buildHttpResponse(statusCode, statusText, contentType, body);
    const bool ok = httpSendAll(socket, response);
    httpClientClose(clientId);
    return ok;
}

bool httpServerClose(int serverId) {
    std::lock_guard<std::mutex> lock(g_httpMutex);
    auto it = g_httpServers.find(serverId);
    if (it == g_httpServers.end()) return false;
    httpCloseSocket(it->second.socket);
    g_httpServers.erase(it);
    return true;
}
}

std::string valueToString(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) return "null";
    if (std::holds_alternative<double>(value)) {
        double n = std::get<double>(value);
        if (std::floor(n) == n) return std::to_string(static_cast<long long>(n));
        std::ostringstream out;
        out << std::setprecision(15) << n;
        return out.str();
    }
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? "true" : "false";
    if (std::holds_alternative<std::string>(value)) return std::get<std::string>(value);
    if (std::holds_alternative<std::shared_ptr<Function>>(value)) {
        const auto fn = std::get<std::shared_ptr<Function>>(value);
        return "<func " + fn->name + ">";
    }
    if (std::holds_alternative<std::shared_ptr<NativeFunction>>(value)) {
        const auto fn = std::get<std::shared_ptr<NativeFunction>>(value);
        return "<native " + fn->name + ">";
    }
    if (std::holds_alternative<std::shared_ptr<ClassValue>>(value)) {
        const auto klass = std::get<std::shared_ptr<ClassValue>>(value);
        return "<class " + klass->name + ">";
    }
    const auto instance = std::get<std::shared_ptr<InstanceValue>>(value);
    return "<" + instance->klass->name + " instance>";
}

bool valueIsTruthy(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) return false;
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
    if (std::holds_alternative<double>(value)) return std::get<double>(value) != 0.0;
    if (std::holds_alternative<std::string>(value)) return !std::get<std::string>(value).empty();
    return true;
}

bool valueEquals(const Value& a, const Value& b) {
    if (a.index() != b.index()) return false;
    if (std::holds_alternative<std::monostate>(a)) return true;
    if (std::holds_alternative<double>(a)) return std::get<double>(a) == std::get<double>(b);
    if (std::holds_alternative<bool>(a)) return std::get<bool>(a) == std::get<bool>(b);
    if (std::holds_alternative<std::string>(a)) return std::get<std::string>(a) == std::get<std::string>(b);
    if (std::holds_alternative<std::shared_ptr<Function>>(a)) {
        return std::get<std::shared_ptr<Function>>(a) == std::get<std::shared_ptr<Function>>(b);
    }
    if (std::holds_alternative<std::shared_ptr<NativeFunction>>(a)) {
        return std::get<std::shared_ptr<NativeFunction>>(a) == std::get<std::shared_ptr<NativeFunction>>(b);
    }
    if (std::holds_alternative<std::shared_ptr<ClassValue>>(a)) {
        return std::get<std::shared_ptr<ClassValue>>(a) == std::get<std::shared_ptr<ClassValue>>(b);
    }
    return std::get<std::shared_ptr<InstanceValue>>(a) == std::get<std::shared_ptr<InstanceValue>>(b);
}

Environment::Environment(std::shared_ptr<Environment> parent) : parent_(std::move(parent)) {}

void Environment::define(const std::string& name, const Value& value) { values_[name] = value; }

void Environment::assign(const std::string& name, const Value& value, int line) {
    auto it = values_.find(name);
    if (it != values_.end()) {
        it->second = value;
        return;
    }
    if (parent_) {
        parent_->assign(name, value, line);
        return;
    }
    throw EppError(ErrorPhase::Runtime,
                   "E-RUN-001",
                   line,
                   1,
                   "Variable no declarada: '" + name + "'.",
                   "Declara la variable con 'var " + name + " = ...' antes de usarla.",
                   name);
}

Value Environment::get(const std::string& name, int line) const {
    auto it = values_.find(name);
    if (it != values_.end()) return it->second;
    if (parent_) return parent_->get(name, line);
    throw EppError(ErrorPhase::Runtime,
                   "E-RUN-001",
                   line,
                   1,
                   "Variable no declarada: '" + name + "'.",
                   "Declara la variable con 'var " + name + " = ...' antes de usarla.",
                   name);
}

bool Environment::hasLocal(const std::string& name) const {
    return values_.find(name) != values_.end();
}

Value Environment::getLocal(const std::string& name) const {
    auto it = values_.find(name);
    if (it == values_.end()) return std::monostate{};
    return it->second;
}

void Environment::eraseLocal(const std::string& name) {
    values_.erase(name);
}

Interpreter::Interpreter() {
    globals_ = std::make_shared<Environment>();
    env_ = globals_;

    auto printFn = std::make_shared<NativeFunction>();
    printFn->name = "print";
    printFn->arity = -1;
    printFn->fn = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            std::cout << "\n";
            return std::monostate{};
        }
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << " ";
            std::cout << valueToString(args[i]);
        }
        std::cout << "\n";
        return std::monostate{};
    };
    globals_->define("print", printFn);

    auto inputFn = std::make_shared<NativeFunction>();
    inputFn->name = "input";
    inputFn->arity = 1;
    inputFn->fn = [](const std::vector<Value>& args) -> Value {
        std::cout << valueToString(args[0]);
        std::string line;
        std::getline(std::cin, line);
        return line;
    };
    globals_->define("input", inputFn);

    auto intFn = std::make_shared<NativeFunction>();
    intFn->name = "int";
    intFn->arity = 1;
    intFn->fn = [](const std::vector<Value>& args) -> Value {
        const Value& v = args[0];
        if (std::holds_alternative<double>(v)) return std::floor(std::get<double>(v));
        if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1.0 : 0.0;
        if (std::holds_alternative<std::string>(v)) {
            try {
                return static_cast<double>(std::stoll(std::get<std::string>(v)));
            } catch (...) {
                throw EppError(ErrorPhase::Runtime,
                               "E-RUN-011",
                               0,
                               1,
                               "No se pudo convertir a int().",
                               "Usa un texto numerico valido, por ejemplo: int(\"42\").");
            }
        }
        if (std::holds_alternative<std::monostate>(v)) return 0.0;
        throw EppError(ErrorPhase::Runtime,
                       "E-RUN-010",
                       0,
                       1,
                       "int() solo acepta numero, bool, texto o null.");
    };
    globals_->define("int", intFn);

    auto defineNative = [this](const std::string& name, int arity, std::function<Value(const std::vector<Value>&)> fn) {
        auto n = std::make_shared<NativeFunction>();
        n->name = name;
        n->arity = arity;
        n->fn = std::move(fn);
        globals_->define(name, n);
    };

    defineNative("time_time", 0, [](const std::vector<Value>&) -> Value {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration<double>(now).count();
    });
    defineNative("time_monotonic", 0, [](const std::vector<Value>&) -> Value {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        return std::chrono::duration<double>(now).count();
    });
    defineNative("time_sleep", 1, [](const std::vector<Value>& args) -> Value {
        const double seconds = std::max(0.0, toNumberOrDefault(args[0], 0.0));
        std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
        return std::monostate{};
    });
    defineNative("datetime_now_iso", 0, [](const std::vector<Value>&) -> Value { return nowIsoLocal(); });
    defineNative("datetime_year", 0, [](const std::vector<Value>&) -> Value {
        std::time_t tt = std::time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        return static_cast<double>(tm.tm_year + 1900);
    });
    defineNative("datetime_month", 0, [](const std::vector<Value>&) -> Value {
        std::time_t tt = std::time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        return static_cast<double>(tm.tm_mon + 1);
    });
    defineNative("datetime_day", 0, [](const std::vector<Value>&) -> Value {
        std::time_t tt = std::time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        return static_cast<double>(tm.tm_mday);
    });

    defineNative("math_pi", 0, [](const std::vector<Value>&) -> Value { return 3.14159265358979323846; });
    defineNative("math_e", 0, [](const std::vector<Value>&) -> Value { return 2.71828182845904523536; });
    defineNative("math_sqrt", 1, [](const std::vector<Value>& a) -> Value { return std::sqrt(toNumberOrDefault(a[0], 0.0)); });
    defineNative("math_pow", 2, [](const std::vector<Value>& a) -> Value {
        return std::pow(toNumberOrDefault(a[0], 0.0), toNumberOrDefault(a[1], 0.0));
    });
    defineNative("math_floor", 1, [](const std::vector<Value>& a) -> Value { return std::floor(toNumberOrDefault(a[0], 0.0)); });
    defineNative("math_ceil", 1, [](const std::vector<Value>& a) -> Value { return std::ceil(toNumberOrDefault(a[0], 0.0)); });
    defineNative("math_sin", 1, [](const std::vector<Value>& a) -> Value { return std::sin(toNumberOrDefault(a[0], 0.0)); });
    defineNative("math_cos", 1, [](const std::vector<Value>& a) -> Value { return std::cos(toNumberOrDefault(a[0], 0.0)); });
    defineNative("math_tan", 1, [](const std::vector<Value>& a) -> Value { return std::tan(toNumberOrDefault(a[0], 0.0)); });
    defineNative("math_log", 1, [](const std::vector<Value>& a) -> Value { return std::log(toNumberOrDefault(a[0], 1.0)); });
    defineNative("math_exp", 1, [](const std::vector<Value>& a) -> Value { return std::exp(toNumberOrDefault(a[0], 0.0)); });

    defineNative("random_seed", -1, [](const std::vector<Value>& a) -> Value {
        if (!a.empty() && std::holds_alternative<double>(a[0])) {
            g_rng.seed(static_cast<std::uint64_t>(std::get<double>(a[0])));
        } else {
            g_rng.seed(std::random_device{}());
        }
        return std::monostate{};
    });
    defineNative("random_random", 0, [](const std::vector<Value>&) -> Value {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(g_rng);
    });
    defineNative("random_randint", 2, [](const std::vector<Value>& a) -> Value {
        long long lo = static_cast<long long>(toNumberOrDefault(a[0], 0.0));
        long long hi = static_cast<long long>(toNumberOrDefault(a[1], 0.0));
        if (lo > hi) std::swap(lo, hi);
        std::uniform_int_distribution<long long> dist(lo, hi);
        return static_cast<double>(dist(g_rng));
    });
    defineNative("random_uniform", 2, [](const std::vector<Value>& a) -> Value {
        double lo = toNumberOrDefault(a[0], 0.0);
        double hi = toNumberOrDefault(a[1], 0.0);
        if (lo > hi) std::swap(lo, hi);
        std::uniform_real_distribution<double> dist(lo, hi);
        return dist(g_rng);
    });

#ifdef _WIN32
    defineNative("sys_platform", 0, [](const std::vector<Value>&) -> Value { return std::string("windows"); });
#elif __APPLE__
    defineNative("sys_platform", 0, [](const std::vector<Value>&) -> Value { return std::string("macos"); });
#elif __linux__
    defineNative("sys_platform", 0, [](const std::vector<Value>&) -> Value { return std::string("linux"); });
#else
    defineNative("sys_platform", 0, [](const std::vector<Value>&) -> Value { return std::string("unknown"); });
#endif
    defineNative("sys_version", 0, [](const std::vector<Value>&) -> Value { return std::string("epp-std-0.2.3"); });
    defineNative("sys_executable", 0, [](const std::vector<Value>&) -> Value { return std::string("epp"); });

    defineNative("os_getcwd", 0, [](const std::vector<Value>&) -> Value { return std::filesystem::current_path().string(); });
    defineNative("os_exists", 1, [](const std::vector<Value>& a) -> Value {
        return std::filesystem::exists(std::filesystem::path(toStringOrDefault(a[0], "")));
    });
    defineNative("os_is_file", 1, [](const std::vector<Value>& a) -> Value {
        return std::filesystem::is_regular_file(std::filesystem::path(toStringOrDefault(a[0], "")));
    });
    defineNative("os_is_dir", 1, [](const std::vector<Value>& a) -> Value {
        return std::filesystem::is_directory(std::filesystem::path(toStringOrDefault(a[0], "")));
    });
    defineNative("os_mkdir", 1, [](const std::vector<Value>& a) -> Value {
        std::error_code ec;
        const bool ok = std::filesystem::create_directories(std::filesystem::path(toStringOrDefault(a[0], "")), ec);
        return ok && !ec;
    });
    defineNative("os_remove", 1, [](const std::vector<Value>& a) -> Value {
        std::error_code ec;
        const bool ok = std::filesystem::remove(std::filesystem::path(toStringOrDefault(a[0], "")), ec);
        return ok && !ec;
    });

    defineNative("json_quote", 1, [](const std::vector<Value>& a) -> Value { return jsonQuote(toStringOrDefault(a[0], "")); });
    defineNative("json_is_valid_number", 1, [](const std::vector<Value>& a) -> Value {
        try {
            std::string s = toStringOrDefault(a[0], "");
            size_t idx = 0;
            (void)std::stod(s, &idx);
            return idx == s.size();
        } catch (...) {
            return false;
        }
    });
    defineNative("json_parse_number", 1, [](const std::vector<Value>& a) -> Value {
        try {
            return static_cast<double>(std::stod(toStringOrDefault(a[0], "")));
        } catch (...) {
            return std::monostate{};
        }
    });
    defineNative("csv_join2", 2, [](const std::vector<Value>& a) -> Value {
        return jsonQuote(toStringOrDefault(a[0], "")) + "," + jsonQuote(toStringOrDefault(a[1], ""));
    });
    defineNative("urllib_quote", 1, [](const std::vector<Value>& a) -> Value { return urlEncode(toStringOrDefault(a[0], "")); });

    defineNative("threading_cpu_count", 0, [](const std::vector<Value>&) -> Value {
        return static_cast<double>(std::thread::hardware_concurrency());
    });
    defineNative("asyncio_sleep", 1, [](const std::vector<Value>& args) -> Value {
        const double seconds = std::max(0.0, toNumberOrDefault(args[0], 0.0));
        std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
        return std::monostate{};
    });
    defineNative("tkinter_available", 0, [](const std::vector<Value>&) -> Value { return false; });
    defineNative("sqlite3_open", 0, [](const std::vector<Value>&) -> Value {
        return std::string("sqlite3 no disponible en runtime actual");
    });

    defineNative("http_get", 0, [](const std::vector<Value>&) -> Value {
        return std::string("http.get no disponible en runtime actual");
    });
    defineNative("http_server_listen", 2, [](const std::vector<Value>& a) -> Value {
        return static_cast<double>(httpServerListen(toStringOrDefault(a[0], "0.0.0.0"),
                                                    static_cast<int>(toNumberOrDefault(a[1], 8080.0))));
    });
    defineNative("http_server_accept", 1, [](const std::vector<Value>& a) -> Value {
        return static_cast<double>(httpServerAccept(static_cast<int>(toNumberOrDefault(a[0], -1.0))));
    });
    defineNative("http_server_close", 1, [](const std::vector<Value>& a) -> Value {
        return httpServerClose(static_cast<int>(toNumberOrDefault(a[0], -1.0)));
    });
    defineNative("http_client_close", 1, [](const std::vector<Value>& a) -> Value {
        httpClientClose(static_cast<int>(toNumberOrDefault(a[0], -1.0)));
        return true;
    });
    defineNative("http_request_line", 1, [](const std::vector<Value>& a) -> Value {
        const int id = static_cast<int>(toNumberOrDefault(a[0], -1.0));
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(id);
        if (it == g_httpClients.end()) return std::string();
        return it->second.req.requestLine;
    });
    defineNative("http_request_method", 1, [](const std::vector<Value>& a) -> Value {
        const int id = static_cast<int>(toNumberOrDefault(a[0], -1.0));
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(id);
        if (it == g_httpClients.end()) return std::string();
        return it->second.req.method;
    });
    defineNative("http_request_path", 1, [](const std::vector<Value>& a) -> Value {
        const int id = static_cast<int>(toNumberOrDefault(a[0], -1.0));
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(id);
        if (it == g_httpClients.end()) return std::string();
        return it->second.req.path;
    });
    defineNative("http_request_version", 1, [](const std::vector<Value>& a) -> Value {
        const int id = static_cast<int>(toNumberOrDefault(a[0], -1.0));
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(id);
        if (it == g_httpClients.end()) return std::string();
        return it->second.req.version;
    });
    defineNative("http_request_body", 1, [](const std::vector<Value>& a) -> Value {
        const int id = static_cast<int>(toNumberOrDefault(a[0], -1.0));
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(id);
        if (it == g_httpClients.end()) return std::string();
        return it->second.req.body;
    });
    defineNative("http_request_header", 2, [](const std::vector<Value>& a) -> Value {
        const int id = static_cast<int>(toNumberOrDefault(a[0], -1.0));
        const std::string key = toLowerCopy(toStringOrDefault(a[1], ""));
        std::lock_guard<std::mutex> lock(g_httpMutex);
        auto it = g_httpClients.find(id);
        if (it == g_httpClients.end()) return std::string();
        auto h = it->second.req.headers.find(key);
        if (h == it->second.req.headers.end()) return std::string();
        return h->second;
    });
    defineNative("http_response_send", 5, [](const std::vector<Value>& a) -> Value {
        const int clientId = static_cast<int>(toNumberOrDefault(a[0], -1.0));
        const int code = static_cast<int>(toNumberOrDefault(a[1], 200.0));
        std::string status = toStringOrDefault(a[2], "");
        if (status.empty()) status = httpStatusTextFromCode(code);
        const std::string contentType = toStringOrDefault(a[3], "text/plain; charset=utf-8");
        const std::string body = toStringOrDefault(a[4], "");
        return httpResponseSend(clientId, code, status, contentType, body);
    });
    defineNative("http_server_once", -1, [](const std::vector<Value>& a) -> Value {
        const int port = (a.size() >= 1) ? static_cast<int>(toNumberOrDefault(a[0], 8080.0)) : 8080;
        const std::string body = (a.size() >= 2) ? toStringOrDefault(a[1], "Hola desde E++") : "Hola desde E++";
        const std::string statusRaw = (a.size() >= 3) ? toStringOrDefault(a[2], "200 OK") : "200 OK";
        int code = 200;
        std::string status = statusRaw;
        {
            std::istringstream iss(statusRaw);
            std::string rest;
            if (iss >> code) {
                std::getline(iss, rest);
                rest = trimCopy(rest);
                if (!rest.empty()) status = rest;
                else status = httpStatusTextFromCode(code);
            }
        }
        const int serverId = httpServerListen("0.0.0.0", port);
        if (serverId <= 0) return std::string("ERROR: no se pudo abrir el servidor");
        const int clientId = httpServerAccept(serverId);
        if (clientId <= 0) {
            httpServerClose(serverId);
            return std::string("ERROR: no se pudo aceptar cliente");
        }
        std::string requestLine = "(sin request-line)";
        {
            std::lock_guard<std::mutex> lock(g_httpMutex);
            auto it = g_httpClients.find(clientId);
            if (it != g_httpClients.end() && !it->second.req.requestLine.empty()) requestLine = it->second.req.requestLine;
        }
        (void)httpResponseSend(clientId, code, status, "text/plain; charset=utf-8", body);
        httpServerClose(serverId);
        return requestLine;
    });
    defineNative("http_server_loop", -1, [](const std::vector<Value>& a) -> Value {
        const int port = (a.size() >= 1) ? static_cast<int>(toNumberOrDefault(a[0], 8080.0)) : 8080;
        const std::string body = (a.size() >= 2) ? toStringOrDefault(a[1], "Hola desde E++") : "Hola desde E++";
        const int maxRequests = (a.size() >= 3) ? static_cast<int>(toNumberOrDefault(a[2], 0.0)) : 0;
        const std::string statusRaw = (a.size() >= 4) ? toStringOrDefault(a[3], "200 OK") : "200 OK";
        int code = 200;
        std::string status = statusRaw;
        {
            std::istringstream iss(statusRaw);
            std::string rest;
            if (iss >> code) {
                std::getline(iss, rest);
                rest = trimCopy(rest);
                if (!rest.empty()) status = rest;
                else status = httpStatusTextFromCode(code);
            }
        }
        const int serverId = httpServerListen("0.0.0.0", port);
        if (serverId <= 0) return -1.0;
        int handled = 0;
        while (maxRequests <= 0 || handled < maxRequests) {
            const int clientId = httpServerAccept(serverId);
            if (clientId <= 0) break;
            if (!httpResponseSend(clientId, code, status, "text/plain; charset=utf-8", body)) break;
            ++handled;
        }
        httpServerClose(serverId);
        return static_cast<double>(handled);
    });

    auto loadLibFn = std::make_shared<NativeFunction>();
    loadLibFn->name = "loadlib";
    loadLibFn->arity = 1;
    loadLibFn->fn = [this](const std::vector<Value>& args) -> Value {
        if (!std::holds_alternative<std::string>(args[0])) {
            throw EppError(ErrorPhase::Runtime,
                           "E-RUN-070",
                           0,
                           1,
                           "loadlib() requiere ruta en texto.",
                           "Ejemplo: loadlib(\"lib/native_math_c.dll\")");
        }

        std::filesystem::path libPath(std::get<std::string>(args[0]));
        std::filesystem::path baseDir = std::filesystem::current_path();
        if (!currentFilePath_.empty()) {
            baseDir = std::filesystem::path(currentFilePath_).parent_path();
        }
        libPath = std::filesystem::absolute(baseDir / libPath).lexically_normal();
        const std::string libKey = libPath.string();
        auto loadedIt = loadedLibRegistryCount_.find(libKey);
        if (loadedIt != loadedLibRegistryCount_.end()) {
            return static_cast<double>(loadedIt->second);
        }

#ifdef _WIN32
        HMODULE handle = LoadLibraryA(libPath.string().c_str());
        if (!handle) {
            throw EppError(ErrorPhase::Runtime,
                           "E-RUN-071",
                           0,
                           1,
                           "No se pudo cargar libreria: " + libPath.string(),
                           "Verifica ruta y dependencias de la DLL.");
        }
        auto regV2 = reinterpret_cast<EppRegisterFnV2>(GetProcAddress(handle, "epp_register_v2"));
        auto reg = reinterpret_cast<EppRegisterFn>(GetProcAddress(handle, "epp_register"));
        if (!regV2 && !reg) {
            FreeLibrary(handle);
            throw EppError(ErrorPhase::Runtime,
                           "E-RUN-072",
                           0,
                           1,
                           "La libreria no exporta epp_register ni epp_register_v2.",
                           "Expone: extern \"C\" __declspec(dllexport) int epp_register_v2(...).");
        }
#else
        void* handle = dlopen(libPath.string().c_str(), RTLD_NOW);
        if (!handle) {
            throw EppError(ErrorPhase::Runtime,
                           "E-RUN-071",
                           0,
                           1,
                           "No se pudo cargar libreria: " + libPath.string());
        }
        auto regV2 = reinterpret_cast<EppRegisterFnV2>(dlsym(handle, "epp_register_v2"));
        auto reg = reinterpret_cast<EppRegisterFn>(dlsym(handle, "epp_register"));
        if (!regV2 && !reg) {
            dlclose(handle);
            throw EppError(ErrorPhase::Runtime,
                           "E-RUN-072",
                           0,
                           1,
                           "La libreria no exporta epp_register ni epp_register_v2.");
        }
#endif

        int count = 0;
        if (regV2) {
            EppNativeEntryV2 entries[128];
            count = regV2(entries, 128);
            if (count < 0) {
#ifdef _WIN32
                FreeLibrary(handle);
#else
                dlclose(handle);
#endif
                throw EppError(ErrorPhase::Runtime,
                               "E-RUN-073",
                               0,
                               1,
                               "epp_register_v2 devolvio cantidad invalida.");
            }

            for (int i = 0; i < count && i < 128; ++i) {
                const EppNativeEntryV2& entry = entries[i];
                if (!entry.name || !entry.fn) continue;

                auto nativeWrap = std::make_shared<NativeFunction>();
                nativeWrap->name = entry.name;
                nativeWrap->arity = entry.arity;
                nativeWrap->fn = [entry](const std::vector<Value>& callArgs) -> Value {
                    std::vector<std::string> stringBuffer;
                    std::vector<EppNativeValue> nativeArgs;
                    stringBuffer.reserve(callArgs.size());
                    nativeArgs.reserve(callArgs.size());

                    for (const auto& a : callArgs) {
                        EppNativeValue v{};
                        if (std::holds_alternative<std::monostate>(a)) {
                            v.type = EPP_NATIVE_NULL;
                        } else if (std::holds_alternative<double>(a)) {
                            v.type = EPP_NATIVE_NUMBER;
                            v.number_value = std::get<double>(a);
                        } else if (std::holds_alternative<bool>(a)) {
                            v.type = EPP_NATIVE_BOOL;
                            v.bool_value = std::get<bool>(a) ? 1 : 0;
                        } else if (std::holds_alternative<std::string>(a)) {
                            v.type = EPP_NATIVE_STRING;
                            stringBuffer.push_back(std::get<std::string>(a));
                            v.string_value = stringBuffer.back().c_str();
                        } else {
                            throw EppError(ErrorPhase::Runtime,
                                           "E-RUN-074",
                                           0,
                                           1,
                                           "Funcion nativa '" + std::string(entry.name) +
                                               "' no admite funciones, clases ni instancias.");
                        }
                        nativeArgs.push_back(v);
                    }

                    EppNativeValue result =
                        entry.fn(nativeArgs.data(), static_cast<int>(nativeArgs.size()));
                    switch (result.type) {
                        case EPP_NATIVE_NULL: return std::monostate{};
                        case EPP_NATIVE_NUMBER: return result.number_value;
                        case EPP_NATIVE_BOOL: return result.bool_value != 0;
                        case EPP_NATIVE_STRING:
                            return result.string_value ? std::string(result.string_value) : std::string();
                        default:
                            throw EppError(ErrorPhase::Runtime,
                                           "E-RUN-075",
                                           0,
                                           1,
                                           "Funcion nativa '" + std::string(entry.name) +
                                               "' devolvio tipo invalido.");
                    }
                };
                globals_->define(entry.name, nativeWrap);
            }
        } else {
            EppNativeEntry entries[128];
            count = reg(entries, 128);
            if (count < 0) {
#ifdef _WIN32
                FreeLibrary(handle);
#else
                dlclose(handle);
#endif
                throw EppError(ErrorPhase::Runtime,
                               "E-RUN-073",
                               0,
                               1,
                               "epp_register devolvio cantidad invalida.");
            }

            for (int i = 0; i < count && i < 128; ++i) {
                const EppNativeEntry& entry = entries[i];
                if (!entry.name || !entry.fn) continue;

                auto nativeWrap = std::make_shared<NativeFunction>();
                nativeWrap->name = entry.name;
                nativeWrap->arity = entry.arity;
                nativeWrap->fn = [entry](const std::vector<Value>& callArgs) -> Value {
                    std::vector<double> numeric;
                    numeric.reserve(callArgs.size());
                    for (const auto& a : callArgs) {
                        if (!std::holds_alternative<double>(a)) {
                            throw EppError(ErrorPhase::Runtime,
                                           "E-RUN-074",
                                           0,
                                           1,
                                           "Funcion nativa '" + std::string(entry.name) + "' solo acepta numeros.");
                        }
                        numeric.push_back(std::get<double>(a));
                    }
                    return entry.fn(numeric.data(), static_cast<int>(numeric.size()));
                };
                globals_->define(entry.name, nativeWrap);
            }
        }

        loadedLibHandles_.push_back(reinterpret_cast<void*>(handle));
        loadedLibRegistryCount_[libKey] = count;
        return static_cast<double>(count);
    };
    globals_->define("loadlib", loadLibFn);
}

Interpreter::Interpreter(const std::string& entryFilePath) : Interpreter() {
    currentFilePath_ = entryFilePath;
    if (!entryFilePath.empty()) {
        importedModules_.insert(std::filesystem::absolute(entryFilePath).string());
    }
}

void Interpreter::executeProgram(const std::vector<StmtPtr>& program) {
    for (const auto& stmt : program) execute(stmt);
}

void Interpreter::executeImportToken(const Token& module, int line, int column) {
    if (module.lexeme.empty()) {
        throw EppError(ErrorPhase::Runtime,
                       "E-RUN-060",
                       line,
                       column,
                       "Modulo de import vacio.",
                       "Usa: import \"ruta/al/modulo.epp\", import paquete.modulo o from paquete import simbolo");
    }

    std::filesystem::path resolved = resolveImportPath(module.lexeme, currentFilePath_, packageIdToModulePath_);
    if (std::filesystem::exists(resolved) && std::filesystem::is_directory(resolved)) {
        const std::filesystem::path initNoExt = resolved / "__init__";
        const std::filesystem::path initEpp = resolved / "__init__.epp";
        const std::filesystem::path dotInitNoExt = resolved / ".__init__";
        const std::filesystem::path dotInitEpp = resolved / ".__init__.epp";
        if (std::filesystem::exists(initNoExt) && std::filesystem::is_regular_file(initNoExt)) {
            resolved = initNoExt;
        } else if (std::filesystem::exists(initEpp) && std::filesystem::is_regular_file(initEpp)) {
            resolved = initEpp;
        } else if (std::filesystem::exists(dotInitNoExt) && std::filesystem::is_regular_file(dotInitNoExt)) {
            resolved = dotInitNoExt;
        } else if (std::filesystem::exists(dotInitEpp) && std::filesystem::is_regular_file(dotInitEpp)) {
            resolved = dotInitEpp;
        }
    }
    const std::string resolvedKey = std::filesystem::absolute(resolved).string();
    if (importedModules_.count(resolvedKey) > 0) return;

    std::ifstream in(resolved, std::ios::binary);
    if (!in) {
        throw EppError(ErrorPhase::Runtime,
                       "E-RUN-061",
                       line,
                       column,
                       "No se pudo abrir modulo importado: " + resolved.string(),
                       "Verifica el modulo. Ejemplos validos: import time, import stdlib.asyncio o from stdlib import time.",
                       module.lexeme);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string moduleSource = buffer.str();

    importedModules_.insert(resolvedKey);
    const std::string previousFile = currentFilePath_;
    auto previousEnv = env_;
    currentFilePath_ = resolvedKey;
    try {
        Lexer lexer(moduleSource);
        auto tokens = lexer.scanTokens();
        Parser parser(tokens);
        auto moduleProgram = parser.parse();
        env_ = globals_;
        for (const auto& mstmt : moduleProgram) execute(mstmt);
        env_ = previousEnv;
    } catch (...) {
        env_ = previousEnv;
        currentFilePath_ = previousFile;
        throw;
    }

    const std::string packageId = readPackageIdFromModule(resolved);
    if (!packageId.empty()) {
        importedModulePackageIds_[resolvedKey] = packageId;
        packageIdToModulePath_[packageId] = resolvedKey;
    }
    currentFilePath_ = previousFile;
}

Value Interpreter::evaluate(const ExprPtr& expr) {
    switch (expr->kind()) {
        case Expr::Kind::Literal:
            return evalLiteral(*static_cast<const LiteralExpr*>(expr.get()));
        case Expr::Kind::Variable: {
            const auto* var = static_cast<const VariableExpr*>(expr.get());
            return env_->get(var->name.lexeme, var->line);
        }
        case Expr::Kind::Assign: {
            const auto* assign = static_cast<const AssignExpr*>(expr.get());
            Value value = evaluate(assign->value);
            env_->assign(assign->name.lexeme, value, assign->line);
            return value;
        }
        case Expr::Kind::Unary:
            return evalUnary(*static_cast<const UnaryExpr*>(expr.get()));
        case Expr::Kind::Binary:
            return evalBinary(*static_cast<const BinaryExpr*>(expr.get()));
        case Expr::Kind::Call:
            return evalCall(*static_cast<const CallExpr*>(expr.get()));
        case Expr::Kind::Get:
            return evalGet(*static_cast<const GetExpr*>(expr.get()));
        case Expr::Kind::Set:
            return evalSet(*static_cast<const SetExpr*>(expr.get()));
    }
    throw EppError(ErrorPhase::Runtime, "E-RUN-090", expr ? expr->line : 0, 1, "Expresion no soportada.");
}

void Interpreter::execute(const StmtPtr& stmt) {
    switch (stmt->kind()) {
        case Stmt::Kind::Expr: {
            const auto* s = static_cast<const ExprStmt*>(stmt.get());
            (void)evaluate(s->expr);
            return;
        }
        case Stmt::Kind::Print: {
            const auto* s = static_cast<const PrintStmt*>(stmt.get());
            std::cout << valueToString(evaluate(s->expr)) << "\n";
            return;
        }
        case Stmt::Kind::VarDecl: {
            const auto* s = static_cast<const VarDeclStmt*>(stmt.get());
            Value init = s->initializer ? evaluate(s->initializer) : Value(std::monostate{});
            env_->define(s->name.lexeme, init);
            return;
        }
        case Stmt::Kind::Block: {
            const auto* s = static_cast<const BlockStmt*>(stmt.get());
            if (blockNeedsOwnScope(*s)) {
                executeBlock(s->statements, std::make_shared<Environment>(env_));
            } else {
                executeBlock(s->statements, env_);
            }
            return;
        }
        case Stmt::Kind::If: {
            const auto* s = static_cast<const IfStmt*>(stmt.get());
            if (valueIsTruthy(evaluate(s->condition))) {
                if (blockNeedsOwnScope(*s->thenBranch)) {
                    executeBlock(s->thenBranch->statements, std::make_shared<Environment>(env_));
                } else {
                    executeBlock(s->thenBranch->statements, env_);
                }
            } else if (s->elseBranch) {
                if (blockNeedsOwnScope(*s->elseBranch)) {
                    executeBlock(s->elseBranch->statements, std::make_shared<Environment>(env_));
                } else {
                    executeBlock(s->elseBranch->statements, env_);
                }
            }
            return;
        }
        case Stmt::Kind::While: {
            const auto* s = static_cast<const WhileStmt*>(stmt.get());
            const bool needsScope = blockNeedsOwnScope(*s->body);
            while (valueIsTruthy(evaluate(s->condition))) {
                if (needsScope) {
                    executeBlock(s->body->statements, std::make_shared<Environment>(env_));
                } else {
                    executeBlock(s->body->statements, env_);
                }
            }
            return;
        }
        case Stmt::Kind::Return: {
            const auto* s = static_cast<const ReturnStmt*>(stmt.get());
            Value v = s->value ? evaluate(s->value) : Value(std::monostate{});
            throw ReturnSignal(v);
        }
        case Stmt::Kind::Import: {
            const auto* s = static_cast<const ImportStmt*>(stmt.get());
            executeImportToken(s->module, s->line, s->module.column);
            return;
        }
        case Stmt::Kind::FromImport: {
            const auto* s = static_cast<const FromImportStmt*>(stmt.get());
            executeImportToken(s->module, s->line, s->module.column);
            if (s->importAll) return;
            for (const auto& name : s->names) {
                if (!globals_->hasLocal(name.lexeme)) {
                    throw EppError(ErrorPhase::Runtime,
                                   "E-RUN-062",
                                   s->line,
                                   name.column,
                                   "No existe simbolo '" + name.lexeme + "' en from-import.",
                                   "Verifica el nombre importado o usa 'from paquete import *'.",
                                   name.lexeme);
                }
            }
            return;
        }
        case Stmt::Kind::PackageDecl: {
            const auto* s = static_cast<const PackageDeclStmt*>(stmt.get());
            if (currentFilePath_.empty()) return;
            const std::string modulePath = std::filesystem::absolute(currentFilePath_).string();
            importedModulePackageIds_[modulePath] = s->id.lexeme;
            packageIdToModulePath_[s->id.lexeme] = modulePath;
            return;
        }
        case Stmt::Kind::FuncDecl: {
            const auto* s = static_cast<const FuncDeclStmt*>(stmt.get());
            auto fn = std::make_shared<Function>();
            fn->name = s->name.lexeme;
            fn->body = s->body;
            fn->closure = env_;
            for (const auto& p : s->params) fn->params.push_back(p.lexeme);
            env_->define(s->name.lexeme, fn);
            return;
        }
        case Stmt::Kind::ClassDecl: {
            const auto* s = static_cast<const ClassDeclStmt*>(stmt.get());
            auto klass = std::make_shared<ClassValue>();
            klass->name = s->name.lexeme;
            env_->define(s->name.lexeme, klass);

            for (const auto& methodStmt : s->methods) {
                auto method = std::make_shared<Function>();
                method->name = methodStmt->name.lexeme;
                method->body = methodStmt->body;
                method->closure = env_;
                method->isInitializer = method->name == "init";
                for (const auto& p : methodStmt->params) method->params.push_back(p.lexeme);
                klass->methods[method->name] = method;
            }
            return;
        }
    }
    throw EppError(ErrorPhase::Runtime, "E-RUN-091", stmt ? stmt->line : 0, 1, "Sentencia no soportada.");
}

void Interpreter::executeBlock(const std::vector<StmtPtr>& statements, const std::shared_ptr<Environment>& env) {
    std::shared_ptr<Environment> previous = env_;
    env_ = env;
    try {
        for (const auto& stmt : statements) execute(stmt);
    } catch (...) {
        env_ = previous;
        throw;
    }
    env_ = previous;
}

Value Interpreter::evalLiteral(const LiteralExpr& expr) const {
    const Token& t = expr.valueToken;
    switch (t.type) {
        case TokenType::True: return true;
        case TokenType::False: return false;
        case TokenType::Null: return std::monostate{};
        case TokenType::String: return t.lexeme;
        case TokenType::Number: return std::stod(t.lexeme);
        default: break;
    }
    throw EppError(ErrorPhase::Runtime, "E-RUN-012", expr.line, 1, "Literal invalido.");
}

Value Interpreter::evalUnary(const UnaryExpr& expr) {
    Value right = evaluate(expr.right);
    switch (expr.op.type) {
        case TokenType::Minus: return -expectNumber(right, expr.line, "negacion");
        case TokenType::Bang:
        case TokenType::Not: return !valueIsTruthy(right);
        default: break;
    }
    throw EppError(ErrorPhase::Runtime, "E-RUN-020", expr.line, 1, "Operador unario invalido.");
}

Value Interpreter::evalBinary(const BinaryExpr& expr) {
    Value left = evaluate(expr.left);
    Value right = evaluate(expr.right);

    switch (expr.op.type) {
        case TokenType::Plus:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                return std::get<double>(left) + std::get<double>(right);
            }
            if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)) {
                return valueToString(left) + valueToString(right);
            }
            throw EppError(ErrorPhase::Runtime,
                           "E-RUN-021",
                           expr.line,
                           1,
                           "No se puede sumar esos tipos.",
                           "Convierte los valores a texto o numero antes de sumarlos.");
        case TokenType::Minus:
            return expectNumber(left, expr.line, "resta") - expectNumber(right, expr.line, "resta");
        case TokenType::Star:
            return expectNumber(left, expr.line, "multiplicacion") * expectNumber(right, expr.line, "multiplicacion");
        case TokenType::Slash: {
            double d = expectNumber(right, expr.line, "division");
            if (d == 0.0) {
                throw EppError(ErrorPhase::Runtime,
                               "E-RUN-022",
                               expr.line,
                               1,
                               "Division por cero.",
                               "Verifica el denominador antes de dividir.");
            }
            return expectNumber(left, expr.line, "division") / d;
        }
        case TokenType::Greater:
            return expectNumber(left, expr.line, "comparacion") > expectNumber(right, expr.line, "comparacion");
        case TokenType::GreaterEqual:
            return expectNumber(left, expr.line, "comparacion") >= expectNumber(right, expr.line, "comparacion");
        case TokenType::Less:
            return expectNumber(left, expr.line, "comparacion") < expectNumber(right, expr.line, "comparacion");
        case TokenType::LessEqual:
            return expectNumber(left, expr.line, "comparacion") <= expectNumber(right, expr.line, "comparacion");
        case TokenType::EqualEqual: return valueEquals(left, right);
        case TokenType::BangEqual: return !valueEquals(left, right);
        case TokenType::And: return valueIsTruthy(left) && valueIsTruthy(right);
        case TokenType::Or: return valueIsTruthy(left) || valueIsTruthy(right);
        default: break;
    }
    throw EppError(ErrorPhase::Runtime, "E-RUN-023", expr.line, 1, "Operador binario invalido.");
}

Value Interpreter::evalCall(const CallExpr& expr) {
    Value callee = evaluate(expr.callee);

    const size_t argCount = expr.args.size();
    std::array<Value, 4> smallArgs;
    std::vector<Value> heapArgs;
    Value* argsPtr = nullptr;

    if (argCount <= smallArgs.size()) {
        for (size_t i = 0; i < argCount; ++i) {
            smallArgs[i] = evaluate(expr.args[i]);
        }
        argsPtr = smallArgs.data();
    } else {
        heapArgs.reserve(argCount);
        for (const auto& argExpr : expr.args) heapArgs.push_back(evaluate(argExpr));
        argsPtr = heapArgs.data();
    }

    if (std::holds_alternative<std::shared_ptr<NativeFunction>>(callee)) {
        auto fn = std::get<std::shared_ptr<NativeFunction>>(callee);
        if (fn->arity >= 0 && static_cast<int>(argCount) != fn->arity) {
            throw EppError(ErrorPhase::Runtime,
                           "E-RUN-030",
                           expr.line,
                           1,
                           "Cantidad de argumentos invalida en llamada a '" + fn->name + "'.");
        }

        std::vector<Value> nativeArgs;
        nativeArgs.reserve(argCount);
        for (size_t i = 0; i < argCount; ++i) nativeArgs.push_back(argsPtr[i]);
        return fn->fn(nativeArgs);
    }

    if (std::holds_alternative<std::shared_ptr<Function>>(callee)) {
        return callFunction(std::get<std::shared_ptr<Function>>(callee), argsPtr, argCount, expr.line);
    }

    if (std::holds_alternative<std::shared_ptr<ClassValue>>(callee)) {
        auto klass = std::get<std::shared_ptr<ClassValue>>(callee);
        auto instance = std::make_shared<InstanceValue>();
        instance->klass = klass;

        auto initIt = klass->methods.find("init");
        if (initIt != klass->methods.end()) {
            auto initMethod = bindThis(initIt->second, instance);
            (void)callFunction(initMethod, argsPtr, argCount, expr.line);
        } else if (argCount > 0) {
            throw EppError(ErrorPhase::Runtime,
                           "E-RUN-040",
                           expr.line,
                           1,
                           "La clase '" + klass->name + "' no acepta argumentos sin init().",
                           "Define func init(...) en la clase o llama sin argumentos.");
        }
        return instance;
    }

    throw EppError(ErrorPhase::Runtime,
                   "E-RUN-031",
                   expr.line,
                   1,
                   "Solo se pueden llamar funciones o clases.");
}

Value Interpreter::callFunction(const std::shared_ptr<Function>& fn,
                                const Value* args,
                                size_t argCount,
                                int line) {
    if (argCount != fn->params.size()) {
        throw EppError(ErrorPhase::Runtime,
                       "E-RUN-030",
                       line,
                       1,
                       "Cantidad de argumentos invalida en llamada a '" + fn->name + "'.");
    }

    // Fast-path: avoid allocating a new environment for simple same-scope calls.
    if (fn->closure.get() == env_.get() && !blockNeedsOwnScope(*fn->body)) {
        struct ParamBackup {
            std::string name;
            bool existed;
            Value oldValue;
        };

        std::vector<ParamBackup> backups;
        backups.reserve(fn->params.size());
        for (size_t i = 0; i < fn->params.size(); ++i) {
            const std::string& name = fn->params[i];
            const bool existed = env_->hasLocal(name);
            backups.push_back(ParamBackup{name, existed, existed ? env_->getLocal(name) : Value(std::monostate{})});
            env_->define(name, args[i]);
        }

        auto restoreParams = [&]() {
            for (auto it = backups.rbegin(); it != backups.rend(); ++it) {
                if (it->existed) {
                    env_->define(it->name, it->oldValue);
                } else {
                    env_->eraseLocal(it->name);
                }
            }
        };

        try {
            executeBlock(fn->body->statements, env_);
            restoreParams();
            if (fn->isInitializer) return env_->get("this", line);
            return std::monostate{};
        } catch (const ReturnSignal& signal) {
            restoreParams();
            if (fn->isInitializer) return env_->get("this", line);
            return signal.value;
        } catch (...) {
            restoreParams();
            throw;
        }
    }

    auto callEnv = std::make_shared<Environment>(fn->closure);
    for (size_t i = 0; i < fn->params.size(); ++i) {
        callEnv->define(fn->params[i], args[i]);
    }

    try {
        executeBlock(fn->body->statements, callEnv);
    } catch (const ReturnSignal& signal) {
        if (fn->isInitializer) return callEnv->get("this", line);
        return signal.value;
    }

    if (fn->isInitializer) return callEnv->get("this", line);
    return std::monostate{};
}

Value Interpreter::evalGet(const GetExpr& expr) {
    Value object = evaluate(expr.object);
    if (!std::holds_alternative<std::shared_ptr<InstanceValue>>(object)) {
        throw EppError(ErrorPhase::Runtime,
                       "E-RUN-050",
                       expr.line,
                       1,
                       "Solo las instancias tienen propiedades.");
    }

    auto instance = std::get<std::shared_ptr<InstanceValue>>(object);
    auto fieldIt = instance->fields.find(expr.name.lexeme);
    if (fieldIt != instance->fields.end()) return fieldIt->second;

    auto methodIt = instance->klass->methods.find(expr.name.lexeme);
    if (methodIt != instance->klass->methods.end()) {
        return bindThis(methodIt->second, instance);
    }

    throw EppError(ErrorPhase::Runtime,
                   "E-RUN-051",
                   expr.line,
                   1,
                   "Propiedad no encontrada: '" + expr.name.lexeme + "'.",
                   "Verifica que el campo/metodo exista en la clase o se haya asignado.",
                   expr.name.lexeme);
}

Value Interpreter::evalSet(const SetExpr& expr) {
    Value object = evaluate(expr.object);
    if (!std::holds_alternative<std::shared_ptr<InstanceValue>>(object)) {
        throw EppError(ErrorPhase::Runtime,
                       "E-RUN-052",
                       expr.line,
                       1,
                       "Solo las instancias permiten asignar propiedades.");
    }

    Value value = evaluate(expr.value);
    auto instance = std::get<std::shared_ptr<InstanceValue>>(object);
    instance->fields[expr.name.lexeme] = value;
    return value;
}

std::shared_ptr<Function> Interpreter::bindThis(const std::shared_ptr<Function>& method,
                                                const std::shared_ptr<InstanceValue>& instance) {
    auto bound = std::make_shared<Function>(*method);
    bound->closure = std::make_shared<Environment>(method->closure);
    bound->closure->define("this", instance);
    return bound;
}

double Interpreter::expectNumber(const Value& value, int line, const std::string& context) {
    if (!std::holds_alternative<double>(value)) {
        throw EppError(ErrorPhase::Runtime,
                       "E-RUN-024",
                       line,
                       1,
                       "Se esperaba numero en " + context + ", se recibio '" + valueToString(value) + "'.");
    }
    return std::get<double>(value);
}
