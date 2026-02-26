# Errores de Parser

## `E-PARSE-001`

- Causa: token o estructura esperada no encontrada.
- Casos comunes:
  - `from ... import` sin simbolos.
  - directiva `@...` no soportada.
  - falta de `)` `}` `end` u otro token obligatorio.
  - modulo invalido en `import`/`from`/`@package`.
- Accion: seguir hint del parser y corregir la forma exacta esperada.

## `E-PARSE-003`

- Causa: expresion invalida.
- Tipico: parentesis desbalanceados u operador fuera de lugar.
- Accion: revisar la expresion completa y tokens cercanos.

## `E-PARSE-004`

- Causa: asignacion no valida.
- Regla: solo se puede asignar a variable o propiedad (`x = ...`, `obj.campo = ...`).
- Accion: mover la asignacion a un identificador o propiedad de instancia.

