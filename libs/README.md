# E++ Standard Libraries (`lib/libs`)

Este directorio contiene las librerias estandar del lenguaje E++.

## Estado actual (v2)

- Arquitectura modular estandarizada:
  - `__init__.epp` como fachada publica.
  - `*.epp` como implementacion interna.
- API ampliada en modulos core: `math`, `random`, `time`, `datetime`, `json`, `os`, `csv`, `urllib`, `http`, `collections`.
- Compatibilidad mantenida con aliases existentes para evitar romper codigo previo.
- Metadatos uniformes por modulo:
  - `module_name()`
  - `module_version()`
  - `native_runtime()`
  - `native_builtin_mode()`
- Modulos con soporte parcial en runtime actual (`asyncio`, `threading`, `sqlite3`) exponen placeholders profesionales y funciones de estado para migracion progresiva.

## Integracion C++

- El backend nativo se expone por `_native` (`stdlib._native`).
- Las librerias principales importan `_native` para usar funciones del runtime C++.
- Diagnostico de rutas de carga:
  - `epp doctor --imports`
- Reparacion de paquetes mal instalados:
  - `did repair`

## Librerias

- `_native`
- `asyncio`
- `collections`
- `color`
- `csv`
- `datetime`
- `http`
- `json`
- `math`
- `os`
- `random`
- `sqlite3`
- `sys`
- `threading`
- `time`
- `tkinter`
- `urllib`

Cada libreria incluye su propio `README.md` con API y ejemplos de uso.
