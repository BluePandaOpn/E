# Contributing to E++

Gracias por contribuir a E++.

## Flujo recomendado

1. Crea una rama desde `main`.
2. Realiza cambios pequenos y enfocados.
3. Ejecuta build y pruebas basicas.
4. Actualiza documentacion si cambias API o comportamiento.
5. Abre Pull Request con contexto tecnico claro.

## Validacion minima antes de PR

```bash
cmake -S . -B build
cmake --build build --config Release
build/Release/epp.exe doctor
build/Release/epp.exe check ../lib/libs/stdlib/collections/__init__.epp
build/Release/epp.exe check ../lib/libs/stdlib/time/__init__.epp
```

## Convenciones

- Mantener compatibilidad hacia atras cuando sea posible.
- Evitar cambios no relacionados en el mismo PR.
- Escribir mensajes de error claros y accionables.
- No subir binarios generados por build.

## Areas del proyecto

- `src/core`: lenguaje, runtime y AOT.
- `src/cli`: comandos de la CLI.
- `../lib/libs/stdlib`: API base para usuarios.
- `../lib/lib`: libreria nativa (`native_std_cpp`).
- `../extensions/epp-language`: extension de VS Code.
