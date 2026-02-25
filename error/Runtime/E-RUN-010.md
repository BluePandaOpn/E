# E-RUN-010

## Fase

Runtime

## Causa

int() con tipo no permitido.

## Accion recomendada

Convierte desde texto o numero valido.

## Ejemplo que lo puede disparar

```epp
int([1,2])
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
