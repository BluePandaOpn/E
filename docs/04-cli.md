# CLI (`epp` y `did`)

## `epp`

Comandos principales:

```powershell
epp run archivo.epp
epp check archivo.epp
epp compile archivo.epp salida.exe --backend cpp --opt 2
epp -V
epp doctor
```

Modos:
- `run`: ejecuta en runtime.
- `check`: valida lexer+parser+semantica basica de ejecucion.
- `compile`: AOT (actualmente `--backend cpp` es el backend operativo).

## `did`

Comandos principales:

```powershell
did install
did install <lib>
did install <lib> --repo owner/repo --branch rama
did stdlib
did list
did repair
did remove <lib>
did doctor
did -v
```

Comportamiento por defecto:
- Version actual documentada de `did`: `V0.1.3`.
- Repo por defecto para librerias: `BluePandaOpn/E`.
- Rama por defecto: `Lib`.
- Estandar: `libs/<lib>`.
- Externa: `Extra/<lib>`.
- `epp` ahora resuelve imports desde rutas de DID automaticamente:
  - `e++/packages`
  - `%EPP_PACKAGES%`
  - `%EPP_HOME%/packages`
  - `%EPP_HOME%/lib/libs` y `%EPP_HOME%/lib/libs/stdlib`
- Prioridad de import: primero rutas de entorno (`EPP_PACKAGES`/`EPP_HOME`) y luego rutas legacy del repo.
- `did repair` normaliza librerias mal extraidas (ej: `color/color/__init__.epp`).
- Prioridad de carga: lo instalado en `packages` se intenta antes que rutas antiguas de stdlib.
- Compatibilidad: `import stdlib.x` tambien intenta `import x` para librerias instaladas con DID.

## Diagnostico rapido

Si el comando `did` no refleja cambios nuevos:
- Ejecutar binario por ruta absoluta (`build/Release/did.exe`).
- Revisar prioridad del `PATH`.
- Correr `did --help` para confirmar texto y version.
