# E++ Language

E++ es un lenguaje en desarrollo con CLI propia, runtime en C++, compilacion AOT y una stdlib inspirada en Python.

Soporta paquetes con `__init__` y `.__init__` (sin extension) para imports estilo Python.

## Estado actual

- CLI funcional (`run`, `check`, `compile`, `doctor`, version manager).
- Gestor `did` para entorno virtual del lenguaje e instalacion de librerias desde GitHub.
- Lexer + Parser + AST + Interpreter.
- Listas nativas estilo Python (`[]`, indexado y metodos base).
- Compilacion AOT a ejecutable nativo.
- Stdlib modular en `../lib/libs/stdlib`.

## Estructura del repo

- `src/cli`: comandos de `epp`.
- `src/core`: lexer, parser, runtime, AOT, version manager.
- `../lib/include`: headers publicos nativos.
- `../lib/lib`: librerias C/C++ nativas (`native_std_cpp`).
- `../lib/libs/stdlib`: stdlib en E++.
- `../extensions/epp-language`: extension de VS Code.

## Build rapido

```bash
cmake -S . -B build
cmake --build build --config Release
```

Windows (atajos):

```bat
Config.bat build
Config.bat shell
```

## Uso rapido del CLI

```bash
build/Release/epp.exe doctor
build/Release/epp.exe run ../lib/libs/stdlib/time/__init__.epp
build/Release/epp.exe check ../lib/libs/stdlib/collections/__init__.epp
build/Release/epp.exe -V
```

## Uso rapido de DID

```bash
build/Release/did.exe install
build/Release/did.exe install nombre_libreria
build/Release/did.exe install nombre_libreria --repo owner/repositorio
build/Release/did.exe list
build/Release/did.exe doctor
```

## Publicacion

- [CONTRIBUTING.md](CONTRIBUTING.md)
- [CHANGELOG.md](CHANGELOG.md)
- [docs/README.md](docs/README.md)
