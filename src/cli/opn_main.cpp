#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr const char* kDefaultOwner = "BluePandaOpn";
constexpr const char* kManifestRelative = "e++/packages/installed.txt";

struct InstalledLib {
    std::string name;
    std::string repo;
    std::string installedAt;
    std::string branch;
};

struct RunResult {
    int rc = 0;
    std::string output;
};

std::string nowIsoLocal() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &t);
#else
    localtime_r(&t, &localTm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
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

std::string psSingleQuoted(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        if (c == '\'') out += "''";
        else out.push_back(c);
    }
    return out;
}

std::string makeTempPath(const std::string& suffix) {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path p = std::filesystem::temp_directory_path() / ("opn_" + std::to_string(now) + suffix);
    return p.string();
}

std::string readTextFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

RunResult runPowerShellScript(const std::string& scriptContent) {
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

    RunResult result;
    result.rc = std::system(cmd.str().c_str());
    result.output = readTextFile(outPath);

    std::error_code ec;
    std::filesystem::remove(scriptPath, ec);
    std::filesystem::remove(outPath, ec);
    return result;
}

std::filesystem::path opnRoot() {
    return std::filesystem::current_path() / "e++";
}

std::filesystem::path manifestPath() {
    return std::filesystem::current_path() / kManifestRelative;
}

void ensureBaseLayout() {
    const std::filesystem::path root = opnRoot();
    const std::vector<std::filesystem::path> dirs = {
        root,
        root / "lib",
        root / "include",
        root / "script",
        root / "packages",
        root / "cache",
        root / "bin",
        root / "tmp",
        root / "project",
    };

    for (const auto& d : dirs) {
        std::filesystem::create_directories(d);
    }

    const std::filesystem::path envFile = root / "opn.env";
    if (!std::filesystem::exists(envFile)) {
        std::ofstream out(envFile.string(), std::ios::binary);
        out << "EPP_HOME=" << root.string() << "\n";
        out << "EPP_LIB=" << (root / "lib").string() << "\n";
        out << "EPP_INCLUDE=" << (root / "include").string() << "\n";
        out << "EPP_PACKAGES=" << (root / "packages").string() << "\n";
    }

    const std::filesystem::path activateBat = root / "script" / "activate_opn.bat";
    if (!std::filesystem::exists(activateBat)) {
        std::ofstream out(activateBat.string(), std::ios::binary);
        out << "@echo off\n";
        out << "set EPP_HOME=" << root.string() << "\n";
        out << "set EPP_LIB=%EPP_HOME%\\lib\n";
        out << "set EPP_INCLUDE=%EPP_HOME%\\include\n";
        out << "set EPP_PACKAGES=%EPP_HOME%\\packages\n";
        out << "set PATH=%EPP_HOME%\\bin;%PATH%\n";
        out << "echo OPN virtual env activo: %EPP_HOME%\n";
    }

    const std::filesystem::path activatePs1 = root / "script" / "activate_opn.ps1";
    if (!std::filesystem::exists(activatePs1)) {
        std::ofstream out(activatePs1.string(), std::ios::binary);
        out << "$env:EPP_HOME = '" << psSingleQuoted(root.string()) << "'\n";
        out << "$env:EPP_LIB = Join-Path $env:EPP_HOME 'lib'\n";
        out << "$env:EPP_INCLUDE = Join-Path $env:EPP_HOME 'include'\n";
        out << "$env:EPP_PACKAGES = Join-Path $env:EPP_HOME 'packages'\n";
        out << "$env:PATH = (Join-Path $env:EPP_HOME 'bin') + ';' + $env:PATH\n";
        out << "Write-Host 'OPN virtual env activo: ' $env:EPP_HOME\n";
    }

    const std::filesystem::path manifest = manifestPath();
    if (!std::filesystem::exists(manifest)) {
        std::ofstream out(manifest.string(), std::ios::binary);
        out << "# name|repo|installed_at|branch\n";
    }
}

void printHelp() {
    std::cout << "OPN package manager\n";
    std::cout << "Uso:\n";
    std::cout << "  opn install                       Inicializa entorno e++ (virtual env)\n";
    std::cout << "  opn install <lib>                 Instala libreria desde GitHub (owner por defecto: BluePandaOpn)\n";
    std::cout << "  opn install <lib> --repo o/r      Instala desde repositorio GitHub especifico\n";
    std::cout << "  opn install <lib> --branch <name> Instala desde rama especifica (default: main)\n";
    std::cout << "  opn list                          Lista librerias instaladas\n";
    std::cout << "  opn remove <lib>                  Elimina libreria instalada\n";
    std::cout << "  opn doctor                        Verifica estructura del entorno\n";
}

std::vector<InstalledLib> loadInstalled() {
    std::vector<InstalledLib> out;
    std::ifstream in(manifestPath().string(), std::ios::binary);
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        InstalledLib lib;
        if (!std::getline(ss, lib.name, '|')) continue;
        if (!std::getline(ss, lib.repo, '|')) continue;
        if (!std::getline(ss, lib.installedAt, '|')) continue;
        if (!std::getline(ss, lib.branch, '|')) lib.branch = "main";
        lib.name = trim(lib.name);
        lib.repo = trim(lib.repo);
        lib.installedAt = trim(lib.installedAt);
        lib.branch = trim(lib.branch);
        if (!lib.name.empty()) out.push_back(lib);
    }
    return out;
}

void saveInstalled(const std::vector<InstalledLib>& libs) {
    std::ofstream out(manifestPath().string(), std::ios::binary | std::ios::trunc);
    out << "# name|repo|installed_at|branch\n";
    for (const auto& lib : libs) {
        out << lib.name << "|" << lib.repo << "|" << lib.installedAt << "|" << lib.branch << "\n";
    }
}

int commandDoctor() {
    ensureBaseLayout();
    const std::filesystem::path root = opnRoot();
    const std::vector<std::filesystem::path> checks = {
        root / "lib",
        root / "include",
        root / "script",
        root / "packages",
        root / "cache",
        root / "bin",
        root / "tmp",
        root / "project",
        root / "script" / "activate_opn.bat",
        root / "script" / "activate_opn.ps1",
        manifestPath(),
    };

    int missing = 0;
    std::cout << "OPN doctor\n";
    std::cout << "cwd: " << std::filesystem::current_path().string() << "\n";
    std::cout << "root: " << root.string() << "\n";
    for (const auto& p : checks) {
        const bool ok = std::filesystem::exists(p);
        std::cout << (ok ? "[OK]   " : "[MISS] ") << p.string() << "\n";
        if (!ok) ++missing;
    }
    if (missing == 0) {
        std::cout << "doctor: entorno OPN listo\n";
        return 0;
    }
    std::cout << "doctor: faltan " << missing << " rutas/archivos\n";
    return 1;
}

int commandInitOnly() {
    ensureBaseLayout();
    std::cout << "Entorno base creado en: " << opnRoot().string() << "\n";
    std::cout << "Activa virtual env con: e++\\script\\activate_opn.bat\n";
    return 0;
}

int commandList() {
    ensureBaseLayout();
    const auto libs = loadInstalled();
    if (libs.empty()) {
        std::cout << "No hay librerias instaladas.\n";
        return 0;
    }
    std::cout << "Librerias instaladas (" << libs.size() << "):\n";
    for (const auto& lib : libs) {
        std::cout << "  - " << lib.name << " | repo: " << lib.repo << " | branch: " << lib.branch
                  << " | instalado: " << lib.installedAt << "\n";
    }
    return 0;
}

bool downloadAndExtractRepo(const std::string& repo,
                            const std::string& branch,
                            const std::filesystem::path& destination,
                            std::string& errorOut) {
    const std::filesystem::path tmpBase = opnRoot() / "tmp";
    std::filesystem::create_directories(tmpBase);
    const std::filesystem::path zipPath = tmpBase / ("repo_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".zip");
    const std::filesystem::path unpackPath = tmpBase / ("repo_unpack_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    const std::string url = "https://codeload.github.com/" + repo + "/zip/refs/heads/" + branch;

    std::ostringstream ps;
    ps << "$zip='" << psSingleQuoted(zipPath.string()) << "'\n";
    ps << "$dest='" << psSingleQuoted(destination.string()) << "'\n";
    ps << "$tmp='" << psSingleQuoted(unpackPath.string()) << "'\n";
    ps << "Invoke-WebRequest -Headers @{ 'User-Agent'='opn-cli' } -Uri '" << psSingleQuoted(url) << "' -OutFile $zip\n";
    ps << "if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }\n";
    ps << "New-Item -ItemType Directory -Force -Path $tmp | Out-Null\n";
    ps << "Expand-Archive -Path $zip -DestinationPath $tmp -Force\n";
    ps << "$root = Get-ChildItem -Path $tmp | Where-Object { $_.PSIsContainer } | Select-Object -First 1\n";
    ps << "if ($null -eq $root) { throw 'No se pudo extraer repositorio' }\n";
    ps << "if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }\n";
    ps << "New-Item -ItemType Directory -Force -Path $dest | Out-Null\n";
    ps << "Copy-Item -Path (Join-Path $root.FullName '*') -Destination $dest -Recurse -Force\n";
    ps << "Remove-Item -Recurse -Force $tmp\n";
    ps << "Remove-Item -Force $zip\n";

    RunResult result = runPowerShellScript(ps.str());
    if (result.rc != 0) {
        errorOut = trim(result.output);
        if (errorOut.empty()) errorOut = "error desconocido de PowerShell";
        return false;
    }
    return true;
}

int commandInstallLibrary(const std::string& nameArg, const std::string& repoArg, const std::string& branchArg) {
    ensureBaseLayout();
    const std::string name = trim(nameArg);
    if (name.empty()) {
        std::cerr << "Nombre de libreria invalido.\n";
        return 1;
    }

    std::string repo = trim(repoArg);
    if (repo.empty()) repo = std::string(kDefaultOwner) + "/" + name;
    std::string branch = trim(branchArg);
    if (branch.empty()) branch = "main";

    const std::filesystem::path destination = opnRoot() / "packages" / name;
    std::string error;
    std::cout << "Instalando '" << name << "' desde " << repo << " (branch " << branch << ")...\n";
    if (!downloadAndExtractRepo(repo, branch, destination, error)) {
        if (branch == "main") {
            std::cout << "Fallo branch main, intentando master...\n";
            if (!downloadAndExtractRepo(repo, "master", destination, error)) {
                std::cerr << "No se pudo instalar la libreria '" << name << "': " << error << "\n";
                return 1;
            }
            branch = "master";
        } else {
            std::cerr << "No se pudo instalar la libreria '" << name << "': " << error << "\n";
            return 1;
        }
    }

    auto libs = loadInstalled();
    auto it = std::find_if(libs.begin(), libs.end(), [&](const InstalledLib& lib) { return toLower(lib.name) == toLower(name); });
    const InstalledLib record{name, repo, nowIsoLocal(), branch};
    if (it == libs.end()) libs.push_back(record);
    else *it = record;
    saveInstalled(libs);

    std::cout << "Libreria instalada: " << name << "\n";
    std::cout << "Ruta: " << destination.string() << "\n";
    return 0;
}

int commandRemoveLibrary(const std::string& nameArg) {
    ensureBaseLayout();
    const std::string name = trim(nameArg);
    if (name.empty()) {
        std::cerr << "Debes indicar el nombre de la libreria a eliminar.\n";
        return 1;
    }

    const std::filesystem::path destination = opnRoot() / "packages" / name;
    std::error_code ec;
    if (std::filesystem::exists(destination)) {
        std::filesystem::remove_all(destination, ec);
        if (ec) {
            std::cerr << "No se pudo eliminar carpeta de libreria: " << destination.string() << "\n";
            return 1;
        }
    }

    auto libs = loadInstalled();
    const auto before = libs.size();
    libs.erase(std::remove_if(libs.begin(),
                              libs.end(),
                              [&](const InstalledLib& lib) { return toLower(lib.name) == toLower(name); }),
               libs.end());
    saveInstalled(libs);

    if (libs.size() == before) {
        std::cout << "La libreria no estaba registrada: " << name << "\n";
    } else {
        std::cout << "Libreria eliminada: " << name << "\n";
    }
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            printHelp();
            return 0;
        }

        const std::string cmd = argv[1];
        if (cmd == "install") {
            if (argc == 2) {
                return commandInitOnly();
            }

            const std::string libName = argv[2];
            std::string repo;
            std::string branch = "main";

            for (int i = 3; i < argc; ++i) {
                const std::string arg = argv[i];
                if (arg == "--repo") {
                    if (i + 1 >= argc) {
                        std::cerr << "Falta valor para --repo\n";
                        return 1;
                    }
                    repo = argv[++i];
                    continue;
                }
                if (arg == "--branch") {
                    if (i + 1 >= argc) {
                        std::cerr << "Falta valor para --branch\n";
                        return 1;
                    }
                    branch = argv[++i];
                    continue;
                }
                std::cerr << "Opcion no reconocida: " << arg << "\n";
                return 1;
            }
            return commandInstallLibrary(libName, repo, branch);
        }

        if (cmd == "list") return commandList();
        if (cmd == "remove") {
            if (argc < 3) {
                std::cerr << "Uso: opn remove <libreria>\n";
                return 1;
            }
            return commandRemoveLibrary(argv[2]);
        }
        if (cmd == "doctor") return commandDoctor();
        if (cmd == "-h" || cmd == "--help" || cmd == "help") {
            printHelp();
            return 0;
        }

        printHelp();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "[OPN FATAL] " << e.what() << "\n";
        return 2;
    }
}
