# time

Paquete: `stdlib.time`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/time`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `time(...)`
- `now(...)`
- `monotonic(...)`
- `sleep(...)`
- `sleep_ms(...)`
- `sleep_seconds(...)`
- `sleep_minutes(...)`
- `elapsed(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.time
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

