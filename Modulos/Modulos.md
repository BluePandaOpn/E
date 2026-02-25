# Modulos

## Formas de import

```epp
import stdlib.time
from stdlib.time import now
from stdlib.time import *
import "ruta/relativa/modulo.epp"
```

## Resolucion de modulos

El runtime intenta rutas locales, rutas de `lib/libs`, y rutas de paquetes DID cuando estan configuradas.

## Reglas operativas

- `import x` intenta `x`, `x.epp`, `x/__init__.epp`.
- `from x import y` exige que `y` exista en el modulo cargado.
- Importar modulo vacio o no accesible dispara errores `E-RUN-060/061/062`.

## Referencias

- [`docs/error/Runtime/E-RUN-060.md`](../error/Runtime/E-RUN-060.md)
- [`docs/error/Runtime/E-RUN-061.md`](../error/Runtime/E-RUN-061.md)
- [`docs/error/Runtime/E-RUN-062.md`](../error/Runtime/E-RUN-062.md)
- [`E++/docs/02-paquetes-modulos.md`](../../E++/docs/02-paquetes-modulos.md)
