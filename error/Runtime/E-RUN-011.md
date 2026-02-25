# E-RUN-011

## Fase

Runtime

## Causa

int() no pudo convertir el valor.

## Accion recomendada

Pasa texto numerico valido.

## Ejemplo que lo puede disparar

```epp
int("abc")
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
