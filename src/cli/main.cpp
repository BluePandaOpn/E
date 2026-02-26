#include "compiler.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "runtime.hpp"
#include "version_manager.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string readFile(const std::string& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input) throw std::runtime_error("No se pudo abrir archivo: " + path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void printError(const EppError& e) {
    std::cerr << "\x1b[31m[E++ " << errorPhaseName(e.phase()) << " " << e.code() << "]\x1b[0m "
              << "Linea " << e.line() << ", Columna " << e.column() << ": " << e.what() << "\n";
    if (!e.near().empty()) {
        std::cerr << "  Cerca de: '" << e.near() << "'\n";
    }
    if (!e.hint().empty()) {
        std::cerr << "  Sugerencia: " << e.hint() << "\n";
    }
}

int runSource(const std::string& source, const std::string& sourcePath, bool execute) {
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto program = parser.parse();
    if (execute) {
        Interpreter interpreter(sourcePath);
        interpreter.executeProgram(program);
    }
    return 0;
}

int compileSourceToExe(const std::string& source,
                       const std::string& sourcePath,
                       const std::string& outputExe,
                       const AotCompileOptions& options) {
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto program = parser.parse();
    AotCompiler compiler;
    compiler.compileProgram(program, outputExe, sourcePath, options);
    std::cout << "Compilacion AOT OK: " << outputExe << "\n";
    return 0;
}

struct CompileCliOptions {
    std::string outputExe;
    AotCompileOptions options;
};

bool parseCompileArgs(int argc,
                      char** argv,
                      const std::string& sourcePath,
                      CompileCliOptions& out,
                      std::string& error) {
    std::filesystem::path p(sourcePath);
    p.replace_extension(".exe");
    out.outputExe = p.string();

    bool outputAssigned = false;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                error = "Falta valor para " + arg;
                return false;
            }
            out.outputExe = argv[++i];
            outputAssigned = true;
            continue;
        }
        if (arg == "--backend") {
            if (i + 1 >= argc) {
                error = "Falta valor para --backend";
                return false;
            }
            const std::string backend = argv[++i];
            if (backend == "cpp") out.options.backend = AotBackend::CppAot;
            else if (backend == "llvm-aot") out.options.backend = AotBackend::LlvmAot;
            else if (backend == "llvm-jit") out.options.backend = AotBackend::LlvmJit;
            else {
                error = "Backend invalido: " + backend + " (usa: cpp, llvm-aot, llvm-jit)";
                return false;
            }
            continue;
        }
        if (arg == "--opt") {
            if (i + 1 >= argc) {
                error = "Falta valor para --opt";
                return false;
            }
            try {
                out.options.optLevel = std::stoi(argv[++i]);
            } catch (...) {
                error = "Nivel de optimizacion invalido para --opt (usa 0..3)";
                return false;
            }
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            error = "Opcion no reconocida: " + arg;
            return false;
        }
        if (!outputAssigned) {
            out.outputExe = arg;
            outputAssigned = true;
            continue;
        }
        error = "Argumento extra no esperado: " + arg;
        return false;
    }
    return true;
}

void printHelp() {
    std::cout << "E++ CLI\n";
    std::cout << "Uso:\n";
    std::cout << "  epp <archivo.epp>              Ejecuta el programa\n";
    std::cout << "  epp <archivo.epp> --check      Valida sintaxis\n";
    std::cout << "  epp <archivo.epp> -o <salida.exe>  Compila a ejecutable\n";
    std::cout << "  epp run <archivo.epp>    Ejecuta el programa\n";
    std::cout << "  epp check <archivo.epp>  Solo valida sintaxis\n";
    std::cout << "  epp compile <archivo.epp> [salida.exe] [--backend cpp|llvm-aot|llvm-jit] [--opt 0..3]\n";
    std::cout << "                                      Compila AOT (LLVM backend: roadmap, no implementado aun)\n";
    std::cout << "  epp -V | --version       Muestra version del CLI\n";
    std::cout << "  epp -v install -l        Lista versiones remotas\n";
    std::cout << "  epp -v install <ver>     Instala version (ej: V0.2.4)\n";
    std::cout << "  epp -v update            Actualiza a la ultima version\n";
    std::cout << "  epp doctor               Verifica entorno y rutas comunes\n";
    std::cout << "  epp doctor --imports     Muestra rutas de carga de librerias/imports\n";
}

int runDoctor() {
    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path parent = cwd.parent_path();

    const std::vector<std::pair<std::string, std::vector<std::filesystem::path>>> checks = {
        {"CMakeLists.txt", {cwd / "CMakeLists.txt", cwd / "E++" / "CMakeLists.txt"}},
        {"stdlib time/__init__.epp",
         {cwd / "lib" / "libs" / "stdlib" / "time" / "__init__.epp",
          parent / "lib" / "libs" / "stdlib" / "time" / "__init__.epp",
          cwd / "examples" / "libs" / "stdlib" / "time" / "__init__.epp"}},
        {"epp_native.h",
         {cwd / "include" / "epp_native.h",
          cwd / "lib" / "include" / "epp_native.h",
          parent / "lib" / "include" / "epp_native.h"}},
        {"native_std_cpp.dll",
         {cwd / "build" / "Release" / "native_std_cpp.dll",
          cwd / "lib" / "lib" / "native_std_cpp.dll",
          parent / "lib" / "lib" / "native_std_cpp.dll"}},
    };

    std::cout << "E++ doctor\n";
    std::cout << "version: " << VersionManager::cliVersion() << "\n";
    std::cout << "cwd: " << cwd.string() << "\n";

    int missing = 0;
    for (const auto& check : checks) {
        bool ok = false;
        std::string usedPath;
        for (const auto& p : check.second) {
            if (std::filesystem::exists(p)) {
                ok = true;
                usedPath = p.string();
                break;
            }
        }
        if (ok) {
            std::cout << "[OK] " << check.first << ": " << usedPath << "\n";
        } else {
            std::cout << "[MISS] " << check.first << "\n";
        }
        if (!ok) ++missing;
    }

    if (missing == 0) {
        std::cout << "doctor: entorno OK\n";
        return 0;
    }
    std::cout << "doctor: faltan " << missing << " rutas/archivos\n";
    return 1;
}

std::vector<std::filesystem::path> splitEnvPathList(const char* value) {
    std::vector<std::filesystem::path> out;
    if (!value || !*value) return out;

#ifdef _WIN32
    constexpr char kPathSep = ';';
#else
    constexpr char kPathSep = ':';
#endif
    std::string raw(value);
    size_t start = 0;
    while (start <= raw.size()) {
        const size_t end = raw.find(kPathSep, start);
        const std::string token = raw.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!token.empty()) out.push_back(std::filesystem::path(token));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return out;
}

int runDoctorImports() {
    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path parent = cwd.parent_path();
    std::cout << "E++ doctor imports\n";
    std::cout << "version: " << VersionManager::cliVersion() << "\n";
    std::cout << "cwd: " << cwd.string() << "\n";

    const char* eppHome = std::getenv("EPP_HOME");
    const char* eppPackages = std::getenv("EPP_PACKAGES");
    const char* eppLib = std::getenv("EPP_LIB");

    std::cout << "EPP_HOME: " << (eppHome && *eppHome ? eppHome : "(no definido)") << "\n";
    std::cout << "EPP_PACKAGES: " << (eppPackages && *eppPackages ? eppPackages : "(no definido)") << "\n";
    std::cout << "EPP_LIB: " << (eppLib && *eppLib ? eppLib : "(no definido)") << "\n";

    std::vector<std::filesystem::path> paths = {
        cwd / "e++" / "packages",
        cwd / "e++" / "lib" / "libs",
        cwd / "e++" / "lib" / "libs" / "stdlib",
        parent / "e++" / "packages",
        parent / "e++" / "lib" / "libs",
        parent / "e++" / "lib" / "libs" / "stdlib",
        cwd / "lib" / "libs",
        cwd / "lib" / "libs" / "stdlib",
        parent / "lib" / "libs",
        parent / "lib" / "libs" / "stdlib",
    };

    if (eppHome && *eppHome) {
        const std::filesystem::path home(eppHome);
        paths.push_back(home / "packages");
        paths.push_back(home / "lib" / "libs");
        paths.push_back(home / "lib" / "libs" / "stdlib");
    }
    for (const auto& p : splitEnvPathList(eppPackages)) paths.push_back(p);
    for (const auto& p : splitEnvPathList(eppLib)) {
        paths.push_back(p);
        paths.push_back(p / "libs");
        paths.push_back(p / "libs" / "stdlib");
    }

    int existsCount = 0;
    std::cout << "Rutas de import candidatas:\n";
    for (const auto& p : paths) {
        const bool ok = std::filesystem::exists(p);
        std::cout << (ok ? "[OK]   " : "[MISS] ") << p.lexically_normal().string() << "\n";
        if (ok) ++existsCount;
    }

    std::cout << "Resumen: " << existsCount << "/" << paths.size() << " rutas existentes\n";
    return existsCount > 0 ? 0 : 1;
}
}

int main(int argc, char** argv) {
    try {
        if (argc >= 2 && std::string(argv[1]) == "doctor") {
            if (argc == 2) return runDoctor();
            if (argc == 3 && std::string(argv[2]) == "--imports") return runDoctorImports();
            std::cerr << "[E++] Opcion no reconocida para doctor.\n";
            printHelp();
            return 1;
        }

        auto runDirectFileMode = [&](const std::string& fileArg) -> int {
            const std::string source = readFile(fileArg);

            if (argc == 2) {
                return runSource(source, fileArg, true);
            }
            if (argc == 3 && std::string(argv[2]) == "--check") {
                return runSource(source, fileArg, false);
            }
            if (argc == 4 && std::string(argv[2]) == "-o") {
                return compileSourceToExe(source, fileArg, argv[3], {});
            }
            printHelp();
            return 1;
        };

        if (argc >= 2) {
            const std::string arg1 = argv[1];
            if (arg1 != "-V" && arg1 != "--version" && arg1 != "--vertion" &&
                arg1 != "-v" && arg1 != "run" && arg1 != "check" && arg1 != "compile" &&
                arg1 != "doctor" &&
                !arg1.empty() && arg1[0] != '-') {
                return runDirectFileMode(arg1);
            }
        }

        if (argc == 2) {
            const std::string arg1 = argv[1];
            if (arg1 == "-V" || arg1 == "--version" || arg1 == "--vertion") {
                std::cout << "E++ CLI v" << VersionManager::cliVersion() << "\n";
                return 0;
            }
        }

        if (argc >= 2) {
            const std::string arg1 = argv[1];
            if (arg1 == "-v") {
                VersionManager vm;
                if (argc == 4 && std::string(argv[2]) == "install" && std::string(argv[3]) == "-l") {
                    return vm.listRemoteVersions();
                }
                if (argc == 4 && std::string(argv[2]) == "install") {
                    return vm.installVersion(argv[3]);
                }
                if (argc == 3 && std::string(argv[2]) == "update") {
                    return vm.updateToLatest();
                }
                printHelp();
                return 1;
            }
        }

        if (argc < 3) {
            printHelp();
            return 1;
        }

        const std::string command = argv[1];
        const std::string path = argv[2];
        const std::string source = readFile(path);

        if (command == "run") return runSource(source, path, true);
        if (command == "check") return runSource(source, path, false);
        if (command == "compile") {
            CompileCliOptions compileCli{};
            std::string parseError;
            if (!parseCompileArgs(argc, argv, path, compileCli, parseError)) {
                std::cerr << "[E++] " << parseError << "\n";
                printHelp();
                return 1;
            }
            return compileSourceToExe(source, path, compileCli.outputExe, compileCli.options);
        }

        printHelp();
        return 1;
    } catch (const EppError& e) {
        printError(e);
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "[E++ FATAL] " << e.what() << "\n";
        return 3;
    }
}
