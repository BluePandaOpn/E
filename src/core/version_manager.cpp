#include "version_manager.hpp"
#include "error.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace {
namespace term {
constexpr const char* kReset = "\x1b[0m";
constexpr const char* kCyan = "\x1b[36m";
constexpr const char* kGreen = "\x1b[32m";
constexpr const char* kYellow = "\x1b[33m";
constexpr const char* kRed = "\x1b[31m";
}

constexpr const char* kRepoApiUrl = "https://api.github.com/repos/BluePandaOpn/E-/contents?ref=Version";

void printStep(const char* color, const std::string& tag, const std::string& message) {
    std::cout << color << "[" << tag << "]" << term::kReset << " " << message << "\n";
}

std::string makeTempPath(const std::string& suffix) {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::filesystem::path p = std::filesystem::temp_directory_path() / ("epp_" + std::to_string(now) + suffix);
    return p.string();
}

std::string readTextFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

struct RunResult {
    int rc = 0;
    std::string output;
};

RunResult runPowerShellScript(const std::string& scriptContent, bool animated, const std::string& progressMsg) {
    const std::string scriptPath = makeTempPath(".ps1");
    const std::string outPath = makeTempPath(".log");
    {
        std::ofstream ps(scriptPath, std::ios::binary);
        ps << "$ErrorActionPreference='Stop'\n";
        ps << "$ProgressPreference='SilentlyContinue'\n";
        ps << scriptContent << "\n";
    }

    std::ostringstream cmd;
    cmd << "powershell -NoProfile -ExecutionPolicy Bypass -File \"" << scriptPath << "\" > \"" << outPath << "\" 2>&1";

    int rc = 0;
    if (!animated) {
        rc = std::system(cmd.str().c_str());
    } else {
        static const std::array<const char*, 4> kFrames = {"/", "-", "|", "\\"};
        auto runner = std::async(std::launch::async, [&cmd]() { return std::system(cmd.str().c_str()); });
        size_t frame = 0;
        while (runner.wait_for(std::chrono::milliseconds(120)) != std::future_status::ready) {
            std::cout << "\r" << term::kCyan << "[" << kFrames[frame % kFrames.size()] << "]" << term::kReset << " " << progressMsg
                      << "   " << std::flush;
            ++frame;
        }
        rc = runner.get();
        std::cout << "\r\033[2K\r" << std::flush;
    }

    RunResult result;
    result.rc = rc;
    result.output = readTextFile(outPath);
    std::error_code ec;
    std::filesystem::remove(scriptPath, ec);
    std::filesystem::remove(outPath, ec);
    return result;
}

std::string trim(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string stripZip(std::string s) {
    if (s.size() >= 4 && toLower(s.substr(s.size() - 4)) == ".zip") {
        s.resize(s.size() - 4);
    }
    return s;
}

std::vector<int> parseVersionParts(const std::string& v) {
    std::string t = v;
    if (!t.empty() && (t[0] == 'V' || t[0] == 'v')) t = t.substr(1);
    std::vector<int> out;
    std::stringstream ss(t);
    std::string part;
    while (std::getline(ss, part, '.')) {
        if (part.empty()) {
            out.push_back(0);
        } else {
            out.push_back(std::stoi(part));
        }
    }
    return out;
}

bool versionDesc(const RemoteVersion& a, const RemoteVersion& b) {
    std::vector<int> pa = parseVersionParts(a.versionName);
    std::vector<int> pb = parseVersionParts(b.versionName);
    const size_t n = std::max(pa.size(), pb.size());
    pa.resize(n, 0);
    pb.resize(n, 0);
    for (size_t i = 0; i < n; ++i) {
        if (pa[i] != pb[i]) return pa[i] > pb[i];
    }
    return a.versionName > b.versionName;
}

std::filesystem::path versionsBaseDir() {
    return std::filesystem::current_path() / ".epp_versions";
}
} // namespace

std::string VersionManager::cliVersion() { return "0.2.3"; }

std::vector<RemoteVersion> VersionManager::fetchRemoteVersions() const {
    const std::string ps = std::string(
        "$items = Invoke-RestMethod -Headers @{ 'User-Agent'='epp-cli' } -Uri '") +
                           kRepoApiUrl + "'\n"
                                         "foreach ($i in $items) {\n"
                                         "  if ($i.name -like '*.zip') { Write-Output ($i.name + '|' + $i.download_url) }\n"
                                         "}\n";
    RunResult res = runPowerShellScript(ps, true, "Consultando versiones remotas");
    if (res.rc != 0) {
        throw EppError(ErrorPhase::Compiler,
                       "E-VER-001",
                       0,
                       1,
                       "No se pudieron consultar versiones remotas.",
                       "Verifica conexion a internet y acceso a GitHub.\n" + res.output);
    }

    std::vector<RemoteVersion> versions;
    std::stringstream ss(res.output);
    std::string line;
    while (std::getline(ss, line)) {
        line = trim(line);
        if (line.empty()) continue;
        const size_t sep = line.find('|');
        if (sep == std::string::npos) continue;
        RemoteVersion v;
        v.zipName = trim(line.substr(0, sep));
        v.versionName = stripZip(v.zipName);
        v.downloadUrl = trim(line.substr(sep + 1));
        if (!v.zipName.empty() && !v.downloadUrl.empty()) versions.push_back(v);
    }

    std::sort(versions.begin(), versions.end(), versionDesc);
    return versions;
}

int VersionManager::listRemoteVersions() const {
    printStep(term::kYellow, "+", "Buscando versiones disponibles en la rama Version...");
    std::vector<RemoteVersion> versions = fetchRemoteVersions();
    if (versions.empty()) {
        printStep(term::kRed, "!", "No se encontraron archivos .zip en la rama Version.");
        return 1;
    }
    printStep(term::kGreen, "+", "Versiones disponibles:");
    for (const auto& v : versions) {
        std::cout << "  - " << v.versionName << "\n";
    }
    return 0;
}

int VersionManager::installVersion(const std::string& versionInput) const {
    const std::string wanted = toLower(stripZip(trim(versionInput)));
    if (wanted.empty()) {
        throw EppError(ErrorPhase::Compiler,
                       "E-VER-002",
                       0,
                       1,
                       "Debes indicar una version valida. Ejemplo: epp -v install V0.2.3");
    }

    printStep(term::kYellow, "+", "Resolviendo version: " + wanted);
    std::vector<RemoteVersion> versions = fetchRemoteVersions();
    auto it = std::find_if(versions.begin(), versions.end(), [&](const RemoteVersion& v) {
        return toLower(v.versionName) == wanted || toLower(v.zipName) == wanted;
    });
    if (it == versions.end()) {
        std::ostringstream available;
        for (const auto& v : versions) available << v.versionName << " ";
        throw EppError(ErrorPhase::Compiler,
                       "E-VER-003",
                       0,
                       1,
                       "Version no encontrada: " + versionInput,
                       "Usa: epp -v install -l para listar versiones.\nDisponibles: " + available.str());
    }

    const std::filesystem::path base = versionsBaseDir();
    const std::filesystem::path tmp = base / "tmp";
    const std::filesystem::path target = base / it->versionName;
    std::filesystem::create_directories(tmp);
    std::filesystem::create_directories(base);
    const std::filesystem::path zipPath = tmp / it->zipName;

    printStep(term::kYellow, "+", "Descargando " + it->zipName);
    {
        std::ostringstream ps;
        ps << "Invoke-WebRequest -Headers @{ 'User-Agent'='epp-cli' } "
           << "-Uri '" << it->downloadUrl << "' "
           << "-OutFile '" << zipPath.string() << "'";
        RunResult res = runPowerShellScript(ps.str(), true, "Descargando paquete");
        if (res.rc != 0) {
            throw EppError(ErrorPhase::Compiler,
                           "E-VER-004",
                           0,
                           1,
                           "Fallo la descarga de la version " + it->versionName,
                           res.output);
        }
    }

    printStep(term::kYellow, "+", "Instalando en " + target.string());
    {
        std::ostringstream ps;
        ps << "if (Test-Path '" << target.string() << "') { Remove-Item -Recurse -Force '" << target.string() << "' }\n";
        ps << "New-Item -ItemType Directory -Force -Path '" << target.string() << "' | Out-Null\n";
        ps << "Expand-Archive -Path '" << zipPath.string() << "' -DestinationPath '" << target.string() << "' -Force\n";
        RunResult res = runPowerShellScript(ps.str(), true, "Instalando paquete");
        if (res.rc != 0) {
            throw EppError(ErrorPhase::Compiler,
                           "E-VER-005",
                           0,
                           1,
                           "Fallo la instalacion de la version " + it->versionName,
                           res.output);
        }
    }

    {
        std::ofstream marker((base / "CURRENT").string(), std::ios::binary);
        marker << it->versionName << "\n";
    }

    printStep(term::kGreen, "+", "Instalacion completada: " + it->versionName);
    printStep(term::kGreen, "+", "Directorio: " + target.string());
    return 0;
}

int VersionManager::updateToLatest() const {
    printStep(term::kYellow, "+", "Buscando ultima version...");
    std::vector<RemoteVersion> versions = fetchRemoteVersions();
    if (versions.empty()) {
        throw EppError(ErrorPhase::Compiler,
                       "E-VER-006",
                       0,
                       1,
                       "No se encontraron versiones remotas para actualizar.");
    }
    printStep(term::kYellow, "+", "Ultima version detectada: " + versions.front().versionName);
    return installVersion(versions.front().versionName);
}
