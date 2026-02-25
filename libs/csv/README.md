# csv

Paquete: `stdlib.csv`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/csv`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `row2(...)`
- `join2(...)`
- `header2(...)`
- `row3(...)`
- `row4(...)`
- `header3(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.csv
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

