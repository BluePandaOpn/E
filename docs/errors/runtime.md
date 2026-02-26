# Errores de Runtime

## Variables e imports

- `E-RUN-001`: variable no declarada.
- `E-RUN-060`: modulo de import vacio.
- `E-RUN-061`: no se pudo abrir modulo importado.
- `E-RUN-062`: simbolo inexistente en `from ... import ...`.

## Tipos, operadores y evaluacion

- `E-RUN-010`: `int()` con tipo no permitido.
- `E-RUN-011`: `int()` no pudo convertir el valor.
- `E-RUN-012`: literal invalido.
- `E-RUN-020`: operador unario invalido.
- `E-RUN-021`: suma entre tipos incompatibles.
- `E-RUN-022`: division por cero.
- `E-RUN-023`: operador binario invalido.
- `E-RUN-024`: se esperaba numero en contexto numerico.
- `E-RUN-090`: expresion no soportada.
- `E-RUN-091`: sentencia no soportada.

## Llamadas, clases y objetos

- `E-RUN-030`: cantidad de argumentos invalida.
- `E-RUN-031`: intento de llamar algo que no es funcion/clase.
- `E-RUN-040`: clase con argumentos sin `init(...)`.
- `E-RUN-050`: acceso a propiedades sobre no-instancia.
- `E-RUN-051`: propiedad no encontrada.
- `E-RUN-052`: asignacion de propiedad sobre no-instancia.

## Carga de librerias nativas (`loadlib`)

- `E-RUN-070`: `loadlib()` requiere ruta de texto.
- `E-RUN-071`: no se pudo cargar libreria compartida.
- `E-RUN-072`: no exporta `epp_register`/`epp_register_v2`.
- `E-RUN-073`: registro nativo devolvio cantidad invalida.
- `E-RUN-074`: firma/argumentos nativos invalidos.
- `E-RUN-075`: funcion nativa devolvio tipo no permitido.

