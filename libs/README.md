# E++ Standard Libraries (`lib/libs`)

Este directorio contiene las librerias estandar del lenguaje E++.

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
