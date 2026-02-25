# sqlite3

Paquete: `stdlib.sqlite3`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/sqlite3`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `init(...)`
- `ok(...)`
- `status(...)`
- `execute(...)`
- `close(...)`
- `open(...)`
- `connect(...)`
- `memory(...)`
- `open_memory(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- `SQLiteConnection`

## Uso rapido
```epp
import stdlib.sqlite3
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

