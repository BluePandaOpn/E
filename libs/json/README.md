# json

Paquete: `stdlib.json`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/json`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `dumps_text(...)`
- `quote(...)`
- `dumps(...)`
- `loads_number(...)`
- `parse_number(...)`
- `is_valid_number(...)`
- `stringify(...)`
- `parse_num(...)`
- `valid_number(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.json
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

