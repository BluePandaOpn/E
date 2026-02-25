# os

Paquete: `stdlib.os`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/os`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `getcwd(...)`
- `cwd(...)`
- `exists(...)`
- `is_file(...)`
- `is_dir(...)`
- `mkdir(...)`
- `makedirs(...)`
- `remove(...)`
- `unlink(...)`
- `join2(...)`
- `join3(...)`
- `pwd(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- (sin clases publicas)

## Uso rapido
```epp
import stdlib.os
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

