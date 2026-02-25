# urllib

Paquete: `stdlib.urllib`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/urllib`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `quote(...)`
- `quote_plus(...)`
- `encode_component(...)`
- `encode(...)`
- `encode_path(...)`
- `encode_query_param(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.urllib
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

