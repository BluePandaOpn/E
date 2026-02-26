# Errores de Compiler y Version Manager

## AOT Compiler (`E-COMP-*`)

- `E-COMP-001`: nodo de expresion no soportado en AOT.
- `E-COMP-002`: nodo de sentencia no soportado en AOT.
- `E-COMP-010`: no se pudo crear archivo temporal AOT.
- `E-COMP-011`: no se pudo crear `CMakeLists.txt` temporal.
- `E-COMP-100`: fallo la compilacion AOT (toolchain/log).
- `E-COMP-201`: backend solicitado aun no implementado.

Acciones recomendadas:
- usar `--backend cpp` para estado actual estable.
- revisar log reportado por `E-COMP-100`.
- confirmar compilador C++ y CMake en `PATH`.

## Version Manager (`E-VER-*`)

Estos codigos son emitidos por `version_manager.cpp` (fase `Compiler`):

- `E-VER-001`: no se pudieron consultar versiones remotas.
- `E-VER-002`: version solicitada invalida o vacia.
- `E-VER-003`: version no encontrada en remoto.
- `E-VER-004`: fallo descarga de version.
- `E-VER-005`: fallo instalacion de version.
- `E-VER-006`: no hay versiones remotas disponibles.

Acciones recomendadas:
- verificar conectividad y acceso a GitHub.
- listar remotas con `epp -v install -l`.
- reintentar instalacion con version exacta.

