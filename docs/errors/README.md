# Catalogo de Errores

Cada error de E++ usa:

- Fase: `LEXER`, `PARSER`, `RUNTIME`, `COMPILER`, `INTERNAL`
- Codigo: `E-<FASE>-NNN`
- Contexto: linea, columna, mensaje, `near`, sugerencia opcional

## Indice por fase

1. [Lexer](./lexer.md)
2. [Parser](./parser.md)
3. [Runtime](./runtime.md)
4. [Compiler y Version Manager](./compiler.md)
5. [Internal](./internal.md)

## Formato de salida esperado en CLI

```text
[E++ <PHASE> <CODE>] Linea <n>, Columna <m>: <mensaje>
  Cerca de: '<token>'
  Sugerencia: <hint>
```

