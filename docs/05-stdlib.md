# Stdlib

La stdlib vive en `lib/libs/stdlib`, con una carpeta por libreria.

## Estructura

Ejemplo:

```text
lib/libs/stdlib/time/
  time.epp
  __init__.epp
```

Todos los `__init__.epp` declaran paquete:

```epp
@package "stdlib.time"
```

## Librerias estandar actuales

- `stdlib.asyncio`
- `stdlib.collections`
- `stdlib.csv`
- `stdlib.datetime`
- `stdlib.http`
- `stdlib.json`
- `stdlib.math`
- `stdlib.os`
- `stdlib.random`
- `stdlib.sqlite3`
- `stdlib.sys`
- `stdlib.threading`
- `stdlib.time`
- `stdlib.tkinter`
- `stdlib.urllib`
- `stdlib._native`

## Uso recomendado

```epp
import stdlib.math
import stdlib.time

print(stdlib.math.sqrt(9))
print(stdlib.time.now())
```

## Criterios para extender stdlib

- Mantener API estable y nombres claros.
- Agregar alias solo cuando mejoren compatibilidad.
- Incluir `@package` en `__init__.epp`.
- Validar con `epp check` antes de commit.

