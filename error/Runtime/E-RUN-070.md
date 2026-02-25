# E-RUN-070

## Fase

Runtime

## Causa

loadlib() requiere ruta de texto.

## Accion recomendada

Pasa string con ruta de libreria.

## Ejemplo que lo puede disparar

```epp
loadlib(123)
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
