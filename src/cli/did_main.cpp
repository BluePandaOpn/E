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
#include <unordered_set>
#include <vector>

namespace {
constexpr const char* kDidVersion = "V0.1.4";
constexpr const char* kManifestRelative = "e++/packages/installed.txt";
constexpr const char* kCuratedRepo = "BluePandaOpn/E";
constexpr const char* kCuratedBranch = "Lib";
constexpr const char* kStdlibRoot = "libs";
constexpr const char* kExtraRoot = "Extra";

const std::vector<std::string> kStandardLibraries = {
    "asyncio",
    "_native",
    "collections",
    "csv",
    "datetime",
    "http",
    "json",
    "math",
    "os",
    "random",
    "sqlite3",
    "sys",
    "threading",
    "time",
    "tkinter",
    "urllib",
    "color",
};

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

bool isStandardLibrary(const std::string& name) {
    const std::string normalized = toLower(trim(name));
    return std::any_of(kStandardLibraries.begin(), kStandardLibraries.end(), [&](const std::string& entry) {
        return toLower(entry) == normalized;
    });
}

bool hasUniqueNames(const std::vector<std::string>& values) {
    std::vector<std::string> normalized;
    normalized.reserve(values.size());
    for (const auto& value : values) {
        normalized.push_back(toLower(trim(value)));
    }
    std::sort(normalized.begin(), normalized.end());
    for (size_t i = 1; i < normalized.size(); ++i) {
        if (!normalized[i].empty() && normalized[i] == normalized[i - 1]) return false;
    }
    return true;
}

std::string makeTempPath(const std::string& suffix) {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path p = std::filesystem::temp_directory_path() / ("did_" + std::to_string(now) + suffix);
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

std::filesystem::path didRoot() {
    return std::filesystem::current_path() / "e++";
}

std::filesystem::path manifestPath() {
    return std::filesystem::current_path() / kManifestRelative;
}

void ensureBaseLayout() {
    const std::filesystem::path root = didRoot();
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

    const std::filesystem::path envFile = root / "did.env";
    if (!std::filesystem::exists(envFile)) {
        std::ofstream out(envFile.string(), std::ios::binary);
        out << "EPP_HOME=" << root.string() << "\n";
        out << "EPP_LIB=" << (root / "lib").string() << "\n";
        out << "EPP_INCLUDE=" << (root / "include").string() << "\n";
        out << "EPP_PACKAGES=" << (root / "packages").string() << "\n";
    }
    const std::filesystem::path legacyEnvFile = root / "opn.env";
    if (!std::filesystem::exists(legacyEnvFile)) {
        std::ofstream out(legacyEnvFile.string(), std::ios::binary);
        out << "EPP_HOME=" << root.string() << "\n";
        out << "EPP_LIB=" << (root / "lib").string() << "\n";
        out << "EPP_INCLUDE=" << (root / "include").string() << "\n";
        out << "EPP_PACKAGES=" << (root / "packages").string() << "\n";
    }

    const std::filesystem::path activateBat = root / "script" / "activate_did.bat";
    if (!std::filesystem::exists(activateBat)) {
        std::ofstream out(activateBat.string(), std::ios::binary);
        out << "@echo off\n";
        out << "set EPP_HOME=" << root.string() << "\n";
        out << "set EPP_LIB=%EPP_HOME%\\lib\n";
        out << "set EPP_INCLUDE=%EPP_HOME%\\include\n";
        out << "set EPP_PACKAGES=%EPP_HOME%\\packages\n";
        out << "set PATH=%EPP_HOME%\\bin;%PATH%\n";
        out << "echo DID virtual env activo: %EPP_HOME%\n";
    }
    const std::filesystem::path activateLegacyBat = root / "script" / "activate_opn.bat";
    if (!std::filesystem::exists(activateLegacyBat)) {
        std::ofstream out(activateLegacyBat.string(), std::ios::binary);
        out << "@echo off\n";
        out << "call \"%~dp0activate_did.bat\"\n";
    }

    const std::filesystem::path activatePs1 = root / "script" / "activate_did.ps1";
    if (!std::filesystem::exists(activatePs1)) {
        std::ofstream out(activatePs1.string(), std::ios::binary);
        out << "$env:EPP_HOME = '" << psSingleQuoted(root.string()) << "'\n";
        out << "$env:EPP_LIB = Join-Path $env:EPP_HOME 'lib'\n";
        out << "$env:EPP_INCLUDE = Join-Path $env:EPP_HOME 'include'\n";
        out << "$env:EPP_PACKAGES = Join-Path $env:EPP_HOME 'packages'\n";
        out << "$env:PATH = (Join-Path $env:EPP_HOME 'bin') + ';' + $env:PATH\n";
        out << "Write-Host 'DID virtual env activo: ' $env:EPP_HOME\n";
    }
    const std::filesystem::path activateLegacyPs1 = root / "script" / "activate_opn.ps1";
    if (!std::filesystem::exists(activateLegacyPs1)) {
        std::ofstream out(activateLegacyPs1.string(), std::ios::binary);
        out << "& (Join-Path $PSScriptRoot 'activate_did.ps1')\n";
    }

    const std::filesystem::path manifest = manifestPath();
    if (!std::filesystem::exists(manifest)) {
        std::ofstream out(manifest.string(), std::ios::binary);
        out << "# name|repo|installed_at|branch\n";
    }
}

void printHelp() {
    std::cout << "DID package manager\n";
    std::cout << "Version: " << kDidVersion << "\n";
    std::cout << "Uso:\n";
    std::cout << "  did install                       Inicializa entorno e++ (virtual env)\n";
    std::cout << "  did install <lib>                 Instala stdlib o externa desde BluePandaOpn/E (branch Lib)\n";
    std::cout << "  did install <lib> --repo o/r      Instala desde repositorio GitHub especifico\n";
    std::cout << "  did install <lib> --branch <name> Instala desde rama especifica (default: Lib)\n";
    std::cout << "  did stdlib                        Lista librerias estandar soportadas\n";
    std::cout << "  did list                          Lista librerias instaladas\n";
    std::cout << "  did repair                        Repara estructura de librerias instaladas\n";
    std::cout << "  did remove <lib>                  Elimina libreria instalada\n";
    std::cout << "  did doctor                        Verifica estructura del entorno\n";
    std::cout << "  did -v | --version                Muestra version de did\n";
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

bool dirHasModuleInit(const std::filesystem::path& dir) {
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) return false;
    const std::vector<std::string> names = {
        "__init__",
        "__init__.epp",
        ".__init__",
        ".__init__.epp",
    };
    for (const auto& name : names) {
        const std::filesystem::path p = dir / name;
        if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) return true;
    }
    return false;
}

bool flattenDirContents(const std::filesystem::path& src, const std::filesystem::path& dest, std::string& errorOut) {
    if (!std::filesystem::exists(src) || !std::filesystem::is_directory(src)) {
        errorOut = "directorio fuente no existe para normalizar: " + src.string();
        return false;
    }

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(src, ec)) {
        if (ec) {
            errorOut = "no se pudo listar contenido de: " + src.string();
            return false;
        }
        const std::filesystem::path target = dest / entry.path().filename();
        if (std::filesystem::exists(target)) {
            std::filesystem::remove_all(target, ec);
            if (ec) {
                errorOut = "no se pudo reemplazar ruta existente: " + target.string();
                return false;
            }
        }
        std::filesystem::rename(entry.path(), target, ec);
        if (ec) {
            errorOut = "no se pudo mover '" + entry.path().string() + "' a '" + target.string() + "'";
            return false;
        }
    }

    std::filesystem::remove_all(src, ec);
    if (ec) {
        errorOut = "no se pudo limpiar carpeta temporal de paquete: " + src.string();
        return false;
    }
    return true;
}

bool normalizePackageLayout(const std::filesystem::path& destination, const std::string& packageName, std::string& errorOut) {
    if (dirHasModuleInit(destination)) return true;

    const std::filesystem::path sameNameNested = destination / packageName;
    if (dirHasModuleInit(sameNameNested)) {
        return flattenDirContents(sameNameNested, destination, errorOut);
    }

    std::vector<std::filesystem::path> childDirs;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(destination, ec)) {
        if (ec) {
            errorOut = "no se pudo inspeccionar paquete instalado: " + destination.string();
            return false;
        }
        if (entry.is_directory(ec)) childDirs.push_back(entry.path());
    }

    if (childDirs.size() == 1 && dirHasModuleInit(childDirs.front())) {
        return flattenDirContents(childDirs.front(), destination, errorOut);
    }

    if (!dirHasModuleInit(destination)) {
        errorOut = "la libreria no contiene __init__ ni __init__.epp en la raiz del paquete";
        return false;
    }
    return true;
}

int commandDoctor() {
    ensureBaseLayout();
    const std::filesystem::path root = didRoot();
    const std::vector<std::filesystem::path> checks = {
        root / "lib",
        root / "include",
        root / "script",
        root / "packages",
        root / "cache",
        root / "bin",
        root / "tmp",
        root / "project",
        root / "script" / "activate_did.bat",
        root / "script" / "activate_did.ps1",
        root / "did.env",
        manifestPath(),
    };

    int missing = 0;
    std::cout << "DID doctor\n";
    std::cout << "cwd: " << std::filesystem::current_path().string() << "\n";
    std::cout << "root: " << root.string() << "\n";
    for (const auto& p : checks) {
        const bool ok = std::filesystem::exists(p);
        std::cout << (ok ? "[OK]   " : "[MISS] ") << p.string() << "\n";
        if (!ok) ++missing;
    }

    const auto libs = loadInstalled();
    std::unordered_set<std::string> checkedNames;
    int brokenLibs = 0;
    for (const auto& lib : libs) {
        const std::filesystem::path p = didRoot() / "packages" / lib.name;
        const bool ok = dirHasModuleInit(p);
        std::cout << (ok ? "[OK]   " : "[MISS] ") << "package " << lib.name << " -> " << p.string() << "\n";
        if (!ok) ++brokenLibs;
        checkedNames.insert(toLower(lib.name));
    }

    std::error_code ec;
    const std::filesystem::path packagesDir = didRoot() / "packages";
    for (const auto& entry : std::filesystem::directory_iterator(packagesDir, ec)) {
        if (ec) break;
        if (!entry.is_directory(ec)) continue;
        const std::string name = entry.path().filename().string();
        if (checkedNames.count(toLower(name)) > 0) continue;
        const bool ok = dirHasModuleInit(entry.path());
        std::cout << (ok ? "[OK]   " : "[MISS] ") << "package " << name << " -> " << entry.path().string()
                  << " (no registrado)\n";
        if (!ok) ++brokenLibs;
    }

    if (missing == 0) {
        if (brokenLibs == 0) {
            std::cout << "doctor: entorno DID listo\n";
            return 0;
        }
        std::cout << "doctor: entorno base OK pero hay " << brokenLibs
                  << " librerias con estructura invalida (sin __init__)\n";
        return 1;
    }

    if (brokenLibs > 0) {
        std::cout << "doctor: faltan " << missing << " rutas/archivos y hay " << brokenLibs
                  << " librerias invalidas\n";
        return 1;
    }
    std::cout << "doctor: faltan " << missing << " rutas/archivos\n";
    return 1;
}

int commandInitOnly() {
    ensureBaseLayout();
    std::cout << "Entorno base creado en: " << didRoot().string() << "\n";
    std::cout << "Activa virtual env con: e++\\script\\activate_did.bat\n";
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

int commandListStdlib() {
    std::cout << "Librerias estandar disponibles (" << kStandardLibraries.size() << "):\n";
    for (const auto& lib : kStandardLibraries) {
        std::cout << "  - " << lib << "\n";
    }
    std::cout << "Ruta stdlib: https://github.com/BluePandaOpn/E/tree/Lib/libs\n";
    std::cout << "Ruta externas: https://github.com/BluePandaOpn/E/tree/Lib/Extra\n";
    return 0;
}

int commandRepair() {
    ensureBaseLayout();
    const std::filesystem::path packagesDir = didRoot() / "packages";
    std::error_code ec;
    if (!std::filesystem::exists(packagesDir) || !std::filesystem::is_directory(packagesDir)) {
        std::cerr << "No existe directorio de paquetes: " << packagesDir.string() << "\n";
        return 1;
    }

    int checked = 0;
    int repaired = 0;
    int broken = 0;

    for (const auto& entry : std::filesystem::directory_iterator(packagesDir, ec)) {
        if (ec) {
            std::cerr << "Error recorriendo paquetes: " << packagesDir.string() << "\n";
            return 1;
        }
        if (!entry.is_directory(ec)) continue;
        const std::filesystem::path packagePath = entry.path();
        const std::string name = packagePath.filename().string();
        ++checked;

        if (dirHasModuleInit(packagePath)) {
            std::cout << "[OK] " << name << "\n";
            continue;
        }

        std::string error;
        if (normalizePackageLayout(packagePath, name, error) && dirHasModuleInit(packagePath)) {
            ++repaired;
            std::cout << "[FIX] " << name << " -> estructura normalizada\n";
        } else {
            ++broken;
            std::cout << "[MISS] " << name << " -> " << error << "\n";
        }
    }

    std::cout << "repair: revisadas " << checked << ", reparadas " << repaired
              << ", pendientes " << broken << "\n";
    return broken == 0 ? 0 : 1;
}

bool downloadAndExtractRepo(const std::string& repo,
                            const std::string& branch,
                            const std::filesystem::path& destination,
                            std::string& errorOut) {
    const std::filesystem::path tmpBase = didRoot() / "tmp";
    std::filesystem::create_directories(tmpBase);
    const std::filesystem::path zipPath =
        tmpBase / ("repo_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) +
                   ".zip");
    const std::filesystem::path unpackPath =
        tmpBase / ("repo_unpack_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    const std::string url = "https://codeload.github.com/" + repo + "/zip/refs/heads/" + branch;

    std::ostringstream ps;
    ps << "$zip='" << psSingleQuoted(zipPath.string()) << "'\n";
    ps << "$dest='" << psSingleQuoted(destination.string()) << "'\n";
    ps << "$tmp='" << psSingleQuoted(unpackPath.string()) << "'\n";
    ps << "Invoke-WebRequest -Headers @{ 'User-Agent'='did-cli' } -Uri '" << psSingleQuoted(url) << "' -OutFile $zip\n";
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

bool downloadAndExtractRepoSubdir(const std::string& repo,
                                  const std::string& branch,
                                  const std::string& subdir,
                                  const std::filesystem::path& destination,
                                  std::string& errorOut) {
    const std::filesystem::path tmpBase = didRoot() / "tmp";
    std::filesystem::create_directories(tmpBase);
    const std::filesystem::path zipPath =
        tmpBase / ("repo_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) +
                   ".zip");
    const std::filesystem::path unpackPath =
        tmpBase / ("repo_unpack_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    const std::string url = "https://codeload.github.com/" + repo + "/zip/refs/heads/" + branch;

    std::ostringstream ps;
    ps << "$zip='" << psSingleQuoted(zipPath.string()) << "'\n";
    ps << "$dest='" << psSingleQuoted(destination.string()) << "'\n";
    ps << "$tmp='" << psSingleQuoted(unpackPath.string()) << "'\n";
    ps << "$sub='" << psSingleQuoted(subdir) << "'\n";
    ps << "Invoke-WebRequest -Headers @{ 'User-Agent'='did-cli' } -Uri '" << psSingleQuoted(url) << "' -OutFile $zip\n";
    ps << "if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }\n";
    ps << "New-Item -ItemType Directory -Force -Path $tmp | Out-Null\n";
    ps << "Expand-Archive -Path $zip -DestinationPath $tmp -Force\n";
    ps << "$root = Get-ChildItem -Path $tmp | Where-Object { $_.PSIsContainer } | Select-Object -First 1\n";
    ps << "if ($null -eq $root) { throw 'No se pudo extraer repositorio' }\n";
    ps << "$src = Join-Path $root.FullName $sub\n";
    ps << "if (-not (Test-Path -LiteralPath $src)) { throw ('No existe la libreria en ruta: ' + $sub) }\n";
    ps << "if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }\n";
    ps << "New-Item -ItemType Directory -Force -Path $dest | Out-Null\n";
    ps << "Get-ChildItem -LiteralPath $src -Force | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $dest -Recurse -Force }\n";
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
    if (!hasUniqueNames(kStandardLibraries)) {
        std::cerr << "Lista interna de stdlib invalida: hay nombres duplicados.\n";
        return 1;
    }

    const std::string name = trim(nameArg);
    if (name.empty()) {
        std::cerr << "Nombre de libreria invalido.\n";
        return 1;
    }

    std::string repo = trim(repoArg);
    const bool useCuratedSources = repo.empty();
    const bool standardLibrary = isStandardLibrary(name);
    const std::string sourceType = standardLibrary ? "stdlib" : "externa";
    std::string sourcePath;
    if (useCuratedSources) {
        repo = kCuratedRepo;
        sourcePath = std::string(standardLibrary ? kStdlibRoot : kExtraRoot) + "/" + name;
    }

    std::string branch = trim(branchArg);
    if (branch.empty()) branch = kCuratedBranch;

    const std::filesystem::path destination = didRoot() / "packages" / name;
    std::string error;
    if (useCuratedSources) {
        std::cout << "Instalando '" << name << "' (" << sourceType << ") desde " << repo << "/" << sourcePath
                  << " (branch " << branch << ")...\n";
        if (!downloadAndExtractRepoSubdir(repo, branch, sourcePath, destination, error)) {
            std::cerr << "No se pudo instalar la libreria '" << name << "': " << error << "\n";
            return 1;
        }
    } else {
        std::cout << "Instalando '" << name << "' desde " << repo << " (branch " << branch << ")...\n";
        if (!downloadAndExtractRepo(repo, branch, destination, error)) {
            if (branch == kCuratedBranch) {
                std::cout << "Fallo branch " << kCuratedBranch << ", intentando main...\n";
                if (downloadAndExtractRepo(repo, "main", destination, error)) {
                    branch = "main";
                } else {
                    std::cout << "Fallo branch main, intentando master...\n";
                    if (!downloadAndExtractRepo(repo, "master", destination, error)) {
                        std::cerr << "No se pudo instalar la libreria '" << name << "': " << error << "\n";
                        return 1;
                    }
                    branch = "master";
                }
            } else if (branch == "main") {
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
    }

    if (!normalizePackageLayout(destination, name, error)) {
        std::error_code ec;
        std::filesystem::remove_all(destination, ec);
        std::cerr << "Paquete instalado con estructura invalida para '" << name << "': " << error << "\n";
        std::cerr << "Se revirtio la instalacion para evitar estado inconsistente.\n";
        return 1;
    }

    auto libs = loadInstalled();
    libs.erase(std::remove_if(libs.begin(),
                              libs.end(),
                              [&](const InstalledLib& lib) { return toLower(lib.name) == toLower(name); }),
               libs.end());
    const std::string repoRecord = useCuratedSources ? (repo + ":" + sourcePath) : repo;
    const InstalledLib record{name, repoRecord, nowIsoLocal(), branch};
    libs.push_back(record);
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

    const std::filesystem::path destination = didRoot() / "packages" / name;
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
            std::string branch;

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
        if (cmd == "repair") return commandRepair();
        if (cmd == "stdlib") return commandListStdlib();
        if (cmd == "remove") {
            if (argc < 3) {
                std::cerr << "Uso: did remove <libreria>\n";
                return 1;
            }
            return commandRemoveLibrary(argv[2]);
        }
        if (cmd == "doctor") return commandDoctor();
        if (cmd == "-v" || cmd == "--version" || cmd == "version") {
            std::cout << kDidVersion << "\n";
            return 0;
        }
        if (cmd == "-h" || cmd == "--help" || cmd == "help") {
            printHelp();
            return 0;
        }

        printHelp();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "[DID FATAL] " << e.what() << "\n";
        return 2;
    }
}
