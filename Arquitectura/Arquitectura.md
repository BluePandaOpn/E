# Arquitectura

## Pipeline

1. Lexer (`E++/src/core/lexer.cpp`)
2. Parser (`E++/src/core/parser.cpp`)
3. AST (`E++/src/core/ast.hpp`)
4. Runtime (`E++/src/core/runtime.cpp`)
5. Compiler AOT (`E++/src/core/compiler.cpp`)

## Componentes

- `error.hpp`: estructura de error y fases.
- `runtime.cpp`: ejecucion, imports, funciones, clases, nativas.
- `compiler.cpp`: generacion C++ y toolchain.
- `version_manager.cpp`: versionado/instalacion de toolchain y binarios.

## Carpetas clave

- `E++/src/cli`: comandos `epp` y `did`.
- `E++/src/core`: frontend y ejecucion.
- `lib/libs`: stdlib E++.
- `lib/lib`: componentes nativos C/C++.
- `extensions/epp-language`: extension VS Code.

## Referencias

- [`E++/docs/06-arquitectura.md`](../../E++/docs/06-arquitectura.md)
- [`docs/error/Errores.md`](../error/Errores.md)
