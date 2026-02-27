# math - Guia General

`math` es una libreria para operaciones matematicas en E++.

## Objetivos
- Exponer funciones matematicas comunes.
- Reusar backend nativo C++ via `_native`.
- Mantener una API estable y facil de importar.

## Importacion
```epp
import math
```

Nota: `import math` no crea un objeto `math`; expone funciones/constantes directamente.

## Estructura interna
- `__init__.epp`: entrada publica del paquete.
- `src/main.epp`: entrada interna consolidada.
- `src/math.epp`: implementacion de funciones.
- `math.epp`: wrapper de compatibilidad.

## Ejemplos
```epp
import math

var area = pi * pow(5, 2)
var h = sqrt(49)
var angle = radians(180)
```

## Recomendaciones
- Usa `clamp` y `normalize_range` para valores de gameplay/UI.
- Usa `is_close(a, b, rel_tol, abs_tol)` para comparar flotantes.
- Si necesitas nuevas funciones, agregalas en `src/math.epp` y documentalas en `docs/API.md`.

