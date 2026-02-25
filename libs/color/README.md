# color

Paquete: `color`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/color`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `init(...)`
- `colorear(...)`
- `estilo(...)`
- `rojo(...)`
- `verde(...)`
- `azul(...)`
- `amarillo(...)`
- `magenta(...)`
- `cyan(...)`
- `blanco(...)`
- `bg_rojo(...)`
- `bg_verde(...)`
- `bg_azul(...)`
- `bg_amarillo(...)`
- `bg_magenta(...)`
- `bg_cyan(...)`
- `bg_blanco(...)`
- `negrita(...)`
- `subrayado(...)`
- `alerta(...)`
- `exito(...)`
- `info(...)`
- `strip_ansi(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- `ConsolaColor`

## Uso rapido
```epp
import color
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

