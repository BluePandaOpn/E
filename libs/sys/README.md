# sys

Paquete: `stdlib.sys`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/sys`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `platform(...)`
- `version(...)`
- `executable(...)`
- `info(...)`
- `is_windows(...)`
- `is_linux(...)`
- `summary(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.sys
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

