# E-RUN-072

## Fase

Runtime

## Causa

Libreria no exporta epp_register o epp_register_v2.

## Accion recomendada

Exporta simbolo de registro requerido.

## Ejemplo que lo puede disparar

```epp
DLL sin entrypoint E++
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
