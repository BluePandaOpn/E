# E++ Language Extension

Soporte avanzado para `.epp` en VS Code.

## Incluye

- Resaltado de sintaxis y snippets.
- Soporte de archivos de paquete `__init__` y `.__init__` (sin extension).
- Diagnosticos con menos falsos positivos:
  - llaves/parentesis desbalanceados
  - cadenas sin cerrar
  - `import` / `from ... import ...` invalido o incompleto
  - modulo no encontrado (warning configurable)
- Hover con firmas builtin/stdlib y origen de simbolos.
- Autocompletado semantico:
  - keywords y builtin
  - simbolos locales
  - simbolos de modulos importados
- Navegacion avanzada:
  - Go to Definition (local + imports)
  - Find References (archivo actual)
  - Outline/Document Symbols
  - Workspace Symbols (`Ctrl+T`)
- Quick Fix para errores comunes de `import`.
- Tema de iconos opcional `E++ File Icons` (incluye `__init__.png`).

## Iconos de paquete

Activa el tema `E++ File Icons` desde `File Icon Theme` en VS Code para ver:
- Icono especial en archivos `__init__` / `.__init__`.
- Iconos de carpeta E++ usando PNG de `assets`.

## Comandos

- `E++: Run Current File`
- `E++: Check Current File`
- `E++: Compile Current File`
- `E++: Doctor`

## Configuracion

- `eppLanguage.cliPath`
- `eppLanguage.autoSaveBeforeRun`
- `eppLanguage.defaultCompileOutput`
- `eppLanguage.diagnostics.styleWarnings`
- `eppLanguage.diagnostics.unresolvedImportWarnings`
