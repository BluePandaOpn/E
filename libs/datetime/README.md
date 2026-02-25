# datetime

Paquete: `stdlib.datetime`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/datetime`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `now_iso(...)`
- `now(...)`
- `year(...)`
- `month(...)`
- `day(...)`
- `init(...)`
- `today(...)`
- `date_info(...)`
- `ymd(...)`
- `iso_now(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- `DateInfo`

## Uso rapido
```epp
import stdlib.datetime
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

