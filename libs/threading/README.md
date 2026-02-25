# threading

Paquete: `stdlib.threading`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/threading`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `cpu_count(...)`
- `hardware_concurrency(...)`
- `active_count(...)`
- `can_parallel(...)`
- `workers_hint(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.threading
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

