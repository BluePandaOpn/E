# E-RUN-031

## Fase

Runtime

## Causa

Se intento llamar algo que no es funcion o clase.

## Accion recomendada

Verifica que el valor sea invocable.

## Ejemplo que lo puede disparar

```epp
var x=1; x()
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
