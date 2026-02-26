# Paquetes y Modulos

E++ soporta modulos por ruta y por identificador de paquete.

## Formas de import

```epp
import stdlib.time
from stdlib.time import now
from stdlib.time import *
import "ruta/relativa/modulo.epp"
```

## Directiva `@package`

Usa `@package` al inicio del modulo para darle identificador estable:

```epp
@package "miorg.utils"
```

Esto permite importar por nombre logico:

```epp
import miorg.utils
```

## Estructura recomendada para paquete

```text
mi_lib/
  __init__.epp
  mi_lib.epp
```

Patron recomendado en `__init__.epp`:

```epp
@package "miorg.mi_lib"
import "mi_lib.epp"
```

## Como resuelve E++ los modulos

El runtime busca en varios prefijos:
- carpeta actual y ancestros
- `libs/`
- `libs/stdlib/`
- `lib/libs/`
- `lib/libs/stdlib/`

Para cada modulo intenta:
- `modulo`
- `modulo.epp`
- `modulo/__init__`
- `modulo/__init__.epp`

## Buenas practicas

- Un paquete por carpeta.
- Siempre crear `__init__.epp`.
- Declarar `@package "..."` en `__init__.epp`.
- Evitar nombres ambiguos entre stdlib y paquetes de app.

