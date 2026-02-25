# math

Paquete: `stdlib.math`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/math`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `sqrt(...)`
- `pow(...)`
- `floor(...)`
- `ceil(...)`
- `sin(...)`
- `cos(...)`
- `tan(...)`
- `log(...)`
- `exp(...)`
- `abs(...)`
- `min(...)`
- `max(...)`
- `clamp(...)`
- `round(...)`
- `lerp(...)`
- `between(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.math
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

