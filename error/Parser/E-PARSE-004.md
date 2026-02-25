# E-PARSE-004

## Fase

Parser

## Causa

Asignacion no valida.

## Accion recomendada

Asigna solo a variable o propiedad.

## Ejemplo que lo puede disparar

```epp
(a + b) = 3
```

## Referencia de implementacion

- `E++/src/core/parser.cpp`
