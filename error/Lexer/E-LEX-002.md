# E-LEX-002

## Fase

Lexer

## Causa

Cadena sin cierre.

## Accion recomendada

Agrega comilla doble de cierre.

## Ejemplo que lo puede disparar

```epp
var s = "hola
```

## Referencia de implementacion

- `E++/src/core/lexer.cpp`
