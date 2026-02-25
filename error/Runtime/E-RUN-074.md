# E-RUN-074

## Fase

Runtime

## Causa

Firma o argumentos nativos invalidos.

## Accion recomendada

Corrige tipos/arity en binding nativo.

## Ejemplo que lo puede disparar

```epp
native fn con firma incompatible
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
