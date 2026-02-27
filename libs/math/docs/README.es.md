# stdlib.math - Guia General

`stdlib.math` es una libreria para operaciones matematicas en E++.

## Objetivos
- Exponer funciones matematicas comunes.
- Reusar backend nativo C++ via `_native`.
- Mantener una API estable y facil de importar.

## Importacion
```epp
import stdlib.math
```

## Estructura interna
- `__init__.epp`: entrada publica del paquete.
- `src/main.epp`: entrada interna consolidada.
- `src/math.epp`: implementacion de funciones.
- `math.epp`: wrapper de compatibilidad.

## Ejemplos
```epp
import stdlib.math

var area = stdlib.math.pi * stdlib.math.pow(5, 2)
var h = stdlib.math.sqrt(49)
var angle = stdlib.math.radians(180)
```

## Recomendaciones
- Usa `clamp` y `normalize_range` para valores de gameplay/UI.
- Usa `is_close(a, b, rel_tol, abs_tol)` para comparar flotantes.
- Si necesitas nuevas funciones, agregalas en `src/math.epp` y documentalas en `docs/API.md`.
