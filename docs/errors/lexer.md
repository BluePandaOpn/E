# Errores de Lexer

## `E-LEX-001`

- Causa: caracter inesperado.
- Tipico: simbolo invalido o typo.
- Accion: revisar operadores, comillas o caracteres no soportados.

## `E-LEX-002`

- Causa: cadena de texto sin cierre.
- Tipico: falta `"` final.
- Accion: cerrar la cadena con comillas dobles.

## `E-LEX-003`

- Causa: comentario de bloque sin cierre.
- Tipico: `/* ...` sin `*/`.
- Accion: cerrar comentario con `*/`.

