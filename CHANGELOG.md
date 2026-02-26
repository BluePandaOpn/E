# Changelog

## v0.2.4

- Soporte de listas nativas estilo Python con `[]`.
- Acceso por indice: `lista[0]`, `lista[-1]`.
- Asignacion por indice: `lista[1] = valor`.
- Metodos de lista en runtime:
  - `append(valor)`
  - `pop()`, `pop(indice)`
  - `len()`
  - `clear()`
- Resolucion de version CLI actualizada a `0.2.4`.

## v0.2.3

- CLI version alineada a `0.2.3`.
- Mejoras de compilacion AOT en Windows (Winsock link).
- Stdlib reorganizada con API mas consistente:
  - mejoras en `http`, `datetime`, `time`, `math`, `os`, `json`, `random`
  - nuevos alias y wrappers en `tkinter`, `sqlite3`, `urllib`, `threading`, `asyncio`
- Documentacion renovada:
  - `README.md` simplificado y estructurado
  - `docs/03-librerias.md` reescrito
  - nueva referencia `docs/07-stdlib-api.md`
  - guia de publicacion mejorada `docs/06-publicar-github.md`
- Robustez en firma de binarios desde `Config.bat sign` con reintentos.
