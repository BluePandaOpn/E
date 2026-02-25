# E-RUN-040

## Fase

Runtime

## Causa

Clase con argumentos sin init(...) compatible.

## Accion recomendada

Agrega init o instancia sin args.

## Ejemplo que lo puede disparar

```epp
MiClase(1) sin init
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
