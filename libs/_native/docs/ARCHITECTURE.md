# Arquitectura

## Objetivo
Mantener `_native` simple, portable y compatible con importaciones existentes.

## Capas
- Capa publica: `__init__.epp`
- Capa legacy: `_native.epp`
- Capa modular: `src/_native.epp`

## Regla de evolucion
- Agregar funciones nuevas en `src/_native.epp`.
- Reflejar API en `_native.epp` para no romper codigo legacy.
- Documentar cambios en `README.md` y `docs/API.md`.

## Calidad minima
- Toda funcion publica debe aparecer en:
  - `src/_native.epp`
  - `_native.epp`
  - `docs/API.md`
- Validar con `scripts/validate.ps1`.
