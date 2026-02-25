# E-RUN-062

## Fase

Runtime

## Causa

Simbolo inexistente en from ... import ....

## Accion recomendada

Importa simbolo exportado por el modulo.

## Ejemplo que lo puede disparar

```epp
from m import x # x no existe
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
