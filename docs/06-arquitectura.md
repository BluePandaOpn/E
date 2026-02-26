# Arquitectura Interna

## Pipeline del lenguaje

1. `Lexer` (`src/core/lexer.cpp`)
2. `Parser` (`src/core/parser.cpp`)
3. AST (`src/core/ast.hpp`)
4. Runtime/Interpreter (`src/core/runtime.cpp`)
5. AOT Compiler (`src/core/compiler.cpp`)

## Componentes clave

- `error.hpp`: tipo `EppError`, fase y codigo (`E-LEX-*`, `E-PARSE-*`, etc.).
- `runtime.cpp`:
  - entorno de variables
  - funciones y clases
  - imports y resolucion de paquetes
  - registro de nativas (`defineNative`)
- `compiler.cpp`:
  - generacion C++ intermedio
  - invocacion de toolchain para `.exe`

## Layout del proyecto

- `src/cli`: comandos `epp` y `did`
- `src/core`: frontend, runtime y compilador
- `lib/libs/stdlib`: stdlib E++
- `lib/lib`: componente nativo C/C++

## Decision de diseno actual

- Frontend pequeno y legible.
- Errores con codigos estables.
- Runtime primero, AOT incremental.
- Soporte de paquetes via `@package` + `__init__.epp`.

