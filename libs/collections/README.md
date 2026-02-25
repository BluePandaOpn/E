# collections

Paquete: `stdlib.collections`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/collections`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `init(...)`
- `left(...)`
- `right(...)`
- `swap(...)`
- `pair(...)`
- `named(...)`
- `tuple2(...)`
- `flipped(...)`
- `named_empty(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- `Pair`
- `NamedValue`

## Uso rapido
```epp
import stdlib.collections
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

