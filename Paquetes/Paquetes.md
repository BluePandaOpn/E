# Paquetes

## Que es un paquete

Un paquete es una carpeta con `__init__.epp` y opcionalmente directiva `@package`.

## Estructura recomendada

```text
mi_lib/
  __init__.epp
  mi_lib.epp
```

## Ejemplo

```epp
@package "miorg.mi_lib"
import "mi_lib.epp"
```

## Buenas practicas

- Un paquete por carpeta.
- Usa nombres estables (`miorg.modulo`).
- Exporta API publica desde `__init__.epp`.

## Referencias

- [`docs/Modulos/Modulos.md`](../Modulos/Modulos.md)
- [`E++/docs/02-paquetes-modulos.md`](../../E++/docs/02-paquetes-modulos.md)
