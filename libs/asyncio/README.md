# asyncio

Paquete: `stdlib.asyncio`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/asyncio`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `sleep(...)`
- `sleep_seconds(...)`
- `sleep_ms(...)`
- `run(...)`
- `sleep_minutes(...)`
- `wait(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.asyncio
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

