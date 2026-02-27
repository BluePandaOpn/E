# Sistema de Errores

## Errores definidos

1. `ERR_HEX_INVALID`
Mensaje: `Formato de color invalido: '<valor>'. Usa #RRGGBB.`
Causa: El color no cumple el patron de 6 digitos hexadecimales.

2. `ERR_NULL_TEXT`
Mensaje: `Text no puede ser vacio.`
Causa: Se intenta pintar texto sin contenido.

3. `ERR_ENGINE_UNAVAILABLE`
Mensaje: `Motor ANSI no disponible en esta consola.`
Causa: La salida no soporta secuencias ANSI.

## Validaciones

- Prefijo `#` obligatorio.
- Longitud exacta de 7 caracteres.
- Digitos permitidos: `0-9`, `A-F`, `a-f`.
