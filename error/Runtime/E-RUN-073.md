# E-RUN-073

## Fase

Runtime

## Causa

Registro nativo devolvio cantidad invalida.

## Accion recomendada

Ajusta contrato de retorno del registro.

## Ejemplo que lo puede disparar

```epp
epp_register devuelve numero invalido
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
